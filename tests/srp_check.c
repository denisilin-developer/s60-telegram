#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS
#include <string.h>
#include <mbedtls/private/bignum.h>

#include "tg_crypto.h"

int srp_selftest(void)
{
    static const unsigned char pw[] = "correct horse";
    unsigned char s1[40], s2[16], x[32], gpad[256], k[32], b[256], bb[256], ar[256], A[256], m1[32];
    unsigned char u[32], sp[256], ka[32], hp[32], hg[32], hs1[32], hs2[32], m1s[32];
    mbedtls_mpi P, G, X, V, K, Bs, Gb, Bb, Am, U, Vu, S;
    int i, ok;
    tg_random(s1, sizeof s1);
    tg_random(s2, sizeof s2);
    tg_random(b, sizeof b);
    tg_random(ar, sizeof ar);
    tg_srp_x(pw, (int)strlen((const char *)pw), s1, sizeof s1, s2, sizeof s2, x);
    memset(gpad, 0, sizeof gpad);
    gpad[255] = 3;
    tg_sha256v(k, 2, tg_dh_prime, 256, gpad, 256);
    mbedtls_mpi_init(&P); mbedtls_mpi_init(&G); mbedtls_mpi_init(&X); mbedtls_mpi_init(&V);
    mbedtls_mpi_init(&K); mbedtls_mpi_init(&Bs); mbedtls_mpi_init(&Gb); mbedtls_mpi_init(&Bb);
    mbedtls_mpi_init(&Am); mbedtls_mpi_init(&U); mbedtls_mpi_init(&Vu); mbedtls_mpi_init(&S);
    mbedtls_mpi_read_binary(&P, tg_dh_prime, 256);
    mbedtls_mpi_lset(&G, 3);
    mbedtls_mpi_read_binary(&X, x, 32);
    mbedtls_mpi_exp_mod(&V, &G, &X, &P, NULL);
    mbedtls_mpi_read_binary(&K, k, 32);
    mbedtls_mpi_read_binary(&Bs, b, 256);
    mbedtls_mpi_exp_mod(&Gb, &G, &Bs, &P, NULL);
    mbedtls_mpi_mul_mpi(&Bb, &K, &V);
    mbedtls_mpi_add_mpi(&Bb, &Bb, &Gb);
    mbedtls_mpi_mod_mpi(&Bb, &Bb, &P);
    mbedtls_mpi_write_binary(&Bb, bb, 256);
    ok = tg_srp(x, s1, sizeof s1, s2, sizeof s2, 3, tg_dh_prime, bb, 256, ar, A, m1) == 0;
    tg_sha256v(u, 2, A, 256, bb, 256);
    mbedtls_mpi_read_binary(&Am, A, 256);
    mbedtls_mpi_read_binary(&U, u, 32);
    mbedtls_mpi_exp_mod(&Vu, &V, &U, &P, NULL);
    mbedtls_mpi_mul_mpi(&Vu, &Vu, &Am);
    mbedtls_mpi_mod_mpi(&Vu, &Vu, &P);
    mbedtls_mpi_exp_mod(&S, &Vu, &Bs, &P, NULL);
    mbedtls_mpi_write_binary(&S, sp, 256);
    tg_sha256v(ka, 1, sp, 256);
    tg_sha256v(hp, 1, tg_dh_prime, 256);
    tg_sha256v(hg, 1, gpad, 256);
    for (i = 0; i < 32; i++)
        hp[i] ^= hg[i];
    tg_sha256v(hs1, 1, s1, (int)sizeof s1);
    tg_sha256v(hs2, 1, s2, (int)sizeof s2);
    tg_sha256v(m1s, 6, hp, 32, hs1, 32, hs2, 32, A, 256, bb, 256, ka, 32);
    mbedtls_mpi_free(&P); mbedtls_mpi_free(&G); mbedtls_mpi_free(&X); mbedtls_mpi_free(&V);
    mbedtls_mpi_free(&K); mbedtls_mpi_free(&Bs); mbedtls_mpi_free(&Gb); mbedtls_mpi_free(&Bb);
    mbedtls_mpi_free(&Am); mbedtls_mpi_free(&U); mbedtls_mpi_free(&Vu); mbedtls_mpi_free(&S);
    return ok && !memcmp(m1, m1s, 32);
}
