/* Functions are or are derived from Al Borchers circular write
 * buffering code from the linux pl2303 usb-serial driver.
 *
 *  Licensed under the GNU GPL, Version 2.
 */ 

#include <stdlib.h>
#include <string.h>
#include "buf.h"
 
 /*
 * buf_alloc
 *
 * Allocate a circular buffer and all associated memory.
 */

struct buf *buf_alloc(unsigned int size)
{
	struct buf *cb;

	if (size == 0)
		return NULL;

	cb = (struct buf *)malloc(sizeof(struct buf));
	if (cb == NULL)
		return NULL;

	cb->buf_buf = malloc(size);
	if (cb->buf_buf == NULL) {
		free(cb);
		return NULL;
	}

	cb->buf_size = size;
	cb->buf_get = cb->buf_put = cb->buf_buf;

	return cb;
}


/*
 * buf_free
 *
 * Free the buffer and all associated memory.
 */

void buf_free(struct buf *cb)
{
	if (cb != NULL) {
		if (cb->buf_buf != NULL)
			free(cb->buf_buf);
		free(cb);
	}
}


/*
 * buf_clear
 *
 * Clear out all data in the circular buffer.
 */

void buf_clear(struct buf *cb)
{
	if (cb != NULL)
		cb->buf_get = cb->buf_put;
		/* equivalent to a get of all data available */
}


/*
 * buf_data_avail
 *
 * Return the number of bytes of data available in the circular
 * buffer.
 */

unsigned int buf_data_avail(struct buf *cb)
{
	if (cb != NULL)
		return ((cb->buf_size + cb->buf_put - cb->buf_get) % cb->buf_size);
	else
		return 0;
}


/*
 * buf_space_avail
 *
 * Return the number of bytes of space available in the circular
 * buffer.
 */

unsigned int buf_space_avail(struct buf *cb)
{
	if (cb != NULL)
		return ((cb->buf_size + cb->buf_get - cb->buf_put - 1) % cb->buf_size);
	else
		return 0;
}


/*
 * buf_put
 *
 * Copy data data from a user buffer and put it into the circular buffer.
 * Restrict to the amount of space available.
 *
 * Return the number of bytes copied.
 */

unsigned int buf_put(struct buf *cb, const char *buf, unsigned int count)
{
	unsigned int len;

	if (cb == NULL)
		return 0;

	len  = buf_space_avail(cb);
	if (count > len)
		count = len;

	if (count == 0)
		return 0;

	len = cb->buf_buf + cb->buf_size - cb->buf_put;
	if (count > len) {
		memcpy(cb->buf_put, buf, len);
		memcpy(cb->buf_buf, buf+len, count - len);
		cb->buf_put = cb->buf_buf + count - len;
	} else {
		memcpy(cb->buf_put, buf, count);
		if (count < len)
			cb->buf_put += count;
		else /* count == len */
			cb->buf_put = cb->buf_buf;
	}

	return count;
}


/*
 * buf_get
 *
 * Get data from the circular buffer and copy to the given buffer.
 * Restrict to the amount of data available.
 *
 * Return the number of bytes copied.
 */

unsigned int buf_get(struct buf *cb, char *buf, unsigned int count)
{
	unsigned int len;

	if (cb == NULL)
		return 0;

	len = buf_data_avail(cb);
	if (count > len)
		count = len;

	if (count == 0)
		return 0;

	len = cb->buf_buf + cb->buf_size - cb->buf_get;
	if (count > len) {
		memcpy(buf, cb->buf_get, len);
		memcpy(buf+len, cb->buf_buf, count - len);
		cb->buf_get = cb->buf_buf + count - len;
	} else {
		memcpy(buf, cb->buf_get, count);
		if (count < len)
			cb->buf_get += count;
		else /* count == len */
			cb->buf_get = cb->buf_buf;
	}

	return count;
}
