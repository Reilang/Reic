# CLAUDE.md — ReiC Project Conventions

## Project

ReiC is the reference compiler for the Rei language, written in C11. It compiles
Rei source to native executables via LLVM. The project is in early conceptual
design phase — everything is subject to change.

## Build & Test

```
meson setup builddir                        # debug (-O0 -g)
meson setup builddir --buildtype=release    # release (-O3, no debug)
ninja -C builddir
```

Tests live in `tests/`. Run them directly after build, e.g. `./builddir/tests/test_parser`.

## Coding Style

### C Standard

C11. Allowed: `//` comments, mid-block declarations, `<stdint.h>`, `<stdbool.h>`,
`inline`, variadic macros, designated initializers, flexible array members,
anonymous structs/unions, `_Static_assert`, `_Generic`. **No VLA.**

### Compilation

```
gcc -Wall -Wextra -pedantic -std=c11
```
Or:
```
clang -Weverything -std=c11
```
Zero warnings required. Address/UB sanitizers during development
(`-fsanitize=address,undefined`).

### Formatting

K&R style: function braces on new line, control-flow braces on same line.
4-space indent, max 100 columns, pointer `*` next to identifier (`int *p`).
Run `clang-format -i` before committing.

### Naming

Adopt C standard library abbreviation style: `str`, `buf`, `len`, `init`, `alloc`,
`idx`, `ptr`, `ctx`, `cfg`, `err`, `msg`, `num`, `cnt`, `src`, `dst`, `tmp` etc.
No abbreviation when there's no widely-recognized short form.

- Types: lowercase snake_case; PascalCase for acronym-heavy context structs (`CgCtx`)
- Functions: snake_case with module prefix (`tokenize`, `diag_add`, `node_vec_push`)
- Enum values: UPPER_SNAKE_CASE (`TK_EOF`, `LEVEL_ERROR`, `PSTATE_FUNC`)
- Macros & constants: UPPER_SNAKE_CASE (`DECLARE_VECTOR`)
- File names: `snake_case.c` / `snake_case.h`
- Include guards: `DIR_FILE_H` (`LEXER_LEXER_H`, `TYPE_TYPE_H`)
- Internal headers shared across a module's translation units: `*_internal.h`

### Modules

One public `.h` per module, one or more `.c` files split by construct type
(e.g. `parser.c`, `parser_decl.c`, `parser_stmt.c`, `parser_expr.c`).
Shared internal declarations go in `*_internal.h`, included only by the module's
own `.c` files. Headers carry only types, declarations, and macros — no
implementations, no global variable definitions, no `malloc` calls.
`#ifndef` guard, never `#pragma once`.

### Error & Memory

- Return `0` on success, non-zero on failure; check all return values on critical paths
- `abort()` on allocation failures (no recovery attempted)
- Allocate and free at the same function level; set pointer to `NULL` after free
- `goto` is encouraged when it unifies error handling and resource cleanup

### File Structure

```
/* banner comment block with GPL copyleft notice */

#ifndef MODULE_FILE_H        (headers only)
#define MODULE_FILE_H

#include "own_header.h"      (.c files: own header first)
#include "project/foo.h"      other project headers, alphabetically
#include "project/bar.h"

#include <stdbool.h>          C system headers, alphabetically
#include <stdio.h>
#include <stdlib.h>
```

Include order: own header, other project headers, C system headers.
Each group separated by a blank line, alphabetically within each group.

### Generic Containers

Use the macro-based `DECLARE_VECTOR(type, name)` in `collect/vector.h` for
type-safe dynamic arrays. It generates `name##_vector` struct and
`name##_vec_new` / `_push` / `_pop` / `_get` / `_free` functions.

### Compiler Pipeline

```
Source -> Lexer -> Parser -> AST -> HIR Lower -> HIR -> Sema -> Codegen (LLVM)
```

- `src/lexer/`  — state-machine tokenizer
- `src/parser/` — state-stack recursive-descent parser (split into decl/stmt/expr)
- `src/ast/`    — AST node definitions and debug printing
- `src/hir/`    — high-level IR, lowered from AST, consumed by codegen
- `src/sema/`   — semantic analysis (symbol resolution, type checking)
- `src/codegen/`— HIR -> LLVM IR emission
- `src/token/`  — token type enum and data structure
- `src/type/`   — builtin type system (tags, metadata table, lookup)
- `src/diag/`   — compiler diagnostics (note/warn/error)
- `src/collect/`— generic data structures (vector, strbuf, hashset)

### License

GPLv3+. Every source file must carry the banner with copyleft notice.
