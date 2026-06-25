/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * type.c — Type system: arena, builtin singletons, constructors, interning,
 *          LLVM mapping, debug printing.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 * Copyright (C) 2026  LLLichlet
 *
 * This file is part of ReiC.
 *
 * ReiC is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * ReiC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ReiC.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "type/type.h"
#include "collect/strbuf.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Arena */

#define ARENA_BLOCK_SIZE (65536)

typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    ArenaBlock *next;
    size_t used;
    char data[ARENA_BLOCK_SIZE];
};

static ArenaBlock *arena_head;

static void *arena_alloc(size_t size)
{
    size = (size + _Alignof(max_align_t) - 1) & ~(_Alignof(max_align_t) - 1);

    if (!arena_head || arena_head->used + size > ARENA_BLOCK_SIZE) {
        ArenaBlock *blk = malloc(sizeof(ArenaBlock));
        if (!blk) abort();
        blk->next = arena_head;
        blk->used = 0;
        arena_head = blk;
    }

    void *ptr = arena_head->data + arena_head->used;
    arena_head->used += size;
    return ptr;
}

static char *arena_strdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dst = arena_alloc(len);
    memcpy(dst, s, len);
    return dst;
}

/* Global state */

static int type_next_id;
static Type *end_singleton;

#define PTR_CACHE_MAX 128
static Type *ptr_cache[PTR_CACHE_MAX];
static int ptr_cache_count;

/* Built-in type singletons */

Type *TYPE_UNIT;
Type *TYPE_BOOL;
Type *TYPE_I8,  *TYPE_I16,  *TYPE_I32,  *TYPE_I64;
Type *TYPE_U8,  *TYPE_U16,  *TYPE_U32,  *TYPE_U64;
Type *TYPE_F32, *TYPE_F64;
Type *TYPE_TYPE;

static Type *prim_new(PrimKind prim, int width, bool is_lin, const char *name)
{
    Type *t = arena_alloc(sizeof(Type));
    t->kind = TYPEK_PRIM;
    t->is_lin = is_lin;
    t->id = type_next_id++;
    t->name = arena_strdup(name);
    t->prim = prim;
    t->width = width;
    return t;
}

#define PRIM(p, w, lin, nm) prim_new(PRIM_##p, w, lin, nm)

void type_sys_init(void)
{
    if (TYPE_UNIT) return;

    TYPE_UNIT = PRIM(UNIT,  0,  true,  "unit");
    TYPE_BOOL = PRIM(BOOL,  0,  false, "bool");
    TYPE_I8   = PRIM(INT,   8,  false, "int8");
    TYPE_I16  = PRIM(INT,   16, false, "int16");
    TYPE_I32  = PRIM(INT,   32, false, "int32");
    TYPE_I64  = PRIM(INT,   64, false, "int64");
    TYPE_U8   = PRIM(NAT,   8,  false, "nat8");
    TYPE_U16  = PRIM(NAT,   16, false, "nat16");
    TYPE_U32  = PRIM(NAT,   32, false, "nat32");
    TYPE_U64  = PRIM(NAT,   64, false, "nat64");
    TYPE_F32  = PRIM(FLOAT, 32, false, "float32");
    TYPE_F64  = PRIM(FLOAT, 64, false, "float64");

    TYPE_TYPE = arena_alloc(sizeof(Type));
    TYPE_TYPE->kind = TYPEK_TYPE;
    TYPE_TYPE->is_lin = false;
    TYPE_TYPE->id = type_next_id++;
    TYPE_TYPE->name = arena_strdup("TYPE");
}

#undef PRIM

/* Ownership derivation for enum / union */

static bool derive_is_lin(const Type *const *types, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        if (types[i] && types[i]->is_lin) return true;
    }
    return false;
}

/* Pointer type (interned) */

Type *type_ptr_of(Type *pointee)
{
    int i;

    if (!pointee) return NULL;

    for (i = 0; i < ptr_cache_count; i++) {
        if (ptr_cache[i]->pointee == pointee) return ptr_cache[i];
    }

    Type *t = arena_alloc(sizeof(Type));
    t->kind = TYPEK_PTR;
    t->is_lin = false;
    t->id = type_next_id++;
    t->name = NULL;
    t->pointee = pointee;

    if (ptr_cache_count < PTR_CACHE_MAX)
        ptr_cache[ptr_cache_count++] = t;

    return t;
}

/* Struct */

Type *type_struct_new(const char *name,
                      const char *const *field_names,
                      const Type *const *field_types,
                      int field_count,
                      bool is_lin)
{
    Type *t = arena_alloc(sizeof(Type));
    int i;

    t->kind = TYPEK_STRUCT;
    t->is_lin = is_lin;
    t->id = type_next_id++;
    t->name = arena_strdup(name);
    t->field_count = field_count;

    if (field_count > 0) {
        t->field_names = arena_alloc((size_t)field_count * sizeof(const char *));
        t->field_types = arena_alloc((size_t)field_count * sizeof(Type *));
        for (i = 0; i < field_count; i++) {
            t->field_names[i] = field_names[i] ? arena_strdup(field_names[i]) : NULL;
            t->field_types[i] = field_types[i];
        }
    } else {
        t->field_names = NULL;
        t->field_types = NULL;
    }

    return t;
}

/* Enum */

Type *type_enum_new(const char *name,
                    const char *const *variant_names,
                    const Type *const *variant_types,
                    int variant_count,
                    bool is_lin_override,
                    bool has_override)
{
    Type *t = arena_alloc(sizeof(Type));
    int i;

    t->kind = TYPEK_ENUM;
    t->id = type_next_id++;
    t->name = arena_strdup(name);
    t->field_count = variant_count;

    t->is_lin = has_override
        ? is_lin_override
        : derive_is_lin(variant_types, variant_count);

    if (variant_count > 0) {
        t->field_names = arena_alloc((size_t)variant_count * sizeof(const char *));
        t->field_types = arena_alloc((size_t)variant_count * sizeof(Type *));
        for (i = 0; i < variant_count; i++) {
            t->field_names[i] = variant_names[i] ? arena_strdup(variant_names[i]) : NULL;
            t->field_types[i] = variant_types ? variant_types[i] : NULL;
        }
    } else {
        t->field_names = NULL;
        t->field_types = NULL;
    }

    return t;
}

/* Union */

Type *type_union_new(const char *name,
                     const char *const *field_names,
                     const Type *const *field_types,
                     int field_count,
                     bool is_lin_override,
                     bool has_override)
{
    Type *t = arena_alloc(sizeof(Type));
    int i;

    t->kind = TYPEK_UNION;
    t->id = type_next_id++;
    t->name = arena_strdup(name);
    t->field_count = field_count;

    t->is_lin = has_override
        ? is_lin_override
        : derive_is_lin(field_types, field_count);

    if (field_count > 0) {
        t->field_names = arena_alloc((size_t)field_count * sizeof(const char *));
        t->field_types = arena_alloc((size_t)field_count * sizeof(Type *));
        for (i = 0; i < field_count; i++) {
            t->field_names[i] = field_names[i] ? arena_strdup(field_names[i]) : NULL;
            t->field_types[i] = field_types[i];
        }
    } else {
        t->field_names = NULL;
        t->field_types = NULL;
    }

    return t;
}

/* Session */

Type *type_session_end(void)
{
    if (end_singleton) return end_singleton;

    Type *t = arena_alloc(sizeof(Type));
    t->kind = TYPEK_SESSION;
    t->is_lin = true;
    t->id = type_next_id++;
    t->name = arena_strdup("End");
    t->sess_kind = SESS_END;
    t->sess_payload = NULL;
    t->sess_cont = NULL;
    t->sess_labels = NULL;
    t->sess_branches = NULL;
    t->sess_branch_count = 0;

    end_singleton = t;
    return t;
}

Type *type_session_send(Type *payload, Type *cont)
{
    Type *t = arena_alloc(sizeof(Type));
    t->kind = TYPEK_SESSION;
    t->is_lin = true;
    t->id = type_next_id++;
    t->name = NULL;
    t->sess_kind = SESS_SEND;
    t->sess_payload = payload;
    t->sess_cont = cont ? cont : type_session_end();
    return t;
}

Type *type_session_recv(Type *payload, Type *cont)
{
    Type *t = arena_alloc(sizeof(Type));
    t->kind = TYPEK_SESSION;
    t->is_lin = true;
    t->id = type_next_id++;
    t->name = NULL;
    t->sess_kind = SESS_RECV;
    t->sess_payload = payload;
    t->sess_cont = cont ? cont : type_session_end();
    return t;
}

Type *type_session_offer(const char *const *labels,
                         Type **branches,
                         int branch_count,
                         Type *cont)
{
    Type *t = arena_alloc(sizeof(Type));
    int i;

    t->kind = TYPEK_SESSION;
    t->is_lin = true;
    t->id = type_next_id++;
    t->name = NULL;
    t->sess_kind = SESS_OFFER;
    t->sess_cont = cont;
    t->sess_branch_count = branch_count;

    t->sess_labels = arena_alloc((size_t)branch_count * sizeof(const char *));
    t->sess_branches = arena_alloc((size_t)branch_count * sizeof(Type *));
    for (i = 0; i < branch_count; i++) {
        t->sess_labels[i] = labels[i] ? arena_strdup(labels[i]) : NULL;
        t->sess_branches[i] = branches[i];
    }

    return t;
}

Type *type_session_choose(const char *const *labels,
                          Type **branches,
                          int branch_count,
                          Type *cont)
{
    Type *t = arena_alloc(sizeof(Type));
    int i;

    t->kind = TYPEK_SESSION;
    t->is_lin = true;
    t->id = type_next_id++;
    t->name = NULL;
    t->sess_kind = SESS_CHOOSE;
    t->sess_cont = cont;
    t->sess_branch_count = branch_count;

    t->sess_labels = arena_alloc((size_t)branch_count * sizeof(const char *));
    t->sess_branches = arena_alloc((size_t)branch_count * sizeof(Type *));
    for (i = 0; i < branch_count; i++) {
        t->sess_labels[i] = labels[i] ? arena_strdup(labels[i]) : NULL;
        t->sess_branches[i] = branches[i];
    }

    return t;
}

/* Lookup */

static struct { const char *name; Type *type; } builtin_table[] = {
    { "unit",    NULL },
    { "bool",    NULL },
    { "int8",    NULL },
    { "int16",   NULL },
    { "int32",   NULL },
    { "int64",   NULL },
    { "nat8",    NULL },
    { "nat16",   NULL },
    { "nat32",   NULL },
    { "nat64",   NULL },
    { "float32", NULL },
    { "float64", NULL },
    { "TYPE",    NULL },
    { NULL,      NULL },
};

static bool builtin_table_patched;

static void patch_builtin_table(void)
{
    if (!TYPE_UNIT) type_sys_init();
    builtin_table[0].type  = TYPE_UNIT;
    builtin_table[1].type  = TYPE_BOOL;
    builtin_table[2].type  = TYPE_I8;
    builtin_table[3].type  = TYPE_I16;
    builtin_table[4].type  = TYPE_I32;
    builtin_table[5].type  = TYPE_I64;
    builtin_table[6].type  = TYPE_U8;
    builtin_table[7].type  = TYPE_U16;
    builtin_table[8].type  = TYPE_U32;
    builtin_table[9].type  = TYPE_U64;
    builtin_table[10].type = TYPE_F32;
    builtin_table[11].type = TYPE_F64;
    builtin_table[12].type = TYPE_TYPE;
    builtin_table_patched = true;
}

Type *type_from_name(const char *name)
{
    int i;

    if (!name) return NULL;
    if (!builtin_table_patched) patch_builtin_table();

    for (i = 0; builtin_table[i].name; i++) {
        if (strcmp(name, builtin_table[i].name) == 0)
            return builtin_table[i].type;
    }
    return NULL;
}

bool type_is_integer(const Type *t)
{
    return t
        && t->kind == TYPEK_PRIM
        && (t->prim == PRIM_INT || t->prim == PRIM_NAT);
}

bool type_is_float(const Type *t)
{
    return t
        && t->kind == TYPEK_PRIM
        && t->prim == PRIM_FLOAT;
}

/* LLVM mapping */

static const char *int_llvm_name(int width)
{
    switch (width) {
    case 8:  return "i8";
    case 16: return "i16";
    case 32: return "i32";
    case 64: return "i64";
    default: return "i32";
    }
}

const char *type_llvm_name(const Type *t)
{
    if (!t) return "void";

    switch (t->kind) {
    case TYPEK_PRIM:
        switch (t->prim) {
        case PRIM_UNIT:  return "void";
        case PRIM_BOOL:  return "i1";
        case PRIM_INT:   return int_llvm_name(t->width);
        case PRIM_NAT:   return int_llvm_name(t->width);
        case PRIM_FLOAT:
            return t->width == 64 ? "double" : "float";
        }
        break;
    case TYPEK_PTR:
        return "ptr";
    case TYPEK_STRUCT:
        if (t->name) {
            static char buf[64];
            snprintf(buf, sizeof(buf), "%%%s", t->name);
            return buf;
        }
        return "ptr";
    case TYPEK_ENUM:
    case TYPEK_UNION:
        return "ptr";
    case TYPEK_SESSION:
    case TYPEK_TYPE:
        return "";
    }
    return "void";
}

int type_width(const Type *t)
{
    if (!t || t->kind != TYPEK_PRIM) return 0;
    return t->width;
}

bool type_is_signed(const Type *t)
{
    return t && t->kind == TYPEK_PRIM && t->prim == PRIM_INT;
}

/* Debug printing */

static void type_print_impl(const Type *t, strbuf *sb, int depth);

static void print_prim(const Type *t, strbuf *sb)
{
    switch (t->prim) {
    case PRIM_UNIT:  strbuf_add(sb, "unit"); break;
    case PRIM_BOOL:  strbuf_add(sb, "bool"); break;
    case PRIM_INT:   strbuf_addf(sb, "int%d", t->width); break;
    case PRIM_NAT:   strbuf_addf(sb, "nat%d", t->width); break;
    case PRIM_FLOAT: strbuf_addf(sb, "float%d", t->width); break;
    }
}

static void print_fields(const Type *t, strbuf *sb, int depth)
{
    int i;
    for (i = 0; i < t->field_count; i++) {
        if (i > 0) strbuf_add(sb, ", ");
        if (t->field_names[i])
            strbuf_addf(sb, "%s: ", t->field_names[i]);
        if (t->field_types[i])
            type_print_impl(t->field_types[i], sb, depth + 1);
        else
            strbuf_add(sb, "?");
    }
}

static void print_session(const Type *t, strbuf *sb, int depth)
{
    int i;
    if (depth > 12) { strbuf_add(sb, "..."); return; }

    switch (t->sess_kind) {
    case SESS_SEND:
        strbuf_add(sb, "!");
        if (t->sess_payload) type_print_impl(t->sess_payload, sb, depth + 1);
        else strbuf_add(sb, "?");
        if (t->sess_cont && t->sess_cont->sess_kind != SESS_END) {
            strbuf_add(sb, ".");
            print_session(t->sess_cont, sb, depth + 1);
        }
        break;
    case SESS_RECV:
        strbuf_add(sb, "?");
        if (t->sess_payload) type_print_impl(t->sess_payload, sb, depth + 1);
        else strbuf_add(sb, "?");
        if (t->sess_cont && t->sess_cont->sess_kind != SESS_END) {
            strbuf_add(sb, ".");
            print_session(t->sess_cont, sb, depth + 1);
        }
        break;
    case SESS_OFFER:
        strbuf_add(sb, "Offer { ");
        for (i = 0; i < t->sess_branch_count; i++) {
            if (i > 0) strbuf_add(sb, ", ");
            strbuf_addf(sb, "%s: ", t->sess_labels[i] ? t->sess_labels[i] : "?");
            print_session(t->sess_branches[i], sb, depth + 1);
        }
        strbuf_add(sb, " }");
        if (t->sess_cont && t->sess_cont->sess_kind != SESS_END) {
            strbuf_add(sb, ".");
            print_session(t->sess_cont, sb, depth + 1);
        }
        break;
    case SESS_CHOOSE:
        strbuf_add(sb, "Choose { ");
        for (i = 0; i < t->sess_branch_count; i++) {
            if (i > 0) strbuf_add(sb, ", ");
            strbuf_addf(sb, "%s: ", t->sess_labels[i] ? t->sess_labels[i] : "?");
            print_session(t->sess_branches[i], sb, depth + 1);
        }
        strbuf_add(sb, " }");
        if (t->sess_cont && t->sess_cont->sess_kind != SESS_END) {
            strbuf_add(sb, ".");
            print_session(t->sess_cont, sb, depth + 1);
        }
        break;
    case SESS_END:
        strbuf_add(sb, "End");
        break;
    }
}

static void type_print_impl(const Type *t, strbuf *sb, int depth)
{
    if (!t) { strbuf_add(sb, "(null)"); return; }
    if (depth > 12) { strbuf_add(sb, "..."); return; }

    switch (t->kind) {
    case TYPEK_PRIM:
        print_prim(t, sb);
        break;
    case TYPEK_PTR:
        strbuf_add(sb, "*");
        type_print_impl(t->pointee, sb, depth + 1);
        break;
    case TYPEK_STRUCT:
        strbuf_add(sb, "struct { ");
        print_fields(t, sb, depth);
        strbuf_add(sb, " }");
        break;
    case TYPEK_ENUM:
        strbuf_add(sb, "enum { ");
        {
            int i;
            for (i = 0; i < t->field_count; i++) {
                if (i > 0) strbuf_add(sb, ", ");
                strbuf_add(sb, t->field_names[i] ? t->field_names[i] : "?");
                if (t->field_types[i]) {
                    strbuf_add(sb, "(");
                    type_print_impl(t->field_types[i], sb, depth + 1);
                    strbuf_add(sb, ")");
                }
            }
        }
        strbuf_add(sb, " }");
        break;
    case TYPEK_UNION:
        strbuf_add(sb, "union { ");
        print_fields(t, sb, depth);
        strbuf_add(sb, " }");
        break;
    case TYPEK_SESSION:
        print_session(t, sb, depth);
        break;
    case TYPEK_TYPE:
        strbuf_add(sb, "TYPE");
        break;
    }
}

char *type_print(const Type *t)
{
    strbuf sb;
    strbuf_init(&sb, 128);
    type_print_impl(t, &sb, 0);
    return strbuf_detach(&sb);
}
