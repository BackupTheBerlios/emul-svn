/*
*  C Implementation: emul_replug
*
* Description: 
*
*
* Author: Lonnie Mendez <lmendez19@austin.rr.com>, (C) 2005
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <emul.h>


int main(int argc, char *argv[])
{
	if (em_open()<0) {
		fprintf(stderr, "The device must be manually replugged.\n");
		exit(1);
	}

	em_replug();
	em_close();

	return EXIT_SUCCESS;
}
