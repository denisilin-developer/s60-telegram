#ifndef TG_QR_H
#define TG_QR_H

#ifdef __cplusplus
extern "C" {
#endif

#define TG_QR_MAX 57

int tg_qr_encode(const char *text, unsigned char out[TG_QR_MAX * TG_QR_MAX]);

#ifdef __cplusplus
}
#endif

#endif
