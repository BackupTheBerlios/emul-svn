/***************************************************************************
 *   Copyright (C) 2005 by Lonnie Mendez                                   *
 *   lmendez19@austin.rr.com                                               *
 *                                                                         *
 *   earthmateusb_userland: Portable user land library providing access to *
 *                          earthmate usb device via libusb.               *
 *                                                                         *
 *   TODO: sort out header files for portability.                          *
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
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_SYS_FILIO
#include <sys/filio.h>
#endif
#include <unistd.h>
#include <string.h>
#include <emul.h>


static int quit = 0;

void handlesig(int sig)
{
	quit = 1;
}


int main(int argc, char *argv[])
{
	struct serconfig sconfig, test;
	fd_set selectset, tset;
	int ret, debuglevel;
	u_int8_t buf[1024];
	
	if (argc > 1 && argv[1][0] == 'd') {
		sscanf(argv[1], "d%d", &debuglevel);
		em_debuglevel(debuglevel);
	}
	
	(void) signal(SIGINT, handlesig);
	
	sconfig.baudrate = 4800;
	sconfig.databits = 8;
	sconfig.stopbits = 1;
	sconfig.parity = PAR_NONE;
	
	ret = em_open();
	
	if (ret < 0) {
		errno = -ret;
		perror("\tem_open");
		fprintf(stderr, "\tCheck node permissions and device attachment.\n");
		exit(EXIT_FAILURE);
	} else
		fprintf(stdout, "Device successfuly opened.\n");
	
	ret = em_serconfig_set(&sconfig);
	if (ret < 0)
		fprintf(stderr, "Failed setting new serial config\n");
	em_linecontrol(CONTROL_DTR | CONTROL_RTS);
	
	FD_ZERO(&selectset);
	FD_SET(0, &selectset);
	
	while (!quit) {
		tset = selectset;
		
		if (em_select(1, &tset, NULL, NULL, NULL)<0) {
			perror("select");
			break;
		}
		
		if (FD_ISSET(0, &tset)) {
			ioctl(0, FIONREAD, &count);
			count = min(count, 1024);
			read(0, buf, count);
			for (x = 0; x < count; x++)
				fprintf(stdout, "%c", buf[x]);
		}
		
		if ((count = em_read_data_avail())) {
			em_read(buf, count);
			for (x = 0; x < count; x++)
				fprintf(stdout, "%c", buf[x]);
		}
	}
	em_linecontrol(CONTROL_DROP);
	
	fprintf(stdout, "\n\n%d byte(s) left in read buffer.", em_read_data_avail());
	
	ret = em_serconfig_get(&test);
	if (ret < 0)
		fprintf(stderr, "Failed to retrieve serial config - ret = %d\n", ret);
	else {
		fprintf(stdout, "Device reports:\n");
		fprintf(stdout, "Baudrate: %ld\n", test.baudrate);
		fprintf(stdout, "Databits: %d\n", test.databits);
		fprintf(stdout, "Stopbits: %d\n", test.stopbits);
		fprintf(stdout, "Parity: ");
		if (test.parity == PAR_NONE)
			fprintf(stdout, "none");
		else
			if (test.parity == PAR_ODD)
				fprintf(stdout, "odd");
			else
				fprintf(stdout, "even");
		fprintf(stdout, "\n");
	}
	
	em_close();
	return 0;
}

