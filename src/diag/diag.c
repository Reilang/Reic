/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * diag.c — Query and display of compiler diagnostics.
 *
 * Implements has_level() to test whether the diagnostic set contains any
 * entry at a given severity, and diag_print() for formatted output.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "diag/diag.h"

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
    char* buffer = NULL;

    buffer = (char*)malloc(256);
    if (!buffer)
        return NULL;

    
}
