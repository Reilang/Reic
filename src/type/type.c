/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * type.c — Builtin type metadata table and lookup functions.
 *
 * This is the single source of truth for every builtin Rei type.  All other
 * modules (parser, codegen, future sema) consult this table instead of
 * hardcoding type-specific logic.
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
