#ifndef TG_CRYPTO_H
#define TG_CRYPTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const unsigned char *n;
    int n_len;
    const unsigned char *e;
    int e_len;
} tg_rsa_key;

extern const tg_rsa_key tg_rsa_keys[];
extern const int tg_rsa_key_count;
extern const unsigned char tg_dh_prime[256];

int tg_crypto_init(void);
int tg_random(void *out, int n);

void tg_sha1v(unsigned char out[20], int parts, ...);
void tg_sha256v(unsigned char out[32], int parts, ...);

int tg_ige(int encrypt, const unsigned char key[32], const unsigned char iv[32],
           const unsigned char *in, unsigned char *out, int len);

int tg_modexp(unsigned char *out, int out_len,
              const unsigned char *base, int base_len,
              const unsigned char *exp, int exp_len,
              const unsigned char *mod, int mod_len);

int tg_dh_in_range(const unsigned char x[256]);

int64_t tg_rsa_fingerprint(const tg_rsa_key *k);

int tg_factor_pq(uint64_t pq, uint64_t *p, uint64_t *q);

void tg_srp_x(const unsigned char *password, int plen, const unsigned char *salt1, int s1len,
              const unsigned char *salt2, int s2len, unsigned char x[32]);
int tg_srp(const unsigned char x[32], const unsigned char *salt1, int s1len,
           const unsigned char *salt2, int s2len, int g, const unsigned char p[256],
           const unsigned char *srp_b, int blen, const unsigned char a_rand[256],
           unsigned char a_out[256], unsigned char m1[32]);

#ifdef __cplusplus
}
#endif

#endif
