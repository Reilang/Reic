/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * type.h — Unified type system: kinds, constructors, interning, LLVM mapping.
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
#ifndef TYPE_TYPE_H
#define TYPE_TYPE_H

#include "collect/vector.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TYPEK_PRIM,      /* unit, bool, intN, natN, floatN      */
    TYPEK_PTR,       /* *T                                  */
    TYPEK_STRUCT,    /* struct { ... }                      */
    TYPEK_ENUM,      /* enum { ... }   — tagged union       */
    TYPEK_UNION,     /* union { ... }  — untagged union     */
    TYPEK_SESSION,   /* session { protocol }                */
    TYPEK_TYPE,      /* TYPE — meta-type of all types       */
} TypeKind;

typedef enum {
    PRIM_UNIT,
    PRIM_BOOL,
    PRIM_INT,
    PRIM_NAT,
    PRIM_FLOAT,
} PrimKind;

typedef enum {
    SESS_SEND,       /* !T.cont                              */
    SESS_RECV,       /* ?T.cont                              */
    SESS_OFFER,      /* Offer { l1: S1, l2: S2 }.cont        */
    SESS_CHOOSE,     /* Choose { l1: S1, l2: S2 }.cont       */
    SESS_END,        /* session termination                  */
} SessionKind;

typedef struct Type Type;
struct Type {
    TypeKind kind;
    bool is_lin;
    int id;
    const char *name;       /* debug name, not used for equality */

    union {
        /* TYPEK_PRIM */
        struct {
            PrimKind prim;
            int width;      /* bit width; 0 for unit and bool */
        };
        /* TYPEK_PTR */
        struct {
            Type *pointee;
        };
        /* TYPEK_STRUCT / TYPEK_ENUM / TYPEK_UNION */
        struct {
            const Type **field_types;
            const char **field_names;
            int field_count;
        };
        /* TYPEK_SESSION */
        struct {
            SessionKind sess_kind;
            Type *sess_payload;
            Type *sess_cont;
            const char **sess_labels;
            Type **sess_branches;
            int sess_branch_count;
        };
        /* TYPEK_TYPE: no extra fields */
    };
};

DECLARE_VECTOR(Type*, type_ptr)

extern Type *TYPE_UNIT;
extern Type *TYPE_BOOL;
extern Type *TYPE_I8,  *TYPE_I16,  *TYPE_I32,  *TYPE_I64;
extern Type *TYPE_U8,  *TYPE_U16,  *TYPE_U32,  *TYPE_U64;
extern Type *TYPE_F32, *TYPE_F64;
extern Type *TYPE_TYPE;

void type_sys_init(void);

Type *type_ptr_of(Type *pointee);

Type *type_struct_new(const char *name,
                      const char *const *field_names,
                      const Type *const *field_types,
                      int field_count,
                      bool is_lin);

Type *type_enum_new(const char *name,
                    const char *const *variant_names,
                    const Type *const *variant_types,
                    int variant_count,
                    bool is_lin_override,
                    bool has_override);

Type *type_union_new(const char *name,
                     const char *const *field_names,
                     const Type *const *field_types,
                     int field_count,
                     bool is_lin_override,
                     bool has_override);

Type *type_session_send(Type *payload, Type *cont);
Type *type_session_recv(Type *payload, Type *cont);
Type *type_session_offer(const char *const *labels,
                         Type **branches,
                         int branch_count,
                         Type *cont);
Type *type_session_choose(const char *const *labels,
                          Type **branches,
                          int branch_count,
                          Type *cont);
Type *type_session_end(void);

Type *type_from_name(const char *name);
bool type_is_integer(const Type *t);
bool type_is_float(const Type *t);

const char *type_llvm_name(const Type *t);
int type_width(const Type *t);
bool type_is_signed(const Type *t);

char *type_print(const Type *t);

#endif /* TYPE_TYPE_H */
