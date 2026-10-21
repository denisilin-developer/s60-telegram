#ifndef TG_MTPROTO_H
#define TG_MTPROTO_H

#include <stdint.h>

#include "tg_net.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TG_OK 0
#define TG_ERR_NET (-1)
#define TG_ERR_PROTO (-2)
#define TG_ERR_RPC (-3)
#define TG_ERR_NOMEM (-4)

typedef struct {
    tg_conn *conn;
    unsigned char auth_key[256];
    unsigned char auth_key_id[8];
    unsigned char salt[8];
    unsigned char session_id[8];
    int time_offset;
    int seq;
    int new_session;
    int64_t acks[32];
    int nacks;
    int rpc_code;
    char rpc_msg[128];
    char err[160];
} tg_session;

int tg_handshake(tg_conn *c, int dc_id, int test, unsigned char auth_key[256],
                 unsigned char salt[8], int *time_offset, char *err, int errlen);

int tg_session_init(tg_session *s, tg_conn *c, const unsigned char auth_key[256],
                    const unsigned char salt[8], int time_offset);
int tg_call(tg_session *s, const unsigned char *body, int len, unsigned char **result,
            int *rlen);
int tg_send_acks(tg_session *s);

#ifdef __cplusplus
}
#endif

#endif
