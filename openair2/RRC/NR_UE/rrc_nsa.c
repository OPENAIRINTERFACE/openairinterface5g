#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common/utils/LOG/log.h"
#include "openair2/RRC/NR_UE/rrc_proto.h"

static const char nsa_ipaddr[] = "127.0.0.1";
static int from_lte_ue_fd = -1;
static int to_lte_ue_fd = -1;
uint16_t ue_id_g;

int get_from_lte_ue_fd()
{
    return from_lte_ue_fd;
}

void nsa_sendmsg_to_lte_ue(const void *message, size_t msg_len, Rrc_Msg_Type_t msg_type)
{
    LOG_I(NR_RRC, "Entered %s \n", __FUNCTION__);
    nsa_msg_t n_msg;
    if (msg_len > sizeof(n_msg.msg_buffer))
    {
        LOG_E(NR_RRC, "%s: message too big: %zu\n", __func__, msg_len);
        abort();
    }
    n_msg.msg_type = msg_type;
    memcpy(n_msg.msg_buffer, message, msg_len);
    size_t to_send = sizeof(n_msg.msg_type) + msg_len;

    struct sockaddr_in sa =
    {
        .sin_family = AF_INET,
        .sin_port = htons(6007 + ue_id_g * 2),
    };
    int sent = sendto(from_lte_ue_fd, &n_msg, to_send, 0,
                      (struct sockaddr *)&sa, sizeof(sa));
    if (sent == -1)
    {
        LOG_E(NR_RRC, "%s: sendto: %s\n", __func__, strerror(errno));
        return;
    }
    if (sent != to_send)
    {
        LOG_E(NR_RRC, "%s: Short send %d != %zu\n", __func__, sent, to_send);
        return;
    }
    LOG_D(NR_RRC, "Sent a %d message to the LTE UE (%d bytes) \n", msg_type, sent);
}

void init_connections_with_lte_ue(void)
{
    struct sockaddr_in sa =
    {
        .sin_family = AF_INET,
        .sin_port = htons(6008 + ue_id_g * 2),
    };
    AssertFatal(from_lte_ue_fd == -1, "from_lte_ue_fd %d was assigned already", from_lte_ue_fd);
    from_lte_ue_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (from_lte_ue_fd == -1)
    {
        LOG_E(NR_RRC, "%s: Error opening socket %d (%d:%s)\n", __FUNCTION__, from_lte_ue_fd, errno, strerror(errno));
        abort();
    }

    if (inet_aton(nsa_ipaddr, &sa.sin_addr) == 0)
    {
        LOG_E(NR_RRC, "Bad nsa_ipaddr '%s'\n", nsa_ipaddr);
        abort();
    }

    if (bind(from_lte_ue_fd, (struct sockaddr *) &sa, sizeof(sa)) == -1)
    {
        LOG_E(NR_RRC,"%s: Failed to bind the socket\n", __FUNCTION__);
        abort();
    }

    AssertFatal(to_lte_ue_fd == -1, "to_lte_ue_fd was assigned already");
    to_lte_ue_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (to_lte_ue_fd == -1)
    {
        LOG_E(NR_RRC, "%s: Error opening socket %d (%d:%s)\n", __FUNCTION__, to_lte_ue_fd, errno, strerror(errno));
        abort();
    }
    LOG_I(NR_RRC, "Started LTE-NR link in the nr-UE\n");
}
