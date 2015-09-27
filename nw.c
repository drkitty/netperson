#define _XOPEN_SOURCE 600

#include "fail.h"

#include <asm/types.h>
#include <inttypes.h>
#include <linux/netlink.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#include <netlink/msg.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>

#define SPACE_WITH_IFI NLMSG_SPACE(sizeof(struct ifinfomsg))


const char* progname;

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
            print("unknown");
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
    progname = argv[0];

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
        struct nlmsghdr* mhdr = malloc(msg_size);
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
        *mifi = (struct ifinfomsg){
            .ifi_family = AF_UNSPEC,
            .ifi_type = 300,
            .ifi_index = 129,
            .ifi_flags = 33,
            .ifi_change = 1,
        };

        if (!NLMSG_OK(mhdr, mhdr->nlmsg_len))
            fatal(1, "Invalid message generated");

        if (verbosity >= 2) {
            print("Sending {");
            print_nlmsghdr(mhdr);
            print("}\n");
        }
        ssize_t count = send(sock, mhdr, mhdr->nlmsg_len, 0);
        if (count == -1)
            fatal_e(1, "Couldn't send message to kernel");
        v2("Sent %zd bytes of data", count);

        while (true) {
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

            print_nlmsghdr(mhdr);
            putchar('\n');

            if (mhdr->nlmsg_type == NLMSG_DONE) {
                break;
                /*
                 *mhdr->nlmsg_type = NLMSG_ERROR;
                 *mhdr->nlmsg_len = NLMSG_LENGTH(sizeof(struct nlmsgerr));
                 *((struct nlmsgerr*)mdata)->error = 0;
                 *ssize_t count = send(sock, mhdr, mhdr->nlmsg_len, 0);
                 *if (count == -1)
                 *    fatal_e(1, "Couldn't send ACK to kernel");
                 *v2("ACKed with %zd bytes of data", count);
                 */

            }

            struct rtattr* ahdr =
                (struct rtattr*)((char*)mhdr + SPACE_WITH_IFI);
            unsigned int arem = mhdr->nlmsg_len - SPACE_WITH_IFI;
            printf("arem = %u\n", arem);
            printf("%u:\n", mifi->ifi_index);

            for (/* */; RTA_OK(ahdr, arem); ahdr = RTA_NEXT(ahdr, arem)) {

                switch (ahdr->rta_type) {
                    case IFLA_UNSPEC:
                        printf("  unspecified (%hu)\n", ahdr->rta_type);
                        break;
                    case IFLA_ADDRESS:
                        print("  interface L2 address: ");
                        print_hardware_address(RTA_DATA(ahdr));
                        putchar('\n');
                        break;
                    case IFLA_BROADCAST:
                        print("  L2 broadcast address: ");
                        print_hardware_address(RTA_DATA(ahdr));
                        putchar('\n');
                        break;
                    case IFLA_IFNAME:
                        printf("  device name: \"%s\"\n",
                            (const char*)RTA_DATA(ahdr));
                        break;
                    case IFLA_MTU:
                        printf("  MTU: %u\n", *(unsigned int*)RTA_DATA(ahdr));
                        break;
                    case IFLA_LINK:
                        printf("  link type: %d\n", *(int*)RTA_DATA(ahdr));
                        break;
                    case IFLA_QDISC:
                        printf("  queueing discipline: \"%s\"\n",
                            (const char*)RTA_DATA(ahdr));
                        break;
                    case IFLA_STATS:
                        puts("  interface statistics");
                        break;
                    default:
                        printf("  unknown (%hu)\n", ahdr->rta_type);
                }
            }
        }

        free(mhdr);
    }

    return 0;
}
