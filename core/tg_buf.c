#include <stdlib.h>
#include <string.h>

#include "tg_buf.h"

uint32_t tg_le32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

int64_t tg_le64(const unsigned char *p)
{
    return (int64_t)((uint64_t)tg_le32(p) | ((uint64_t)tg_le32(p + 4) << 32));
}

void tg_put_le32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

void tg_put_le64(unsigned char *p, uint64_t v)
{
    tg_put_le32(p, (uint32_t)v);
    tg_put_le32(p + 4, (uint32_t)(v >> 32));
}

void tg_buf_init(tg_buf *b)
{
    b->p = NULL;
    b->len = 0;
    b->cap = 0;
    b->oom = 0;
}

void tg_buf_free(tg_buf *b)
{
    free(b->p);
    tg_buf_init(b);
}

static int grow(tg_buf *b, int need)
{
    int cap;
    unsigned char *np;
    if (b->oom)
        return 0;
    if (b->len + need <= b->cap)
        return 1;
    cap = b->cap ? b->cap : 256;
    while (cap < b->len + need)
        cap *= 2;
    np = (unsigned char *)realloc(b->p, cap);
    if (!np) {
        b->oom = 1;
        return 0;
    }
    b->p = np;
    b->cap = cap;
    return 1;
}

void tg_buf_put(tg_buf *b, const void *data, int n)
{
    if (n <= 0 || !grow(b, n))
        return;
    memcpy(b->p + b->len, data, n);
    b->len += n;
}

void tg_buf_u32(tg_buf *b, uint32_t v)
{
    unsigned char t[4];
    tg_put_le32(t, v);
    tg_buf_put(b, t, 4);
}

void tg_buf_i32(tg_buf *b, int32_t v)
{
    tg_buf_u32(b, (uint32_t)v);
}

void tg_buf_i64(tg_buf *b, int64_t v)
{
    unsigned char t[8];
    tg_put_le64(t, (uint64_t)v);
    tg_buf_put(b, t, 8);
}

void tg_buf_bytes(tg_buf *b, const void *data, int n)
{
    static const unsigned char zeros[4] = {0, 0, 0, 0};
    unsigned char h[4];
    int total;
    if (n <= 253) {
        h[0] = (unsigned char)n;
        tg_buf_put(b, h, 1);
        total = 1 + n;
    } else {
        h[0] = 254;
        h[1] = (unsigned char)n;
        h[2] = (unsigned char)(n >> 8);
        h[3] = (unsigned char)(n >> 16);
        tg_buf_put(b, h, 4);
        total = 4 + n;
    }
    tg_buf_put(b, data, n);
    if (total % 4)
        tg_buf_put(b, zeros, 4 - total % 4);
}

void tg_buf_str(tg_buf *b, const char *s)
{
    tg_buf_bytes(b, s, (int)strlen(s));
}

void tg_rd_init(tg_rd *r, const void *p, int len)
{
    r->p = (const unsigned char *)p;
    r->len = len;
    r->pos = 0;
    r->err = 0;
}

const unsigned char *tg_rd_raw(tg_rd *r, int n)
{
    const unsigned char *q;
    if (r->err || n < 0 || r->pos + n > r->len) {
        r->err = 1;
        return NULL;
    }
    q = r->p + r->pos;
    r->pos += n;
    return q;
}

uint32_t tg_rd_u32(tg_rd *r)
{
    const unsigned char *q = tg_rd_raw(r, 4);
    return q ? tg_le32(q) : 0;
}

int32_t tg_rd_i32(tg_rd *r)
{
    return (int32_t)tg_rd_u32(r);
}

int64_t tg_rd_i64(tg_rd *r)
{
    const unsigned char *q = tg_rd_raw(r, 8);
    return q ? tg_le64(q) : 0;
}

double tg_rd_double(tg_rd *r)
{
    double d = 0;
    uint64_t v;
    const unsigned char *q = tg_rd_raw(r, 8);
    if (q) {
        v = (uint64_t)tg_le64(q);
        memcpy(&d, &v, 8);
    }
    return d;
}

const unsigned char *tg_rd_bytes(tg_rd *r, int *n)
{
    const unsigned char *h, *q;
    int len, consumed;
    *n = 0;
    h = tg_rd_raw(r, 1);
    if (!h)
        return NULL;
    if (h[0] <= 253) {
        len = h[0];
        consumed = 1;
    } else {
        h = tg_rd_raw(r, 3);
        if (!h)
            return NULL;
        len = h[0] | (h[1] << 8) | (h[2] << 16);
        consumed = 4;
    }
    q = tg_rd_raw(r, len);
    if (!q)
        return NULL;
    consumed += len;
    if (consumed % 4 && !tg_rd_raw(r, 4 - consumed % 4))
        return NULL;
    *n = len;
    return q;
}

#define ARENA_BLOCK 16384
#define ARENA_HDR ((int)((sizeof(tg_arena_blk) + 7) & ~7u))

struct tg_arena_blk {
    tg_arena_blk *next;
    int used;
    int cap;
};

void tg_arena_init(tg_arena *a)
{
    a->head = NULL;
    a->oom = 0;
}

void *tg_arena_alloc(tg_arena *a, int n)
{
    tg_arena_blk *b = a->head;
    unsigned char *p;
    n = (n + 7) & ~7;
    if (!b || b->used + n > b->cap) {
        int cap = n > ARENA_BLOCK ? n : ARENA_BLOCK;
        b = (tg_arena_blk *)malloc(ARENA_HDR + cap);
        if (!b) {
            a->oom = 1;
            return NULL;
        }
        b->next = a->head;
        b->used = 0;
        b->cap = cap;
        a->head = b;
    }
    p = (unsigned char *)b + ARENA_HDR + b->used;
    b->used += n;
    memset(p, 0, n);
    return p;
}

void tg_arena_free(tg_arena *a)
{
    tg_arena_blk *b = a->head;
    while (b) {
        tg_arena_blk *next = b->next;
        free(b);
        b = next;
    }
    tg_arena_init(a);
}
