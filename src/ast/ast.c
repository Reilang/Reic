/*
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  R e i L a n g                                                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ast.c — AST node utility: debug string conversion.
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#include "ast/ast.h"

#include <stdio.h>
#include <stdlib.h>

static const char *kind_name(anode_kind kind)
{
    switch (kind) {
    case ANODE_NONE:       return "NONE";
    case ANODE_IDENT:      return "IDENT";
    case ANODE_IDENT_FUNC: return "IDENT_FUNC";
    case ANODE_IDENT_VAR:  return "IDENT_VAR";
    case ANODE_IDENT_TYPE: return "IDENT_TYPE";
    case ANODE_ILITERAL:   return "ILITERAL";
    case ANODE_FLITERAL:   return "FLITERAL";
    case ANODE_SLITERAL:   return "SLITERAL";
    case ANODE_CLITERAL:   return "CLITERAL";
    case ANODE_BINOP:      return "BINOP";
    case ANODE_UNOP:       return "UNOP";
    case ANODE_ASSIGN:     return "ASSIGN";
    case ANODE_CALL:       return "CALL";
    case ANODE_INDEX:      return "INDEX";
    case ANODE_BLOCK:      return "BLOCK";
    case ANODE_IF:         return "IF";
    case ANODE_WHILE:      return "WHILE";
    case ANODE_FOR:        return "FOR";
    case ANODE_RETURN:     return "RETURN";
    case ANODE_MATCHARM:   return "MATCHARM";
    case ANODE_FUNCDECL:   return "FUNCDECL";
    case ANODE_VARDECL:    return "VARDECL";
    case ANODE_TYPEDECL:   return "TYPEDECL";
    default:               return "???";
    }
}

char *anode_print(anode node_)
{
    char *buf = (char*)malloc(256);
    if (!buf)
        return NULL;

    switch (node_.kind) {
    case ANODE_IDENT:
    case ANODE_IDENT_FUNC:
    case ANODE_IDENT_VAR:
    case ANODE_IDENT_TYPE:
        snprintf(buf, 256, "%s '%s'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 node_.child, node_.next);
        break;
    case ANODE_ILITERAL:
        snprintf(buf, 256, "%s %lld  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 (long long)node_.iv,
                 node_.child, node_.next);
        break;
    case ANODE_FLITERAL:
        snprintf(buf, 256, "%s %f  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.fv,
                 node_.child, node_.next);
        break;
    case ANODE_SLITERAL:
        snprintf(buf, 256, "%s \"%s\"  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.sv ? node_.sv : "(null)",
                 node_.child, node_.next);
        break;
    case ANODE_CLITERAL:
        snprintf(buf, 256, "%s '%c'  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.cv,
                 node_.child, node_.next);
        break;
    default:
        snprintf(buf, 256, "%s  (child=%d, next=%d)",
                 kind_name(node_.kind),
                 node_.child, node_.next);
        break;
    }

    return buf;
}
