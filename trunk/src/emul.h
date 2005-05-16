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
 
#ifndef EMUL_H
#define EMUL_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file emul.h
    \brief Header file for the emul library.

    Contains facilities for accessing the earthmate.
*/

/* TODO: add functions to read uart status data - pointless really, but oh well. */
#define UART_DSR         0x20 /* data set ready - flow control - device to host */
#define UART_CTS         0x10 /* clear to send - flow control - device to host */
#define UART_RI          0x10 /* ring indicator - modem - device to host */ 
#define UART_CD          0x40 /* carrier detect - modem - device to host */
#define PARITY_ERROR     0x08 /* received from input report - device to host */


/* The header file is heavily doxygenated... please have a look at html/index.html if
 * you find reading this header is starting to give you a headache.
 */

/*! \brief For use with em_devtype function.

    When used in conjunction with \a em_devtype, this will set the active device to a
    normal tty device.
 */
#define NORMAL  0x0

/*! \brief For use with em_devtype function.

    When used in conjunction with \a em_devtype, this will set the active device to an
    Earthmate device.
 */
#define EMATE   0x1

/*! \brief Sets the device type, either a normal tty or earthmate device.

    \param type The type of device (\a NORMAL or \a EMATE).
 */
void em_devtype(int type);
/*! \brief Indicates whether or not device type is set to normal tty or earthmate device.

    \return Returns 1 if device type is \a EMATE or 0 if device type is \a NORMAL.
 */
int  em_isemate(void);
/*! \brief Indicates whether or not the device has been opened and thread is active.

    \return Returns 1 if device is open and thread is active, 0 otherwise.
 */
int  em_isactive(void);
/*! \brief Opens the earthmate device and starts thread.

    This must be the first function called before device can be accessed.  Only
    a few functions can be called without opening the device first.

    \return Returns 0 on success, -1 on error.
*/
int  em_open(void);
/*! \brief Closes the earthmate device and stops thread.

    \a em_close should be called when the program has finished communicating with the device.
 */
void em_close(void);
/*! \brief Causes reenumeration after device and thread have been stopped.

    After a successful call, a new usb device handle will have been obtained and
    the thread will start again.  The usual functions may be called once more.

    \return Returns 0 on success, -1 on error.
 */
int em_replug(void);
/*! \brief For use with em_linecontrol function.

    When used in conjunction with \a em_linecontrol, this will raise the dtr line on the
    Earthmate device.  This can be bitmasked with \a CONTROL_RTS and \a CONTROL_RESET.

    FYI, DTR is an acronym for data terminal ready.
 */
#define CONTROL_DTR      0x20 /* data terminal ready - flow control - host to device */
/*! \brief For use with em_linecontrol function.

    When used in conjunction with \a em_linecontrol, this will raise the rts line on the
    Earthmate device.  This can be bitmasked with \a CONTROL_DTR and \a CONTROL_RESET.

    FYI, RTS is an acronym for request to send.
 */
#define CONTROL_RTS      0x10 /* request to send - flow control - host to device */
/*! \brief For use with em_linecontrol function.

    When used in conjunction with \a em_linecontrol, this will cause a device reset which has
    and as yet unknown affect on the Earthmate device.  This can be bitmasked with
    \a CONTROL_DTR and \a CONTROL_RTS.
 */
#define CONTROL_RESET    0x08 /* sent with output report (unknown affect) - host to device */
/*! \brief For use with em_linecontrol function.

    When used in conjunction with \a em_linecontrol, this will drop both dtr and rts lines on
    the Earthmate device.  This should be issued without bitmask.
 */
#define CONTROL_DROP     0x00 /* drop all lines */
/*! \brief Sets the line state for the earthmate device.

    \param lines This parameter can be bit masked with \a CONTROL_DTR, \a CONTROL_RTS, and \a CONTROL_RESET.
    \a CONTROL_DROP should be issued by itself and never masked.  \a CONTROL_RESET has an unknown affect.
 */
void em_linecontrol(u_int8_t lines);
/*! \brief For use with parameter parity of structure serconfig.

    Indicates odd parity.
 */
#define PAR_ODD          0x30
/*! \brief For use with parameter parity of structure serconfig.

    Indicates even parity.
 */
#define PAR_EVEN         0x10
/*! \brief For use with parameter parity of structure serconfig.

    Indicates no parity.
 */
#define PAR_NONE         0x00
/*! \brief Structure used with em_serconfig_set and em_serconfig_get.

    This object is used for setting and retrieving the serial configuration.
 */
struct serconfig {
/*! \brief Parameter for baud rate.

    Valid rates are 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, and 115200.
 */
	long  baudrate;
/*! \brief Parameter for data bits.

    Valid range is 5 - 8.
 */
	short databits;
/*! \brief Parameter for stop bits.

    Valid values are 1 or 2.
 */
	short stopbits; /* 1, 2 stop bits */
/*! \brief Parameter for parity type.

    Valid values are \a PAR_NONE, \a PAR_EVEN, or \a PAR_ODD.
 */
	short parity;   /* 0 - none, 1 - odd, 2 - even parity */
};
/*! \brief Function for setting serial parameters using serconfig object.
    \param sconfig Object of \a serconfig which holds serial parameters to be set.
    \note The \a serconfig object being passed must have all fields set.
    \return Returns 0 on success, <0 on error.
 */
int  em_serconfig_set(struct serconfig *sconfig);
/*! \brief Function for retrieving serial parameters using serconfig object.

    \return Returns 0 on success, <0 on error.
 */
int  em_serconfig_get(struct serconfig *sconfig);

/*! \brief For use with em_read function.

    This is the max amount of data the read buffer can store.
 */
#define EM_MAX_READ       4096
/*! \brief For use with em_write function.

    This is the max amount of data the write buffer can store.
 */
#define EM_MAX_WRITE      4096
/*! \brief Function for retrieving read data from Earthmate device from read buffer.
    \param buffer An array of unsigned 8 bit elements pre-allocated before being passed.
    \param count The amount of data that the function should try to fill \a buffer with.
    \note This function is non-blocking.
    \return Returns amount of data read.  >=0 on success, <0 on error.
 */
int  em_read(u_int8_t buffer[], int count);
/*! \brief Function for sending write data from the program to the write buffer.
    \param buffer Pointer to the data that is to be transfered into the write buffer.
    \param count The amount of data to try and write to the write buffer.
    \note This function is non-blocking.
    \return Returns amount of data written.  >=0 on success, <0 on error.
 */
int  em_write(const u_int8_t *buffer, int count);

/*! \brief Function for clearing either the read, write, or both buffers.
    \param queue_selector The type of flush to perform.  Valid value for now is \a TCIOFLUSH.
    \return Returns 0 on success, <0 on error.
 */
int em_flush(int queue_selector);

/*! \brief Function that indicates how much data is available in the read buffer.
    \note This function is non-blocking.
    \return On success the amount of data in read buffer is returned, <0 on error.
 */
int  em_read_data_avail(void);
/*! \brief Function that indicates how much data is available in the write buffer.
    \note This function is non-blocking.
    \return On success the amount of data in write buffer is returned, <0 on error.
 */
int  em_write_data_avail(void);

/*! \brief Function that stops execution by sleeping either until data becomes available
    in the read buffer, or the timeout value expires.  Behaves almost like select.
    \param tv The timeout value in seconds and microseconds.  Can be NULL for infinite block.
    \param jumpout Can be used to skip over entirely the block on certain previous events like
    a select call.  Not all that useful, see \a em_select below for a better way.
    \return Returns 1 if data becomes available in read buffer, or 0 if either timeout value
    has expired or \a jumpout has value of 1.
 */
int  em_datawait(struct timeval *tv, int jumpout);

/*! \brief Function that waits on select activity and/or read buffer data becoming available.
    \param n The number of fds to watch (always +1).  Typically, FD_SETSIZE is passed.
    \param readfds The fd_set to watch for read activity.  Can be NULL.
    \param writefds The fd_set to watch for write activity.  Can be NULL.
    \param exceptfds The fd_set to watch for exceptional activity, like errors.  Can be NULL.
    \param timeout The time in seconds and microseconds to wait.  Can be NULL.
    \note The function does not return on data becoming available in the write buffer.
    \return Returns the number of fds with activity.  This can include read buffer activity
    as well.
 */
/* counterpart to select
 */ 
int em_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/*! \brief Counterpart to tcdrain.  Sleeps until all data in write buffer has been written.
 */
void em_writewait(void);

/*! \brief Function that sets the debugging level of the emul library.
    \note Debugging is by default 0, which means completely disabled.  When enabled, data is
    printed to stdout or stderr depending on the situation.
 */
void em_debuglevel(int mode);

/*! \brief For use with em_change_state function.

    When used in conjunction with the \a em_change_state function, this will change the thread
    state to active.
 */
#define EMT_ACTIVE 1
/*! \brief For use with em_change_state function.

    When used in conjunction with the \a em_change_state function, the thread state will change
    to idle.
 */
#define EMT_IDLING 0
void em_change_state(int state);

/*! \brief Function that can be placed wherever read() is used safely.
    \param fd File descriptor to read from.
    \param buf Buffer to read into.
    \param count Amount of data to try and read.
    \note Also have a look at \a em_read().
    \return Returns amount of data read, or <0 on error.
 */
ssize_t READ(int fd, void *buf, size_t count);
/*! \brief Function that can be placed wherever write() is used safely.
    \param fd File descriptor to write to.
    \param buf Buffer data to write.
    \param count Amount of data to try and write.
    \return Returns amount of data written, or <0 on error.
 */
ssize_t WRITE(int fd, const void *buf, size_t count);

/*! \brief Function for retrieving integer baud rate from masked rate.
    \param rate The masked rate to lookup for the integer value.
    \return Returns the integer value of the given masked rate.
 */
unsigned int em_getbaudmask(unsigned int rate);
/*! \brief Function for retriving current baudrate of connection with the
    Earthmate device.
    \return Returns the current baud rate in integer form, or <0 on error.
 */
long em_getbaudrate(void);

/* usb devid/vendid */
/*! \brief The USB Vendor ID used to find the Earthmate device with libusb.
 */
#define  VENDOR_ID_DELORME        0x1163
/*! \brief The USB Product ID used to find the Earthmate device with libusb.
 */
#define  PRODUCT_ID_EARTHMATEUSB  0x0100
/*! \brief The USB Product ID of the DeLorme Earthmate lt-20.
 */
#define  PRODUCT_ID_EARTHMATELT20 0x0200


/*! \brief For use with em_raw_read and em_raw_write.

    This is the max amount of data that can be transmitted with \a em_raw_read and \a em_raw_write.

    The device can handle 32 bytes for the input/output reports, but 2 bytes are used for
    length, control line, uart status, etc.
 */
#define MAX_READ_WRITE 30

/* TODO: consider removing the below two functions from public api view */
/*! \brief Function that reads directly from the Earthmate device.
    If the thread is active, this function should \a never be called as libusb
    will have you pulling your hair out.
    \note Always use the \a MAX_READ_WRITE define if you dare to use this function.
 */
int  em_raw_read(u_int8_t buffer[]);
/*! \brief Function that writes directly to the Earthmate device.
    If the thread is active, this function should \a never be called as libusb
    will have you pulling your hair out.
    \note Always use the \a MAX_READ_WRITE define if you dare to use this function.
 */
int  em_raw_write(u_int8_t *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif /* ifndef EMUL_H */
