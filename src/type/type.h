/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * type.h — Unified type system: enum tag + metadata table.
 *
 * Every builtin type has a type_tag.  The TYPE_TABLE stores name, LLVM name,
 * bit width, and signedness for each tag.  All type queries go through
 * type_from_name() (parser side) or type_info_of() (codegen side) — string
 * comparison happens exactly once, at name resolution time.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef TYPE_TYPE_H
#define TYPE_TYPE_H

#include <stdbool.h>

typedef enum {
    TYPE_VOID,

    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,

    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,

    TYPE_COUNT
} type_tag;

typedef struct {
    type_tag tag;
    const char *name;
    const char *llvm_name;
    int width;
    bool is_signed;
} type_info;

extern const type_info TYPE_TABLE[];

type_tag         type_from_name(const char *name);
const type_info *type_info_of(type_tag tag);

#endif /* TYPE_TYPE_H */
