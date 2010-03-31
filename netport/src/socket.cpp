/* socket.cpp

   netport
*/

#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>

namespace wii
{
#include <network.h>
};

#define restrict 

int accept(int s, struct sockaddr * addr restrict, socklen_t * addrlen restrict)
{
    return wii::net_accept(s, (wii::sockaddr *) addr,  addrlen);
}

int bind(int s, const struct sockaddr * addr, socklen_t addrlen)
{
	return wii::net_bind(s, (wii::sockaddr *)addr, addrlen);
}


int connect(int s, const struct sockaddr * addr, socklen_t addrlen)
{
	return wii::net_connect(s, (wii::sockaddr *)addr, addrlen);
}


ssize_t recv(int s, void *mem, size_t len, int flags)
{
	return wii::net_recv(s, mem, len, flags);

}

ssize_t recvfrom(int s, void * mem restrict, size_t len, int flags,
        struct sockaddr * from restrict, socklen_t * fromlen restrict)
		{
	return wii::net_recvfrom(s, mem, len, flags, (wii::sockaddr *)from, fromlen);
}

int     shutdown(int s , int h)
{
	return wii::net_shutdown(s,h);
}
int     socket(int s, int a, int b)
{
	return wii::net_socket(s, a, b);
}

int     listen(int s, int a)
{
	return wii::net_listen(s,a);
}

ssize_t send(int s, const void *msg, size_t len, int flags)
{
	return wii::net_send(s, msg, len, flags);
}

ssize_t sendto(int s, const void * msg, size_t len, int flags, const struct sockaddr * addr,
        socklen_t tolen)
{
	return wii::net_sendto(s,msg,len,flags, (wii::sockaddr *)addr,tolen);
}

int setsockopt(int s, int a, int b, const void *c, socklen_t d)
{
	return wii::net_setsockopt(s,a,b,c,d);
}


// STUBS

int getsockopt(int s, int a, int b, void * c, socklen_t *d)
{
	//return wii::net_getsockopt(s, a, b, c, d);
	return -EINVAL;
}

int getsockname(int, struct sockaddr *restrict, socklen_t *restrict)
{
	return -EINVAL;
}
		
int     getpeername(int, struct sockaddr *restrict, socklen_t *restrict)
{
	return -EINVAL;
}

ssize_t recvmsg(int, struct msghdr *, int)
{
	return -EINVAL;
}

ssize_t sendmsg(int, const struct msghdr *, int)
{
	return -EINVAL;
}

int sockatmark(int)
{
	return -EINVAL;
}

int socketpair(int, int, int, int[2])
{
	return -EINVAL;
}
