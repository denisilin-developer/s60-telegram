#include <string.h>

#include "tg_qr.h"

static const short kTotal[11] = {0, 26, 44, 70, 100, 134, 172, 196, 242, 292, 346};
static const unsigned char kEcc[11] = {0, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18};
static const unsigned char kBlocks[11] = {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4};

typedef struct {
    int size;
    unsigned char *m;
    unsigned char fn[TG_QR_MAX * TG_QR_MAX];
} qr;

static void set_fn(qr *q, int x, int y, int dark)
{
    q->m[y * q->size + x] = (unsigned char)dark;
    q->fn[y * q->size + x] = 1;
}

static int gf_mul(int x, int y)
{
    int z = 0, i;
    for (i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >> 7) * 0x11d);
        z ^= ((y >> i) & 1) * x;
    }
    return z & 0xff;
}

static void rs_ecc(const unsigned char *data, int n, int degree, unsigned char *out)
{
    unsigned char div[30];
    int i, j, root = 1;
    memset(div, 0, sizeof div);
    div[degree - 1] = 1;
    for (i = 0; i < degree; i++) {
        for (j = 0; j < degree; j++) {
            div[j] = (unsigned char)gf_mul(div[j], root);
            if (j + 1 < degree)
                div[j] ^= div[j + 1];
        }
        root = gf_mul(root, 2);
    }
    memset(out, 0, degree);
    for (i = 0; i < n; i++) {
        int f = data[i] ^ out[0];
        memmove(out, out + 1, degree - 1);
        out[degree - 1] = 0;
        for (j = 0; j < degree; j++)
            out[j] ^= (unsigned char)gf_mul(div[j], f);
    }
}

static void finder(qr *q, int cx, int cy)
{
    int dx, dy;
    for (dy = -4; dy <= 4; dy++)
        for (dx = -4; dx <= 4; dx++) {
            int x = cx + dx, y = cy + dy;
            int d = dx < 0 ? -dx : dx, e = dy < 0 ? -dy : dy;
            if (e > d)
                d = e;
            if (x >= 0 && x < q->size && y >= 0 && y < q->size)
                set_fn(q, x, y, d != 2 && d != 4);
        }
}

static void format_bits(qr *q, int mask)
{
    int data = (1 << 3) | mask, rem = data, bits, i, s = q->size;
    for (i = 0; i < 10; i++)
        rem = (rem << 1) ^ ((rem >> 9) * 0x537);
    bits = ((data << 10) | rem) ^ 0x5412;
    for (i = 0; i <= 5; i++)
        set_fn(q, 8, i, (bits >> i) & 1);
    set_fn(q, 8, 7, (bits >> 6) & 1);
    set_fn(q, 8, 8, (bits >> 7) & 1);
    set_fn(q, 7, 8, (bits >> 8) & 1);
    for (i = 9; i < 15; i++)
        set_fn(q, 14 - i, 8, (bits >> i) & 1);
    for (i = 0; i < 8; i++)
        set_fn(q, s - 1 - i, 8, (bits >> i) & 1);
    for (i = 8; i < 15; i++)
        set_fn(q, 8, s - 15 + i, (bits >> i) & 1);
    set_fn(q, 8, s - 8, 1);
}

int tg_qr_encode(const char *text, unsigned char out[TG_QR_MAX * TG_QR_MAX])
{
    static unsigned char data[346], all[346];
    qr q;
    int len = (int)strlen(text), v, ndata = 0, bits = 0, i, j, k, x, y, s;
    int nblocks, ecc, short_len, nshort, pos[7], npos = 0;

    for (v = 1; v <= 10; v++) {
        ndata = kTotal[v] - kEcc[v] * kBlocks[v];
        if (4 + (v < 10 ? 8 : 16) + 8 * len <= 8 * ndata)
            break;
    }
    if (v > 10)
        return 0;

    memset(data, 0, sizeof data);
#define PUT(val, n)                                                    \
    for (k = (n) - 1; k >= 0; k--, bits++)                             \
        data[bits >> 3] |= (unsigned char)((((val) >> k) & 1) << (7 - (bits & 7)));
    PUT(4, 4);
    PUT(len, v < 10 ? 8 : 16);
    for (i = 0; i < len; i++)
        PUT((unsigned char)text[i], 8);
#undef PUT
    bits += 8 * ndata - bits < 4 ? 8 * ndata - bits : 4;
    for (i = (bits + 7) / 8, j = 0; i < ndata; i++, j++)
        data[i] = j & 1 ? 0x11 : 0xec;

    nblocks = kBlocks[v];
    ecc = kEcc[v];
    nshort = nblocks - kTotal[v] % nblocks;
    short_len = kTotal[v] / nblocks;
    {
        unsigned char blk[4][150];
        int dlen[4], off = 0, n = 0;
        for (i = 0; i < nblocks; i++) {
            dlen[i] = short_len - ecc + (i >= nshort);
            memcpy(blk[i], data + off, dlen[i]);
            rs_ecc(data + off, dlen[i], ecc, blk[i] + dlen[i]);
            off += dlen[i];
        }
        for (i = 0; i <= short_len - ecc; i++)
            for (j = 0; j < nblocks; j++)
                if (i < dlen[j])
                    all[n++] = blk[j][i];
        for (i = 0; i < ecc; i++)
            for (j = 0; j < nblocks; j++)
                all[n++] = blk[j][dlen[j] + i];
    }

    s = q.size = 17 + 4 * v;
    q.m = out;
    memset(out, 0, s * s);
    memset(q.fn, 0, sizeof q.fn);
    for (i = 0; i < s; i++) {
        set_fn(&q, 6, i, i % 2 == 0);
        set_fn(&q, i, 6, i % 2 == 0);
    }
    finder(&q, 3, 3);
    finder(&q, s - 4, 3);
    finder(&q, 3, s - 4);
    if (v > 1) {
        int na = v / 7 + 2, step = (v * 8 + na * 3 + 5) / (na * 4 - 4) * 2;
        pos[0] = 6;
        for (i = na - 1, k = s - 7; i >= 1; i--, k -= step)
            pos[i] = k;
        npos = na;
    }
    for (i = 0; i < npos; i++)
        for (j = 0; j < npos; j++) {
            int dx, dy;
            if ((i == 0 && j == 0) || (i == 0 && j == npos - 1) || (i == npos - 1 && j == 0))
                continue;
            for (dy = -2; dy <= 2; dy++)
                for (dx = -2; dx <= 2; dx++) {
                    int d = dx < 0 ? -dx : dx, e = dy < 0 ? -dy : dy;
                    set_fn(&q, pos[i] + dx, pos[j] + dy, (d > e ? d : e) != 1);
                }
        }
    format_bits(&q, 0);
    if (v >= 7) {
        int rem = v, vb;
        for (i = 0; i < 12; i++)
            rem = (rem << 1) ^ ((rem >> 11) * 0x1f25);
        vb = (v << 12) | rem;
        for (i = 0; i < 18; i++) {
            int bit = (vb >> i) & 1, a = s - 11 + i % 3, b = i / 3;
            set_fn(&q, a, b, bit);
            set_fn(&q, b, a, bit);
        }
    }

    k = 0;
    for (x = s - 1; x >= 1; x -= 2) {
        if (x == 6)
            x = 5;
        for (i = 0; i < s; i++)
            for (j = 0; j < 2; j++) {
                int cx = x - j, up = ((x + 1) & 2) == 0;
                y = up ? s - 1 - i : i;
                if (!q.fn[y * s + cx] && k < kTotal[v] * 8) {
                    out[y * s + cx] = (all[k >> 3] >> (7 - (k & 7))) & 1;
                    k++;
                }
            }
    }
    for (y = 0; y < s; y++)
        for (x = 0; x < s; x++)
            if (!q.fn[y * s + x] && (x + y) % 2 == 0)
                out[y * s + x] ^= 1;
    return s;
}
