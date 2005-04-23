/***************************************************************************
 *   Copyright (C) 2005 by Lonnie Mendez                                   *
 *   lmendez19@austin.rr.com                                               *
 *                                                                         *
 *   emul_test: shows how to use the library and tests device.             *
 *                The program will output data from both standard in       *
 *                and the earthmate device.                                *
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
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO
#include <sys/filio.h>
#endif
#include <unistd.h>
#include <string.h>
#include <emul.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))


static int quit = 0;

void handlesig(int sig)
{
	quit = 1;
}


int main(int argc, char *argv[])
{
	struct serconfig sconfig1, sconfig2;
	fd_set selectset, tset;
	int ret, debuglevel, x, count;
	u_int8_t buf[1024];
	
	if (argc > 1 && argv[1][0] == 'd') {
		sscanf(argv[1], "d%d", &debuglevel);
		em_debuglevel(debuglevel);
	}
	
	(void) signal(SIGINT, handlesig);

	sconfig1.baudrate = 4800;
	sconfig1.databits = 8;
	sconfig1.stopbits = 1;
	sconfig1.parity = PAR_NONE;
	
	ret = em_open();
	if (ret < 0) {
		errno = -ret;
		perror("\tem_open");
		fprintf(stderr, "\tCheck node permissions and device attachment.\n");
		exit(EXIT_FAILURE);
	} else
		fprintf(stdout, "Device successfuly opened.\n");

	/* NOTE: you don't have to issue em_linecontrol at first start... em_serconfig_set
		is enough to begin receiving data, but it doesn't hurt either.
	 */
	ret = em_serconfig_set(&sconfig1);
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
			count = MIN(count, 1024);
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
	
	fprintf(stdout, "\n\n%d byte(s) left in read buffer.\n", em_read_data_avail());
	
	ret = em_serconfig_get(&sconfig2);
	if (ret < 0)
		fprintf(stderr, "Failed to retrieve serial config - ret = %d\n", ret);
	else {
		fprintf(stdout, "Device reports:\n");
		fprintf(stdout, "Baudrate: %ld\n", sconfig2.baudrate);
		fprintf(stdout, "Databits: %d\n", sconfig2.databits);
		fprintf(stdout, "Stopbits: %d\n", sconfig2.stopbits);
		fprintf(stdout, "Parity: ");
		if (sconfig2.parity == PAR_NONE)
			fprintf(stdout, "none");
		else
			if (sconfig2.parity == PAR_ODD)
				fprintf(stdout, "odd");
			else
				fprintf(stdout, "even");
		fprintf(stdout, "\n");
	}
	
	em_close();
	return 0;
}

