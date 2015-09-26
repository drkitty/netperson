#define _XOPEN_SOURCE 600

#include <linux/netlink.h>
#include <inttypes.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>

#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>

#define print(x) fputs((x), stdout)


void print_hardware_address(uint8_t* addr)
{
    for (int i = 0; i <= 5; ++i) {
        printf("%02"PRIX8, addr[i]);
        if (i != 5)
            putchar(':');
    }
}


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
                    print("  interface L2 address: ");
                    print_hardware_address(nla_data(ahdr));
                    putchar('\n');
                    break;
                case IFLA_BROADCAST:
                    print("  L2 broadcast address: ");
                    print_hardware_address(nla_data(ahdr));
                    putchar('\n');
                    break;
                case IFLA_IFNAME:
                    printf("  device name: \"%s\"\n",
                        (const char*)nla_data(ahdr));
                    break;
                case IFLA_MTU:
                    printf("  MTU: %u\n", *(unsigned int*)nla_data(ahdr));
                    break;
                case IFLA_LINK:
                    printf("  link type: %d\n", *(int*)nla_data(ahdr));
                    break;
                case IFLA_QDISC:
                    printf("  queueing discipline: \"%s\"\n",
                        (const char*)nla_data(ahdr));
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
