#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tg_buf.h"
#include "tg_client.h"
#include "../src/tg_config.h"
#include "tg_qr.h"

int srp_selftest(void);
#include "tg_crypto.h"
#include "tg_mtproto.h"
#include "tg_net.h"
#include "tg_tl.h"

static int fails;

#define CHECK(cond, what)                          \
    do {                                           \
        if (cond)                                  \
            printf("  ok   %s\n", what);          \
        else {                                     \
            printf("  FAIL %s\n", what);          \
            fails++;                               \
        }                                          \
    } while (0)

static int selftest(void)
{
    unsigned char key[32], iv[32], data[1024], enc[1024], dec[1024], h[20];
    uint64_t p, q;
    tg_buf b;
    tg_rd r;
    int n;
    const unsigned char *s;

    printf("[selftest]\n");
    CHECK(srp_selftest(), "2FA SRP: client M1 matches a simulated server");
    CHECK(!strcmp(tg_mime_for("IMG_001.JPG"), "image/jpeg") &&
          !strcmp(tg_mime_for("song.mp3"), "audio/mpeg") &&
          !strcmp(tg_mime_for("app.sisx"), "application/vnd.symbian.install") &&
          !strcmp(tg_mime_for("a.jpgx"), "application/octet-stream") &&
          !strcmp(tg_mime_for("noext"), "application/octet-stream"),
          "tg_mime_for maps extensions case-insensitively");
    {
        tg_folder_peer peers[2] = {{0, 42, 0}, {2, 7, 1}};
        tg_folder f = {1, 1 | 4 | 32, 0, 0, (char *)"Work", peers, 2};
        tg_dialog contact = {0, TG_DLG_CONTACT, 5}, stranger = {0, 0, 6};
        tg_dialog group = {1, TG_DLG_GROUP, 9}, muted_group = {1, TG_DLG_GROUP | TG_DLG_MUTED, 10};
        tg_dialog excluded = {0, TG_DLG_CONTACT, 42}, channel = {2, TG_DLG_CHANNEL, 7};
        tg_dialog other_channel = {2, TG_DLG_CHANNEL, 8};
        CHECK(tg_folder_has(&f, &contact) && !tg_folder_has(&f, &stranger) &&
              tg_folder_has(&f, &group) && !tg_folder_has(&f, &muted_group) &&
              !tg_folder_has(&f, &excluded) && tg_folder_has(&f, &channel) &&
              !tg_folder_has(&f, &other_channel),
              "tg_folder_has: types, exclude_muted, include/exclude peers");
        {
            tg_dialog arch_group = {1, TG_DLG_GROUP | TG_DLG_ARCHIVED, 11};
            tg_folder f2 = f;
            CHECK(tg_folder_has(&f, &arch_group), "tg_folder_has: archived chats count by default");
            f2.flags |= 128;
            CHECK(!tg_folder_has(&f2, &arch_group), "tg_folder_has: exclude_archived");
        }
    }
    {
        static const struct { const char *in, *out; } fx[] = {
            {"\xF0\x9F\x87\xBA\xF0\x9F\x87\xA6 Kharkiv", "UA Kharkiv"},
            {"I love you \xE2\x9D\xA4\xEF\xB8\x8F", "I love you <3"},
            {"\xD0\x9D\xD0\xB5 \xF0\x9F\x9A\x80 \xD0\xB7\xD0\xB0", "\xD0\x9D\xD0\xB5 \xD0\xB7\xD0\xB0"},
            {"\xE2\x9D\x97 alert \xF0\x9F\x91\x8D", "! alert +1"},
            {"\xC2\xAB\xD0\xA2\xC2\xBB \xE2\x80\x94 ok", "\xC2\xAB\xD0\xA2\xC2\xBB \xE2\x80\x94 ok"},
            {"\xF0\x9F\x8E\x81", ""},
            {"\xD0\x9F\xD0\xBE\xD0", "\xD0\x9F\xD0\xBE"},
        };
        int k;
        for (k = 0; k < (int)(sizeof fx / sizeof fx[0]); k++) {
            char b[64];
            strcpy(b, fx[k].in);
            tg_text_fix(b);
            CHECK(!strcmp(b, fx[k].out), "tg_text_fix maps emoji / keeps Cyrillic and punctuation");
        }
    }
    tg_random(key, 32);
    tg_random(iv, 32);
    tg_random(data, sizeof data);
    tg_ige(1, key, iv, data, enc, sizeof data);
    tg_ige(0, key, iv, enc, dec, sizeof data);
    CHECK(!memcmp(data, dec, sizeof data) && memcmp(data, enc, 16), "AES-IGE roundtrip");

    CHECK(!tg_factor_pq(2548648775246505931ULL, &p, &q) && p == 1306821937ULL &&
              q == 1950264763ULL,
          "factor pq 2548648775246505931 -> 1306821937 x 1950264763");

    tg_sha1v(h, 2, "a", 1, "bc", 2);
    CHECK(h[0] == 0xa9 && h[1] == 0x99 && h[19] == 0x9d, "sha1 multi-part = sha1(abc)");

    tg_buf_init(&b);
    tg_buf_str(&b, "hello");
    memset(data, 'x', 300);
    tg_buf_bytes(&b, data, 300);
    tg_rd_init(&r, b.p, b.len);
    s = tg_rd_bytes(&r, &n);
    CHECK(n == 5 && !memcmp(s, "hello", 5) && b.len % 4 == 0, "TL short string");
    s = tg_rd_bytes(&r, &n);
    CHECK(n == 300 && s && r.pos == b.len && !r.err, "TL long string");
    tg_buf_free(&b);

    CHECK(tg_rsa_key_count > 0 && tg_find_decl(0x9cb490e9u) &&
              !strcmp(tg_find_decl(0x9cb490e9u)->name, "message"),
          "schema table lookup (message#9cb490e9)");
    return fails;
}

static int ping(int test)
{
    unsigned char auth_key[256], salt[8], *res;
    char err[160];
    int off, rc, err_code = 0, rlen;
    tg_conn *c;
    tg_session s;
    tg_buf b;
    int64_t ping_id;

    printf("[handshake + encrypted ping on %s DC2]\n", test ? "test" : "prod");
    c = tg_conn_open(tg_dc_host(2, test), 443, &err_code);
    if (!c) {
        printf("  FAIL connect (%d)\n", err_code);
        return 1;
    }
    rc = tg_handshake(c, 2, test, auth_key, salt, &off, err, sizeof err);
    CHECK(!rc, rc ? err : "DH handshake -> auth_key");
    if (rc) {
        tg_conn_close(c);
        return 1;
    }
    tg_session_init(&s, c, auth_key, salt, off);
    tg_random(&ping_id, sizeof ping_id);
    tg_buf_init(&b);
    tg_buf_u32(&b, TL_PING);
    tg_buf_i64(&b, ping_id);
    rc = tg_call(&s, b.p, b.len, &res, &rlen);
    tg_buf_free(&b);
    CHECK(!rc && rlen >= 20 && tg_le32(res) == TL_PONG && tg_le64(res + 12) == ping_id,
          rc ? s.err : "encrypted ping -> pong with matching ping_id");
    free(res);
    tg_conn_close(c);
    return rc ? 1 : 0;
}

static void redact(const char *s, char *out, int n)
{
    int chars = 0, first = 0;
    const unsigned char *p = (const unsigned char *)s;
    for (; *p; p++)
        if ((*p & 0xc0) != 0x80)
            chars++;
    p = (const unsigned char *)s;
    if (*p) {
        first = 1;
        while (p[first] && (p[first] & 0xc0) == 0x80)
            first++;
    }
    snprintf(out, n, "%.*s*** (%d chars)", first, s, chars);
}

static int live(const char *dir, int show)
{
    tg_client *c = tg_client_new(dir, TG_API_ID, "", 0);
    tg_dialog *d;
    tg_message *m;
    int n, nm, i, rc;
    char a[96], b[96];
    printf("[dialogs + history on the real account, read-only]\n");
    rc = tg_client_dialogs(c, 0, 10, &d, &n);
    CHECK(!rc && n > 0, rc ? tg_client_error(c) : "messages.getDialogs -> parsed dialog list");
    for (i = 0; !rc && i < n; i++) {
        if (show) {
            printf("    %c %-24.24s %.40s%s\n", d[i].out ? '>' : ' ', d[i].name, d[i].text,
                   d[i].unread ? " *" : "");
        } else {
            redact(d[i].name, a, sizeof a);
            redact(d[i].text, b, sizeof b);
            printf("    %c %-26s %s%s\n", d[i].out ? '>' : ' ', a, b, d[i].unread ? " *" : "");
        }
    }
    if (!rc && n > 0) {
        rc = tg_client_history(c, &d[0], 0, 8, &m, &nm);
        CHECK(!rc && nm > 0, rc ? tg_client_error(c) : "messages.getHistory on the first chat");
        tg_messages_free(m, nm);
    }
    tg_dialogs_free(d, n);
    tg_client_free(c);
    return rc ? 1 : 0;
}

static void kinds(const char *label, const tg_dialog *d, int n)
{
    int i, k[3] = {0, 0, 0}, g = 0, ch = 0, ct = 0, bot = 0, mu = 0;
    for (i = 0; i < n; i++) {
        if (d[i].kind >= 0 && d[i].kind < 3)
            k[d[i].kind]++;
        g += !!(d[i].flags & TG_DLG_GROUP);
        ch += !!(d[i].flags & TG_DLG_CHANNEL);
        ct += !!(d[i].flags & TG_DLG_CONTACT);
        bot += !!(d[i].flags & TG_DLG_BOT);
        mu += !!(d[i].flags & TG_DLG_MUTED);
    }
    printf("    %-8s %3d dialogs: users %d chats %d channels %d | flags group %d channel %d "
           "contact %d bot %d muted %d\n", label, n, k[0], k[1], k[2], g, ch, ct, bot, mu);
}

static int inspect(const char *dir)
{
    tg_client *c = tg_client_new(dir, TG_API_ID, "", 0);
    tg_dialog *d = NULL, *ar = NULL;
    tg_folder *f = NULL;
    int n = 0, na = 0, nf = 0, i, j, rc;
    printf("[inspect: counts only, read-only]\n");
    rc = tg_client_dialogs(c, 0, 100, &d, &n);
    CHECK(!rc, rc ? tg_client_error(c) : "main list (folder 0)");
    kinds("main", d, n);
    rc = tg_client_dialogs(c, 1, 50, &ar, &na);
    CHECK(!rc, rc ? tg_client_error(c) : "archive (folder 1)");
    kinds("archive", ar, na);
    for (i = 0, j = 0; i < na; i++) {
        int k;
        for (k = 0; k < n; k++)
            if (d[k].id == ar[i].id && d[k].kind == ar[i].kind)
                j++;
    }
    printf("    archive entries also in main: %d\n", j);
    if (n > 0) {
        int32_t ro = -1;
        rc = tg_client_read_out(c, &d[0], &ro);
        CHECK(!rc && ro == d[0].read_out_max,
              rc ? tg_client_error(c) : "messages.getPeerDialogs read_outbox_max_id matches the dialog list");
    }
    for (i = 0; i < n + na && i < 12; i++) {
        tg_dialog *dd = i < n ? &d[i] : &ar[i - n];
        tg_message *m = NULL, *m2 = NULL;
        int nm = 0, nm2 = 0, k, rep = 0, fw = 0, ed = 0, re = 0, older = 1;
        int32_t when;
        char *pin = NULL;
        if (tg_client_history(c, dd, 0, 30, &m, &nm))
            continue;
        for (k = 0; k < nm; k++) {
            rep += m[k].reply != NULL;
            fw += m[k].fwd != NULL;
            ed += m[k].edited;
            re += m[k].reactions != NULL;
        }
        if (nm && !tg_client_history(c, dd, m[0].id, 30, &m2, &nm2))
            for (k = 0; k < nm2; k++)
                older &= m2[k].id < m[0].id;
        tg_client_pinned(c, dd, &pin);
        printf("    chat %d kind %d: %d msgs (reply %d fwd %d edited %d reactions %d), "
               "page2 %d all-older %d, pinned %s, status %d\n", i, dd->kind, nm, rep, fw, ed, re,
               nm2, older, pin ? "yes" : "no", tg_client_status(c, &when));
        free(pin);
        tg_messages_free(m, nm);
        tg_messages_free(m2, nm2);
    }
    {
        tg_info info;
        tg_dialog *ct = NULL;
        int nct = 0;
        for (i = 0; i < n + na && i < 6; i++) {
            tg_dialog *dd = i < n ? &d[i] : &ar[i - n];
            rc = tg_client_info(c, dd, &info);
            CHECK(!rc && info.name, rc ? tg_client_error(c) : "info (getFullUser/getFullChannel)");
            printf("       kind %d: username %d phone %d about %d members %d channel %d\n", dd->kind,
                   info.username != NULL, info.phone != NULL, info.about != NULL, info.members,
                   info.channel);
            tg_info_free(&info);
        }
        rc = tg_client_contacts(c, &ct, &nct);
        CHECK(!rc, rc ? tg_client_error(c) : "contacts.getContacts");
        printf("       %d contacts\n", nct);
        tg_dialogs_free(ct, nct);
        {
            tg_dialog r;
            rc = tg_client_resolve(c, "@telegram", &r);
            CHECK(!rc && r.kind == 2, rc ? tg_client_error(c) : "resolveUsername @telegram -> channel");
            free(r.name);
            free(r.text);
        }
    }
    rc = tg_client_folders(c, &f, &nf);
    CHECK(!rc, rc ? tg_client_error(c) : "messages.getDialogFilters");
    for (i = 0; i < nf; i++) {
        int k, match = 0, inc = 0, exc = 0;
        for (k = 0; k < f[i].npeers; k++)
            f[i].peers[k].include ? inc++ : exc++;
        for (k = 0; k < n; k++)
            match += tg_folder_has(&f[i], &d[k]);
        printf("    folder %d: all %d id %d title %d bytes chatlist %d flags 0x%x include %d exclude %d"
               " -> matches %d of main\n", i, f[i].all, f[i].id, (int)strlen(f[i].title), f[i].chatlist,
               f[i].flags, inc, exc, match);
    }
    tg_folders_free(f, nf);
    tg_dialogs_free(d, n);
    tg_dialogs_free(ar, na);
    tg_client_free(c);
    return 0;
}

static int jpeg_ok(const char *path, long *size)
{
    unsigned char h[3] = {0, 0, 0};
    FILE *f = fopen(path, "rb");
    *size = 0;
    if (!f)
        return 0;
    fread(h, 1, 3, f);
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fclose(f);
    return h[0] == 0xff && h[1] == 0xd8 && h[2] == 0xff;
}

static int media(const char *dir, const char *out)
{
    tg_client *c = tg_client_new(dir, TG_API_ID, "", 0);
    tg_dialog *d = NULL;
    int n = 0, i, k, got = 0, seen[6] = {0}, rc;
    char p[400];
    long size;
    printf("[media: avatars + one photo, read-only]\n");
    rc = tg_client_dialogs(c, 1, 50, &d, &n);
    CHECK(!rc, rc ? tg_client_error(c) : "dialogs for avatars");
    for (i = 0; i < n && got < 4; i++) {
        tg_file_loc *loc = &d[i].photo;
        if (loc->type != TG_LOC_PEER_PHOTO || loc->dc < 1 || loc->dc > 5 || (seen[loc->dc] && got >= 2))
            continue;
        seen[loc->dc] = 1;
        snprintf(p, sizeof p, "%s/a%d.jpg", out, i);
        rc = tg_client_download(c, loc, p);
        CHECK(!rc && jpeg_ok(p, &size), rc ? tg_client_error(c) : "avatar download -> JPEG");
        printf("       avatar on DC%d: %ld bytes\n", loc->dc, size);
        got++;
    }
    for (i = 0; i < n; i++) {
        tg_message *m = NULL;
        int nm = 0;
        if (tg_client_history(c, &d[i], 0, 30, &m, &nm))
            continue;
        for (k = 0; k < nm && m[k].media.type == TG_LOC_NONE; k++)
            ;
        if (k < nm) {
            snprintf(p, sizeof p, "%s/m.jpg", out);
            rc = tg_client_download(c, &m[k].media, p);
            CHECK(!rc && jpeg_ok(p, &size), rc ? tg_client_error(c) : "message media download -> JPEG");
            printf("       media type %d size '%s' on DC%d: %ld bytes\n", m[k].media.type,
                   m[k].media.thumb, m[k].media.dc, size);
            tg_messages_free(m, nm);
            break;
        }
        tg_messages_free(m, nm);
    }
    tg_dialogs_free(d, n);
    tg_client_free(c);
    return 0;
}

static int qr_dump(const char *text)
{
    static unsigned char m[TG_QR_MAX * TG_QR_MAX];
    int n = tg_qr_encode(text, m), x, y;
    for (y = 0; y < n; y++) {
        for (x = 0; x < n; x++)
            putchar(m[y * n + x] ? '1' : '0');
        putchar('\n');
    }
    return n ? 0 : 1;
}

static int qr_login(const char *dir)
{
    static char url[600];
    int rc, i;
    const char *hash = getenv("TG_API_HASH");
    tg_client *c;
    if (!hash) {
        fprintf(stderr, "qr-login needs TG_API_HASH in the environment\n");
        return 1;
    }
    c = tg_client_new(dir, TG_API_ID, hash, 0);
    printf("[qr-login, fresh session in %s]\n", dir);
    for (i = 0; i < 2; i++) {
        rc = tg_client_qr_login(c, url, sizeof url);
        CHECK(rc == TG_OK && !strncmp(url, "tg://login?token=", 17),
              rc ? tg_client_error(c) : i ? "poll (updates.getState) -> still waiting"
                                          : "auth.exportLoginToken -> tg://login?token=...");
        printf("       url length %d\n", (int)strlen(url));
    }
    tg_client_free(c);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 && !strcmp(argv[1], "qr"))
        return qr_dump(argv[2]);
    const char *cmd = argc > 1 ? argv[1] : "selftest";
    if (tg_crypto_init()) {
        printf("crypto init failed\n");
        return 1;
    }
    if (!strcmp(cmd, "selftest"))
        selftest();
    else if (!strcmp(cmd, "ping"))
        fails += ping(1);
    else if (!strcmp(cmd, "ping-prod"))
        fails += ping(0);
    else if (!strcmp(cmd, "qr-login") && argc > 2)
        qr_login(argv[2]);
    else if (!strcmp(cmd, "media") && argc > 3)
        media(argv[2], argv[3]);
    else if (!strcmp(cmd, "inspect") && argc > 2)
        inspect(argv[2]);
    else if (!strcmp(cmd, "live") && argc > 2)
        fails += live(argv[2], getenv("TG_SHOW") != NULL);
    else {
        printf("usage: tg_cli selftest|ping|ping-prod|live <session dir>\n");
        return 2;
    }
    printf(fails ? "\n%d FAILED\n" : "\nall passed\n", fails);
    return fails ? 1 : 0;
}
