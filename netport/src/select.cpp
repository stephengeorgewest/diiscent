#include <sys/select.h>

namespace wii
{
#include <network.h>
};


int  select(int s, fd_set * a, fd_set * b, fd_set * c,
         struct timeval *d)
{
	return wii::net_select(s,a,b,c,d);
}
