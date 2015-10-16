#define _XOPEN_SOURCE 600

#include "fail.h"

#include <asm/types.h>
#include <inttypes.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <linux/if.h>

#define SPACE_WITH_IFI NLMSG_SPACE(sizeof(struct ifinfomsg))


const int verbosity = 2;


void print_hardware_address(uint8_t* addr)
{
    for (int i = 0; i <= 5; ++i) {
        printf("%02"PRIX8, addr[i]);
        if (i != 5)
            putchar(':');
    }
}


void print_nlmsghdr(struct nlmsghdr* mhdr)
{
    printf("len = %"PRIu32", type = ", mhdr->nlmsg_len);
    switch (mhdr->nlmsg_type) {
        case NLMSG_NOOP:
            print("NOOP");
            break;
        case NLMSG_ERROR:
            print("ERROR");
            break;
        case NLMSG_DONE:
            print("DONE");
            break;
        default:
            print("???");
            break;
    }

    print(", flags =");
    bool flags = false;
    if (mhdr->nlmsg_flags & NLM_F_REQUEST) {
        print(" REQUEST");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_MULTI) {
        print(" MULTI");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_ACK) {
        print(" ACK");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_ECHO) {
        print(" ECHO");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_ROOT) {
        print(" ROOT");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_MATCH) {
        print(" MATCH");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_ATOMIC) {
        print(" ATOMIC");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_DUMP) {
        print(" DUMP");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_REPLACE) {
        print(" REPLACE");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_EXCL) {
        print(" EXCL");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_CREATE) {
        print(" CREATE");
        flags = true;
    }
    if (mhdr->nlmsg_flags & NLM_F_APPEND) {
        print(" APPEND");
        flags = true;
    }
    if (!flags)
        print(" (none)");

    printf(", seq = %"PRIu32", pid = %"PRIu32,
        mhdr->nlmsg_seq, mhdr->nlmsg_pid);
}


int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    int sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (sock == -1)
        fatal_e(1, "Can't create socket");

    {
        struct sockaddr_nl addr = {
            .nl_family = AF_NETLINK,
            .nl_pad = 0,
            .nl_pid = 0, // kernel
            .nl_groups = 0,
        };
        if (-1 == connect(sock, (const struct sockaddr*)&addr, sizeof(addr)))
            fatal_e(1, "Can't connect to kernel");
    }

    {
        ssize_t msg_size = (ssize_t)NLMSG_SPACE(sizeof(struct ifinfomsg));
        void* msg_start = malloc(msg_size);
        struct nlmsghdr* mhdr = msg_start;
        void* mdata = NLMSG_DATA(mhdr);
        struct ifinfomsg* mifi = mdata;
        /*
         *printf(
         *    "NLMSG_LENGTH(...) = %d, NLMSG_SPACE(...) = %d, "
         *    "NLMSG_ALIGN(NLMSG_LENGTH(...)) = %d\n",
         *    NLMSG_LENGTH(9),
         *    NLMSG_SPACE(9),
         *    NLMSG_ALIGN(NLMSG_LENGTH(9)));
         */
        *mhdr = (struct nlmsghdr){
            .nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
            .nlmsg_type = RTM_GETLINK,
            .nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT,
            .nlmsg_seq = 0,
            .nlmsg_pid = 0,
        };

        if (!NLMSG_OK(mhdr, mhdr->nlmsg_len))
            fatal(1, "Invalid message generated");

        if (verbosity >= 2) {
            print("TX {");
            print_nlmsghdr(mhdr);
            print("}\n");
        }
        ssize_t count = send(sock, mhdr, mhdr->nlmsg_len, 0);
        if (count == -1)
            fatal_e(1, "Couldn't send message to kernel");
        v2("Sent %zd bytes of data", count);

        count = recv(sock, mhdr, 0, MSG_PEEK | MSG_TRUNC);
        if (count == -1) {
            fatal_e(1, "Couldn't peek at message size");
        } else if (count >= msg_size) {
            v2("Resizing msg buffer to %zd", count);
            msg_size = count;
            mhdr = realloc(mhdr, msg_size);
        }

        count = recv(sock, mhdr, count, 0);
        if (count == -1)
            fatal_e(1, "Couldn't receive message from kernel");
        else if (count == 0)
            fatal(1, "Kernel socket shut down");

        int mrem = count;

        for (/* */; mhdr->nlmsg_type != NLMSG_DONE && NLMSG_OK(mhdr, mrem);
                mhdr = NLMSG_NEXT(mhdr, mrem)) {
            print("RX {");
            print_nlmsghdr(mhdr);
            print("}\n");

            mifi = NLMSG_DATA(mhdr);
            struct rtattr* ahdr =
                (struct rtattr*)((char*)mhdr + SPACE_WITH_IFI);
            unsigned int arem = mhdr->nlmsg_len - SPACE_WITH_IFI;
            printf("arem = %u\n", arem);
            printf("%u:\n", mifi->ifi_index);

            for (/* */; RTA_OK(ahdr, arem); ahdr = RTA_NEXT(ahdr, arem)) {
                void* adata = RTA_DATA(ahdr);

                switch (ahdr->rta_type) {
                    case IFLA_UNSPEC:
                        printf("  unspecified (%hu)\n", ahdr->rta_type);
                        break;
                    case IFLA_IFNAME:
                        printf("  device name: \"%s\"\n", (char*)adata);
                        break;
                    case IFLA_ADDRESS:
                        print("  interface L2 address: ");
                        print_hardware_address(adata);
                        putchar('\n');
                        break;
                    case IFLA_BROADCAST:
                        print("  L2 broadcast address: ");
                        print_hardware_address(adata);
                        putchar('\n');
                        break;
                    case IFLA_MAP:
                        print("  map (?)\n");
                        break;
                    case IFLA_MTU:
                        printf("  MTU: %u\n", *(unsigned int*)adata);
                        break;
                    case IFLA_LINK:
                        printf("  link type: %u\n", *(unsigned int*)adata);
                        break;
                    case IFLA_MASTER:
                        printf("  master (?): %u\n",
                            *(unsigned int*)adata);
                        break;
                    case IFLA_CARRIER:
                        printf("  carrier (?): %hhu\n",
                            *(unsigned char*)adata);
                        break;
                    case IFLA_TXQLEN:
                        printf("  TXQLEN (?): %u\n", *(unsigned int*)adata);
                        break;
                    case IFLA_WEIGHT:
                        printf("  weight: %hhu\n", *(unsigned int*)adata);
                        break;
                    case IFLA_OPERSTATE:
                        print("  operational state: ");
                        switch (*(unsigned int*)adata) {
                            case IF_OPER_UNKNOWN:
                                print("unknown"); break;
                            case IF_OPER_NOTPRESENT:
                                print("not present"); break;
                            case IF_OPER_DOWN:
                                print("down"); break;
                            case IF_OPER_LOWERLAYERDOWN:
                                print("lower layer down"); break;
                            case IF_OPER_TESTING:
                                print("testing"); break;
                            case IF_OPER_DORMANT:
                                print("dormant"); break;
                            case IF_OPER_UP:
                                print("up"); break;
                            default:
                                print("???"); break;
                        }
                        putchar('\n');
                        break;
                    case IFLA_LINKMODE:
                        printf("  mode: %hhu\n", *(unsigned char*)adata);
                        break;
                    case IFLA_LINKINFO:
                        print("  link info (?)\n");
                        break;
                    case IFLA_NET_NS_PID:
                        print("  NET_NS_PID (?)\n");
                        break;
                    case IFLA_NET_NS_FD:
                        print("  NET_NS_FD (?)\n");
                        break;
                    case IFLA_IFALIAS:
                        printf("  alias: \"%s\"\n", (char*)adata);
                        break;
                    case IFLA_VFINFO_LIST:
                        print("  VFINFO_LIST (?)\n");
                        break;
                    case IFLA_VF_PORTS:
                        print("  VF_PORTS (?)\n");
                        break;
                    case IFLA_PORT_SELF:
                        print("  PORT_SELF (?)\n");
                        break;
                    case IFLA_AF_SPEC:
                        print("  AF_SPEC\n");
                        break;
                    case IFLA_EXT_MASK:
                        printf("  EXT_MASK (?): %u\n", *(unsigned int*)adata);
                        break;
                    case IFLA_PROMISCUITY:
                        printf("  promiscuity: %u\n", *(unsigned int*)adata);
                        break;
                    case IFLA_NUM_TX_QUEUES:
                        printf("  NUM_TX_QUEUES (?): %u\n",
                            *(unsigned int*)adata);
                        break;
                    case IFLA_NUM_RX_QUEUES:
                        printf("  NUM_RX_QUEUES (?): %u\n",
                            *(unsigned int*)adata);
                        break;
                    case IFLA_PHYS_PORT_ID:
                        print("  PHYS_PORT_ID (?)\n");
                        break;
                    case IFLA_QDISC:
                        printf("  queueing discipline: \"%s\"\n",
                            (const char*)adata);
                        break;
                    case IFLA_STATS:
                        puts("  interface statistics");
                        break;
                    default:
                        printf("  ??? (%hu)\n", ahdr->rta_type);
                        break;
                }
            }
        }

        print_nlmsghdr(mhdr);
        putchar('\n');

        free(msg_start);
    }

    return 0;
}
