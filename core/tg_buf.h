#ifndef TG_BUF_H
#define TG_BUF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char *p;
    int len;
    int cap;
    int oom;
} tg_buf;

void tg_buf_init(tg_buf *b);
void tg_buf_free(tg_buf *b);
void tg_buf_put(tg_buf *b, const void *data, int n);
void tg_buf_u32(tg_buf *b, uint32_t v);
void tg_buf_i32(tg_buf *b, int32_t v);
void tg_buf_i64(tg_buf *b, int64_t v);
void tg_buf_bytes(tg_buf *b, const void *data, int n);
void tg_buf_str(tg_buf *b, const char *s);

typedef struct {
    const unsigned char *p;
    int len;
    int pos;
    int err;
} tg_rd;

void tg_rd_init(tg_rd *r, const void *p, int len);
const unsigned char *tg_rd_raw(tg_rd *r, int n);
uint32_t tg_rd_u32(tg_rd *r);
int32_t tg_rd_i32(tg_rd *r);
int64_t tg_rd_i64(tg_rd *r);
double tg_rd_double(tg_rd *r);
const unsigned char *tg_rd_bytes(tg_rd *r, int *n);

uint32_t tg_le32(const unsigned char *p);
int64_t tg_le64(const unsigned char *p);
void tg_put_le32(unsigned char *p, uint32_t v);
void tg_put_le64(unsigned char *p, uint64_t v);

typedef struct tg_arena_blk tg_arena_blk;
typedef struct {
    tg_arena_blk *head;
    int oom;
} tg_arena;

void tg_arena_init(tg_arena *a);
void *tg_arena_alloc(tg_arena *a, int n);
void tg_arena_free(tg_arena *a);

#ifdef __cplusplus
}
#endif

#endif
