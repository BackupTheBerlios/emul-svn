/* Functions are or are derived from Al Borchers circular write
 * buffering code from the linux pl2303 usb-serial driver.
 *
 *  Licensed under the GNU GPL, Version 2.
 */

#ifndef BUF_H
#define BUF_H
#include <pthread.h>

/* read/write buffer structure */
struct buf {
	unsigned int	buf_size;
	char		*buf_buf;
	char		*buf_get;
	char		*buf_put;
	
	/* protect buffer with thread mutex */
	pthread_mutex_t buf_mutex;
};

struct buf *buf_alloc(unsigned int size);
void buf_free(struct buf *cb);
void buf_clear(struct buf *cb);
unsigned int buf_data_avail(struct buf *cb);
unsigned int buf_space_avail(struct buf *cb);
unsigned int buf_put(struct buf *cb, const char *buf, unsigned int count);
unsigned int buf_get(struct buf *cb, char *buf, unsigned int count);

#endif /* BUF_H */
