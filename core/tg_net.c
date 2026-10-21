#include <stdlib.h>
#include <string.h>

#include "rsym_tcp.h"
#include "tg_net.h"

#define TG_TIMEOUT_MS 30000
#define TG_MAX_FRAME (8 * 1024 * 1024)

struct tg_conn {
    rsym_tcp *t;
};

const char *tg_dc_host(int dc_id, int test)
{
    static const char *prod[] = {"149.154.175.53", "149.154.167.51", "149.154.175.100",
                                 "149.154.167.91", "91.108.56.130"};
    static const char *testdc[] = {"149.154.175.10", "149.154.167.40", "149.154.175.117"};
    if (test)
        return dc_id >= 1 && dc_id <= 3 ? testdc[dc_id - 1] : NULL;
    return dc_id >= 1 && dc_id <= 5 ? prod[dc_id - 1] : NULL;
}

tg_conn *tg_conn_open(const char *host, int port, int *err)
{
    unsigned char ef = 0xef;
    tg_conn *c = (tg_conn *)calloc(1, sizeof *c);
    if (!c) {
        *err = -4;
        return NULL;
    }
    c->t = rsym_tcp_connect(host, port, TG_TIMEOUT_MS, err);
    if (!c->t) {
        free(c);
        return NULL;
    }
    if (rsym_tcp_send(c->t, &ef, 1, TG_TIMEOUT_MS) != 1) {
        *err = -1;
        tg_conn_close(c);
        return NULL;
    }
    return c;
}

int tg_conn_send(tg_conn *c, const unsigned char *payload, int len)
{
    unsigned char *frame;
    int n = len / 4, head, rc;
    if (len % 4)
        return -1;
    frame = (unsigned char *)malloc(len + 4);
    if (!frame)
        return -4;
    if (n < 0x7f) {
        frame[0] = (unsigned char)n;
        head = 1;
    } else {
        frame[0] = 0x7f;
        frame[1] = (unsigned char)n;
        frame[2] = (unsigned char)(n >> 8);
        frame[3] = (unsigned char)(n >> 16);
        head = 4;
    }
    memcpy(frame + head, payload, len);
    rc = rsym_tcp_send(c->t, frame, head + len, TG_TIMEOUT_MS);
    free(frame);
    return rc == head + len ? 0 : (rc < 0 ? rc : -1);
}

static int read_exact(rsym_tcp *t, unsigned char *p, int n)
{
    int got = 0;
    while (got < n) {
        int r = rsym_tcp_recv(t, p + got, n - got, TG_TIMEOUT_MS);
        if (r <= 0)
            return r < 0 ? r : -1;
        got += r;
    }
    return 0;
}

int tg_conn_recv(tg_conn *c, unsigned char **frame, int *len)
{
    unsigned char h[3];
    int n, rc;
    *frame = NULL;
    *len = 0;
    rc = read_exact(c->t, h, 1);
    if (rc)
        return rc;
    n = h[0];
    if (n == 0x7f) {
        rc = read_exact(c->t, h, 3);
        if (rc)
            return rc;
        n = h[0] | (h[1] << 8) | (h[2] << 16);
    }
    n *= 4;
    if (n <= 0 || n > TG_MAX_FRAME)
        return -1;
    *frame = (unsigned char *)malloc(n);
    if (!*frame)
        return -4;
    rc = read_exact(c->t, *frame, n);
    if (rc) {
        free(*frame);
        *frame = NULL;
        return rc;
    }
    *len = n;
    return 0;
}

void tg_conn_close(tg_conn *c)
{
    if (!c)
        return;
    if (c->t)
        rsym_tcp_close(c->t);
    free(c);
}
