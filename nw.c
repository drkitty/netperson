#include <linux/netlink.h>
#include <netlink/socket.h>

int main()
{
    struct nl_sock* sock = nl_socket_alloc();
    nl_socket_free(sock);
    return 0;
}
