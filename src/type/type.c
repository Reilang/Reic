/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * type.c — Builtin type metadata table and lookup functions.
 *
 * This is the single source of truth for every builtin Rei type.  All other
 * modules (parser, codegen, future sema) consult this table instead of
 * hardcoding type-specific logic.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "type/type.h"

#include <string.h>

const type_info TYPE_TABLE[] = {
    { TYPE_VOID, "void",  "void",  0, false },
    { TYPE_I8,   "int8",  "i8",    8, true  },
    { TYPE_I16,  "int16", "i16",  16, true  },
    { TYPE_I32,  "int32", "i32",  32, true  },
    { TYPE_I64,  "int64", "i64",  64, true  },
    { TYPE_U8,   "nat8",  "i8",    8, false },
    { TYPE_U16,  "nat16", "i16",  16, false },
    { TYPE_U32,  "nat32", "i32",  32, false },
    { TYPE_U64,  "nat64", "i64",  64, false },
};

type_tag type_from_name(const char *name)
{
    int i;
    if (!name) return TYPE_VOID;
    for (i = 0; i < TYPE_COUNT; i++) {
        if (strcmp(name, TYPE_TABLE[i].name) == 0)
            return TYPE_TABLE[i].tag;
    }
    return TYPE_COUNT;
}

const type_info *type_info_of(type_tag tag)
{
    if (tag >= TYPE_COUNT) return &TYPE_TABLE[TYPE_VOID];
    return &TYPE_TABLE[tag];
}
