#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifdef __SYMBIAN32__
#include <ezlib.h>
#else
#include <zlib.h>
#endif

#include "tg_buf.h"
#include "tg_crypto.h"
#include "tg_mtproto.h"
#include "tg_tl.h"

enum { H_CONT = 0, H_FOUND = 1, H_RETRY = 2 };

static int64_t g_last_msg_id;

static int64_t make_msg_id(int time_offset)
{
    struct timeval tv;
    uint64_t id;
    gettimeofday(&tv, NULL);
    id = ((uint64_t)(uint32_t)(tv.tv_sec + time_offset) << 32) |
         ((((uint64_t)tv.tv_usec << 32) / 1000000) & ~(uint64_t)3);
    if ((int64_t)id <= g_last_msg_id)
        id = (uint64_t)g_last_msg_id + 4;
    g_last_msg_id = (int64_t)id;
    return (int64_t)id;
}

static void seterr(char *err, int errlen, const char *msg)
{
    if (err && errlen > 0) {
        strncpy(err, msg, errlen - 1);
        err[errlen - 1] = 0;
    }
}

static int plain_call(tg_conn *c, int time_offset, const tg_buf *body, unsigned char **frame,
                      tg_rd *r, char *err, int errlen)
{
    tg_buf b;
    int flen, rc, len;
    tg_buf_init(&b);
    tg_buf_i64(&b, 0);
    tg_buf_i64(&b, make_msg_id(time_offset));
    tg_buf_i32(&b, body->len);
    tg_buf_put(&b, body->p, body->len);
    if (b.oom || body->oom) {
        tg_buf_free(&b);
        seterr(err, errlen, "out of memory");
        return TG_ERR_NOMEM;
    }
    rc = tg_conn_send(c, b.p, b.len);
    tg_buf_free(&b);
    if (!rc)
        rc = tg_conn_recv(c, frame, &flen);
    if (rc) {
        seterr(err, errlen, "network error during handshake");
        return TG_ERR_NET;
    }
    if (flen == 4) {
        char m[64];
        snprintf(m, sizeof m, "transport error %d", (int)tg_le32(*frame));
        seterr(err, errlen, m);
        return TG_ERR_PROTO;
    }
    if (flen < 20 || tg_le64(*frame) != 0) {
        seterr(err, errlen, "bad unencrypted reply");
        return TG_ERR_PROTO;
    }
    len = (int)tg_le32(*frame + 16);
    if (len < 0 || 20 + len > flen) {
        seterr(err, errlen, "bad reply length");
        return TG_ERR_PROTO;
    }
    tg_rd_init(r, *frame + 20, len);
    return TG_OK;
}

static int minimal_len(const unsigned char *p, int n)
{
    int i = 0;
    while (i < n - 1 && p[i] == 0)
        i++;
    return i;
}

static void be_u64(uint64_t v, unsigned char out[8], int *off)
{
    int i;
    for (i = 7; i >= 0; i--) {
        out[i] = (unsigned char)v;
        v >>= 8;
    }
    *off = minimal_len(out, 8);
}

int tg_handshake(tg_conn *c, int dc_id, int test, unsigned char auth_key[256],
                 unsigned char salt[8], int *time_offset, char *err, int errlen)
{
    unsigned char nonce[16], server_nonce[16], new_nonce[32];
    unsigned char pb[8], qb[8], rsa_blob[255], encrypted[256];
    unsigned char h_ns[20], h_sn[20], h_nn[20], tmp_key[32], tmp_iv[32];
    unsigned char g_a[256], b[256], g_b[256], gbyte, sha[20], aux[20];
    unsigned char *frame = NULL, *answer = NULL, *padded = NULL;
    const unsigned char *pq_raw, *fps_n, *dh_prime, *ga_raw, *enc_answer, *s;
    const tg_rsa_key *key = NULL;
    int64_t fp = 0;
    uint64_t pq = 0, p, q;
    int pq_len, i, j, count, poff, qoff, enc_len, dh_len, ga_len, g, server_time, n;
    int rc = TG_ERR_PROTO, which, padded_len;
    uint32_t cid;
    tg_buf body, inner;
    tg_rd r, ar;

    tg_buf_init(&body);
    tg_buf_init(&inner);
    *time_offset = 0;
    if (tg_random(nonce, 16) || tg_random(new_nonce, 32) || tg_random(b, 256)) {
        seterr(err, errlen, "random source failed");
        return TG_ERR_PROTO;
    }

    tg_buf_u32(&body, TL_REQ_PQ_MULTI);
    tg_buf_put(&body, nonce, 16);
    rc = plain_call(c, 0, &body, &frame, &r, err, errlen);
    if (rc)
        goto out;
    rc = TG_ERR_PROTO;
    if (tg_rd_u32(&r) != TL_RES_PQ || memcmp(tg_rd_raw(&r, 16) ? r.p + r.pos - 16 : nonce + 1, nonce, 16)) {
        seterr(err, errlen, "bad resPQ");
        goto out;
    }
    memcpy(server_nonce, tg_rd_raw(&r, 16) ? r.p + r.pos - 16 : nonce, 16);
    pq_raw = tg_rd_bytes(&r, &pq_len);
    if (tg_rd_u32(&r) != TL_VECTOR || r.err || pq_len < 1 || pq_len > 8) {
        seterr(err, errlen, "bad resPQ body");
        goto out;
    }
    for (i = 0; i < pq_len; i++)
        pq = (pq << 8) | pq_raw[i];
    count = tg_rd_i32(&r);
    fps_n = count >= 0 && count <= 64 ? tg_rd_raw(&r, count * 8) : NULL;
    if (!fps_n) {
        seterr(err, errlen, "bad fingerprint list");
        goto out;
    }
    for (i = 0; i < tg_rsa_key_count && !key; i++) {
        int64_t mine = tg_rsa_fingerprint(&tg_rsa_keys[i]);
        for (j = 0; j < count; j++) {
            if (tg_le64(fps_n + j * 8) == mine) {
                key = &tg_rsa_keys[i];
                fp = mine;
                break;
            }
        }
    }
    if (!key) {
        seterr(err, errlen, "no matching RSA key");
        goto out;
    }
    if (tg_factor_pq(pq, &p, &q)) {
        seterr(err, errlen, "could not factor pq");
        goto out;
    }
    be_u64(p, pb, &poff);
    be_u64(q, qb, &qoff);

    tg_buf_u32(&inner, TL_P_Q_INNER_DATA_DC);
    tg_buf_bytes(&inner, pq_raw, pq_len);
    tg_buf_bytes(&inner, pb + poff, 8 - poff);
    tg_buf_bytes(&inner, qb + qoff, 8 - qoff);
    tg_buf_put(&inner, nonce, 16);
    tg_buf_put(&inner, server_nonce, 16);
    tg_buf_put(&inner, new_nonce, 32);
    tg_buf_i32(&inner, dc_id + (test ? 10000 : 0));
    if (inner.oom || inner.len > 235) {
        seterr(err, errlen, "p_q_inner_data too large");
        goto out;
    }
    tg_sha1v(rsa_blob, 1, inner.p, inner.len);
    memcpy(rsa_blob + 20, inner.p, inner.len);
    tg_random(rsa_blob + 20 + inner.len, 255 - 20 - inner.len);
    if (tg_modexp(encrypted, 256, rsa_blob, 255, key->e, key->e_len, key->n, key->n_len)) {
        seterr(err, errlen, "RSA failed");
        goto out;
    }

    free(frame);
    frame = NULL;
    tg_buf_free(&body);
    tg_buf_u32(&body, TL_REQ_DH_PARAMS);
    tg_buf_put(&body, nonce, 16);
    tg_buf_put(&body, server_nonce, 16);
    tg_buf_bytes(&body, pb + poff, 8 - poff);
    tg_buf_bytes(&body, qb + qoff, 8 - qoff);
    tg_buf_i64(&body, fp);
    tg_buf_bytes(&body, encrypted, 256);
    rc = plain_call(c, 0, &body, &frame, &r, err, errlen);
    if (rc)
        goto out;
    rc = TG_ERR_PROTO;
    cid = tg_rd_u32(&r);
    if (cid != TL_SERVER_DH_PARAMS_OK) {
        seterr(err, errlen, cid == TL_SERVER_DH_PARAMS_FAIL ? "server_DH_params_fail"
                                                           : "unexpected req_DH_params reply");
        goto out;
    }
    tg_rd_raw(&r, 32);
    enc_answer = tg_rd_bytes(&r, &enc_len);
    if (r.err || enc_len < 32 || enc_len % 16) {
        seterr(err, errlen, "bad encrypted_answer");
        goto out;
    }

    tg_sha1v(h_ns, 2, new_nonce, 32, server_nonce, 16);
    tg_sha1v(h_sn, 2, server_nonce, 16, new_nonce, 32);
    tg_sha1v(h_nn, 2, new_nonce, 32, new_nonce, 32);
    memcpy(tmp_key, h_ns, 20);
    memcpy(tmp_key + 20, h_sn, 12);
    memcpy(tmp_iv, h_sn + 12, 8);
    memcpy(tmp_iv + 8, h_nn, 20);
    memcpy(tmp_iv + 28, new_nonce, 4);

    answer = (unsigned char *)malloc(enc_len);
    if (!answer) {
        rc = TG_ERR_NOMEM;
        goto out;
    }
    tg_ige(0, tmp_key, tmp_iv, enc_answer, answer, enc_len);
    tg_rd_init(&ar, answer + 20, enc_len - 20);
    if (tg_rd_u32(&ar) != TL_SERVER_DH_INNER_DATA) {
        seterr(err, errlen, "bad server_DH_inner_data");
        goto out;
    }
    s = tg_rd_raw(&ar, 16);
    if (!s || memcmp(s, nonce, 16)) {
        seterr(err, errlen, "server_DH_inner_data nonce mismatch");
        goto out;
    }
    s = tg_rd_raw(&ar, 16);
    if (!s || memcmp(s, server_nonce, 16)) {
        seterr(err, errlen, "server_DH_inner_data server_nonce mismatch");
        goto out;
    }
    g = tg_rd_i32(&ar);
    dh_prime = tg_rd_bytes(&ar, &dh_len);
    ga_raw = tg_rd_bytes(&ar, &ga_len);
    server_time = tg_rd_i32(&ar);
    if (ar.err) {
        seterr(err, errlen, "truncated server_DH_inner_data");
        goto out;
    }
    tg_sha1v(sha, 1, answer + 20, ar.pos);
    if (memcmp(sha, answer, 20)) {
        seterr(err, errlen, "server_DH_inner_data hash mismatch");
        goto out;
    }
    if (dh_len != 256 || memcmp(dh_prime, tg_dh_prime, 256)) {
        seterr(err, errlen, "unexpected dh_prime");
        goto out;
    }
    if (g < 2 || g > 7 || ga_len > 256) {
        seterr(err, errlen, "bad DH generator");
        goto out;
    }
    memset(g_a, 0, 256);
    memcpy(g_a + 256 - ga_len, ga_raw, ga_len);
    if (!tg_dh_in_range(g_a)) {
        seterr(err, errlen, "g_a out of range");
        goto out;
    }
    *time_offset = server_time - (int)time(NULL);

    gbyte = (unsigned char)g;
    if (tg_modexp(g_b, 256, &gbyte, 1, b, 256, tg_dh_prime, 256) || !tg_dh_in_range(g_b)) {
        seterr(err, errlen, "g_b computation failed");
        goto out;
    }
    tg_buf_free(&inner);
    tg_buf_u32(&inner, TL_CLIENT_DH_INNER_DATA);
    tg_buf_put(&inner, nonce, 16);
    tg_buf_put(&inner, server_nonce, 16);
    tg_buf_i64(&inner, 0);
    n = minimal_len(g_b, 256);
    tg_buf_bytes(&inner, g_b + n, 256 - n);
    padded_len = 20 + inner.len;
    padded_len += (16 - padded_len % 16) % 16;
    padded = (unsigned char *)malloc(padded_len);
    if (!padded || inner.oom) {
        rc = TG_ERR_NOMEM;
        goto out;
    }
    tg_sha1v(padded, 1, inner.p, inner.len);
    memcpy(padded + 20, inner.p, inner.len);
    tg_random(padded + 20 + inner.len, padded_len - 20 - inner.len);
    tg_ige(1, tmp_key, tmp_iv, padded, padded, padded_len);

    free(frame);
    frame = NULL;
    tg_buf_free(&body);
    tg_buf_u32(&body, TL_SET_CLIENT_DH_PARAMS);
    tg_buf_put(&body, nonce, 16);
    tg_buf_put(&body, server_nonce, 16);
    tg_buf_bytes(&body, padded, padded_len);
    rc = plain_call(c, 0, &body, &frame, &r, err, errlen);
    if (rc)
        goto out;
    rc = TG_ERR_PROTO;
    cid = tg_rd_u32(&r);
    tg_rd_raw(&r, 32);
    s = tg_rd_raw(&r, 16);
    which = cid == TL_DH_GEN_OK ? 1 : cid == TL_DH_GEN_RETRY ? 2 : cid == TL_DH_GEN_FAIL ? 3 : 0;
    if (!which || !s) {
        seterr(err, errlen, "unexpected set_client_DH_params reply");
        goto out;
    }
    if (tg_modexp(auth_key, 256, g_a, 256, b, 256, tg_dh_prime, 256)) {
        seterr(err, errlen, "auth_key computation failed");
        goto out;
    }
    tg_sha1v(aux, 1, auth_key, 256);
    gbyte = (unsigned char)which;
    tg_sha1v(sha, 3, new_nonce, 32, &gbyte, 1, aux, 8);
    if (memcmp(sha + 4, s, 16)) {
        seterr(err, errlen, "new_nonce_hash mismatch");
        goto out;
    }
    if (which != 1) {
        seterr(err, errlen, which == 2 ? "dh_gen_retry" : "dh_gen_fail");
        goto out;
    }
    for (i = 0; i < 8; i++)
        salt[i] = new_nonce[i] ^ server_nonce[i];
    rc = TG_OK;

out:
    free(frame);
    free(answer);
    free(padded);
    tg_buf_free(&body);
    tg_buf_free(&inner);
    memset(b, 0, sizeof b);
    return rc;
}

int tg_session_init(tg_session *s, tg_conn *c, const unsigned char auth_key[256],
                    const unsigned char salt[8], int time_offset)
{
    unsigned char h[20];
    memset(s, 0, sizeof *s);
    s->conn = c;
    memcpy(s->auth_key, auth_key, 256);
    tg_sha1v(h, 1, auth_key, 256);
    memcpy(s->auth_key_id, h + 12, 8);
    memcpy(s->salt, salt, 8);
    s->time_offset = time_offset;
    s->new_session = 1;
    return tg_random(s->session_id, 8) ? TG_ERR_PROTO : TG_OK;
}

static void kdf(const tg_session *s, const unsigned char msg_key[16], int x, unsigned char key[32],
                unsigned char iv[32])
{
    unsigned char a[32], b[32];
    tg_sha256v(a, 2, msg_key, 16, s->auth_key + x, 36);
    tg_sha256v(b, 2, s->auth_key + 40 + x, 36, msg_key, 16);
    memcpy(key, a, 8);
    memcpy(key + 8, b + 8, 16);
    memcpy(key + 24, a + 24, 8);
    memcpy(iv, b, 8);
    memcpy(iv + 8, a + 8, 16);
    memcpy(iv + 24, b + 24, 8);
}

static int send_encrypted(tg_session *s, const unsigned char *body, int len, int content,
                          int64_t *msg_id_out)
{
    unsigned char hdr[32], msg_key_large[32], key[32], iv[32], pad[32];
    int64_t msg_id = make_msg_id(s->time_offset);
    int seq, padn, plain_len, rc;
    tg_buf plain, out;
    if (content) {
        seq = s->seq * 2 + 1;
        s->seq++;
    } else {
        seq = s->seq * 2;
    }
    memcpy(hdr, s->salt, 8);
    memcpy(hdr + 8, s->session_id, 8);
    tg_put_le64(hdr + 16, (uint64_t)msg_id);
    tg_put_le32(hdr + 24, (uint32_t)seq);
    tg_put_le32(hdr + 28, (uint32_t)len);
    plain_len = 32 + len;
    padn = (16 - plain_len % 16) % 16;
    if (padn < 12)
        padn += 16;
    tg_random(pad, padn);
    tg_buf_init(&plain);
    tg_buf_put(&plain, hdr, 32);
    tg_buf_put(&plain, body, len);
    tg_buf_put(&plain, pad, padn);
    if (plain.oom) {
        tg_buf_free(&plain);
        return TG_ERR_NOMEM;
    }
    tg_sha256v(msg_key_large, 2, s->auth_key + 88, 32, plain.p, plain.len);
    kdf(s, msg_key_large + 8, 0, key, iv);
    tg_buf_init(&out);
    tg_buf_put(&out, s->auth_key_id, 8);
    tg_buf_put(&out, msg_key_large + 8, 16);
    tg_buf_put(&out, plain.p, plain.len);
    if (out.oom) {
        tg_buf_free(&plain);
        tg_buf_free(&out);
        return TG_ERR_NOMEM;
    }
    tg_ige(1, key, iv, out.p + 24, out.p + 24, plain.len);
    rc = tg_conn_send(s->conn, out.p, out.len);
    tg_buf_free(&plain);
    tg_buf_free(&out);
    if (msg_id_out)
        *msg_id_out = msg_id;
    if (rc) {
        strcpy(s->err, "network error while sending");
        return TG_ERR_NET;
    }
    return TG_OK;
}

static int decrypt_frame(tg_session *s, unsigned char *frame, int flen, int64_t *msg_id,
                         unsigned char **body, int *blen)
{
    unsigned char key[32], iv[32], check[32];
    unsigned char *plain = frame + 24;
    int plen = flen - 24, len;
    if (flen < 24 + 32 || plen % 16 || memcmp(frame, s->auth_key_id, 8)) {
        strcpy(s->err, "bad encrypted frame");
        return TG_ERR_PROTO;
    }
    kdf(s, frame + 8, 8, key, iv);
    tg_ige(0, key, iv, plain, plain, plen);
    tg_sha256v(check, 2, s->auth_key + 96, 32, plain, plen);
    if (memcmp(check + 8, frame + 8, 16)) {
        strcpy(s->err, "msg_key mismatch");
        return TG_ERR_PROTO;
    }
    if (memcmp(plain + 8, s->session_id, 8)) {
        strcpy(s->err, "session_id mismatch");
        return TG_ERR_PROTO;
    }
    *msg_id = tg_le64(plain + 16);
    len = (int)tg_le32(plain + 28);
    if (len < 0 || 32 + len > plen) {
        strcpy(s->err, "bad message length");
        return TG_ERR_PROTO;
    }
    *body = plain + 32;
    *blen = len;
    return TG_OK;
}

static int gunzip(const unsigned char *in, int n, unsigned char **out, int *outlen)
{
    z_stream z;
    int cap = n * 4 + 1024, rc;
    unsigned char *buf;
    int h = 10, flg;
    if (n < 18 || in[0] != 0x1f || in[1] != 0x8b || in[2] != 8)
        return TG_ERR_PROTO;
    flg = in[3];
    if (flg & 4)
        h += 2 + (in[10] | in[11] << 8);
    if (flg & 8)
        while (h < n && in[h++])
            ;
    if (flg & 16)
        while (h < n && in[h++])
            ;
    if (flg & 2)
        h += 2;
    if (h >= n)
        return TG_ERR_PROTO;
    in += h;
    n -= h;
    buf = (unsigned char *)malloc(cap);
    if (!buf)
        return TG_ERR_NOMEM;
    memset(&z, 0, sizeof z);
    if (inflateInit2(&z, -MAX_WBITS) != Z_OK) {
        free(buf);
        return TG_ERR_PROTO;
    }
    z.next_in = (Bytef *)in;
    z.avail_in = (uInt)n;
    for (;;) {
        z.next_out = buf + z.total_out;
        z.avail_out = (uInt)(cap - (int)z.total_out);
        rc = inflate(&z, Z_NO_FLUSH);
        if (rc == Z_STREAM_END)
            break;
        if (rc != Z_OK && rc != Z_BUF_ERROR) {
            inflateEnd(&z);
            free(buf);
            return TG_ERR_PROTO;
        }
        if (z.avail_out == 0) {
            unsigned char *nb;
            cap *= 2;
            nb = (unsigned char *)realloc(buf, cap);
            if (!nb) {
                inflateEnd(&z);
                free(buf);
                return TG_ERR_NOMEM;
            }
            buf = nb;
        } else if (rc == Z_BUF_ERROR) {
            inflateEnd(&z);
            free(buf);
            return TG_ERR_PROTO;
        }
    }
    *outlen = (int)z.total_out;
    inflateEnd(&z);
    *out = buf;
    return TG_OK;
}

static void add_ack(tg_session *s, int64_t id)
{
    if (s->nacks == (int)(sizeof s->acks / sizeof s->acks[0]))
        tg_send_acks(s);
    s->acks[s->nacks++] = id;
}

static int handle(tg_session *s, int64_t recv_id, const unsigned char *body, int blen,
                  int64_t want, unsigned char **result, int *rlen)
{
    tg_rd r;
    uint32_t cid;
    tg_rd_init(&r, body, blen);
    cid = tg_rd_u32(&r);
    if (r.err)
        return H_CONT;

    if (cid == TL_MSG_CONTAINER) {
        int count = tg_rd_i32(&r), i, found = 0;
        for (i = 0; i < count && !r.err; i++) {
            int64_t inner_id = tg_rd_i64(&r);
            int inner_len, rc;
            const unsigned char *inner;
            tg_rd_i32(&r);
            inner_len = tg_rd_i32(&r);
            inner = tg_rd_raw(&r, inner_len);
            if (!inner)
                break;
            rc = handle(s, inner_id, inner, inner_len, want, result, rlen);
            if (rc == H_FOUND)
                found = 1;
            else if (rc != H_CONT) {
                if (found) {
                    free(*result);
                    *result = NULL;
                }
                return rc;
            }
        }
        return found ? H_FOUND : H_CONT;
    }

    if (cid == TL_GZIP_PACKED) {
        int n, olen, rc;
        unsigned char *out;
        const unsigned char *packed = tg_rd_bytes(&r, &n);
        if (!packed || gunzip(packed, n, &out, &olen))
            return H_CONT;
        rc = handle(s, recv_id, out, olen, want, result, rlen);
        free(out);
        return rc;
    }

    if (cid == TL_RPC_RESULT) {
        int64_t req = tg_rd_i64(&r);
        const unsigned char *inner = r.p + r.pos;
        int inner_len = r.len - r.pos;
        unsigned char *unpacked = NULL;
        add_ack(s, recv_id);
        if (inner_len >= 4 && tg_le32(inner) == TL_GZIP_PACKED) {
            int n, olen;
            const unsigned char *packed;
            tg_rd zr;
            tg_rd_init(&zr, inner + 4, inner_len - 4);
            packed = tg_rd_bytes(&zr, &n);
            if (!packed || gunzip(packed, n, &unpacked, &olen)) {
                strcpy(s->err, "bad gzip_packed result");
                return TG_ERR_PROTO;
            }
            inner = unpacked;
            inner_len = olen;
        }
        if (req != want) {
            free(unpacked);
            return H_CONT;
        }
        if (inner_len >= 4 && tg_le32(inner) == TL_RPC_ERROR) {
            tg_rd er;
            int n;
            const unsigned char *m;
            tg_rd_init(&er, inner + 4, inner_len - 4);
            s->rpc_code = tg_rd_i32(&er);
            m = tg_rd_bytes(&er, &n);
            if (n > (int)sizeof s->rpc_msg - 1)
                n = sizeof s->rpc_msg - 1;
            memcpy(s->rpc_msg, m ? m : (const unsigned char *)"", m ? n : 0);
            s->rpc_msg[m ? n : 0] = 0;
            snprintf(s->err, sizeof s->err, "%d %s", s->rpc_code, s->rpc_msg);
            free(unpacked);
            return TG_ERR_RPC;
        }
        *result = (unsigned char *)malloc(inner_len > 0 ? inner_len : 1);
        if (!*result) {
            free(unpacked);
            return TG_ERR_NOMEM;
        }
        memcpy(*result, inner, inner_len);
        *rlen = inner_len;
        free(unpacked);
        return H_FOUND;
    }

    if (cid == TL_NEW_SESSION_CREATED) {
        tg_rd_i64(&r);
        tg_rd_i64(&r);
        tg_put_le64(s->salt, (uint64_t)tg_rd_i64(&r));
        add_ack(s, recv_id);
        return H_CONT;
    }

    if (cid == TL_BAD_SERVER_SALT) {
        tg_rd_i64(&r);
        tg_rd_i32(&r);
        tg_rd_i32(&r);
        tg_put_le64(s->salt, (uint64_t)tg_rd_i64(&r));
        return H_RETRY;
    }

    if (cid == TL_BAD_MSG_NOTIFICATION) {
        int code;
        tg_rd_i64(&r);
        tg_rd_i32(&r);
        code = tg_rd_i32(&r);
        if (code == 16 || code == 17) {
            s->time_offset = (int)((uint64_t)recv_id >> 32) - (int)time(NULL);
            s->seq = 0;
            s->new_session = 1;
            tg_random(s->session_id, 8);
            return H_RETRY;
        }
        snprintf(s->err, sizeof s->err, "bad_msg_notification %d", code);
        return TG_ERR_PROTO;
    }

    if (cid == TL_PONG) {
        *result = (unsigned char *)malloc(blen);
        if (!*result)
            return TG_ERR_NOMEM;
        memcpy(*result, body, blen);
        *rlen = blen;
        return H_FOUND;
    }

    if (cid == TL_MSGS_ACK)
        return H_CONT;

    add_ack(s, recv_id);
    return H_CONT;
}

int tg_call(tg_session *s, const unsigned char *body, int len, unsigned char **result, int *rlen)
{
    int attempt, rc;
    *result = NULL;
    *rlen = 0;
    s->rpc_code = 0;
    s->rpc_msg[0] = 0;
    s->err[0] = 0;
    for (attempt = 0; attempt < 3; attempt++) {
        int64_t msg_id;
        int seen;
        rc = send_encrypted(s, body, len, 1, &msg_id);
        if (rc)
            return rc;
        rc = H_CONT;
        for (seen = 0; seen < 16 && rc == H_CONT; seen++) {
            unsigned char *frame, *b;
            int flen, blen;
            int64_t recv_id;
            if (tg_conn_recv(s->conn, &frame, &flen)) {
                strcpy(s->err, "network error while receiving");
                return TG_ERR_NET;
            }
            if (flen == 4) {
                snprintf(s->err, sizeof s->err, "transport error %d", (int)tg_le32(frame));
                free(frame);
                return TG_ERR_NET;
            }
            rc = decrypt_frame(s, frame, flen, &recv_id, &b, &blen);
            if (!rc)
                rc = handle(s, recv_id, b, blen, msg_id, result, rlen);
            free(frame);
        }
        if (rc == H_FOUND)
            return TG_OK;
        if (rc == H_RETRY)
            continue;
        if (rc == H_CONT) {
            strcpy(s->err, "no reply from server");
            return TG_ERR_PROTO;
        }
        return rc;
    }
    strcpy(s->err, "gave up after retries");
    return TG_ERR_PROTO;
}

int tg_send_acks(tg_session *s)
{
    tg_buf b;
    int i, rc;
    if (!s->nacks)
        return TG_OK;
    tg_buf_init(&b);
    tg_buf_u32(&b, TL_MSGS_ACK);
    tg_buf_u32(&b, TL_VECTOR);
    tg_buf_i32(&b, s->nacks);
    for (i = 0; i < s->nacks; i++)
        tg_buf_i64(&b, s->acks[i]);
    s->nacks = 0;
    rc = b.oom ? TG_ERR_NOMEM : send_encrypted(s, b.p, b.len, 0, NULL);
    tg_buf_free(&b);
    return rc;
}
