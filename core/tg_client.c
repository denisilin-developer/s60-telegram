#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tg_buf.h"
#include "tg_client.h"
#include "tg_crypto.h"
#include "tg_mtproto.h"
#include "tg_net.h"
#include "tg_tl.h"

#define TG_QR_MIGRATE 2

struct tg_client {
    char dir[200];
    int api_id;
    char api_hash[64];
    int test;
    int dc_id;
    tg_conn *conn;
    tg_session sess;
    int connected;
    char phone[32];
    char phone_code_hash[64];
    char err[200];
    unsigned char qr_token[256];
    int qr_token_len;
    int32_t qr_expires;
    int32_t state[4];
    int have_state;
    int status;
    int32_t status_when;
    struct {
        tg_conn *conn;
        tg_session sess;
        int ready;
    } links[6];
};

static void set_err(tg_client *c, const char *msg)
{
    strncpy(c->err, msg, sizeof c->err - 1);
    c->err[sizeof c->err - 1] = 0;
}

static void path(const tg_client *c, char *out, int n, const char *name)
{
    if (c->dir[0])
        snprintf(out, n, "%s/%s", c->dir, name);
    else
        snprintf(out, n, "%s", name);
}

static void key_path(const tg_client *c, int dc, char *out, int n)
{
    char name[48];
    snprintf(name, sizeof name, "session_%s_dc%d.key", c->test ? "test" : "prod", dc);
    path(c, out, n, name);
}

static int load_key(const tg_client *c, int dc, unsigned char key[256])
{
    char p[256];
    FILE *f;
    int n;
    key_path(c, dc, p, sizeof p);
    f = fopen(p, "rb");
    if (!f)
        return 0;
    n = (int)fread(key, 1, 256, f);
    fclose(f);
    return n == 256;
}

static void save_key(const tg_client *c, int dc, const unsigned char key[256])
{
    char p[256];
    FILE *f;
    key_path(c, dc, p, sizeof p);
    f = fopen(p, "wb");
    if (f) {
        fwrite(key, 1, 256, f);
        fclose(f);
    }
}

static int load_active_dc(const tg_client *c)
{
    char p[256], name[32];
    FILE *f;
    int dc = 0;
    snprintf(name, sizeof name, "active_dc_%s.txt", c->test ? "test" : "prod");
    path(c, p, sizeof p, name);
    f = fopen(p, "rb");
    if (f) {
        if (fscanf(f, "%d", &dc) != 1)
            dc = 0;
        fclose(f);
    }
    return dc >= 1 && dc <= 5 ? dc : 2;
}

static void save_active_dc(const tg_client *c)
{
    char p[256], name[32];
    FILE *f;
    snprintf(name, sizeof name, "active_dc_%s.txt", c->test ? "test" : "prod");
    path(c, p, sizeof p, name);
    f = fopen(p, "wb");
    if (f) {
        fprintf(f, "%d", c->dc_id);
        fclose(f);
    }
}

tg_client *tg_client_new(const char *dir, int api_id, const char *api_hash, int test)
{
    tg_client *c = (tg_client *)calloc(1, sizeof *c);
    if (!c)
        return NULL;
    strncpy(c->dir, dir ? dir : "", sizeof c->dir - 1);
    strncpy(c->api_hash, api_hash, sizeof c->api_hash - 1);
    c->api_id = api_id;
    c->test = test;
    c->dc_id = load_active_dc(c);
    if (tg_crypto_init()) {
        free(c);
        return NULL;
    }
    return c;
}

static void disconnect(tg_client *c)
{
    if (c->conn) {
        tg_send_acks(&c->sess);
        tg_conn_close(c->conn);
    }
    c->conn = NULL;
    c->connected = 0;
}

void tg_client_free(tg_client *c)
{
    int i;
    if (!c)
        return;
    disconnect(c);
    for (i = 0; i < 6; i++)
        if (c->links[i].conn)
            tg_conn_close(c->links[i].conn);
    memset(c, 0, sizeof *c);
    free(c);
}

const char *tg_client_error(const tg_client *c)
{
    return c->err;
}

int tg_client_connect(tg_client *c)
{
    unsigned char key[256], salt[8];
    int off = 0, rc, neterr = 0;
    const char *host = tg_dc_host(c->dc_id, c->test);
    disconnect(c);
    if (!host) {
        set_err(c, "unknown datacenter");
        return TG_ERR_PROTO;
    }
    c->conn = tg_conn_open(host, 443, &neterr);
    if (!c->conn) {
        snprintf(c->err, sizeof c->err, "cannot connect to Telegram (%d)", neterr);
        return TG_ERR_NET;
    }
    memset(salt, 0, sizeof salt);
    if (!load_key(c, c->dc_id, key)) {
        rc = tg_handshake(c->conn, c->dc_id, c->test, key, salt, &off, c->err, sizeof c->err);
        if (rc) {
            disconnect(c);
            return rc;
        }
        save_key(c, c->dc_id, key);
    }
    tg_session_init(&c->sess, c->conn, key, salt, off);
    memset(key, 0, sizeof key);
    c->connected = 1;
    return TG_OK;
}

static void wrap_layer(tg_client *c, tg_buf *out, const tg_buf *query)
{
    tg_buf_u32(out, TL_INVOKE_WITH_LAYER);
    tg_buf_i32(out, TL_LAYER);
    tg_buf_u32(out, TL_INIT_CONNECTION);
    tg_buf_i32(out, 0);
    tg_buf_i32(out, c->api_id);
    tg_buf_str(out, "Nokia E72");
    tg_buf_str(out, "Symbian S60 3.2");
    tg_buf_str(out, "0.1");
    tg_buf_str(out, "en");
    tg_buf_str(out, "");
    tg_buf_str(out, "en");
    tg_buf_put(out, query->p, query->len);
}

static int session_call(tg_client *c, tg_session *sess, const tg_buf *query, unsigned char **res,
                        int *rlen)
{
    int attempt, rc = TG_ERR_PROTO;
    if (query->oom) {
        set_err(c, "out of memory");
        return TG_ERR_NOMEM;
    }
    for (attempt = 0; attempt < 2; attempt++) {
        int wrapped = sess->new_session;
        tg_buf body;
        tg_buf_init(&body);
        if (wrapped)
            wrap_layer(c, &body, query);
        else
            tg_buf_put(&body, query->p, query->len);
        rc = body.oom ? TG_ERR_NOMEM : tg_call(sess, body.p, body.len, res, rlen);
        tg_buf_free(&body);
        if (rc == TG_OK || rc == TG_ERR_RPC)
            if (wrapped)
                sess->new_session = 0;
        if (rc == TG_ERR_RPC && strstr(sess->rpc_msg, "CONNECTION_NOT_INITED") && !wrapped) {
            sess->new_session = 1;
            continue;
        }
        break;
    }
    tg_send_acks(sess);
    if (rc == TG_ERR_RPC && sess->rpc_code == 401) {
        set_err(c, sess->rpc_msg);
        return TG_NEED_LOGIN;
    }
    if (rc != TG_OK)
        set_err(c, sess->err[0] ? sess->err : "request failed");
    return rc;
}

static int api_call(tg_client *c, const tg_buf *query, unsigned char **res, int *rlen)
{
    int rc;
    if (!c->connected && (rc = tg_client_connect(c)) != TG_OK)
        return rc;
    rc = session_call(c, &c->sess, query, res, rlen);
    if (rc == TG_ERR_NET)
        disconnect(c);
    return rc;
}

static int parse_result(tg_client *c, tg_arena *a, unsigned char *res, int rlen, tgv **out)
{
    tg_rd r;
    uint32_t unknown = 0;
    tg_rd_init(&r, res, rlen);
    *out = tg_parse(a, &r, &unknown);
    if (!*out) {
        if (unknown)
            snprintf(c->err, sizeof c->err, "unknown TL constructor 0x%08x", (unsigned)unknown);
        else
            set_err(c, "could not parse server reply");
        return TG_ERR_PROTO;
    }
    return TG_OK;
}

static char *dup_n(const unsigned char *p, int n)
{
    char *s = (char *)malloc(n + 1);
    if (s) {
        if (n)
            memcpy(s, p, n);
        s[n] = 0;
    }
    return s;
}

static char *dup_s(const char *s)
{
    return dup_n((const unsigned char *)s, (int)strlen(s));
}

static void flatten(char *s)
{
    for (; s && *s; s++)
        if (*s == '\n' || *s == '\r')
            *s = ' ';
}

static void fill_loc_bytes(tg_file_loc *loc, const tgv *o, int type)
{
    int n;
    const unsigned char *ref = tgv_str(o, "file_reference", &n);
    loc->type = type;
    loc->id = tgv_i64(o, "id", 0);
    loc->access_hash = tgv_i64(o, "access_hash", 0);
    loc->dc = tgv_i32(o, "dc_id", 0);
    if (n > (int)sizeof loc->ref)
        n = 0;
    if (n)
        memcpy(loc->ref, ref, n);
    loc->ref_len = n;
}

static int best_size(const tgv *sizes, int limit, char *type)
{
    int i, best = -1, best_dim = 0, small = -1, small_dim = 1 << 30, n;
    for (i = 0; i < tgv_vec_len(sizes); i++) {
        const tgv *z = tgv_vec_at(sizes, i);
        int w = tgv_i32(z, "w", 0), h = tgv_i32(z, "h", 0), dim = w > h ? w : h;
        if (!tgv_is(z, "photoSize") && !tgv_is(z, "photoSizeProgressive"))
            continue;
        if (dim <= limit && dim > best_dim) {
            best = i;
            best_dim = dim;
        }
        if (dim < small_dim) {
            small = i;
            small_dim = dim;
        }
    }
    if (best < 0)
        best = small;
    if (best < 0)
        return 0;
    {
        const unsigned char *t = tgv_str(tgv_vec_at(sizes, best), "type", &n);
        if (n < 1 || n > 7)
            return 0;
        memcpy(type, t, n);
        type[n] = 0;
    }
    return 1;
}

static int channel_readonly(const tgv *ch)
{
    return tgv_flag(ch, "broadcast") && !tgv_flag(ch, "creator") &&
           !tgv_flag(tgv_get(ch, "admin_rights"), "post_messages");
}

static void media_loc(const tgv *media, tg_message *m)
{
    tg_file_loc *loc = &m->media, *full = &m->full;
    memset(loc, 0, sizeof *loc);
    memset(full, 0, sizeof *full);
    memset(&m->thumb, 0, sizeof m->thumb);
    m->save_kind = TG_SAVE_NONE;
    m->file_name[0] = 0;
    if (tgv_is(media, "messageMediaPhoto")) {
        const tgv *ph = tgv_get(media, "photo");
        if (tgv_is(ph, "photo") && best_size(tgv_get(ph, "sizes"), 400, loc->thumb)) {
            fill_loc_bytes(loc, ph, TG_LOC_PHOTO);
            *full = *loc;
            m->thumb = *loc;
            best_size(tgv_get(ph, "sizes"), 1 << 20, full->thumb);
            best_size(tgv_get(ph, "sizes"), 120, m->thumb.thumb);
            m->save_kind = TG_SAVE_IMAGE;
        }
    } else if (tgv_is(media, "messageMediaDocument")) {
        const tgv *doc = tgv_get(media, "document"), *attrs;
        int n, i;
        const unsigned char *mime = tgv_str(doc, "mime_type", &n);
        if (!tgv_is(doc, "document"))
            return;
        fill_loc_bytes(full, doc, TG_LOC_DOCUMENT);
        if (n >= 6 && !memcmp(mime, "image/", 6))
            m->save_kind = TG_SAVE_IMAGE;
        else if (n >= 6 && !memcmp(mime, "video/", 6))
            m->save_kind = TG_SAVE_VIDEO;
        else if (n >= 6 && !memcmp(mime, "audio/", 6))
            m->save_kind = TG_SAVE_AUDIO;
        else
            m->save_kind = TG_SAVE_OTHER;
        attrs = tgv_get(doc, "attributes");
        for (i = 0; i < tgv_vec_len(attrs); i++)
            if (tgv_is(tgv_vec_at(attrs, i), "documentAttributeFilename")) {
                int len;
                const unsigned char *t = tgv_str(tgv_vec_at(attrs, i), "file_name", &len);
                if (len >= (int)sizeof m->file_name)
                    len = sizeof m->file_name - 1;
                memcpy(m->file_name, t, len);
                m->file_name[len] = 0;
            }
        if ((n >= 10 && !memcmp(mime, "image/webp", 10)) ||
            (n >= 11 && !memcmp(mime, "application", 11)))
            return;
        if (best_size(tgv_get(doc, "thumbs"), 400, loc->thumb)) {
            fill_loc_bytes(loc, doc, TG_LOC_DOCUMENT);
            m->thumb = *loc;
            best_size(tgv_get(doc, "thumbs"), 120, m->thumb.thumb);
        }
    }
}

static int peer_of(const tgv *peer, int64_t *id)
{
    if (tgv_is(peer, "peerUser")) {
        *id = tgv_i64(peer, "user_id", 0);
        return 0;
    }
    if (tgv_is(peer, "peerChat")) {
        *id = tgv_i64(peer, "chat_id", 0);
        return 1;
    }
    if (tgv_is(peer, "peerChannel")) {
        *id = tgv_i64(peer, "channel_id", 0);
        return 2;
    }
    *id = 0;
    return -1;
}

static int entity_kind(const tgv *e)
{
    const char *n = tgv_name(e);
    if (!strcmp(n, "user") || !strcmp(n, "userEmpty"))
        return 0;
    if (!strcmp(n, "chat") || !strcmp(n, "chatEmpty") || !strcmp(n, "chatForbidden"))
        return 1;
    if (!strcmp(n, "channel") || !strcmp(n, "channelForbidden"))
        return 2;
    return -1;
}

static char *display_name(const tgv *e)
{
    int n1, n2;
    const unsigned char *a, *b;
    char buf[160];
    if (entity_kind(e) == 0) {
        a = tgv_str(e, "first_name", &n1);
        b = tgv_str(e, "last_name", &n2);
        if (n1 + n2 > 0) {
            snprintf(buf, sizeof buf, "%.*s%s%.*s", n1, a ? (const char *)a : "",
                     n1 && n2 ? " " : "", n2, b ? (const char *)b : "");
            return dup_s(buf);
        }
        a = tgv_str(e, "phone", &n1);
        if (n1)
            return dup_n(a, n1);
        snprintf(buf, sizeof buf, "user %lld", (long long)tgv_i64(e, "id", 0));
        return dup_s(buf);
    }
    a = tgv_str(e, "title", &n1);
    return n1 ? dup_n(a, n1) : dup_s("chat");
}

static int g_lang;

void tg_set_lang(int lang)
{
    g_lang = lang;
}

static const char *tr(const char *en)
{
    static const char *const tab[][3] = {
    {"Photo", "Фото", "Фото"},
    {"Video", "Видео", "Відео"},
    {"Video message", "Видеосообщение", "Відеоповідомлення"},
    {"Voice message", "Голосовое сообщение", "Голосове повідомлення"},
    {"Music", "Музыка", "Музика"},
    {"GIF", "GIF", "GIF"},
    {"Sticker", "Стикер", "Стікер"},
    {"File", "Файл", "Файл"},
    {"Location", "Геопозиция", "Геопозиція"},
    {"Contact", "Контакт", "Контакт"},
    {"Poll", "Опрос", "Опитування"},
    {"Dice", "Кубик", "Кубик"},
    {"Link", "Ссылка", "Посилання"},
    {"Missed call", "Пропущенный звонок", "Пропущений дзвінок"},
    {"Missed video call", "Пропущенный видеозвонок", "Пропущений відеодзвінок"},
    {"Incoming call", "Входящий звонок", "Вхідний дзвінок"},
    {"Outgoing call", "Исходящий звонок", "Вихідний дзвінок"},
    {"Incoming video call", "Входящий видеозвонок", "Вхідний відеодзвінок"},
    {"Outgoing video call", "Исходящий видеозвонок", "Вихідний відеодзвінок"},
    {"added a member", "добавил(а) участника", "додав(ла) учасника"},
    {"joined the group", "присоединился(-ась) к группе", "приєднався(-лась) до групи"},
    {"left the group", "покинул(а) группу", "залишив(ла) групу"},
    {"pinned a message", "закрепил(а) сообщение", "закріпив(ла) повідомлення"},
    {"changed the group name", "изменил(а) название группы", "змінив(ла) назву групи"},
    {"changed the group photo", "изменил(а) фото группы", "змінив(ла) фото групи"},
    {"removed the group photo", "удалил(а) фото группы", "видалив(ла) фото групи"},
    {"created the group", "создал(а) группу", "створив(ла) групу"},
    {"channel created", "канал создан", "канал створено"},
    {"joined Telegram", "теперь в Telegram", "тепер у Telegram"},
    {"history cleared", "история очищена", "історію очищено"},
    {"took a screenshot", "сделал(а) скриншот", "зробив(ла) знімок екрана"},
    {"voice chat", "голосовой чат", "голосовий чат"},
    {"sent a gift", "отправил(а) подарок", "надіслав(ла) подарунок"},
    {"changed auto-delete timer", "изменил(а) таймер автоудаления", "змінив(ла) таймер автовидалення"},
    {"Saved Messages", "Избранное", "Збережене"},
    {"You", "Вы", "Ви"},
    {"message", "сообщение", "повідомлення"},
    {"unknown", "неизвестно", "невідомо"},
    {"[deleted message]", "[удалённое сообщение]", "[видалене повідомлення]"}
    };
    int i;
    if (g_lang)
        for (i = 0; i < (int)(sizeof tab / sizeof tab[0]); i++)
            if (!strcmp(tab[i][0], en))
                return tab[i][g_lang];
    return en;
}

static const char *action_text(const tgv *m, char *buf, int n)
{
    static const char *const names[][2] = {
        {"messageActionChatAddUser", "added a member"},
        {"messageActionChatJoinedByLink", "joined the group"},
        {"messageActionChatJoinedByRequest", "joined the group"},
        {"messageActionChatDeleteUser", "left the group"},
        {"messageActionPinMessage", "pinned a message"},
        {"messageActionChatEditTitle", "changed the group name"},
        {"messageActionChatEditPhoto", "changed the group photo"},
        {"messageActionChatDeletePhoto", "removed the group photo"},
        {"messageActionChatCreate", "created the group"},
        {"messageActionChannelCreate", "channel created"},
        {"messageActionContactSignUp", "joined Telegram"},
        {"messageActionHistoryClear", "history cleared"},
        {"messageActionScreenshotTaken", "took a screenshot"},
        {"messageActionGroupCall", "voice chat"},
        {"messageActionGiftPremium", "sent a gift"},
        {"messageActionStarGift", "sent a gift"},
        {"messageActionSetMessagesTTL", "changed auto-delete timer"},
    };
    const tgv *a = tgv_get(m, "action");
    const char *name = tgv_name(a);
    int i;
    if (tgv_is(a, "messageActionPhoneCall")) {
        int video = tgv_flag(a, "video"), dur = tgv_i32(a, "duration", 0);
        const char *what;
        if (tgv_is(tgv_get(a, "reason"), "phoneCallDiscardReasonMissed"))
            what = video ? "Missed video call" : "Missed call";
        else if (tgv_flag(m, "out"))
            what = video ? "Outgoing video call" : "Outgoing call";
        else
            what = video ? "Incoming video call" : "Incoming call";
        if (dur > 0)
            snprintf(buf, n, "%s (%d:%02d)", tr(what), dur / 60, dur % 60);
        else
            snprintf(buf, n, "%s", tr(what));
        return buf;
    }
    for (i = 0; i < (int)(sizeof names / sizeof names[0]); i++)
        if (!strcmp(name, names[i][0]))
            return tr(names[i][1]);
    snprintf(buf, n, "[%s]", !strncmp(name, "messageAction", 13) ? name + 13 : name);
    return buf;
}

static const char *media_text(const tgv *media, char *buf, int n)
{
    const char *name = tgv_name(media);
    if (tgv_is(media, "messageMediaPhoto"))
        return tr("Photo");
    if (tgv_is(media, "messageMediaDocument")) {
        const tgv *attrs = tgv_get(tgv_get(media, "document"), "attributes");
        int i, len;
        const unsigned char *t;
        if (tgv_flag(media, "voice"))
            return tr("Voice message");
        if (tgv_flag(media, "round"))
            return tr("Video message");
        for (i = 0; i < tgv_vec_len(attrs); i++) {
            const tgv *at = tgv_vec_at(attrs, i);
            if (tgv_is(at, "documentAttributeSticker")) {
                t = tgv_str(at, "alt", &len);
                snprintf(buf, n, "%s %.*s", tr("Sticker"), len, t ? (const char *)t : "");
                return buf;
            }
            if (tgv_is(at, "documentAttributeAnimated"))
                return tr("GIF");
            if (tgv_is(at, "documentAttributeAudio"))
                return tr(tgv_flag(at, "voice") ? "Voice message" : "Music");
        }
        for (i = 0; i < tgv_vec_len(attrs); i++)
            if (tgv_is(tgv_vec_at(attrs, i), "documentAttributeVideo"))
                return tr("Video");
        for (i = 0; i < tgv_vec_len(attrs); i++) {
            const tgv *at = tgv_vec_at(attrs, i);
            if (tgv_is(at, "documentAttributeFilename")) {
                t = tgv_str(at, "file_name", &len);
                snprintf(buf, n, "%s: %.*s", tr("File"), len, t ? (const char *)t : "");
                return buf;
            }
        }
        return tr("File");
    }
    if (tgv_is(media, "messageMediaGeo") || tgv_is(media, "messageMediaGeoLive") ||
        tgv_is(media, "messageMediaVenue")) {
        const tgv *geo = tgv_get(media, "geo"), *la = tgv_get(geo, "lat"), *lo = tgv_get(geo, "long");
        if (la && lo && la->kind == TGV_DOUBLE && lo->kind == TGV_DOUBLE) {
            snprintf(buf, n, "%s %.5f, %.5f", tr("Location"), la->u.d, lo->u.d);
            return buf;
        }
        return tr("Location");
    }
    if (tgv_is(media, "messageMediaContact"))
        return tr("Contact");
    if (tgv_is(media, "messageMediaPoll"))
        return tr("Poll");
    if (tgv_is(media, "messageMediaDice"))
        return tr("Dice");
    if (tgv_is(media, "messageMediaWebPage"))
        return tr("Link");
    snprintf(buf, n, "[%s]", !strncmp(name, "messageMedia", 12) ? name + 12 : name);
    return buf;
}

static const char *emoji_text(unsigned cp)
{
    static const struct {
        unsigned cp;
        const char *text;
    } map[] = {
        {0x2764, "<3"},  {0x1F499, "<3"}, {0x1F49B, "<3"}, {0x1F49A, "<3"}, {0x1F49C, "<3"},
        {0x1F5A4, "<3"}, {0x1F90D, "<3"}, {0x1F60D, "<3"}, {0x1F970, "<3"}, {0x1F618, ":*"},
        {0x1F44D, "+1"}, {0x1F44E, "-1"}, {0x1F642, ":)"}, {0x1F60A, ":)"}, {0x263A, ":)"},
        {0x1F600, ":D"}, {0x1F601, ":D"}, {0x1F603, ":D"}, {0x1F604, ":D"}, {0x1F602, ":'D"},
        {0x1F923, ":'D"}, {0x1F609, ";)"}, {0x1F61C, ";P"}, {0x1F61B, ":P"}, {0x1F622, ":'("},
        {0x1F62D, ":'("}, {0x1F61E, ":("}, {0x1F641, ":("}, {0x2639, ":("}, {0x1F621, ">:("},
        {0x1F620, ">:("}, {0x1F62E, ":O"}, {0x1F631, ":O"}, {0x2757, "!"},  {0x2755, "!"},
        {0x2753, "?"},   {0x2754, "?"},   {0x203C, "!!"},  {0x2049, "!?"},  {0x2B50, "*"},
        {0x2705, "+"},   {0x2714, "+"},   {0x274C, "x"},   {0x2716, "x"},   {0x27A1, "->"},
        {0x2B05, "<-"},  {0x2192, "->"},  {0x2190, "<-"},
    };
    int i;
    for (i = 0; i < (int)(sizeof map / sizeof map[0]); i++)
        if (map[i].cp == cp)
            return map[i].text;
    return NULL;
}

void tg_text_fix(char *s)
{
    unsigned char *r = (unsigned char *)s, *w = r;
    if (!s)
        return;
    while (*r) {
        unsigned cp = *r;
        int len = 1, i;
        const char *t;
        if (cp >= 0xf0 && (r[1] & 0xc0) == 0x80 && (r[2] & 0xc0) == 0x80 && (r[3] & 0xc0) == 0x80) {
            cp = (cp & 7) << 18 | (r[1] & 63) << 12 | (r[2] & 63) << 6 | (r[3] & 63);
            len = 4;
        } else if (cp >= 0xe0 && (r[1] & 0xc0) == 0x80 && (r[2] & 0xc0) == 0x80) {
            cp = (cp & 15) << 12 | (r[1] & 63) << 6 | (r[2] & 63);
            len = 3;
        } else if (cp >= 0xc0 && (r[1] & 0xc0) == 0x80) {
            len = 2;
            cp = (cp & 31) << 6 | (r[1] & 63);
        }
        t = len > 1 ? emoji_text(cp) : NULL;
        if (t) {
            while (*t)
                *w++ = (unsigned char)*t++;
        } else if (cp >= 0x1F1E6 && cp <= 0x1F1FF) {
            *w++ = (unsigned char)('A' + cp - 0x1F1E6);
        } else if (!(cp >= 0x10000 || (len == 1 && cp >= 0x80) || (cp >= 0x2190 && cp < 0x2C00) ||
                     (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0x200B && cp <= 0x200F) ||
                     (cp >= 0x2060 && cp <= 0x206F) || cp == 0x20E3)) {
            for (i = 0; i < len; i++)
                *w++ = r[i];
        }
        r += len;
    }
    while (w > (unsigned char *)s && w[-1] == ' ')
        w--;
    *w = 0;
    for (r = w = (unsigned char *)s; *r; r++)
        if (!(*r == ' ' && (w == (unsigned char *)s || w[-1] == ' ')))
            *w++ = *r;
    *w = 0;
}

static char *strip_custom_emoji(const tgv *m, const unsigned char *t, int n)
{
    const tgv *ents = tgv_get(m, "entities");
    char *out = (char *)malloc(n + 1);
    int i = 0, o = 0, u16 = 0, k;
    if (!out)
        return NULL;
    while (i < n) {
        int len = t[i] >= 0xf0 ? 4 : t[i] >= 0xe0 ? 3 : t[i] >= 0xc0 ? 2 : 1, skip = 0;
        if (i + len > n)
            len = n - i;
        for (k = 0; k < tgv_vec_len(ents) && !skip; k++) {
            const tgv *e = tgv_vec_at(ents, k);
            int off = tgv_i32(e, "offset", 0);
            skip = tgv_is(e, "messageEntityCustomEmoji") && u16 >= off &&
                   u16 < off + tgv_i32(e, "length", 0);
        }
        if (!skip) {
            memcpy(out + o, t + i, len);
            o += len;
        }
        i += len;
        u16 += len == 4 ? 2 : 1;
    }
    out[o] = 0;
    return out;
}

static char *message_text(const tgv *m)
{
    int n;
    const unsigned char *t;
    const tgv *media;
    char buf[160], *out, *s;
    if (tgv_is(m, "messageService"))
        return dup_s(action_text(m, buf, sizeof buf));
    if (!tgv_is(m, "message"))
        return dup_s(tr("[deleted message]"));
    t = tgv_str(m, "message", &n);
    s = strip_custom_emoji(m, t, n);
    if (!s)
        return NULL;
    n = (int)strlen(s);
    media = tgv_get(m, "media");
    if (media && !tgv_is(media, "messageMediaEmpty") && !tgv_is(media, "messageMediaWebPage")) {
        const char *what = media_text(media, buf, sizeof buf);
        int size = (int)strlen(what) + n + 3;
        out = (char *)malloc(size);
        if (out)
            snprintf(out, size, n ? "%s: %s" : "%s", what, s);
        free(s);
        return out;
    }
    return s;
}

int tg_client_dialogs(tg_client *c, int folder, int limit, tg_dialog **out, int *count)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc, i, j, nd, nm;
    tg_arena a;
    tgv *top, *dialogs, *msgs, *ents[2];
    tg_dialog *d;
    *out = NULL;
    *count = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_GET_DIALOGS);
    tg_buf_i32(&q, 2);
    tg_buf_i32(&q, folder);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_u32(&q, TL_INPUT_PEER_EMPTY);
    tg_buf_i32(&q, limit);
    tg_buf_i64(&q, 0);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc)
        return rc;
    tg_arena_init(&a);
    rc = parse_result(c, &a, res, rlen, &top);
    if (rc)
        goto done;
    dialogs = tgv_get(top, "dialogs");
    msgs = tgv_get(top, "messages");
    ents[0] = tgv_get(top, "users");
    ents[1] = tgv_get(top, "chats");
    nd = tgv_vec_len(dialogs);
    nm = tgv_vec_len(msgs);
    d = (tg_dialog *)calloc(nd ? nd : 1, sizeof *d);
    if (!d) {
        rc = TG_ERR_NOMEM;
        goto done;
    }
    for (i = 0; i < nd; i++) {
        tgv *dlg = tgv_vec_at(dialogs, i), *best = NULL, *ent = NULL;
        int kind, g;
        int64_t id, best_id = -1;
        if (!tgv_is(dlg, "dialog") || tgv_i32(dlg, "folder_id", 0) != folder)
            continue;
        kind = peer_of(tgv_get(dlg, "peer"), &id);
        for (g = 0; g < 2 && !ent; g++)
            for (j = 0; j < tgv_vec_len(ents[g]); j++) {
                tgv *e = tgv_vec_at(ents[g], j);
                if (entity_kind(e) == kind && tgv_i64(e, "id", 0) == id) {
                    ent = e;
                    break;
                }
            }
        for (j = 0; j < nm; j++) {
            tgv *m = tgv_vec_at(msgs, j);
            int64_t pid;
            if (peer_of(tgv_get(m, "peer_id"), &pid) == kind && pid == id &&
                tgv_i64(m, "id", 0) > best_id) {
                best = m;
                best_id = tgv_i64(m, "id", 0);
            }
        }
        d[*count].kind = kind;
        d[*count].id = id;
        if (kind == 0 && ent) {
            if (tgv_flag(ent, "contact"))
                d[*count].flags |= TG_DLG_CONTACT;
            if (tgv_flag(ent, "bot"))
                d[*count].flags |= TG_DLG_BOT;
            if (tgv_flag(ent, "self"))
                d[*count].flags |= TG_DLG_SELF;
        } else if (kind == 1 || (kind == 2 && ent && tgv_flag(ent, "megagroup"))) {
            d[*count].flags |= TG_DLG_GROUP;
        } else if (kind == 2) {
            d[*count].flags |= TG_DLG_CHANNEL;
            if (ent && channel_readonly(ent))
                d[*count].flags |= TG_DLG_READONLY;
        }
        if (tgv_i32(tgv_get(dlg, "notify_settings"), "mute_until", 0) >
            (int32_t)time(NULL) + c->sess.time_offset)
            d[*count].flags |= TG_DLG_MUTED;
        if (tgv_flag(dlg, "unread_mark"))
            d[*count].flags |= TG_DLG_UNREAD_MARK;
        if (tgv_flag(dlg, "pinned"))
            d[*count].flags |= TG_DLG_PINNED;
        if (ent) {
            const tgv *ph = tgv_get(ent, "photo");
            if (tgv_is(ph, "userProfilePhoto") || tgv_is(ph, "chatPhoto")) {
                tg_file_loc *loc = &d[*count].photo;
                loc->type = TG_LOC_PEER_PHOTO;
                loc->id = tgv_i64(ph, "photo_id", 0);
                loc->dc = tgv_i32(ph, "dc_id", 0);
                loc->peer_kind = kind;
                loc->peer_id = id;
                loc->peer_hash = tgv_i64(ent, "access_hash", 0);
            }
        }
        if (folder == 1)
            d[*count].flags |= TG_DLG_ARCHIVED;
        d[*count].access_hash = ent ? tgv_i64(ent, "access_hash", 0) : 0;
        d[*count].unread = tgv_i32(dlg, "unread_count", 0);
        d[*count].read_max = tgv_i32(dlg, "read_inbox_max_id", 0);
        d[*count].read_out_max = tgv_i32(dlg, "read_outbox_max_id", 0);
        d[*count].top_id = tgv_i32(dlg, "top_message", 0);
        if (d[*count].flags & TG_DLG_SELF)
            d[*count].name = dup_s(tr("Saved Messages"));
        else if (ent)
            d[*count].name = display_name(ent);
        else {
            char buf[48];
            snprintf(buf, sizeof buf, "chat %lld", (long long)id);
            d[*count].name = dup_s(buf);
        }
        tg_text_fix(d[*count].name);
        d[*count].text = best ? message_text(best) : dup_s("");
        flatten(d[*count].text);
        tg_text_fix(d[*count].text);
        d[*count].out = best ? tgv_flag(best, "out") : 0;
        d[*count].date = best ? tgv_i32(best, "date", 0) : 0;
        (*count)++;
    }
    *out = d;
done:
    tg_arena_free(&a);
    free(res);
    return rc;
}

static void put_input_peer(tg_buf *b, const tg_dialog *d)
{
    if (d->kind == 1) {
        tg_buf_u32(b, TL_INPUT_PEER_CHAT);
        tg_buf_i64(b, d->id);
    } else {
        tg_buf_u32(b, d->kind == 2 ? TL_INPUT_PEER_CHANNEL : TL_INPUT_PEER_USER);
        tg_buf_i64(b, d->id);
        tg_buf_i64(b, d->access_hash);
    }
}

static tgv *find_entity(tgv *top, const tgv *peer)
{
    int64_t id;
    int kind = peer_of(peer, &id), g, j;
    const char *lists[2] = {"users", "chats"};
    if (kind < 0)
        return NULL;
    for (g = 0; g < 2; g++) {
        tgv *v = tgv_get(top, lists[g]);
        for (j = 0; j < tgv_vec_len(v); j++)
            if (entity_kind(tgv_vec_at(v, j)) == kind && tgv_i64(tgv_vec_at(v, j), "id", 0) == id)
                return tgv_vec_at(v, j);
    }
    return NULL;
}

static unsigned first_codepoint(const unsigned char *t, int n)
{
    if (n >= 4 && t[0] >= 0xf0)
        return (t[0] & 7) << 18 | (t[1] & 63) << 12 | (t[2] & 63) << 6 | (t[3] & 63);
    if (n >= 3 && t[0] >= 0xe0)
        return (t[0] & 15) << 12 | (t[1] & 63) << 6 | (t[2] & 63);
    if (n >= 2 && t[0] >= 0xc0)
        return (t[0] & 31) << 6 | (t[1] & 63);
    return n ? t[0] : 0;
}

static void reactions_info(const tgv *msg, tg_message *m)
{
    const tgv *res = tgv_get(tgv_get(msg, "reactions"), "results");
    char full[160], brief[48];
    int i, len = 0, blen = 0, kinds = 0, best = -1;
    full[0] = brief[0] = 0;
    for (i = 0; i < tgv_vec_len(res) && len < (int)sizeof full - 24; i++) {
        const tgv *rc = tgv_vec_at(res, i), *r = tgv_get(rc, "reaction");
        char e[32];
        int n = 0, count = tgv_i32(rc, "count", 0);
        const unsigned char *t = tgv_is(r, "reactionEmoji") ? tgv_str(r, "emoticon", &n) : NULL;
        m->reaction_total += count;
        if (count > best && n) {
            best = count;
            m->top_reaction = first_codepoint(t, n);
        }
        if (n >= (int)sizeof e)
            n = sizeof e - 1;
        if (n)
            memcpy(e, t, n);
        e[n] = 0;
        if (m->react_kinds < 8 && count > 0) {
            m->react_cp[m->react_kinds] = n ? first_codepoint(t, n) : 0;
            m->react_count[m->react_kinds] = count;
        }
        tg_text_fix(e);
        if (m->react_kinds < 8 && count > 0) {
            strncpy(m->react_text[m->react_kinds], e, sizeof m->react_text[0] - 1);
            m->react_kinds++;
        }
        len += snprintf(full + len, sizeof full - len, "%s%s\xC3\x97%d", len ? "  " : "",
                        e[0] ? e : "*", count);
        if (e[0] && kinds < 2) {
            blen += snprintf(brief + blen, sizeof brief - blen, "%s%s\xC3\x97%d", blen ? " " : "", e,
                             count);
            kinds++;
        }
    }
    m->reactions = len ? dup_s(full) : NULL;
    m->reactions_short = blen ? dup_s(brief) : NULL;
}

static void set_status(tg_client *c, const tgv *user)
{
    const tgv *st = tgv_get(user, "status");
    c->status_when = 0;
    if (tgv_is(st, "userStatusOnline")) {
        c->status = TG_STATUS_ONLINE;
    } else if (tgv_is(st, "userStatusOffline")) {
        c->status = TG_STATUS_SEEN;
        c->status_when = tgv_i32(st, "was_online", 0);
    } else if (tgv_is(st, "userStatusRecently")) {
        c->status = TG_STATUS_RECENTLY;
    } else if (tgv_is(st, "userStatusLastWeek")) {
        c->status = TG_STATUS_WEEK;
    } else if (tgv_is(st, "userStatusLastMonth")) {
        c->status = TG_STATUS_MONTH;
    } else {
        c->status = TG_STATUS_NONE;
    }
}

int tg_client_status(const tg_client *c, int32_t *when)
{
    *when = c->status_when;
    return c->status;
}

int tg_client_history(tg_client *c, const tg_dialog *d, int32_t offset_id, int limit,
                      tg_message **out, int *count)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc, i, n;
    tg_arena a;
    tgv *top, *msgs;
    tg_message *m;
    *out = NULL;
    *count = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_GET_HISTORY);
    put_input_peer(&q, d);
    tg_buf_i32(&q, offset_id);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, limit);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i64(&q, 0);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc)
        return rc;
    tg_arena_init(&a);
    rc = parse_result(c, &a, res, rlen, &top);
    if (rc)
        goto done;
    msgs = tgv_get(top, "messages");
    n = tgv_vec_len(msgs);
    c->status = TG_STATUS_NONE;
    if (d->kind == 0) {
        tgv *users = tgv_get(top, "users");
        for (i = 0; i < tgv_vec_len(users); i++)
            if (tgv_i64(tgv_vec_at(users, i), "id", 0) == d->id)
                set_status(c, tgv_vec_at(users, i));
    }
    m = (tg_message *)calloc(n ? n : 1, sizeof *m);
    if (!m) {
        rc = TG_ERR_NOMEM;
        goto done;
    }
    for (i = n - 1; i >= 0; i--) {
        tgv *msg = tgv_vec_at(msgs, i), *ent;
        const tgv *rh = tgv_get(msg, "reply_to"), *fwd = tgv_get(msg, "fwd_from");
        tg_message *cur = &m[*count];
        int j;
        if (!tgv_is(msg, "message") && !tgv_is(msg, "messageService"))
            continue;
        cur->id = tgv_i32(msg, "id", 0);
        cur->out = tgv_flag(msg, "out");
        if (!cur->out && d->kind != 0 && (ent = find_entity(top, tgv_get(msg, "from_id"))) &&
            entity_kind(ent) == 0) {
            cur->from = display_name(ent);
            tg_text_fix(cur->from);
        }
        cur->date = tgv_i32(msg, "date", 0);
        cur->edited = tgv_get(msg, "edit_date") && !tgv_flag(msg, "edit_hide");
        cur->grouped_id = tgv_i64(msg, "grouped_id", 0);
        media_loc(tgv_get(msg, "media"), cur);
        {
            int tn;
            tgv_str(msg, "message", &tn);
            cur->has_text = tn > 0;
        }
        cur->text = message_text(msg);
        tg_text_fix(cur->text);
        reactions_info(msg, cur);
        if (fwd) {
            ent = find_entity(top, tgv_get(fwd, "from_id"));
            if (ent) {
                cur->fwd = display_name(ent);
            } else {
                int fn;
                const unsigned char *ft = tgv_str(fwd, "from_name", &fn);
                cur->fwd = fn ? dup_n(ft, fn) : dup_s(tr("unknown"));
            }
            tg_text_fix(cur->fwd);
        }
        if (tgv_is(rh, "messageReplyHeader")) {
            cur->reply_to = tgv_i32(rh, "reply_to_msg_id", 0);
            for (j = 0; j < n; j++) {
                tgv *o = tgv_vec_at(msgs, j);
                if (tgv_i32(o, "id", -1) == cur->reply_to) {
                    char *who = NULL, *txt = message_text(o), buf[200];
                    if (tgv_flag(o, "out"))
                        who = dup_s(tr("You"));
                    else if ((ent = find_entity(top, tgv_get(o, "from_id"))) ||
                             (ent = find_entity(top, tgv_get(o, "peer_id"))))
                        who = display_name(ent);
                    flatten(txt);
                    snprintf(buf, sizeof buf, "%s: %.120s", who ? who : "", txt ? txt : "");
                    cur->reply = dup_s(buf);
                    tg_text_fix(cur->reply);
                    free(who);
                    free(txt);
                    break;
                }
            }
            if (!cur->reply && cur->reply_to)
                cur->reply = dup_s(tr("message"));
        }
        (*count)++;
    }
    *out = m;
done:
    tg_arena_free(&a);
    free(res);
    return rc;
}

static void put_input_channel(tg_buf *q, const tg_dialog *d)
{
    tg_buf_u32(q, TL_INPUT_CHANNEL);
    tg_buf_i64(q, d->id);
    tg_buf_i64(q, d->access_hash);
}

static int simple_call(tg_client *c, tg_buf *q)
{
    unsigned char *res = NULL;
    int rlen, rc = api_call(c, q, &res, &rlen);
    tg_buf_free(q);
    free(res);
    return rc;
}

int tg_client_edit(tg_client *c, const tg_dialog *d, int32_t id, const char *utf8)
{
    tg_buf q;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_EDIT_MESSAGE);
    tg_buf_i32(&q, 1 << 11);
    put_input_peer(&q, d);
    tg_buf_i32(&q, id);
    tg_buf_str(&q, utf8);
    return simple_call(c, &q);
}

int tg_client_delete(tg_client *c, const tg_dialog *d, const int32_t *ids, int n, int revoke)
{
    tg_buf q;
    int i;
    tg_buf_init(&q);
    if (d->kind == 2) {
        tg_buf_u32(&q, TL_CHANNELS_DELETE_MESSAGES);
        put_input_channel(&q, d);
    } else {
        tg_buf_u32(&q, TL_MESSAGES_DELETE_MESSAGES);
        tg_buf_i32(&q, revoke ? 1 : 0);
    }
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, n);
    for (i = 0; i < n; i++)
        tg_buf_i32(&q, ids[i]);
    return simple_call(c, &q);
}

int tg_client_forward(tg_client *c, const tg_dialog *from, int32_t id, const tg_dialog *to)
{
    tg_buf q;
    int64_t random_id;
    tg_random(&random_id, sizeof random_id);
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_FORWARD_MESSAGES);
    tg_buf_i32(&q, 0);
    put_input_peer(&q, from);
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, 1);
    tg_buf_i32(&q, id);
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, 1);
    tg_buf_i64(&q, random_id);
    put_input_peer(&q, to);
    return simple_call(c, &q);
}

int tg_client_react(tg_client *c, const tg_dialog *d, int32_t id, const char *emoticon)
{
    tg_buf q;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_SEND_REACTION);
    tg_buf_i32(&q, emoticon ? 1 : 0);
    put_input_peer(&q, d);
    tg_buf_i32(&q, id);
    if (emoticon) {
        tg_buf_u32(&q, TL_VECTOR);
        tg_buf_i32(&q, 1);
        tg_buf_u32(&q, TL_REACTION_EMOJI);
        tg_buf_str(&q, emoticon);
    }
    return simple_call(c, &q);
}

int tg_client_pinned(tg_client *c, const tg_dialog *d, char **text)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    *text = NULL;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_SEARCH);
    tg_buf_i32(&q, 0);
    put_input_peer(&q, d);
    tg_buf_str(&q, "");
    tg_buf_u32(&q, TL_INPUT_MESSAGES_FILTER_PINNED);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 1);
    tg_buf_i32(&q, 0);
    tg_buf_i32(&q, 0);
    tg_buf_i64(&q, 0);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK) {
        tg_arena a;
        tgv *top;
        tg_arena_init(&a);
        rc = parse_result(c, &a, res, rlen, &top);
        if (!rc && tgv_vec_len(tgv_get(top, "messages")) > 0) {
            *text = message_text(tgv_vec_at(tgv_get(top, "messages"), 0));
            flatten(*text);
            tg_text_fix(*text);
        }
        tg_arena_free(&a);
    }
    free(res);
    return rc;
}

int tg_client_chat_action(tg_client *c, const tg_dialog *d, int action)
{
    tg_buf q;
    tg_buf_init(&q);
    switch (action) {
    case TG_ACT_PIN:
    case TG_ACT_UNPIN:
        tg_buf_u32(&q, TL_MESSAGES_TOGGLE_DIALOG_PIN);
        tg_buf_i32(&q, action == TG_ACT_PIN ? 1 : 0);
        tg_buf_u32(&q, TL_INPUT_DIALOG_PEER);
        put_input_peer(&q, d);
        break;
    case TG_ACT_MUTE:
    case TG_ACT_UNMUTE:
        tg_buf_u32(&q, TL_ACCOUNT_UPDATE_NOTIFY_SETTINGS);
        tg_buf_u32(&q, TL_INPUT_NOTIFY_PEER);
        put_input_peer(&q, d);
        tg_buf_u32(&q, TL_INPUT_PEER_NOTIFY_SETTINGS);
        tg_buf_i32(&q, 1 << 2);
        tg_buf_i32(&q, action == TG_ACT_MUTE ? 0x7fffffff : 0);
        break;
    case TG_ACT_READ:
        if (d->unread && tg_client_read_history(c, d, d->top_id) != TG_OK) {
            tg_buf_free(&q);
            return TG_ERR_RPC;
        }
        if (!(d->flags & TG_DLG_UNREAD_MARK)) {
            tg_buf_free(&q);
            return TG_OK;
        }
    case TG_ACT_UNREAD:
        tg_buf_u32(&q, TL_MESSAGES_MARK_DIALOG_UNREAD);
        tg_buf_i32(&q, action == TG_ACT_UNREAD ? 1 : 0);
        tg_buf_u32(&q, TL_INPUT_DIALOG_PEER);
        put_input_peer(&q, d);
        break;
    case TG_ACT_ARCHIVE:
    case TG_ACT_UNARCHIVE:
        tg_buf_u32(&q, TL_FOLDERS_EDIT_PEER_FOLDERS);
        tg_buf_u32(&q, TL_VECTOR);
        tg_buf_i32(&q, 1);
        tg_buf_u32(&q, TL_INPUT_FOLDER_PEER);
        put_input_peer(&q, d);
        tg_buf_i32(&q, action == TG_ACT_ARCHIVE ? 1 : 0);
        break;
    default:
        tg_buf_free(&q);
        set_err(c, "unknown action");
        return TG_ERR_PROTO;
    }
    return simple_call(c, &q);
}

static char *opt_str(const tgv *o, const char *field)
{
    int n;
    const unsigned char *t = tgv_str(o, field, &n);
    char *r;
    if (!n)
        return NULL;
    r = dup_n(t, n);
    tg_text_fix(r);
    return r;
}

static void dialog_from_entity(tg_dialog *d, const tgv *ent)
{
    tg_file_loc *loc = &d->photo;
    const tgv *ph = tgv_get(ent, "photo");
    memset(d, 0, sizeof *d);
    d->kind = entity_kind(ent);
    d->id = tgv_i64(ent, "id", 0);
    d->access_hash = tgv_i64(ent, "access_hash", 0);
    d->name = display_name(ent);
    tg_text_fix(d->name);
    d->text = dup_s("");
    if (d->kind == 0) {
        if (tgv_flag(ent, "contact"))
            d->flags |= TG_DLG_CONTACT;
        if (tgv_flag(ent, "bot"))
            d->flags |= TG_DLG_BOT;
    } else if (d->kind == 1 || tgv_flag(ent, "megagroup")) {
        d->flags |= TG_DLG_GROUP;
    } else {
        d->flags |= TG_DLG_CHANNEL;
        if (channel_readonly(ent))
            d->flags |= TG_DLG_READONLY;
    }
    if (tgv_is(ph, "userProfilePhoto") || tgv_is(ph, "chatPhoto")) {
        loc->type = TG_LOC_PEER_PHOTO;
        loc->id = tgv_i64(ph, "photo_id", 0);
        loc->dc = tgv_i32(ph, "dc_id", 0);
        loc->peer_kind = d->kind;
        loc->peer_id = d->id;
        loc->peer_hash = d->access_hash;
    }
}

static int call_parse(tg_client *c, tg_buf *q, tg_arena *a, unsigned char **res, tgv **top)
{
    int rlen, rc = api_call(c, q, res, &rlen);
    tg_buf_free(q);
    tg_arena_init(a);
    if (rc == TG_OK)
        rc = parse_result(c, a, *res, rlen, top);
    return rc;
}

int tg_client_info(tg_client *c, const tg_dialog *d, tg_info *out)
{
    tg_buf q;
    tg_arena a;
    unsigned char *res = NULL;
    tgv *top;
    int rc, i;
    memset(out, 0, sizeof *out);
    tg_buf_init(&q);
    if (d->kind == 0) {
        tg_buf_u32(&q, TL_USERS_GET_FULL_USER);
        if (d->flags & TG_DLG_SELF) {
            tg_buf_u32(&q, TL_INPUT_USER_SELF);
        } else {
            tg_buf_u32(&q, TL_INPUT_USER);
            tg_buf_i64(&q, d->id);
            tg_buf_i64(&q, d->access_hash);
        }
    } else if (d->kind == 2) {
        tg_buf_u32(&q, TL_CHANNELS_GET_FULL_CHANNEL);
        put_input_channel(&q, d);
    } else {
        tg_buf_u32(&q, TL_MESSAGES_GET_FULL_CHAT);
        tg_buf_i64(&q, d->id);
    }
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc == TG_OK) {
        tgv *full = tgv_get(top, d->kind == 0 ? "full_user" : "full_chat");
        tgv *list = tgv_get(top, d->kind == 0 ? "users" : "chats");
        out->about = opt_str(full, "about");
        if (d->kind == 2) {
            out->members = tgv_i32(full, "participants_count", 0);
        } else if (d->kind == 1) {
            out->members = tgv_vec_len(tgv_get(tgv_get(full, "participants"), "participants"));
        }
        for (i = 0; i < tgv_vec_len(list); i++) {
            tgv *e = tgv_vec_at(list, i);
            if (tgv_i64(e, "id", 0) != d->id && !(d->flags & TG_DLG_SELF && tgv_flag(e, "self")))
                continue;
            out->name = display_name(e);
            tg_text_fix(out->name);
            out->username = opt_str(e, "username");
            out->phone = opt_str(e, "phone");
            out->channel = d->kind == 2 && tgv_flag(e, "broadcast");
            break;
        }
    }
    tg_arena_free(&a);
    free(res);
    return rc;
}

void tg_info_free(tg_info *info)
{
    free(info->name);
    free(info->username);
    free(info->phone);
    free(info->about);
    memset(info, 0, sizeof *info);
}

int tg_client_contacts(tg_client *c, tg_dialog **out, int *count)
{
    tg_buf q;
    tg_arena a;
    unsigned char *res = NULL;
    tgv *top;
    int rc, i;
    *out = NULL;
    *count = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_CONTACTS_GET_CONTACTS);
    tg_buf_i64(&q, 0);
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc == TG_OK) {
        tgv *users = tgv_get(top, "users");
        int n = tgv_vec_len(users);
        tg_dialog *d = (tg_dialog *)calloc(n ? n : 1, sizeof *d);
        if (!d) {
            rc = TG_ERR_NOMEM;
        } else {
            for (i = 0; i < n; i++)
                if (tgv_is(tgv_vec_at(users, i), "user") && !tgv_flag(tgv_vec_at(users, i), "deleted"))
                    dialog_from_entity(&d[(*count)++], tgv_vec_at(users, i));
            *out = d;
        }
    }
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_client_resolve(tg_client *c, const char *username, tg_dialog *out)
{
    tg_buf q;
    tg_arena a;
    unsigned char *res = NULL;
    tgv *top, *ent;
    int rc;
    memset(out, 0, sizeof *out);
    if (*username == '@')
        username++;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_CONTACTS_RESOLVE_USERNAME);
    tg_buf_i32(&q, 0);
    tg_buf_str(&q, username);
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc == TG_OK) {
        ent = find_entity(top, tgv_get(top, "peer"));
        if (ent) {
            dialog_from_entity(out, ent);
        } else {
            set_err(c, "not found");
            rc = TG_ERR_PROTO;
        }
    } else if (rc == TG_ERR_RPC && strstr(c->sess.rpc_msg, "USERNAME_NOT_OCCUPIED")) {
        set_err(c, "no one has this username");
    }
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_client_password_hint(tg_client *c, char *hint, int hint_len)
{
    tg_buf q;
    tg_arena a;
    unsigned char *res = NULL;
    tgv *top;
    int rc, n;
    const unsigned char *t;
    hint[0] = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_ACCOUNT_GET_PASSWORD);
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc == TG_OK && (t = tgv_str(top, "hint", &n)) && n) {
        if (n >= hint_len)
            n = hint_len - 1;
        memcpy(hint, t, n);
        hint[n] = 0;
        tg_text_fix(hint);
    }
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_client_check_password(tg_client *c, const char *password)
{
    tg_buf q;
    tg_arena a;
    unsigned char *res = NULL, x[32], ar[256], A[256], m1[32];
    tgv *top, *algo;
    int rc, n1 = 0, n2 = 0, np = 0, nb = 0;
    const unsigned char *s1, *s2, *p, *b;
    int64_t srp_id;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_ACCOUNT_GET_PASSWORD);
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc != TG_OK) {
        tg_arena_free(&a);
        free(res);
        return rc;
    }
    algo = tgv_get(top, "current_algo");
    s1 = tgv_str(algo, "salt1", &n1);
    s2 = tgv_str(algo, "salt2", &n2);
    p = tgv_str(algo, "p", &np);
    b = tgv_str(top, "srp_B", &nb);
    srp_id = tgv_i64(top, "srp_id", 0);
    if (!tgv_is(algo, "passwordKdfAlgoSHA256SHA256PBKDF2HMACSHA512iter100000SHA256ModPow") ||
        np != 256 || memcmp(p, tg_dh_prime, 256) || !nb) {
        set_err(c, "unsupported password settings");
        rc = TG_ERR_PROTO;
    } else {
        tg_srp_x((const unsigned char *)password, (int)strlen(password), s1, n1, s2, n2, x);
        tg_random(ar, sizeof ar);
        if (tg_srp(x, s1, n1, s2, n2, tgv_i32(algo, "g", 0), p, b, nb, ar, A, m1)) {
            set_err(c, "password check failed");
            rc = TG_ERR_PROTO;
        }
    }
    if (rc == TG_OK) {
        unsigned char *res2 = NULL;
        int rlen;
        tg_buf_init(&q);
        tg_buf_u32(&q, TL_AUTH_CHECK_PASSWORD);
        tg_buf_u32(&q, TL_INPUT_CHECK_PASSWORD_SRP);
        tg_buf_i64(&q, srp_id);
        tg_buf_bytes(&q, A, 256);
        tg_buf_bytes(&q, m1, 32);
        rc = api_call(c, &q, &res2, &rlen);
        tg_buf_free(&q);
        free(res2);
        if (rc == TG_ERR_RPC && strstr(c->sess.rpc_msg, "PASSWORD_HASH_INVALID"))
            set_err(c, "wrong password");
        if (rc == TG_OK)
            save_active_dc(c);
    }
    memset(ar, 0, sizeof ar);
    memset(x, 0, sizeof x);
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_client_send(tg_client *c, const tg_dialog *d, const char *utf8, int32_t reply_to)
{
    tg_buf q;
    unsigned char *res = NULL;
    int64_t random_id;
    int rlen, rc;
    tg_random(&random_id, sizeof random_id);
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_SEND_MESSAGE);
    tg_buf_i32(&q, reply_to ? 1 : 0);
    put_input_peer(&q, d);
    if (reply_to) {
        tg_buf_u32(&q, TL_INPUT_REPLY_TO_MESSAGE);
        tg_buf_i32(&q, 0);
        tg_buf_i32(&q, reply_to);
    }
    tg_buf_str(&q, utf8);
    tg_buf_i64(&q, random_id);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    free(res);
    return rc;
}

const char *tg_mime_for(const char *name)
{
    static const char *const map[][2] = {
        {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
        {"gif", "image/gif"},  {"bmp", "image/bmp"},   {"mp4", "video/mp4"},
        {"3gp", "video/3gpp"}, {"avi", "video/x-msvideo"}, {"mp3", "audio/mpeg"},
        {"aac", "audio/aac"},  {"m4a", "audio/mp4"},   {"amr", "audio/amr"},
        {"wav", "audio/wav"},  {"ogg", "audio/ogg"},   {"txt", "text/plain"},
        {"pdf", "application/pdf"}, {"zip", "application/zip"},
        {"sis", "application/vnd.symbian.install"}, {"sisx", "application/vnd.symbian.install"},
    };
    const char *dot = strrchr(name, '.');
    int i, k;
    if (dot)
        for (i = 0; i < (int)(sizeof map / sizeof map[0]); i++) {
            const char *e = map[i][0];
            for (k = 0; e[k] && dot[k + 1] && (dot[k + 1] | 32) == e[k]; k++)
                ;
            if (!e[k] && !dot[k + 1])
                return map[i][1];
        }
    return "application/octet-stream";
}

static int upload_file(tg_client *c, tg_read_fn rd, void *ctx, int64_t size, const char *name,
                       volatile int *progress, tg_buf *input_file)
{
    enum { PART = 131072 };
    int64_t file_id;
    int parts, part, rc = TG_OK, big = size > 10 * 1024 * 1024, rlen;
    unsigned char *buf, *res = NULL;
    tg_buf q;
    if (size <= 0 || size > (int64_t)2000 * 1024 * 1024) {
        set_err(c, "file is empty or too big");
        return TG_ERR_PROTO;
    }
    parts = (int)((size + PART - 1) / PART);
    buf = (unsigned char *)malloc(PART);
    if (!buf) {
        set_err(c, "out of memory");
        return TG_ERR_NOMEM;
    }
    tg_random(&file_id, sizeof file_id);
    for (part = 0; part < parts && rc == TG_OK; part++) {
        int64_t off = (int64_t)part * PART;
        int want = size - off < PART ? (int)(size - off) : PART, n, attempt;
        n = rd(ctx, buf, off, want);
        if (n != want) {
            set_err(c, "cannot read the file");
            rc = TG_ERR_PROTO;
            break;
        }
        for (attempt = 0; attempt < 3; attempt++) {
            tg_buf_init(&q);
            tg_buf_u32(&q, big ? TL_UPLOAD_SAVE_BIG_FILE_PART : TL_UPLOAD_SAVE_FILE_PART);
            tg_buf_i64(&q, file_id);
            tg_buf_i32(&q, part);
            if (big)
                tg_buf_i32(&q, parts);
            tg_buf_bytes(&q, buf, n);
            rc = api_call(c, &q, &res, &rlen);
            tg_buf_free(&q);
            if (rc == TG_OK && (rlen < 4 || tg_le32(res) != TL_BOOL_TRUE)) {
                set_err(c, "upload rejected");
                rc = TG_ERR_PROTO;
            }
            free(res);
            res = NULL;
            if (rc != TG_ERR_NET)
                break;
        }
        if (progress)
            *progress = (part + 1) * 100 / parts;
    }
    free(buf);
    if (rc)
        return rc;
    tg_buf_u32(input_file, big ? TL_INPUT_FILE_BIG : TL_INPUT_FILE);
    tg_buf_i64(input_file, file_id);
    tg_buf_i32(input_file, parts);
    tg_buf_str(input_file, name);
    if (!big)
        tg_buf_str(input_file, "");
    return TG_OK;
}

int tg_client_send_file(tg_client *c, const tg_dialog *d, tg_read_fn rd, void *ctx, int64_t size,
                        const char *name, int as_photo, const char *caption,
                        volatile int *progress)
{
    tg_buf q, file;
    unsigned char *res = NULL;
    int64_t random_id;
    int rc, rlen;
    tg_buf_init(&file);
    rc = upload_file(c, rd, ctx, size, name, progress, &file);
    if (rc) {
        tg_buf_free(&file);
        return rc;
    }
    tg_random(&random_id, sizeof random_id);
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_SEND_MEDIA);
    tg_buf_i32(&q, 0);
    put_input_peer(&q, d);
    tg_buf_u32(&q, as_photo ? TL_INPUT_MEDIA_UPLOADED_PHOTO : TL_INPUT_MEDIA_UPLOADED_DOCUMENT);
    tg_buf_i32(&q, 0);
    tg_buf_put(&q, file.p, file.len);
    if (!as_photo) {
        tg_buf_str(&q, tg_mime_for(name));
        tg_buf_u32(&q, TL_VECTOR);
        tg_buf_i32(&q, 1);
        tg_buf_u32(&q, TL_DOCUMENT_ATTRIBUTE_FILENAME);
        tg_buf_str(&q, name);
    }
    tg_buf_str(&q, caption ? caption : "");
    tg_buf_i64(&q, random_id);
    tg_buf_free(&file);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    free(res);
    return rc;
}

int tg_client_upload_photo(tg_client *c, const tg_dialog *d, tg_read_fn rd, void *ctx,
                           int64_t size, const char *name, tg_file_loc *out,
                           volatile int *progress)
{
    tg_buf q, file;
    tg_arena a;
    unsigned char *res = NULL;
    tgv *top, *ph;
    int rc;
    memset(out, 0, sizeof *out);
    tg_buf_init(&file);
    rc = upload_file(c, rd, ctx, size, name, progress, &file);
    if (rc) {
        tg_buf_free(&file);
        return rc;
    }
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_UPLOAD_MEDIA);
    tg_buf_i32(&q, 0);
    put_input_peer(&q, d);
    tg_buf_u32(&q, TL_INPUT_MEDIA_UPLOADED_PHOTO);
    tg_buf_i32(&q, 0);
    tg_buf_put(&q, file.p, file.len);
    tg_buf_free(&file);
    rc = call_parse(c, &q, &a, &res, &top);
    if (rc == TG_OK) {
        ph = tgv_get(top, "photo");
        if (tgv_is(top, "messageMediaPhoto") && tgv_is(ph, "photo")) {
            fill_loc_bytes(out, ph, TG_LOC_PHOTO);
        } else {
            set_err(c, "unexpected upload reply");
            rc = TG_ERR_PROTO;
        }
    }
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_client_send_album(tg_client *c, const tg_dialog *d, const tg_file_loc *photos, int n,
                         const char *caption)
{
    tg_buf q;
    int i;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_SEND_MULTI_MEDIA);
    tg_buf_i32(&q, 0);
    put_input_peer(&q, d);
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, n);
    for (i = 0; i < n; i++) {
        int64_t random_id;
        tg_random(&random_id, sizeof random_id);
        tg_buf_u32(&q, TL_INPUT_SINGLE_MEDIA);
        tg_buf_i32(&q, 0);
        tg_buf_u32(&q, TL_INPUT_MEDIA_PHOTO);
        tg_buf_i32(&q, 0);
        tg_buf_u32(&q, TL_INPUT_PHOTO);
        tg_buf_i64(&q, photos[i].id);
        tg_buf_i64(&q, photos[i].access_hash);
        tg_buf_bytes(&q, photos[i].ref, photos[i].ref_len);
        tg_buf_i64(&q, random_id);
        tg_buf_str(&q, i == 0 && caption ? caption : "");
    }
    return simple_call(c, &q);
}

static int migrate_dc(const char *msg)
{
    static const char *prefixes[] = {"PHONE_MIGRATE_", "NETWORK_MIGRATE_", "USER_MIGRATE_",
                                     "FILE_MIGRATE_"};
    int i;
    for (i = 0; i < 4; i++) {
        size_t n = strlen(prefixes[i]);
        if (!strncmp(msg, prefixes[i], n))
            return atoi(msg + n);
    }
    return 0;
}

static int sent_code(tg_client *c, unsigned char *res, int rlen, char *code_type,
                     int code_type_len)
{
    tg_arena a;
    tgv *top;
    int rc;
    tg_arena_init(&a);
    rc = parse_result(c, &a, res, rlen, &top);
    if (!rc) {
        if (tgv_is(top, "auth.sentCodeSuccess")) {
            snprintf(code_type, code_type_len, "authorized");
        } else if (tgv_is(top, "auth.sentCode")) {
            int n, len = 0;
            const unsigned char *h = tgv_str(top, "phone_code_hash", &n);
            tgv *type = tgv_get(top, "type"), *next = tgv_get(top, "next_type");
            if (n >= (int)sizeof c->phone_code_hash)
                n = sizeof c->phone_code_hash - 1;
            memcpy(c->phone_code_hash, h, n);
            c->phone_code_hash[n] = 0;
            len = snprintf(code_type, code_type_len, "%s", tgv_name(type));
            if (tgv_get(type, "length") && len < code_type_len)
                len += snprintf(code_type + len, code_type_len - len, "; %d digits",
                                tgv_i32(type, "length", 0));
            if (next && len < code_type_len)
                len += snprintf(code_type + len, code_type_len - len, "; next %s",
                                tgv_name(next));
            if (tgv_get(top, "timeout") && len < code_type_len)
                snprintf(code_type + len, code_type_len - len, "; wait %ds",
                         tgv_i32(top, "timeout", 0));
        } else {
            snprintf(c->err, sizeof c->err, "unexpected reply %s", tgv_name(top));
            rc = TG_ERR_PROTO;
        }
    }
    tg_arena_free(&a);
    save_active_dc(c);
    return rc;
}

int tg_client_send_code(tg_client *c, const char *phone, char *code_type, int code_type_len)
{
    int attempt, rc = TG_ERR_PROTO, rlen;
    unsigned char *res = NULL;
    strncpy(c->phone, phone, sizeof c->phone - 1);
    c->phone[sizeof c->phone - 1] = 0;
    c->phone_code_hash[0] = 0;
    for (attempt = 0; attempt < 3; attempt++) {
        tg_buf q;
        int dc;
        tg_buf_init(&q);
        tg_buf_u32(&q, TL_AUTH_SEND_CODE);
        tg_buf_str(&q, c->phone);
        tg_buf_i32(&q, c->api_id);
        tg_buf_str(&q, c->api_hash);
        tg_buf_u32(&q, TL_CODE_SETTINGS);
        tg_buf_i32(&q, 0);
        rc = api_call(c, &q, &res, &rlen);
        tg_buf_free(&q);
        if (rc != TG_ERR_RPC || !(dc = migrate_dc(c->sess.rpc_msg)))
            break;
        disconnect(c);
        c->dc_id = dc;
        save_active_dc(c);
    }
    if (rc == TG_OK)
        rc = sent_code(c, res, rlen, code_type, code_type_len);
    free(res);
    return rc;
}

int tg_client_resend_code(tg_client *c, char *code_type, int code_type_len)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_AUTH_RESEND_CODE);
    tg_buf_i32(&q, 0);
    tg_buf_str(&q, c->phone);
    tg_buf_str(&q, c->phone_code_hash);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK)
        rc = sent_code(c, res, rlen, code_type, code_type_len);
    free(res);
    return rc;
}

static void qr_url(const tg_client *c, char *url, int urllen)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    int i, n = snprintf(url, urllen, "tg://login?token=");
    for (i = 0; i < c->qr_token_len && n + 5 < urllen; i += 3) {
        int left = c->qr_token_len - i;
        unsigned v = c->qr_token[i] << 16 | (left > 1 ? c->qr_token[i + 1] << 8 : 0) |
                     (left > 2 ? c->qr_token[i + 2] : 0);
        url[n++] = b64[v >> 18 & 63];
        url[n++] = b64[v >> 12 & 63];
        if (left > 1)
            url[n++] = b64[v >> 6 & 63];
        if (left > 2)
            url[n++] = b64[v & 63];
    }
    url[n] = 0;
}

static int qr_reply(tg_client *c, unsigned char *res, int rlen, char *url, int urllen)
{
    tg_arena a;
    tgv *top;
    int rc, n;
    const unsigned char *tok;
    tg_arena_init(&a);
    rc = parse_result(c, &a, res, rlen, &top);
    if (rc) {
        tg_arena_free(&a);
        return rc;
    }
    if (tgv_is(top, "auth.loginTokenSuccess")) {
        save_active_dc(c);
        rc = TG_QR_DONE;
    } else if (tgv_is(top, "auth.loginToken") || tgv_is(top, "auth.loginTokenMigrateTo")) {
        tok = tgv_str(top, "token", &n);
        if (n > (int)sizeof c->qr_token)
            n = sizeof c->qr_token;
        memcpy(c->qr_token, tok, n);
        c->qr_token_len = n;
        if (tgv_is(top, "auth.loginToken")) {
            c->qr_expires = tgv_i32(top, "expires", 0);
            qr_url(c, url, urllen);
        } else {
            c->qr_expires = 0;
            disconnect(c);
            c->dc_id = tgv_i32(top, "dc_id", c->dc_id);
            rc = TG_QR_MIGRATE;
        }
    } else {
        snprintf(c->err, sizeof c->err, "unexpected reply %s", tgv_name(top));
        rc = TG_ERR_PROTO;
    }
    tg_arena_free(&a);
    return rc;
}

int tg_client_qr_login(tg_client *c, char *url, int urllen)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    tg_buf_init(&q);
    if (c->qr_token_len && c->connected &&
        (int32_t)time(NULL) + c->sess.time_offset < c->qr_expires - 3) {
        tg_buf_u32(&q, TL_UPDATES_GET_STATE);
        rc = api_call(c, &q, &res, &rlen);
        tg_buf_free(&q);
        free(res);
        if (rc == TG_OK) {
            save_active_dc(c);
            return TG_QR_DONE;
        }
        if (rc != TG_NEED_LOGIN)
            return rc;
        qr_url(c, url, urllen);
        return TG_OK;
    }
    tg_buf_u32(&q, TL_AUTH_EXPORT_LOGIN_TOKEN);
    tg_buf_i32(&q, c->api_id);
    tg_buf_str(&q, c->api_hash);
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, 0);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK)
        rc = qr_reply(c, res, rlen, url, urllen);
    free(res);
    if (rc == TG_QR_MIGRATE) {
        res = NULL;
        tg_buf_init(&q);
        tg_buf_u32(&q, TL_AUTH_IMPORT_LOGIN_TOKEN);
        tg_buf_bytes(&q, c->qr_token, c->qr_token_len);
        rc = api_call(c, &q, &res, &rlen);
        tg_buf_free(&q);
        if (rc == TG_OK)
            rc = qr_reply(c, res, rlen, url, urllen);
        free(res);
        if (rc == TG_QR_MIGRATE) {
            set_err(c, "login token migrated twice");
            rc = TG_ERR_PROTO;
        }
    }
    if (rc == TG_ERR_RPC && strstr(c->sess.rpc_msg, "SESSION_PASSWORD_NEEDED")) {
        set_err(c, "two-step verification password required");
        rc = TG_NEED_PASSWORD;
    }
    if (rc == TG_QR_DONE)
        c->qr_token_len = 0;
    return rc;
}

int tg_client_poll(tg_client *c, int *changed)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    *changed = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_UPDATES_GET_STATE);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK) {
        tg_arena a;
        tgv *top;
        tg_arena_init(&a);
        rc = parse_result(c, &a, res, rlen, &top);
        if (!rc) {
            int32_t st[4];
            st[0] = tgv_i32(top, "pts", 0);
            st[1] = tgv_i32(top, "qts", 0);
            st[2] = tgv_i32(top, "seq", 0);
            st[3] = tgv_i32(top, "unread_count", 0);
            *changed = c->have_state && memcmp(st, c->state, sizeof st) != 0;
            memcpy(c->state, st, sizeof st);
            c->have_state = 1;
        }
        tg_arena_free(&a);
    }
    free(res);
    return rc;
}

static void link_close(tg_client *c, int dc)
{
    if (c->links[dc].conn) {
        tg_send_acks(&c->links[dc].sess);
        tg_conn_close(c->links[dc].conn);
    }
    c->links[dc].conn = NULL;
    c->links[dc].ready = 0;
}

static void media_key_path(const tg_client *c, int dc, char *out, int n)
{
    char name[48];
    snprintf(name, sizeof name, "media_%s_dc%d.key", c->test ? "test" : "prod", dc);
    path(c, out, n, name);
}

static int link_call(tg_client *c, int dc, const tg_buf *query, unsigned char **res, int *rlen);

static int link_open(tg_client *c, int dc)
{
    unsigned char key[256], salt[8], *res = NULL;
    char p[256];
    int off = 0, rc, neterr = 0, rlen, n;
    const char *host = tg_dc_host(dc, c->test);
    FILE *f;
    tg_buf q;
    tg_rd r;
    int64_t id;
    const unsigned char *bytes;
    if (!host) {
        set_err(c, "unknown datacenter");
        return TG_ERR_PROTO;
    }
    c->links[dc].conn = tg_conn_open(host, 443, &neterr);
    if (!c->links[dc].conn) {
        snprintf(c->err, sizeof c->err, "cannot connect to DC%d (%d)", dc, neterr);
        return TG_ERR_NET;
    }
    memset(salt, 0, sizeof salt);
    media_key_path(c, dc, p, sizeof p);
    f = fopen(p, "rb");
    n = f ? (int)fread(key, 1, 256, f) : 0;
    if (f)
        fclose(f);
    if (n == 256) {
        tg_session_init(&c->links[dc].sess, c->links[dc].conn, key, salt, 0);
        c->links[dc].ready = 1;
        memset(key, 0, sizeof key);
        return TG_OK;
    }
    rc = tg_handshake(c->links[dc].conn, dc, c->test, key, salt, &off, c->err, sizeof c->err);
    if (rc) {
        link_close(c, dc);
        return rc;
    }
    tg_session_init(&c->links[dc].sess, c->links[dc].conn, key, salt, off);
    c->links[dc].ready = 1;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_AUTH_EXPORT_AUTHORIZATION);
    tg_buf_i32(&q, dc);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK) {
        tg_rd_init(&r, res, rlen);
        if (tg_rd_u32(&r) != TL_AUTH_EXPORTED_AUTHORIZATION)
            rc = TG_ERR_PROTO;
        id = tg_rd_i64(&r);
        bytes = tg_rd_bytes(&r, &n);
        if (r.err)
            rc = TG_ERR_PROTO;
        if (rc == TG_OK) {
            unsigned char *res2 = NULL;
            tg_buf_init(&q);
            tg_buf_u32(&q, TL_AUTH_IMPORT_AUTHORIZATION);
            tg_buf_i64(&q, id);
            tg_buf_bytes(&q, bytes, n);
            rc = link_call(c, dc, &q, &res2, &rlen);
            tg_buf_free(&q);
            free(res2);
        } else {
            set_err(c, "bad exportAuthorization reply");
        }
    }
    free(res);
    if (rc == TG_OK) {
        f = fopen(p, "wb");
        if (f) {
            fwrite(key, 1, 256, f);
            fclose(f);
        }
    } else {
        link_close(c, dc);
    }
    memset(key, 0, sizeof key);
    return rc;
}

static int link_call(tg_client *c, int dc, const tg_buf *query, unsigned char **res, int *rlen)
{
    int rc;
    if (dc <= 0 || dc == c->dc_id)
        return api_call(c, query, res, rlen);
    if (dc > 5) {
        set_err(c, "unknown datacenter");
        return TG_ERR_PROTO;
    }
    if (!c->links[dc].ready && (rc = link_open(c, dc)) != TG_OK)
        return rc;
    rc = session_call(c, &c->links[dc].sess, query, res, rlen);
    if (rc == TG_NEED_LOGIN) {
        char p[256];
        media_key_path(c, dc, p, sizeof p);
        remove(p);
        link_close(c, dc);
        rc = TG_ERR_RPC;
    } else if (rc == TG_ERR_NET) {
        link_close(c, dc);
    }
    return rc;
}

static void put_location(tg_buf *q, const tg_file_loc *loc)
{
    if (loc->type == TG_LOC_PEER_PHOTO) {
        tg_dialog peer;
        memset(&peer, 0, sizeof peer);
        peer.kind = loc->peer_kind;
        peer.id = loc->peer_id;
        peer.access_hash = loc->peer_hash;
        tg_buf_u32(q, TL_INPUT_PEER_PHOTO_FILE_LOCATION);
        tg_buf_i32(q, 0);
        put_input_peer(q, &peer);
        tg_buf_i64(q, loc->id);
    } else {
        tg_buf_u32(q, loc->type == TG_LOC_PHOTO ? TL_INPUT_PHOTO_FILE_LOCATION
                                               : TL_INPUT_DOCUMENT_FILE_LOCATION);
        tg_buf_i64(q, loc->id);
        tg_buf_i64(q, loc->access_hash);
        tg_buf_bytes(q, loc->ref, loc->ref_len);
        tg_buf_str(q, loc->thumb);
    }
}

int tg_client_read_out(tg_client *c, const tg_dialog *d, int32_t *read_out_max)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_GET_PEER_DIALOGS);
    tg_buf_u32(&q, TL_VECTOR);
    tg_buf_i32(&q, 1);
    tg_buf_u32(&q, TL_INPUT_DIALOG_PEER);
    put_input_peer(&q, d);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_OK) {
        tg_arena a;
        tgv *top;
        tg_arena_init(&a);
        rc = parse_result(c, &a, res, rlen, &top);
        if (!rc) {
            tgv *dl = tgv_vec_at(tgv_get(top, "dialogs"), 0);
            if (tgv_is(dl, "dialog"))
                *read_out_max = tgv_i32(dl, "read_outbox_max_id", 0);
            else
                rc = TG_ERR_PROTO;
        }
        tg_arena_free(&a);
    }
    free(res);
    return rc;
}

int tg_client_read_history(tg_client *c, const tg_dialog *d, int32_t max_id)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    tg_buf_init(&q);
    if (d->kind == 2) {
        tg_buf_u32(&q, TL_CHANNELS_READ_HISTORY);
        tg_buf_u32(&q, TL_INPUT_CHANNEL);
        tg_buf_i64(&q, d->id);
        tg_buf_i64(&q, d->access_hash);
    } else {
        tg_buf_u32(&q, TL_MESSAGES_READ_HISTORY);
        put_input_peer(&q, d);
    }
    tg_buf_i32(&q, max_id);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    free(res);
    return rc;
}

int tg_client_download(tg_client *c, const tg_file_loc *loc, const char *dest)
{
    enum { LIMIT = 65536 };
    char tmp[300];
    FILE *f;
    int64_t offset = 0;
    int rc = TG_OK, dc = loc->dc, hops = 0;
    if (loc->type == TG_LOC_NONE) {
        set_err(c, "nothing to download");
        return TG_ERR_PROTO;
    }
    snprintf(tmp, sizeof tmp, "%s.part", dest);
    f = fopen(tmp, "wb");
    if (!f) {
        set_err(c, "cannot write the cache file");
        return TG_ERR_PROTO;
    }
    for (;;) {
        tg_buf q;
        unsigned char *res = NULL;
        const unsigned char *data;
        int rlen, n, mdc;
        tg_rd r;
        tg_buf_init(&q);
        tg_buf_u32(&q, TL_UPLOAD_GET_FILE);
        tg_buf_i32(&q, 0);
        put_location(&q, loc);
        tg_buf_i64(&q, offset);
        tg_buf_i32(&q, LIMIT);
        rc = link_call(c, dc, &q, &res, &rlen);
        tg_buf_free(&q);
        if (rc == TG_ERR_RPC &&
            (mdc = migrate_dc(dc <= 0 || dc == c->dc_id || dc > 5 ? c->sess.rpc_msg
                                                                   : c->links[dc].sess.rpc_msg)) &&
            hops++ < 2) {
            dc = mdc;
            free(res);
            continue;
        }
        if (rc) {
            free(res);
            break;
        }
        tg_rd_init(&r, res, rlen);
        if (tg_rd_u32(&r) != TL_UPLOAD_FILE) {
            set_err(c, "unexpected file reply");
            rc = TG_ERR_PROTO;
            free(res);
            break;
        }
        tg_rd_u32(&r);
        tg_rd_i32(&r);
        data = tg_rd_bytes(&r, &n);
        if (r.err || (n && fwrite(data, 1, n, f) != (size_t)n)) {
            set_err(c, "bad file data");
            rc = TG_ERR_PROTO;
            free(res);
            break;
        }
        free(res);
        offset += n;
        if (n < LIMIT)
            break;
        if (offset >= 64 * 1024 * 1024) {
            set_err(c, "file is too big");
            rc = TG_ERR_PROTO;
            break;
        }
    }
    fclose(f);
    if (rc == TG_OK) {
        remove(dest);
        if (rename(tmp, dest) != 0)
            rc = TG_ERR_PROTO;
    }
    if (rc != TG_OK)
        remove(tmp);
    return rc;
}

int tg_client_sign_in(tg_client *c, const char *code)
{
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_AUTH_SIGN_IN);
    tg_buf_i32(&q, 1);
    tg_buf_str(&q, c->phone);
    tg_buf_str(&q, c->phone_code_hash);
    tg_buf_str(&q, code);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc == TG_ERR_RPC && strstr(c->sess.rpc_msg, "SESSION_PASSWORD_NEEDED")) {
        set_err(c, "two-step verification password required");
        rc = TG_NEED_PASSWORD;
    } else if (rc == TG_OK && res && rlen >= 4 && tg_le32(res) != 0x2ea2c0d4u) {
        set_err(c, "this number has no Telegram account");
        rc = TG_ERR_PROTO;
    }
    free(res);
    return rc;
}

int tg_client_logout(tg_client *c)
{
    tg_buf q;
    unsigned char *res = NULL;
    char p[256];
    int rlen;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_AUTH_LOG_OUT);
    api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    free(res);
    disconnect(c);
    key_path(c, c->dc_id, p, sizeof p);
    remove(p);
    return TG_OK;
}

static int input_peer(const tgv *p, tg_folder_peer *out)
{
    if (tgv_is(p, "inputPeerUser") || tgv_is(p, "inputPeerUserFromMessage")) {
        out->kind = 0;
        out->id = tgv_i64(p, "user_id", 0);
    } else if (tgv_is(p, "inputPeerChat")) {
        out->kind = 1;
        out->id = tgv_i64(p, "chat_id", 0);
    } else if (tgv_is(p, "inputPeerChannel") || tgv_is(p, "inputPeerChannelFromMessage")) {
        out->kind = 2;
        out->id = tgv_i64(p, "channel_id", 0);
    } else if (tgv_is(p, "inputPeerSelf")) {
        out->kind = 0;
        out->id = -1;
    } else {
        return 0;
    }
    return 1;
}

int tg_client_folders(tg_client *c, tg_folder **out, int *count)
{
    static const char *const lists[] = {"pinned_peers", "include_peers", "exclude_peers"};
    static const char *const bits[] = {"contacts", "non_contacts", "groups", "broadcasts",
                                       "bots", "exclude_muted", "exclude_read",
                                       "exclude_archived"};
    tg_buf q;
    unsigned char *res = NULL;
    int rlen, rc, i, j, k, n;
    tg_arena a;
    tgv *top, *filters;
    tg_folder *f;
    *out = NULL;
    *count = 0;
    tg_buf_init(&q);
    tg_buf_u32(&q, TL_MESSAGES_GET_DIALOG_FILTERS);
    rc = api_call(c, &q, &res, &rlen);
    tg_buf_free(&q);
    if (rc)
        return rc;
    tg_arena_init(&a);
    rc = parse_result(c, &a, res, rlen, &top);
    if (rc)
        goto done;
    filters = tgv_get(top, "filters");
    n = tgv_vec_len(filters);
    f = (tg_folder *)calloc(n + 1, sizeof *f);
    if (!f) {
        rc = TG_ERR_NOMEM;
        goto done;
    }
    for (i = 0; i < n; i++) {
        tgv *flt = tgv_vec_at(filters, i);
        tg_folder *cur = &f[*count];
        int len, total = 0;
        const unsigned char *t;
        if (tgv_is(flt, "dialogFilterDefault")) {
            cur->all = 1;
            cur->title = dup_s("All chats");
            (*count)++;
            continue;
        }
        if (!tgv_is(flt, "dialogFilter") && !tgv_is(flt, "dialogFilterChatlist"))
            continue;
        cur->id = tgv_i32(flt, "id", 0);
        cur->chatlist = tgv_is(flt, "dialogFilterChatlist");
        for (k = 0; k < 8; k++)
            if (tgv_flag(flt, bits[k]))
                cur->flags |= 1 << k;
        t = tgv_str(tgv_get(flt, "title"), "text", &len);
        cur->title = dup_n(t ? t : (const unsigned char *)"", len);
        tg_text_fix(cur->title);
        for (k = 0; k < 3; k++)
            total += tgv_vec_len(tgv_get(flt, lists[k]));
        cur->peers = (tg_folder_peer *)calloc(total ? total : 1, sizeof *cur->peers);
        for (k = 0; k < 3 && cur->peers; k++) {
            tgv *v = tgv_get(flt, lists[k]);
            for (j = 0; j < tgv_vec_len(v); j++)
                if (input_peer(tgv_vec_at(v, j), &cur->peers[cur->npeers])) {
                    cur->peers[cur->npeers].include = k < 2;
                    cur->npeers++;
                }
        }
        (*count)++;
    }
    for (i = 0; i < *count && !f[i].all; i++)
        ;
    if (i == *count) {
        memmove(f + 1, f, *count * sizeof *f);
        memset(f, 0, sizeof *f);
        f->all = 1;
        f->title = dup_s("All chats");
        (*count)++;
    }
    *out = f;
done:
    tg_arena_free(&a);
    free(res);
    return rc;
}

int tg_folder_has(const tg_folder *f, const tg_dialog *d)
{
    int i, type;
    if (f->all)
        return 1;
    for (i = 0; i < f->npeers; i++) {
        const tg_folder_peer *p = &f->peers[i];
        if (p->kind == d->kind &&
            (p->id == d->id || (p->id == -1 && (d->flags & TG_DLG_SELF))))
            return p->include;
    }
    if (f->chatlist)
        return 0;
    if ((f->flags & 128) && (d->flags & TG_DLG_ARCHIVED))
        return 0;
    if ((f->flags & 32) && (d->flags & TG_DLG_MUTED))
        return 0;
    if ((f->flags & 64) && !d->unread && !(d->flags & TG_DLG_UNREAD_MARK))
        return 0;
    if (d->flags & TG_DLG_BOT)
        type = 16;
    else if (d->flags & TG_DLG_GROUP)
        type = 4;
    else if (d->flags & TG_DLG_CHANNEL)
        type = 8;
    else if (d->flags & (TG_DLG_CONTACT | TG_DLG_SELF))
        type = 1;
    else
        type = 2;
    return (f->flags & type) != 0;
}

void tg_folders_free(tg_folder *f, int n)
{
    int i;
    for (i = 0; f && i < n; i++) {
        free(f[i].title);
        free(f[i].peers);
    }
    free(f);
}

void tg_dialogs_free(tg_dialog *d, int n)
{
    int i;
    for (i = 0; d && i < n; i++) {
        free(d[i].name);
        free(d[i].text);
    }
    free(d);
}

void tg_messages_free(tg_message *m, int n)
{
    int i;
    for (i = 0; m && i < n; i++) {
        free(m[i].from);
        free(m[i].text);
        free(m[i].reply);
        free(m[i].fwd);
        free(m[i].reactions);
        free(m[i].reactions_short);
    }
    free(m);
}
