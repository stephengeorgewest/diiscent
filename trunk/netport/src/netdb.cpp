/* socket.cpp

   netport
*/

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

namespace wii
{
#include <network.h>
};

struct hostent *gethostbyname(const char *addrString)
{
	return (struct hostent   *)wii::net_gethostbyname((char *)addrString);
}

struct servent   *getservbyname(const char *, const char *)
{
	return 0;
}
