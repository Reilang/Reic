/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
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

#include "collect/vector.h"

#include <stdbool.h>

/*
 * Type tag enum.  Each tag indexes directly into TYPE_TABLE[].
 * TYPE_COUNT serves as the "not found" sentinel from type_from_name().
 */
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

/*
 * Type metadata — single source of truth for each builtin type.
 * All other modules consult this table instead of hardcoding type logic.
 */
typedef struct {
    type_tag tag;
    const char *name;       /* Rei source name, e.g. "int32" */
    const char *llvm_name;  /* LLVM IR type, e.g. "i32" */
    int width;              /* bit width (0 for void) */
    bool is_signed;         /* true for int*, false for nat* and void */
} type_info;

/* Type tag vector — used for function parameter lists, etc. */
DECLARE_VECTOR(type_tag, type_tag)

/* The builtin type table, indexed by type_tag. */
extern const type_info TYPE_TABLE[];

/*
 * Looks up a type name (Rei source string).  Returns the corresponding tag,
 * or TYPE_COUNT if the name is not a known builtin type.
 */
type_tag type_from_name(const char *name);

/*
 * Returns a pointer to the type_info for the given tag.
 * Falls back to TYPE_VOID for out-of-range tags.
 */
const type_info *type_info_of(type_tag tag);

#endif /* TYPE_TYPE_H */
