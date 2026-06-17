/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * diag.h — Diagnostic levels, structure, and utilities for compiler messages.
 *
 * Diagnostics carry a severity level (note / warning / error), a message
 * string, and the source location they refer to.  Provides the diag_vector
 * type and helper queries over the diagnostic collection.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifndef DIAG_DIAG_H
#define DIAG_DIAG_H

#include "collect/vector.h"

typedef enum {
    LEVEL_NOTE,
    LEVEL_WARN,
    LEVEL_ERROR
} level;

typedef struct {
    char whaterr[256];
    int line;
    int column;
    level level_;
} diag;

DECLARE_VECTOR(diag, diag)

void diag_add(diag_vector *diags, level lv, const char *msg, int line, int column);

int has_level(const diag_vector *diags, level lv);

char *diag_print(diag diag_);

#endif /* DIAG_DIAG_H */
