#ifndef TG_NET_H
#define TG_NET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tg_conn tg_conn;

const char *tg_dc_host(int dc_id, int test);
tg_conn *tg_conn_open(const char *host, int port, int *err);
int tg_conn_send(tg_conn *c, const unsigned char *payload, int len);
int tg_conn_recv(tg_conn *c, unsigned char **frame, int *len);
void tg_conn_close(tg_conn *c);

#ifdef __cplusplus
}
#endif

#endif
