#include <string.h>

#include "tg_tl.h"

const tg_decl *tg_find_decl(uint32_t cid)
{
    int lo = 0, hi = tg_decl_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (tg_decls[mid].cid == cid)
            return &tg_decls[mid];
        if (tg_decls[mid].cid < cid)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return NULL;
}

typedef struct {
    tg_arena *a;
    tg_rd *r;
    uint32_t unknown;
    int depth;
} pctx;

static tgv *parse_value(pctx *c, const char *spec);

static tgv *new_val(pctx *c, int kind)
{
    tgv *v = (tgv *)tg_arena_alloc(c->a, sizeof(tgv));
    if (v)
        v->kind = kind;
    return v;
}

static tgv *parse_object(pctx *c)
{
    const tg_decl *d;
    tgv *o;
    uint32_t cid;
    int i;
    if (++c->depth > 64) {
        c->r->err = 1;
        return NULL;
    }
    cid = tg_rd_u32(c->r);
    if (c->r->err)
        return NULL;
    d = tg_find_decl(cid);
    if (!d) {
        c->unknown = cid;
        c->r->err = 1;
        return NULL;
    }
    o = new_val(c, TGV_OBJ);
    if (!o)
        return NULL;
    o->u.o.decl = d;
    if (d->nfields) {
        o->u.o.vals = (tgv **)tg_arena_alloc(c->a, d->nfields * (int)sizeof(tgv *));
        if (!o->u.o.vals)
            return NULL;
    }
    for (i = 0; i < d->nfields; i++) {
        const tg_field *f = &d->fields[i];
        if (f->flag) {
            int j;
            int32_t flags = 0;
            for (j = 0; j < i; j++) {
                if (!strcmp(d->fields[j].name, f->flag) && o->u.o.vals[j]) {
                    flags = o->u.o.vals[j]->u.i;
                    break;
                }
            }
            if (!((flags >> f->bit) & 1))
                continue;
        }
        o->u.o.vals[i] = parse_value(c, f->spec);
        if (c->r->err || c->a->oom)
            return NULL;
    }
    c->depth--;
    return o;
}

static tgv *parse_value(pctx *c, const char *spec)
{
    tgv *v;
    if (!strcmp(spec, "int") || !strcmp(spec, "#")) {
        v = new_val(c, TGV_INT);
        if (v)
            v->u.i = tg_rd_i32(c->r);
        return v;
    }
    if (!strcmp(spec, "long")) {
        v = new_val(c, TGV_LONG);
        if (v)
            v->u.l = tg_rd_i64(c->r);
        return v;
    }
    if (!strcmp(spec, "string") || !strcmp(spec, "bytes")) {
        v = new_val(c, TGV_BYTES);
        if (v)
            v->u.s.p = tg_rd_bytes(c->r, &v->u.s.n);
        return v;
    }
    if (!strcmp(spec, "true")) {
        v = new_val(c, TGV_BOOL);
        if (v)
            v->u.b = 1;
        return v;
    }
    if (!strcmp(spec, "double")) {
        v = new_val(c, TGV_DOUBLE);
        if (v)
            v->u.d = tg_rd_double(c->r);
        return v;
    }
    if (!strcmp(spec, "int128") || !strcmp(spec, "int256")) {
        int n = spec[3] == '1' ? 16 : 32;
        v = new_val(c, TGV_BYTES);
        if (v) {
            v->u.s.p = tg_rd_raw(c->r, n);
            v->u.s.n = n;
        }
        return v;
    }
    if (!strcmp(spec, "Bool")) {
        uint32_t cid = tg_rd_u32(c->r);
        v = new_val(c, TGV_BOOL);
        if (cid != TL_BOOL_TRUE && cid != TL_BOOL_FALSE)
            c->r->err = 1;
        if (v)
            v->u.b = cid == TL_BOOL_TRUE;
        return v;
    }
    if (!strncmp(spec, "Vector<", 7)) {
        char inner[64];
        int n = (int)strlen(spec) - 8, count, i;
        if (n <= 0 || n >= (int)sizeof inner) {
            c->r->err = 1;
            return NULL;
        }
        memcpy(inner, spec + 7, n);
        inner[n] = 0;
        if (tg_rd_u32(c->r) != TL_VECTOR) {
            c->r->err = 1;
            return NULL;
        }
        count = tg_rd_i32(c->r);
        if (c->r->err || count < 0 || count > c->r->len) {
            c->r->err = 1;
            return NULL;
        }
        v = new_val(c, TGV_VEC);
        if (!v)
            return NULL;
        v->u.v.n = count;
        if (count) {
            v->u.v.items = (tgv **)tg_arena_alloc(c->a, count * (int)sizeof(tgv *));
            if (!v->u.v.items)
                return NULL;
        }
        for (i = 0; i < count; i++) {
            v->u.v.items[i] = parse_value(c, inner);
            if (c->r->err || c->a->oom)
                return NULL;
        }
        return v;
    }
    return parse_object(c);
}

tgv *tg_parse(tg_arena *a, tg_rd *r, uint32_t *unknown_cid)
{
    pctx c;
    tgv *v;
    c.a = a;
    c.r = r;
    c.unknown = 0;
    c.depth = 0;
    v = parse_object(&c);
    if (unknown_cid)
        *unknown_cid = c.unknown;
    if (r->err || a->oom)
        return NULL;
    return v;
}

const char *tgv_name(const tgv *o)
{
    return o && o->kind == TGV_OBJ ? o->u.o.decl->name : "";
}

int tgv_is(const tgv *o, const char *name)
{
    return !strcmp(tgv_name(o), name);
}

tgv *tgv_get(const tgv *o, const char *field)
{
    int i;
    if (!o || o->kind != TGV_OBJ)
        return NULL;
    for (i = 0; i < o->u.o.decl->nfields; i++)
        if (!strcmp(o->u.o.decl->fields[i].name, field))
            return o->u.o.vals[i];
    return NULL;
}

int32_t tgv_i32(const tgv *o, const char *field, int32_t dflt)
{
    tgv *v = tgv_get(o, field);
    return v && v->kind == TGV_INT ? v->u.i : dflt;
}

int64_t tgv_i64(const tgv *o, const char *field, int64_t dflt)
{
    tgv *v = tgv_get(o, field);
    if (v && v->kind == TGV_LONG)
        return v->u.l;
    if (v && v->kind == TGV_INT)
        return v->u.i;
    return dflt;
}

int tgv_flag(const tgv *o, const char *field)
{
    tgv *v = tgv_get(o, field);
    return v && v->kind == TGV_BOOL && v->u.b;
}

const unsigned char *tgv_str(const tgv *o, const char *field, int *n)
{
    tgv *v = tgv_get(o, field);
    if (v && v->kind == TGV_BYTES) {
        *n = v->u.s.n;
        return v->u.s.p;
    }
    *n = 0;
    return NULL;
}

int tgv_vec_len(const tgv *v)
{
    return v && v->kind == TGV_VEC ? v->u.v.n : 0;
}

tgv *tgv_vec_at(const tgv *v, int i)
{
    return v && v->kind == TGV_VEC && i >= 0 && i < v->u.v.n ? v->u.v.items[i] : NULL;
}
