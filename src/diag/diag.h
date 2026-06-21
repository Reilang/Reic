/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * diag.h — Diagnostic levels, structure, and utilities for compiler messages.
 *
 * Diagnostics carry a severity level (note / warning / error), a message
 * string, and the source location they refer to.  Provides the diag_vector
 * type and helper queries over the diagnostic collection.
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
#ifndef DIAG_DIAG_H
#define DIAG_DIAG_H

#include "collect/vector.h"

/* Severity level for a diagnostic. */
typedef enum {
    LEVEL_NOTE,    /* informational */
    LEVEL_WARN,    /* warning — does not prevent codegen */
    LEVEL_ERROR    /* error — codegen is skipped when errors are present */
} level;

/* A single diagnostic message with source location. */
typedef struct {
    char whaterr[256];  /* human-readable message */
    int line;            /* 1-based source line */
    int col;             /* 0-based column */
    level level_;
} diag;

DECLARE_VECTOR(diag, diag)

/*
 * Appends a diagnostic to the vector.  msg is copied into whaterr[]
 * (truncated at 255 characters).
 */
void diag_add(diag_vector *diags, level lv, const char *msg, int line, int col);

/* Returns non-zero if any diagnostic in the vector has at least the given level. */
int has_level(const diag_vector *diags, level lv);

/*
 * Allocates and returns a human-readable string for a single diagnostic.
 * Caller must free() the result.
 */
char *diag_print(diag diag_);

#endif /* DIAG_DIAG_H */
