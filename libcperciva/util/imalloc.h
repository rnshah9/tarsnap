#ifndef _IMALLOC_H_
#define _IMALLOC_H_

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * imalloc(nrec, reclen):
 * Allocate ${nrec} records of length ${reclen}.  Check for size_t overflow.
 * ${reclen} must not be zero.  If ${nrec} is zero, return NULL.
 */
static inline void *
imalloc(size_t nrec, size_t reclen)
{

	/* Sanity check. */
	assert(reclen != 0);

	if (nrec == 0)
		return (NULL);

	if (nrec > SIZE_MAX / reclen) {
		errno = ENOMEM;
		return (NULL);
	} else {
		return (malloc(nrec * reclen));
	}
}

/**
 * IMALLOC(p, nrec, type):
 * Allocate ${nrec} records of type ${type} and store the pointer in ${p}.
 * Return non-zero on failure.
 */
#define IMALLOC(p, nrec, type)						\
	((((p) = (type *)imalloc((nrec), sizeof(type))) == NULL) &&	\
	    ((nrec) > 0))

#endif /* !_IMALLOC_H_ */
