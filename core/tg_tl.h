#ifndef TG_TL_H
#define TG_TL_H

#include <stdint.h>

#include "tg_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TL_VECTOR 0x1cb5c415u
#define TL_BOOL_TRUE 0x997275b5u
#define TL_BOOL_FALSE 0xbc799737u

#define TL_REQ_PQ_MULTI 0xbe7e8ef1u
#define TL_RES_PQ 0x05162463u
#define TL_P_Q_INNER_DATA_DC 0xa9f55f95u
#define TL_REQ_DH_PARAMS 0xd712e4beu
#define TL_SERVER_DH_PARAMS_OK 0xd0e8075cu
#define TL_SERVER_DH_PARAMS_FAIL 0x79cb045du
#define TL_SERVER_DH_INNER_DATA 0xb5890dbau
#define TL_CLIENT_DH_INNER_DATA 0x6643b654u
#define TL_SET_CLIENT_DH_PARAMS 0xf5045f1fu
#define TL_DH_GEN_OK 0x3bcbf734u
#define TL_DH_GEN_RETRY 0x46dc1fb9u
#define TL_DH_GEN_FAIL 0xa69dae02u

#define TL_RPC_RESULT 0xf35c6d01u
#define TL_RPC_ERROR 0x2144ca19u
#define TL_BAD_MSG_NOTIFICATION 0xa7eff811u
#define TL_BAD_SERVER_SALT 0xedab447bu
#define TL_NEW_SESSION_CREATED 0x9ec20908u
#define TL_MSG_CONTAINER 0x73f1f8dcu
#define TL_GZIP_PACKED 0x3072cfa1u
#define TL_MSGS_ACK 0x62d6b459u
#define TL_PING 0x7abe77ecu
#define TL_PONG 0x347773c5u

#define TL_LAYER 222
#define TL_INVOKE_WITH_LAYER 0xda9b0d0du
#define TL_INIT_CONNECTION 0xc1cd5ea9u
#define TL_HELP_GET_NEAREST_DC 0x1fb33026u

#define TL_AUTH_SEND_CODE 0xa677244fu
#define TL_AUTH_SIGN_IN 0x8d52a951u
#define TL_AUTH_RESEND_CODE 0xcae47523u
#define TL_AUTH_EXPORT_LOGIN_TOKEN 0xb7e085feu
#define TL_AUTH_IMPORT_LOGIN_TOKEN 0x95ac5ce4u
#define TL_UPDATES_GET_STATE 0xedd4882au
#define TL_UPLOAD_GET_FILE 0xbe5335beu
#define TL_UPLOAD_SAVE_FILE_PART 0xb304a621u
#define TL_UPLOAD_SAVE_BIG_FILE_PART 0xde7b673du
#define TL_MESSAGES_SEND_MEDIA 0x0330e77fu
#define TL_MESSAGES_UPLOAD_MEDIA 0x14967978u
#define TL_MESSAGES_SEND_MULTI_MEDIA 0x1bf89d74u
#define TL_INPUT_SINGLE_MEDIA 0x1cc6e91fu
#define TL_INPUT_MEDIA_PHOTO 0xb3ba0635u
#define TL_INPUT_PHOTO 0x3bb3b94au
#define TL_INPUT_MEDIA_UPLOADED_PHOTO 0x1e287d04u
#define TL_INPUT_MEDIA_UPLOADED_DOCUMENT 0x037c9330u
#define TL_INPUT_FILE 0xf52ff27fu
#define TL_INPUT_FILE_BIG 0xfa4f0bb5u
#define TL_DOCUMENT_ATTRIBUTE_FILENAME 0x15590068u
#define TL_MESSAGES_READ_HISTORY 0x0e306d3au
#define TL_MESSAGES_EDIT_MESSAGE 0x51e842e1u
#define TL_ACCOUNT_GET_PASSWORD 0x548a30f5u
#define TL_AUTH_CHECK_PASSWORD 0xd18b4d16u
#define TL_INPUT_CHECK_PASSWORD_SRP 0xd27ff082u
#define TL_USERS_GET_FULL_USER 0xb60f5918u
#define TL_INPUT_USER 0xf21158c6u
#define TL_INPUT_USER_SELF 0xf7c1b13fu
#define TL_CHANNELS_GET_FULL_CHANNEL 0x08736a09u
#define TL_MESSAGES_GET_FULL_CHAT 0xaeb00b34u
#define TL_CONTACTS_GET_CONTACTS 0x5dd69e12u
#define TL_CONTACTS_RESOLVE_USERNAME 0x725afbbcu
#define TL_MESSAGES_TOGGLE_DIALOG_PIN 0xa731e257u
#define TL_ACCOUNT_UPDATE_NOTIFY_SETTINGS 0x84be5b93u
#define TL_INPUT_NOTIFY_PEER 0xb8bc5b0cu
#define TL_INPUT_PEER_NOTIFY_SETTINGS 0xcacb6ae2u
#define TL_MESSAGES_MARK_DIALOG_UNREAD 0x8c5006f8u
#define TL_FOLDERS_EDIT_PEER_FOLDERS 0x6847d0abu
#define TL_INPUT_FOLDER_PEER 0xfbd2c296u
#define TL_MESSAGES_DELETE_MESSAGES 0xe58e95d2u
#define TL_CHANNELS_DELETE_MESSAGES 0x84c1fd4eu
#define TL_MESSAGES_FORWARD_MESSAGES 0x13704a7cu
#define TL_INPUT_REPLY_TO_MESSAGE 0x869fbe10u
#define TL_MESSAGES_SEND_REACTION 0xd30d78d4u
#define TL_REACTION_EMOJI 0x1b2286b8u
#define TL_MESSAGES_SEARCH 0x29ee847au
#define TL_INPUT_MESSAGES_FILTER_PINNED 0x1bb00451u
#define TL_MESSAGES_GET_PEER_DIALOGS 0xe470bcfdu
#define TL_INPUT_DIALOG_PEER 0xfcaafeb7u
#define TL_CHANNELS_READ_HISTORY 0xcc104937u
#define TL_INPUT_CHANNEL 0xf35aec28u
#define TL_UPLOAD_FILE 0x096a18d5u
#define TL_AUTH_EXPORT_AUTHORIZATION 0xe5bfffcdu
#define TL_AUTH_EXPORTED_AUTHORIZATION 0xb434e2b8u
#define TL_AUTH_IMPORT_AUTHORIZATION 0xa57a7dadu
#define TL_INPUT_PEER_PHOTO_FILE_LOCATION 0x37257e99u
#define TL_INPUT_PHOTO_FILE_LOCATION 0x40181ffeu
#define TL_INPUT_DOCUMENT_FILE_LOCATION 0xbad07584u
#define TL_AUTH_LOG_OUT 0x3e72ba19u
#define TL_CODE_SETTINGS 0xad253d78u

#define TL_MESSAGES_GET_DIALOGS 0xa0f4cb4fu
#define TL_MESSAGES_GET_DIALOG_FILTERS 0xefd48c89u
#define TL_MESSAGES_GET_HISTORY 0x4423e6c5u
#define TL_MESSAGES_SEND_MESSAGE 0x545cd15au
#define TL_INPUT_PEER_EMPTY 0x7f3b18eau
#define TL_INPUT_PEER_CHAT 0x35a95cb9u
#define TL_INPUT_PEER_USER 0xdde8a54cu
#define TL_INPUT_PEER_CHANNEL 0x27bcbbfcu

typedef struct {
    const char *name;
    const char *spec;
    const char *flag;
    int bit;
} tg_field;

typedef struct {
    uint32_t cid;
    const char *name;
    const tg_field *fields;
    int nfields;
} tg_decl;

extern const tg_decl tg_decls[];
extern const int tg_decl_count;

enum {
    TGV_INT,
    TGV_LONG,
    TGV_DOUBLE,
    TGV_BYTES,
    TGV_BOOL,
    TGV_OBJ,
    TGV_VEC
};

typedef struct tgv tgv;
struct tgv {
    int kind;
    union {
        int32_t i;
        int64_t l;
        double d;
        int b;
        struct {
            const unsigned char *p;
            int n;
        } s;
        struct {
            const tg_decl *decl;
            tgv **vals;
        } o;
        struct {
            tgv **items;
            int n;
        } v;
    } u;
};

const tg_decl *tg_find_decl(uint32_t cid);
tgv *tg_parse(tg_arena *a, tg_rd *r, uint32_t *unknown_cid);

const char *tgv_name(const tgv *o);
int tgv_is(const tgv *o, const char *name);
tgv *tgv_get(const tgv *o, const char *field);
int32_t tgv_i32(const tgv *o, const char *field, int32_t dflt);
int64_t tgv_i64(const tgv *o, const char *field, int64_t dflt);
int tgv_flag(const tgv *o, const char *field);
const unsigned char *tgv_str(const tgv *o, const char *field, int *n);
int tgv_vec_len(const tgv *v);
tgv *tgv_vec_at(const tgv *v, int i);

#ifdef __cplusplus
}
#endif

#endif
