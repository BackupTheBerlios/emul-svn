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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <emul.h>

static int quit = 0;

void endprog(int sig)
{
	quit = 1;
}

int main(void)
{
	fd_set selectset, tset;
	int count, x;
	u_int8_t buf[2048];
	
	signal(SIGINT, endprog);
	
	em_debuglevel(0);
	if (em_open()<0) {
		printf("failed to open earthmate\n");
		exit(1);
	}
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
			read(0, buf, count);
			for (x = 0; x < count; x++)
				printf("%c", buf[x]);
			if (count == 4 && strncmp((char *)buf, "exit", 4) == 0)
				break;
		}
		
		if ((count = em_read_data_avail())) {
			em_read(buf, count);
			for (x = 0; x < count; x++)
				printf("%c", buf[x]);
		}
	}

	printf("\n");
	
	em_linecontrol(CONTROL_DROP);
	em_close();
	
	return EXIT_SUCCESS;
}
