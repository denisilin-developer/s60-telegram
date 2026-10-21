#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS

#include <stdarg.h>
#include <string.h>

#include <psa/crypto.h>
#include <mbedtls/private/aes.h>
#include <mbedtls/private/bignum.h>
#include <mbedtls/private/sha1.h>
#include <mbedtls/private/sha256.h>
#include <mbedtls/private/pkcs5.h>
#include <mbedtls/md.h>

#include "tg_buf.h"
#include "tg_crypto.h"

int tg_crypto_init(void)
{
    return psa_crypto_init() == PSA_SUCCESS ? 0 : -1;
}

int tg_random(void *out, int n)
{
    return psa_generate_random((uint8_t *)out, (size_t)n) == PSA_SUCCESS ? 0 : -1;
}

void tg_sha1v(unsigned char out[20], int parts, ...)
{
    mbedtls_sha1_context c;
    va_list ap;
    int i;
    mbedtls_sha1_init(&c);
    mbedtls_sha1_starts(&c);
    va_start(ap, parts);
    for (i = 0; i < parts; i++) {
        const void *p = va_arg(ap, const void *);
        int n = va_arg(ap, int);
        mbedtls_sha1_update(&c, (const unsigned char *)p, (size_t)n);
    }
    va_end(ap);
    mbedtls_sha1_finish(&c, out);
    mbedtls_sha1_free(&c);
}

void tg_sha256v(unsigned char out[32], int parts, ...)
{
    mbedtls_sha256_context c;
    va_list ap;
    int i;
    mbedtls_sha256_init(&c);
    mbedtls_sha256_starts(&c, 0);
    va_start(ap, parts);
    for (i = 0; i < parts; i++) {
        const void *p = va_arg(ap, const void *);
        int n = va_arg(ap, int);
        mbedtls_sha256_update(&c, (const unsigned char *)p, (size_t)n);
    }
    va_end(ap);
    mbedtls_sha256_finish(&c, out);
    mbedtls_sha256_free(&c);
}

int tg_ige(int encrypt, const unsigned char key[32], const unsigned char iv[32],
           const unsigned char *in, unsigned char *out, int len)
{
    mbedtls_aes_context a;
    unsigned char pc[16], pp[16], cur[16], x[16], y[16];
    int off, i;
    if (len % 16)
        return -1;
    mbedtls_aes_init(&a);
    if (encrypt)
        mbedtls_aes_setkey_enc(&a, key, 256);
    else
        mbedtls_aes_setkey_dec(&a, key, 256);
    memcpy(pc, iv, 16);
    memcpy(pp, iv + 16, 16);
    for (off = 0; off < len; off += 16) {
        memcpy(cur, in + off, 16);
        if (encrypt) {
            for (i = 0; i < 16; i++)
                x[i] = cur[i] ^ pc[i];
            mbedtls_aes_crypt_ecb(&a, MBEDTLS_AES_ENCRYPT, x, y);
            for (i = 0; i < 16; i++)
                out[off + i] = y[i] ^ pp[i];
            memcpy(pc, out + off, 16);
            memcpy(pp, cur, 16);
        } else {
            for (i = 0; i < 16; i++)
                x[i] = cur[i] ^ pp[i];
            mbedtls_aes_crypt_ecb(&a, MBEDTLS_AES_DECRYPT, x, y);
            for (i = 0; i < 16; i++)
                out[off + i] = y[i] ^ pc[i];
            memcpy(pc, cur, 16);
            memcpy(pp, out + off, 16);
        }
    }
    mbedtls_aes_free(&a);
    return 0;
}

int tg_modexp(unsigned char *out, int out_len,
              const unsigned char *base, int base_len,
              const unsigned char *exp, int exp_len,
              const unsigned char *mod, int mod_len)
{
    mbedtls_mpi B, E, M, X;
    int rc;
    mbedtls_mpi_init(&B);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&M);
    mbedtls_mpi_init(&X);
    rc = mbedtls_mpi_read_binary(&B, base, (size_t)base_len);
    if (!rc)
        rc = mbedtls_mpi_read_binary(&E, exp, (size_t)exp_len);
    if (!rc)
        rc = mbedtls_mpi_read_binary(&M, mod, (size_t)mod_len);
    if (!rc)
        rc = mbedtls_mpi_exp_mod(&X, &B, &E, &M, NULL);
    if (!rc)
        rc = mbedtls_mpi_write_binary(&X, out, (size_t)out_len);
    mbedtls_mpi_free(&B);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&M);
    mbedtls_mpi_free(&X);
    return rc ? -1 : 0;
}

int tg_dh_in_range(const unsigned char x[256])
{
    mbedtls_mpi X, P, lo, hi, one;
    int ok = 0;
    mbedtls_mpi_init(&X);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&lo);
    mbedtls_mpi_init(&hi);
    mbedtls_mpi_init(&one);
    if (!mbedtls_mpi_read_binary(&X, x, 256) &&
        !mbedtls_mpi_read_binary(&P, tg_dh_prime, 256) &&
        !mbedtls_mpi_lset(&lo, 1) && !mbedtls_mpi_shift_l(&lo, 1984) &&
        !mbedtls_mpi_sub_mpi(&hi, &P, &lo) && !mbedtls_mpi_lset(&one, 1)) {
        ok = mbedtls_mpi_cmp_int(&X, 1) > 0 &&
             mbedtls_mpi_cmp_mpi(&lo, &X) <= 0 &&
             mbedtls_mpi_cmp_mpi(&X, &hi) <= 0;
        if (ok && !mbedtls_mpi_sub_mpi(&P, &P, &one))
            ok = mbedtls_mpi_cmp_mpi(&X, &P) < 0;
    }
    mbedtls_mpi_free(&X);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&lo);
    mbedtls_mpi_free(&hi);
    mbedtls_mpi_free(&one);
    return ok;
}

int64_t tg_rsa_fingerprint(const tg_rsa_key *k)
{
    tg_buf b;
    unsigned char h[20];
    int64_t fp;
    tg_buf_init(&b);
    tg_buf_bytes(&b, k->n, k->n_len);
    tg_buf_bytes(&b, k->e, k->e_len);
    tg_sha1v(h, 1, b.p, b.len);
    tg_buf_free(&b);
    fp = tg_le64(h + 12);
    return fp;
}

static uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m)
{
    uint64_t r = 0;
    a %= m;
    while (b) {
        if (b & 1) {
            r += a;
            if (r >= m)
                r -= m;
        }
        a <<= 1;
        if (a >= m)
            a -= m;
        b >>= 1;
    }
    return r;
}

static uint64_t gcd64(uint64_t a, uint64_t b)
{
    while (b) {
        uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static uint64_t absdiff(uint64_t a, uint64_t b)
{
    return a > b ? a - b : b - a;
}

static uint64_t rand_below(uint64_t n)
{
    uint64_t v = 0;
    tg_random(&v, sizeof v);
    return 1 + v % (n - 1);
}

int tg_factor_pq(uint64_t pq, uint64_t *p, uint64_t *q)
{
    int attempts;
    if (pq < 4)
        return -1;
    if (pq % 2 == 0) {
        *p = 2;
        *q = pq / 2;
        return 0;
    }
    for (attempts = 0; attempts < 20; attempts++) {
        uint64_t y = rand_below(pq), c = rand_below(pq);
        uint64_t m = 128, g = 1, r = 1, qq = 1, x = 0, ys = 0, i, k, lim;
        while (g == 1) {
            x = y;
            for (i = 0; i < r; i++)
                y = (mulmod(y, y, pq) + c) % pq;
            k = 0;
            while (k < r && g == 1) {
                ys = y;
                lim = m < r - k ? m : r - k;
                for (i = 0; i < lim; i++) {
                    y = (mulmod(y, y, pq) + c) % pq;
                    qq = mulmod(qq, absdiff(x, y), pq);
                }
                g = gcd64(qq, pq);
                k += m;
            }
            r *= 2;
        }
        if (g == pq) {
            g = 1;
            while (g == 1) {
                ys = (mulmod(ys, ys, pq) + c) % pq;
                g = gcd64(absdiff(x, ys), pq);
            }
        }
        if (g != pq && g != 0) {
            uint64_t other = pq / g;
            *p = g < other ? g : other;
            *q = g < other ? other : g;
            return 0;
        }
    }
    return -1;
}

void tg_srp_x(const unsigned char *password, int plen, const unsigned char *salt1, int s1len,
              const unsigned char *salt2, int s2len, unsigned char x[32])
{
    unsigned char h1[32], h2[32], kdf[64];
    tg_sha256v(h1, 3, salt1, s1len, password, plen, salt1, s1len);
    tg_sha256v(h2, 3, salt2, s2len, h1, 32, salt2, s2len);
    mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA512, h2, 32, salt1, (size_t)s1len, 100000, 64, kdf);
    tg_sha256v(x, 3, salt2, s2len, kdf, 64, salt2, s2len);
}

static int pad256(const mbedtls_mpi *v, unsigned char out[256])
{
    return mbedtls_mpi_write_binary(v, out, 256);
}

int tg_srp(const unsigned char x[32], const unsigned char *salt1, int s1len,
           const unsigned char *salt2, int s2len, int g, const unsigned char p[256],
           const unsigned char *srp_b, int blen, const unsigned char a_rand[256],
           unsigned char a_out[256], unsigned char m1[32])
{
    mbedtls_mpi P, G, X, A, B, K, U, V, KV, T, E, S, a;
    unsigned char gpad[256], bpad[256], k[32], u[32], spad[256], ka[32], hp[32], hg[32], hs1[32],
        hs2[32];
    int rc, i;
    if (blen <= 0 || blen > 256)
        return -1;
    memset(gpad, 0, sizeof gpad);
    gpad[252] = (unsigned char)(g >> 24);
    gpad[253] = (unsigned char)(g >> 16);
    gpad[254] = (unsigned char)(g >> 8);
    gpad[255] = (unsigned char)g;
    memset(bpad, 0, sizeof bpad);
    memcpy(bpad + 256 - blen, srp_b, blen);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&G);
    mbedtls_mpi_init(&X);
    mbedtls_mpi_init(&A);
    mbedtls_mpi_init(&B);
    mbedtls_mpi_init(&K);
    mbedtls_mpi_init(&U);
    mbedtls_mpi_init(&V);
    mbedtls_mpi_init(&KV);
    mbedtls_mpi_init(&T);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&S);
    mbedtls_mpi_init(&a);
    tg_sha256v(k, 2, p, 256, gpad, 256);
    rc = mbedtls_mpi_read_binary(&P, p, 256);
    rc = rc ? rc : mbedtls_mpi_lset(&G, g);
    rc = rc ? rc : mbedtls_mpi_read_binary(&X, x, 32);
    rc = rc ? rc : mbedtls_mpi_read_binary(&B, bpad, 256);
    rc = rc ? rc : mbedtls_mpi_read_binary(&a, a_rand, 256);
    rc = rc ? rc : mbedtls_mpi_read_binary(&K, k, 32);
    if (!rc && (mbedtls_mpi_cmp_int(&B, 0) <= 0 || mbedtls_mpi_cmp_mpi(&B, &P) >= 0))
        rc = -1;
    rc = rc ? rc : mbedtls_mpi_exp_mod(&A, &G, &a, &P, NULL);
    rc = rc ? rc : pad256(&A, a_out);
    if (!rc) {
        tg_sha256v(u, 2, a_out, 256, bpad, 256);
        rc = mbedtls_mpi_read_binary(&U, u, 32);
    }
    rc = rc ? rc : mbedtls_mpi_exp_mod(&V, &G, &X, &P, NULL);
    rc = rc ? rc : mbedtls_mpi_mul_mpi(&KV, &K, &V);
    rc = rc ? rc : mbedtls_mpi_mod_mpi(&KV, &KV, &P);
    rc = rc ? rc : mbedtls_mpi_sub_mpi(&T, &B, &KV);
    rc = rc ? rc : mbedtls_mpi_mod_mpi(&T, &T, &P);
    rc = rc ? rc : mbedtls_mpi_mul_mpi(&E, &U, &X);
    rc = rc ? rc : mbedtls_mpi_add_mpi(&E, &E, &a);
    rc = rc ? rc : mbedtls_mpi_exp_mod(&S, &T, &E, &P, NULL);
    rc = rc ? rc : pad256(&S, spad);
    if (!rc) {
        tg_sha256v(ka, 1, spad, 256);
        tg_sha256v(hp, 1, p, 256);
        tg_sha256v(hg, 1, gpad, 256);
        for (i = 0; i < 32; i++)
            hp[i] ^= hg[i];
        tg_sha256v(hs1, 1, salt1, s1len);
        tg_sha256v(hs2, 1, salt2, s2len);
        tg_sha256v(m1, 6, hp, 32, hs1, 32, hs2, 32, a_out, 256, bpad, 256, ka, 32);
    }
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&G);
    mbedtls_mpi_free(&X);
    mbedtls_mpi_free(&A);
    mbedtls_mpi_free(&B);
    mbedtls_mpi_free(&K);
    mbedtls_mpi_free(&U);
    mbedtls_mpi_free(&V);
    mbedtls_mpi_free(&KV);
    mbedtls_mpi_free(&T);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&S);
    mbedtls_mpi_free(&a);
    return rc ? -1 : 0;
}
