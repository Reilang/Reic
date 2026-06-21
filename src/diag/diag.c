/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i C                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * diag.c — Query and display of compiler diagnostics.
 *
 * Implements has_level() to test whether the diagnostic set contains any
 * entry at a given severity, and diag_print() for formatted output.
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
#include <stdio.h>
#include <stdlib.h>

#include "diag/diag.h"

void diag_add(diag_vector *diags, level lv, const char *msg, int line, int col)
{
    diag d = {0};
    snprintf(d.whaterr, sizeof(d.whaterr), "%s", msg);
    d.line = line;
    d.col = col;
    d.level_ = lv;
    diag_vec_push(diags, d);
}

int has_level(const diag_vector *diags, level lv)
{
    int i = 0;

    for (i = 0; i < diags->size; i++) {
        if (diags->data[i].level_ == lv)
            return 1;
    }
    return 0;
}

char *diag_print(diag diag_)
{
    char *buffer = NULL;

    buffer = (char*)malloc(512);
    if (!buffer)
        return NULL;
    
    switch (diag_.level_) {
    case LEVEL_NOTE:
        snprintf(buffer, 512, "[NOTE][%d:%d] %s", diag_.line, diag_.col,
                diag_.whaterr);
        break;
    case LEVEL_WARN:
        snprintf(buffer, 512, "[WARN][%d:%d] %s", diag_.line, diag_.col,
                diag_.whaterr);
        break;
    case LEVEL_ERROR:
        snprintf(buffer, 512, "[ERROR][%d:%d] %s", diag_.line, diag_.col,
                diag_.whaterr);
        break;
    default:
        snprintf(buffer, 512, "[UNKNOWN][%d:%d] ???", diag_.line, diag_.col);
    }

    return buffer;
}
