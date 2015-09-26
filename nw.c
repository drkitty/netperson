#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <linux/netlink.h>
#include <netdb.h>

#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>


int handle_msg(struct nl_msg* msg, void* arg)
{
    (void)arg;

    struct nlmsghdr* mhdr = nlmsg_hdr(msg);
    struct ifinfomsg* mdata = nlmsg_data(mhdr);
    printf("%u:\n", mdata->ifi_index);

    {
        struct nlattr* ahdr = nlmsg_attrdata(mhdr, sizeof(*mdata));
        int arem = nlmsg_attrlen(mhdr, sizeof(*mdata));

        for (/* */; nla_ok(ahdr, arem); ahdr = nla_next(ahdr, &arem)) {
            switch (nla_type(ahdr)) {
                case IFLA_UNSPEC:
                    puts("  unspecified");
                    break;
                case IFLA_ADDRESS:
                    puts("  interface L2 address");
                    break;
                case IFLA_BROADCAST:
                    puts("  L2 broadcast address");
                    break;
                case IFLA_IFNAME:
                    puts("  device name");
                    break;
                case IFLA_MTU:
                    puts("  MTU");
                    break;
                case IFLA_LINK:
                    puts("  link type");
                    break;
                case IFLA_QDISC:
                    puts("  queueing discipline");
                    break;
                case IFLA_STATS:
                    puts("  interface statistics");
                    break;
            }
        }
    }

    return NL_OK;
}


int main()
{
    struct nl_sock* sock = nl_socket_alloc();
    nl_connect(sock, NETLINK_ROUTE);
    {
        struct nl_cb* cb = nl_cb_alloc(NL_CB_VERBOSE);
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handle_msg, NULL);
        nl_socket_set_cb(sock, cb);
    }

    struct ifinfomsg ifi = {
        .ifi_family = AF_UNSPEC,
        .ifi_change = 0xFFFFFFFF,
    };

    nl_send_simple(sock, RTM_GETLINK, NLM_F_DUMP, &ifi, sizeof(ifi));
    nl_recvmsgs_default(sock);
    nl_socket_free(sock);
    return 0;
}
