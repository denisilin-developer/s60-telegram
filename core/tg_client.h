#ifndef TG_CLIENT_H
#define TG_CLIENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TG_NEED_LOGIN (-10)
#define TG_NEED_PASSWORD (-11)
#define TG_QR_DONE 1

typedef struct tg_client tg_client;

#define TG_LOC_NONE 0
#define TG_LOC_PEER_PHOTO 1
#define TG_LOC_PHOTO 2
#define TG_LOC_DOCUMENT 3

#define TG_SAVE_NONE 0
#define TG_SAVE_IMAGE 1
#define TG_SAVE_VIDEO 2
#define TG_SAVE_AUDIO 3
#define TG_SAVE_OTHER 4

typedef struct {
    int type;
    int dc;
    int64_t id;
    int64_t access_hash;
    int peer_kind;
    int64_t peer_id;
    int64_t peer_hash;
    unsigned char ref[128];
    int ref_len;
    char thumb[8];
} tg_file_loc;

#define TG_DLG_CONTACT 1
#define TG_DLG_BOT 2
#define TG_DLG_GROUP 4
#define TG_DLG_CHANNEL 8
#define TG_DLG_MUTED 16
#define TG_DLG_UNREAD_MARK 32
#define TG_DLG_SELF 64
#define TG_DLG_ARCHIVED 128
#define TG_DLG_PINNED 256
#define TG_DLG_READONLY 512

typedef struct {
    int kind;
    int flags;
    int64_t id;
    int64_t access_hash;
    char *name;
    char *text;
    int unread;
    int out;
    int32_t date;
    int32_t read_max;
    int32_t read_out_max;
    int32_t top_id;
    tg_file_loc photo;
} tg_dialog;

typedef struct {
    int32_t id;
    int out;
    int32_t date;
    char *from;
    char *text;
    char *reply;
    char *fwd;
    char *reactions;
    char *reactions_short;
    unsigned top_reaction;
    int reaction_total;
    unsigned react_cp[8];
    int react_count[8];
    char react_text[8][8];
    int react_kinds;
    int32_t reply_to;
    int64_t grouped_id;
    int edited;
    tg_file_loc media;
    tg_file_loc full;
    tg_file_loc thumb;
    int has_text;
    int save_kind;
    char file_name[64];
} tg_message;

typedef struct {
    int kind;
    int64_t id;
    int include;
} tg_folder_peer;

typedef struct {
    int id;
    int flags;
    int chatlist;
    int all;
    char *title;
    tg_folder_peer *peers;
    int npeers;
} tg_folder;

tg_client *tg_client_new(const char *dir, int api_id, const char *api_hash, int test);
void tg_client_free(tg_client *c);
const char *tg_client_error(const tg_client *c);

int tg_client_connect(tg_client *c);
int tg_client_dialogs(tg_client *c, int folder, int limit, tg_dialog **out, int *n);
int tg_client_folders(tg_client *c, tg_folder **out, int *n);
int tg_folder_has(const tg_folder *f, const tg_dialog *d);
void tg_folders_free(tg_folder *f, int n);
#define TG_STATUS_NONE 0
#define TG_STATUS_ONLINE 1
#define TG_STATUS_SEEN 2
#define TG_STATUS_RECENTLY 3
#define TG_STATUS_WEEK 4
#define TG_STATUS_MONTH 5

int tg_client_history(tg_client *c, const tg_dialog *d, int32_t offset_id, int limit,
                      tg_message **out, int *n);
int tg_client_status(const tg_client *c, int32_t *when);
int tg_client_send(tg_client *c, const tg_dialog *d, const char *utf8, int32_t reply_to);
int tg_client_edit(tg_client *c, const tg_dialog *d, int32_t id, const char *utf8);
int tg_client_delete(tg_client *c, const tg_dialog *d, const int32_t *ids, int n, int revoke);
int tg_client_forward(tg_client *c, const tg_dialog *from, int32_t id, const tg_dialog *to);
int tg_client_react(tg_client *c, const tg_dialog *d, int32_t id, const char *emoticon);
int tg_client_pinned(tg_client *c, const tg_dialog *d, char **text);

#define TG_ACT_PIN 1
#define TG_ACT_UNPIN 2
#define TG_ACT_MUTE 3
#define TG_ACT_UNMUTE 4
#define TG_ACT_READ 5
#define TG_ACT_UNREAD 6
#define TG_ACT_ARCHIVE 7
#define TG_ACT_UNARCHIVE 8
int tg_client_chat_action(tg_client *c, const tg_dialog *d, int action);

typedef struct {
    char *name;
    char *username;
    char *phone;
    char *about;
    int members;
    int channel;
} tg_info;

int tg_client_info(tg_client *c, const tg_dialog *d, tg_info *out);
void tg_info_free(tg_info *info);
int tg_client_contacts(tg_client *c, tg_dialog **out, int *n);
int tg_client_resolve(tg_client *c, const char *username, tg_dialog *out);
int tg_client_password_hint(tg_client *c, char *hint, int hint_len);
int tg_client_check_password(tg_client *c, const char *password);
int tg_client_send_code(tg_client *c, const char *phone, char *code_type, int code_type_len);
int tg_client_resend_code(tg_client *c, char *code_type, int code_type_len);
int tg_client_qr_login(tg_client *c, char *url, int urllen);
int tg_client_poll(tg_client *c, int *changed);
void tg_text_fix(char *s);
void tg_set_lang(int lang);
typedef int (*tg_read_fn)(void *ctx, unsigned char *buf, int64_t offset, int len);

const char *tg_mime_for(const char *name);
int tg_client_send_file(tg_client *c, const tg_dialog *d, tg_read_fn rd, void *ctx, int64_t size,
                        const char *name, int as_photo, const char *caption,
                        volatile int *progress);
int tg_client_upload_photo(tg_client *c, const tg_dialog *d, tg_read_fn rd, void *ctx,
                           int64_t size, const char *name, tg_file_loc *out,
                           volatile int *progress);
int tg_client_send_album(tg_client *c, const tg_dialog *d, const tg_file_loc *photos, int n,
                         const char *caption);
int tg_client_read_out(tg_client *c, const tg_dialog *d, int32_t *read_out_max);
int tg_client_read_history(tg_client *c, const tg_dialog *d, int32_t max_id);
int tg_client_download(tg_client *c, const tg_file_loc *loc, const char *path);
int tg_client_sign_in(tg_client *c, const char *code);
int tg_client_logout(tg_client *c);

void tg_dialogs_free(tg_dialog *d, int n);
void tg_messages_free(tg_message *m, int n);

#ifdef __cplusplus
}
#endif

#endif
