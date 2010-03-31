/* sys/select.h

   netport
*/

#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Get fd_set, and macros like FD_SET */
#include <sys/types.h>
#include <gctypes.h>

/* Get definition of timeval.  */
#include <sys/time.h>
#include <time.h>

/* Get definition of sigset_t. */
#include <signal.h>

#undef  FD_SETSIZE
#define FD_SETSIZE		16
#define FD_SET(n, p)		((p)->fd_bits[(n)/8] |=  (1 << ((n) & 7)))
#define FD_CLR(n, p)		((p)->fd_bits[(n)/8] &= ~(1 << ((n) & 7)))
#define FD_ISSET(n,p)		((p)->fd_bits[(n)/8] &   (1 << ((n) & 7)))
#define FD_ZERO(p)		memset((void*)(p),0,sizeof(*(p)))

typedef struct fd_set {
	u8 fd_bits [(FD_SETSIZE+7)/8];
} fd_set;

int  pselect(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
         const struct timespec *restrict, const sigset_t *restrict);
int  select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict,
         struct timeval *restrict);

#ifdef __cplusplus
};
#endif

#endif /* sys/select.h */
