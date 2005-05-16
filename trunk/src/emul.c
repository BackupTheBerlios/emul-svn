/***************************************************************************
 *   Copyright (C) 2005 by Lonnie Mendez                                   *
 *   lmendez19@austin.rr.com                                               *
 *                                                                         *
 *   earthmateusb_userland: Portable user land library providing access to *
 *                          earthmate usb device via libusb.               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <usb.h>

#include "emul.h"
#include "buf.h"


/* required for usb_control_msg */
#define HID_REQ_GET_REPORT 0x01
#define HID_REQ_SET_REPORT 0x09
#define CONTROL_TIMEOUT 500

/* for use with thread_state to control thread */
#define DOREAD  1
#define DOWRITE 2
#define CONTROL 3
#define IDLE    4
#define QUIT    5

#ifndef WORDS_BIGENDIAN
# define cpu_to_le32(x) (x)
#else
  u_int32_t cpu_to_le32(u_int32_t x) { return (((x&0x000000FF)<<24)+((x&0x0000FF00)<<8) +
		((x&0x00FF0000)>>8)+((x&0xFF000000)>>24)); }
#endif
#define le32_to_cpu(x) cpu_to_le32(x)


/* device handle, etc */
static struct EMUL {
	long                  baudrate;
	u_int8_t              lines;
	struct usb_dev_handle *udev;
	int                   thread_state;
} em_device;

/* data buffers */
static struct buf *read_buffer;
static struct buf *write_buffer;

/* read/write thread */
static pthread_t rwthread;
static void *rw_thread(void *arg);

/* library debugging on/off */
static int em_debug = 0;

/* Device type - either NORMAL or EMATE.
 *
 * Should be considered deprecated.
 */
static int DEVICE_TYPE = NORMAL;


/* Start of api implementation
 */

int em_isactive()
{
	return (em_device.udev ? 1 : 0);
}

int em_open() {
	struct usb_bus *busses;
	struct usb_bus *bus;
	int ret;

	if (em_device.udev) {
		if (em_debug) fprintf(stderr, "em_open: Device has already been opened.\n");
		return -1;
	}
	
	em_device.baudrate = 0;
	em_device.thread_state = DOREAD;
	
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	busses = usb_get_busses();
	
	for (bus = busses; bus; bus = bus->next) {
		struct usb_device *dev;
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == VENDOR_ID_DELORME &&
				(dev->descriptor.idProduct == PRODUCT_ID_EARTHMATEUSB ||
				 dev->descriptor.idProduct == PRODUCT_ID_EARTHMATELT20)) {
				em_device.udev = usb_open(dev);
				if (em_device.udev) {
					if (em_debug) fprintf(stdout, "em_open - Successfuly opened device.\n");
					goto grab_interface;
				} else {
					if (em_debug) fprintf(stderr, "em_open - Could not open device\n");
					em_device.udev = NULL;
					return -1;
				}
			}
		}
	}
	/* no device matching vid/pid found */
	em_device.udev = NULL;
	return -ENODEV;
	
grab_interface:
	/* release cypress_m8 module for linux kernel 2.6.10+
	 * and hiddev module for kernels 2.4.x/2.6.x
	 */
	#ifdef __linux__
	{
		char dname[32];
		usb_get_driver_np(em_device.udev, 0, dname, 31);
		if (strcmp(dname, "cypress") == 0 || strncmp(dname, "hid", 3) == 0) {
			usb_detach_kernel_driver_np(em_device.udev, 0);
			if (em_debug) fprintf(stdout, "em_open: dettached driver %s from device\n", dname);
		}
	}
	#endif
	
	usb_set_configuration(em_device.udev, 1);
	ret = usb_claim_interface(em_device.udev, 0);
	if (ret < 0) {
		if (em_debug) fprintf(stderr, "em_open: failed to claim device\n");
		usb_close(em_device.udev);
		em_device.udev = NULL;
		return ret;
	}
	
	/* clear halts */
	usb_clear_halt(em_device.udev, 0x81);
	usb_clear_halt(em_device.udev, 0x02);
	
	/* allocate read/write buffers */
	read_buffer = (struct buf *)buf_alloc(EM_MAX_READ);
	write_buffer = (struct buf *)buf_alloc(EM_MAX_WRITE);
	
	ret = pthread_mutex_init(&read_buffer->buf_mutex, NULL);
	if (ret != 0) {
		fprintf(stderr, "em_open: failed to init read buffer lock\n");
		exit(EXIT_FAILURE);
	}
	
	ret = pthread_mutex_init(&write_buffer->buf_mutex, NULL);
	if (ret != 0) {
		fprintf(stderr, "em_open: failed to init write buffer lock\n");
		exit(EXIT_FAILURE);
	}
	
	ret = pthread_create(&rwthread, NULL, rw_thread, NULL);
	if (ret != 0) {
		fprintf(stderr, "em_open: failed to create read/write thread\n");
		exit(EXIT_FAILURE);
	}
	
	sleep(1);
	return 0;
}

void em_close() {
	if (em_device.udev) {
		em_device.thread_state = QUIT;
		pthread_join(rwthread, NULL);
		usb_release_interface(em_device.udev, 0);
		usb_close(em_device.udev);
		em_device.udev = NULL;
		
		pthread_mutex_destroy(&write_buffer->buf_mutex);
		pthread_mutex_destroy(&read_buffer->buf_mutex);
		
		buf_free(write_buffer);
		buf_free(read_buffer);
	}
}

int em_replug() {
	if (em_device.udev) {
		em_device.thread_state = QUIT;
		pthread_join(rwthread, NULL);
		usb_reset(em_device.udev);
		usb_release_interface(em_device.udev, 0);
		usb_close(em_device.udev);
		em_device.udev = NULL;

		pthread_mutex_destroy(&write_buffer->buf_mutex);
		pthread_mutex_destroy(&read_buffer->buf_mutex);
		
		buf_free(write_buffer);
		buf_free(read_buffer);

		return em_open();
	}
	return em_open();
}

void em_linecontrol(u_int8_t lines)
{
	if (!em_device.udev)
		return;
	
	if (em_device.lines != lines) {
		em_device.lines = lines;
		em_device.thread_state = CONTROL;
	}
	
	/* wait for thread to branch into CONTROL...
	 * must sleep as long as interrupt_{read,write} timeout value
	 */
	usleep(100000);
}

int em_serconfig_set(struct serconfig *sconfig)
{
	u_int8_t feature_buffer[5];
	u_int8_t config = 0;
	int databits, stopbits;
	int ret, tries = 0;
	
	if (!(em_device.udev && sconfig))
		return -1;
		
	databits = sconfig->databits - 5;
	stopbits = sconfig->stopbits - 1;
	
	memset(feature_buffer, 0x0, 5);
	*((u_int32_t *)feature_buffer) = le32_to_cpu(sconfig->baudrate);
	
	/* set config */
	config |= databits; /* 3 = 8 data bits, 0 = 5 stop bits */
	config |= (stopbits << 3); /* 0 - 1 stop bit, 1 - 2 stop bits */
	config |= sconfig->parity;
	feature_buffer[4] = config;

	do {
		ret = usb_control_msg(em_device.udev, USB_ENDPOINT_OUT | USB_RECIP_INTERFACE |
						      USB_TYPE_CLASS, HID_REQ_SET_REPORT, 0x0300, 0,
						      (char *)feature_buffer, 5, CONTROL_TIMEOUT);
		if (tries++ >= 3)
			break;
		
		if (ret == EPIPE)
			usb_clear_halt(em_device.udev, 0x00);
	} while (ret != 5 && ret != ENODEV);

	if (ret != 5)
		return ret;

	return 0;
}

int em_serconfig_get(struct serconfig *sconfig)
{
	u_int8_t feature_buffer[5];
	int ret, tries = 0;
	
	if (!(em_device.udev && sconfig))
		return -1;
	
	memset(feature_buffer, 0x0, 5);

	do {
		ret = usb_control_msg(em_device.udev, USB_ENDPOINT_IN | USB_RECIP_INTERFACE |
						      USB_TYPE_CLASS, HID_REQ_GET_REPORT, 0x0300, 0,
						      (char *)feature_buffer, 5, CONTROL_TIMEOUT);
		
		if (tries++ >=3)
			break;
		
		if (ret == EPIPE)
			usb_clear_halt(em_device.udev, 0x00);
	} while (ret != 5 && ret != ENODEV);

	if (ret != 5) {
		sconfig = NULL;
		return ret;
	}
	
	sconfig->baudrate = cpu_to_le32(*((u_int32_t *)feature_buffer));
	sconfig->databits = (feature_buffer[4] & 0x3) + 5;
	sconfig->stopbits = ((feature_buffer[4] >> 3) & 0x1) + 1;
	sconfig->parity = feature_buffer[4] & 0x30;
	
	em_device.baudrate = sconfig->baudrate;
	
	return 0;
}

long em_getbaudrate()
{
	if (!em_device.udev)
		return -1;

	return em_device.baudrate;
}

int em_raw_read(u_int8_t buffer[])
{
	int transfered, ret;
	u_int8_t tbuf[32];
	
	if (!em_device.udev)
		return -1;
	
	memset(tbuf, 0x0, 32);
	
#ifdef HAVE_USBINT
	ret = usb_interrupt_read(em_device.udev, 0x81, (char *)tbuf, 32, 100);
#else
	ret = usb_bulk_read(em_device.udev, 0x81, (char *)tbuf, 32, 100);
#endif
	
	if (ret < 0)
		return ret;
	
	transfered = tbuf[1];
	
	if ((em_debug > 2) && (transfered > 0)) {
		int x;
		fprintf(stdout, "em_raw_read(size: %d): ", transfered);
		for (x = 0; x < transfered + 2; x++)
			fprintf(stdout, "%02X ", tbuf[x]);
		fprintf(stdout, "\n");
	}
	
	if ((em_debug > 2) && (transfered > 0)) {
		fprintf(stdout, "UART state: ");
		if (tbuf[0] & UART_DSR)
			fprintf(stdout, "DSR ");
		if (tbuf[0] & UART_CTS)
			fprintf(stdout, "CTS ");
		if (tbuf[0] & UART_CD)
			fprintf(stdout, "CD ");
		if (tbuf[0] & PARITY_ERROR)
			fprintf(stdout, "parity error!");
		if (!(tbuf[0] & UART_DSR) && !(tbuf[0] & UART_CTS) && !(tbuf[0] & UART_CD))
			fprintf(stdout, "No UART Flags set.");
		fprintf(stdout, "\n");
	}
	
	if (ret > 0);
		memcpy(buffer, tbuf+2, transfered);
	
	return transfered;
}

int em_raw_write(u_int8_t *buffer, int size)
{
	u_int8_t *tbuf;
	int ret, x;
	
	if (!em_device.udev)
		return -1;
	
	if (size == 0)
		return 0;
	else if (size > MAX_READ_WRITE)
		return -1;
	
	tbuf = (u_int8_t *)malloc(sizeof(u_int8_t) * (size+2));
	if (!tbuf)
		return 0;
	
	memset(tbuf, 0x0, size+2);
	memcpy(tbuf+2, buffer, size);
	
	tbuf[0] = em_device.lines;
	tbuf[1] = size;
	
	if (em_debug > 2) {
		fprintf(stdout, "em_raw_write(size: %d): ", size);
		for (x = 0; x < size+2; x++)
			fprintf(stdout, "%02X ", tbuf[x]);
		fprintf(stdout, "\n");
	}
	
#ifdef HAVE_USBINT
	ret = usb_interrupt_write(em_device.udev, 0x02, (char *)tbuf, size+2, 100);
#else
	ret = usb_bulk_write(em_device.udev, 0x02, (char *)tbuf, size+2, 100);
#endif
	
	em_device.lines &= ~CONTROL_RESET;
	
	free(tbuf);
	
	if (ret < 0)
		return ret;
	
	return size;
}

void em_change_state(int state)
{
	if (!em_device.udev)
		return;
	
	switch (state) {
		case EMT_ACTIVE:
			em_device.thread_state = DOREAD;
			break;
		case EMT_IDLING:
			em_device.thread_state = IDLE;
			break;
		default:
			em_device.thread_state = DOREAD;
	}
}

/* the read/write/control handling thread
 */
void *rw_thread(void *arg) {
	u_int8_t buffer[MAX_READ_WRITE], wbuffer[30];
	int ret, EXIT=0, count, wbuf_count;
	u_int8_t buf[2];
	
	em_device.thread_state = DOREAD;
	
	while (!EXIT) {
	
		pthread_mutex_lock(&write_buffer->buf_mutex);
		wbuf_count = buf_data_avail(write_buffer);
		pthread_mutex_unlock(&write_buffer->buf_mutex);
		
		/* don't change state when in below states */
		if (em_device.thread_state == IDLE ||
			 em_device.thread_state == QUIT ||
			 em_device.thread_state == CONTROL);
		else if (wbuf_count && em_device.thread_state == DOWRITE) {
			em_device.thread_state = DOREAD;
		} else if (wbuf_count) {
			em_device.thread_state = DOWRITE;
		} else
			em_device.thread_state = DOREAD;
		
		switch (em_device.thread_state) {
			case DOREAD:
				ret = em_raw_read(buffer);
				if (ret > 0) {
					pthread_mutex_lock(&read_buffer->buf_mutex);
					if (ret > buf_space_avail(read_buffer))
						buf_clear(read_buffer);
					buf_put(read_buffer, (char *)buffer, ret);
					if (em_debug>2) fprintf(stdout, "rw_thread: read buffer reports %d byte load\n", buf_data_avail(read_buffer));
					pthread_mutex_unlock(&read_buffer->buf_mutex);
				} else if (ret < 0 && (ret != -110)) {
					EXIT = 1;
					if (em_debug) fprintf(stderr, "rw_thread: DOREAD error - %d\n", ret);
				}
				break;
			case DOWRITE:
				pthread_mutex_lock(&write_buffer->buf_mutex);
				count = buf_get(write_buffer, (char *)wbuffer, 30);
				pthread_mutex_unlock(&write_buffer->buf_mutex);
				ret = em_raw_write(wbuffer, count);
				if (ret < 0) {
					EXIT = 1;
					if (em_debug) fprintf(stderr, "rw_thread: DOWRITE error - %d\n", ret);
				}
				break;
			case CONTROL:
				buf[0] = em_device.lines;
				buf[1] = 0;
			#ifdef HAVE_USBINT
				ret = usb_interrupt_write(em_device.udev, 0x02, (char *)buf, 2, 100);
			#else
				ret = usb_bulk_write(em_device.udev, 0x02, (char *)buf, 2, 100);
			#endif
				if (ret<0) {
					if (em_debug) fprintf(stderr, "rw_thread: DOCONTROL error - %d\n", ret);
					EXIT = 1;
				}
				em_device.thread_state = DOREAD;
			case IDLE:
				usleep(5000); /* 5 ms */
				break;
			case QUIT:
				EXIT = 1;
		}
	}
	
	pthread_exit(NULL);
}

int em_read(u_int8_t buffer[], int count)
{
	int ret = -1, hasdata;
	u_int8_t *tbuf;
	
	if (!em_device.udev)
		return ret;

	hasdata = em_read_data_avail();
	if (!hasdata)
		return 0;
	
	tbuf = (u_int8_t *)malloc(sizeof(u_int8_t) * count);
	
	pthread_mutex_lock(&read_buffer->buf_mutex);
	ret = buf_get(read_buffer, (char *)tbuf, count);
	pthread_mutex_unlock(&read_buffer->buf_mutex);
	if (ret > 0)
		memcpy(buffer, tbuf, ret);
	
	free(tbuf);
	
	if (em_debug>2) fprintf(stdout, "em_read: read %d bytes from read buffer\n", ret);
	
	return ret;
}

int em_write(const u_int8_t *buffer, int count)
{
	int space, ret = -1;
	
	if (!em_device.udev)
		return ret;

	pthread_mutex_lock(&write_buffer->buf_mutex); 
	space = buf_space_avail(write_buffer);
	pthread_mutex_unlock(&write_buffer->buf_mutex);
	
	if (count > space)
		count = space; 
	
	pthread_mutex_lock(&write_buffer->buf_mutex);
	ret = buf_put(write_buffer, (char *)buffer, count);
	if (em_debug>2) fprintf(stdout, "em_write: write_buffer reports %d byte load\n", buf_data_avail(write_buffer));
	pthread_mutex_unlock(&write_buffer->buf_mutex);
	
	return ret;
}

int em_flush(int queue_selector)
{
	if (!em_device.udev)
		return -1;

	switch (queue_selector) {
		case TCIOFLUSH:
			pthread_mutex_lock(&read_buffer->buf_mutex);
			buf_clear(read_buffer);
			pthread_mutex_unlock(&read_buffer->buf_mutex);
			pthread_mutex_lock(&write_buffer->buf_mutex);
			buf_clear(write_buffer);
			pthread_mutex_unlock(&write_buffer->buf_mutex);
			break;
	}
	
	return 0;
}

/* NULL timeval blocks until data becomes available in read buffer.
 * tv value blocks until data becomes available in read buffer, or
 * time runs out.
 */
int em_datawait(struct timeval *tv, int jumpout)
{
	struct timeval curtime, endtime;
	int block = (tv ? 0 : 1), timeup = (tv ? 0 : 1);
	int ret = 0;
	
	if (!em_device.udev)
		return -1;
		
	if (jumpout) {
		return ret;
	}
	
	if (tv) {
		gettimeofday(&curtime, NULL);
		gettimeofday(&endtime, NULL);
		endtime.tv_sec = endtime.tv_sec + (*tv).tv_sec;
		endtime.tv_usec = endtime.tv_usec + (*tv).tv_usec;
	}
	
	while (!timeup || block) {
		if (tv) {
			if ((curtime.tv_sec >= endtime.tv_sec) && (curtime.tv_usec >= endtime.tv_usec))
				timeup = 1;
		
			gettimeofday(&curtime, NULL);
		}
		
		if (em_read_data_avail()>0) {
			ret++;
			goto finish;
		}
			
		usleep(50);
	}
	
finish:
	return ret;
}

int em_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	struct timeval tv, curtime, endtime;
	int count_fdschanged = 0;
	fd_set rfds, wfds, efds;
	int ret = 0;
	
	if (timeout) {
		gettimeofday(&curtime, NULL);
		gettimeofday(&endtime, NULL);
	
		endtime.tv_sec += timeout->tv_sec;
		endtime.tv_usec += timeout->tv_usec;
	}
	
	while (1) {
		if (timeout) {
			if ((curtime.tv_sec >= endtime.tv_sec) && (curtime.tv_usec >= endtime.tv_usec))
				break;
		}
	
		if (em_read_data_avail())
			count_fdschanged++;
		
		if (readfds)
			rfds = *readfds;
		if (writefds)
			wfds = *writefds;
		if (exceptfds)
			efds = *exceptfds;
		
		/* if we try to return on data in the read buffer, the information in fd_set
		 * structures will never be filled and bogus hits will result... so this is
	 	 * correct. */
		tv.tv_sec = 0;
		tv.tv_usec = 10;
		ret = select(n, (readfds ? &rfds : NULL), (writefds ? &wfds : NULL), (exceptfds ? &efds : NULL), &tv);
		if (ret<0)
			goto quit;
		else if (ret > 0)
			count_fdschanged+=ret;
		
		if (count_fdschanged > 0)
			break;
		
		if (timeout)
			gettimeofday(&curtime, NULL);
	}
	ret = count_fdschanged;
	
quit:
	if (readfds)
		*readfds = rfds;
	if (writefds)
		*writefds = wfds;
	if (exceptfds)
		*exceptfds = efds;

	return ret;
}

void em_writewait()
{
	if (!em_device.udev)
		return;
	
	while(em_write_data_avail())
		usleep(5000);
}

int em_read_data_avail()
{
	int ret = 0;
	
	if (!em_device.udev)
		return -1;
	
	if (em_device.udev) {
		pthread_mutex_lock(&read_buffer->buf_mutex);
		ret = buf_data_avail(read_buffer);
		pthread_mutex_unlock(&read_buffer->buf_mutex);
	}
	
	return ret;
}

int em_write_data_avail()
{
	int ret = 0;
	
	if (!em_device.udev)
		return -1;
	
	if (em_device.udev) {
		pthread_mutex_lock(&write_buffer->buf_mutex);
		ret = buf_data_avail(write_buffer);
		pthread_mutex_unlock(&write_buffer->buf_mutex);
	}
	
	return ret;
}

void em_debuglevel(int mode)
{
	em_debug = mode;
}

/*** Utility functions - deprecated ***/

void em_devtype(int type)
{
	DEVICE_TYPE = type;
}

int em_isemate(void)
{
	return DEVICE_TYPE;
}

ssize_t READ(int fd, void *buf, size_t count)
{
	int ret = 0;
	
	if (DEVICE_TYPE == EMATE)
		ret = em_read(buf, count);
	else
		ret = read(fd, buf, count);
		
	return ret;
}

ssize_t WRITE(int fd, const void *buf, size_t count)
{
	int ret = 0;
	
	if (DEVICE_TYPE == EMATE)
		ret = em_write(buf, count);
	else
		ret = write(fd, buf, count);
		
	return ret;
}

unsigned int em_getbaudmask(unsigned int rate)
{
	switch (rate) {
		case B300:
			return 300;
		case B600:
			return 600;
		case B1200:
			return 1200;
		case B2400:
			return 2400;
		case B4800:
			return 4800;
		case B9600:
			return 9600;
		case B19200:
			return 19200;
		case B38400:
			return 38400;
		case B57600:
			return 57600;
		default:
			return 57600;
	}
}
