/* socket.cpp

   netport
*/

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>

namespace wii
{
#include <network.h>
};

int32_t ioctl(int32_t s, int32_t cmd, void *argp)
{
	return wii::net_ioctl(s, cmd, argp);
}
