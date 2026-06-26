# ReiC

ReiC is the reference compiler for the [Rei](https://github.com/reilang) language,
compiling Rei source code to native executables via LLVM IR.

> **Status: early development.** The full pipeline (lex -> parse -> sema -> lower -> codegen)
> runs end-to-end and emits readable LLVM IR. The language is still small — structs, control
> flow, and arithmetic work, but many features are missing. Not yet suitable for production use.

## Pipeline

```
Source -> Lexer -> Parser -> AST -> Sema -> HIR Lower -> HIR -> Codegen (LLVM IR text)
```

- **Lexer** (`src/lexer/`) — state-machine tokenizer with paren-aware newline suppression
- **Parser** (`src/parser/`) — state-stack recursive-descent parser, emits a flat AST
- **Sema** (`src/sema/`) — semantic analysis: nested symbol tables, type inference, type checking
- **HIR Lower** (`src/hir/`) — translates AST + sema annotations into typed HIR
- **Codegen** (`src/codegen/`) — walks HIR and emits LLVM IR text (no library dependency)
- **Type** (`src/type/`) — interning type system with primitives, pointers, structs, sessions
- **Token** (`src/token/`) — token type definitions and debug printing
- **Diag** (`src/diag/`) — compiler diagnostics (note / warning / error)
- **Collect** (`src/collect/`) — generic data structures (vector, hashset, strbuf)

## Language Tour

### Functions

```
fn add(a: int32, b: int32) -> int32 {
    return a + b
}
```

### Variables

```
var x: int32 := 1          // typed with initializer
var y := x + 2            // type inferred
x := x + y                // reassignment
```

### Compile-Time Constants

`Name = expr` binds a name to a value known at compile time. The name can then be
used wherever the value is expected — no `var`, no `let`, just an equals sign.
The right-hand side can be any compile-time expression: an integer literal, a type
literal, or (in the future) more complex constant expressions.

**Type constants** — the value is a type:

```
Vec2 = struct {
    x: int32
    y: int32
}

fn make_vec() -> int32 {
    var v := Vec2 { x: 1, y: 2 }
    v.x := v.y + 1
    return v.x
}
```

`Vec2 = struct { ... }` and `X = 42` are the same syntactic form — the right-hand side
just happens to be a type literal rather than an integer literal. Once bound, `Vec2` can
be used wherever a type name is expected: variable declarations, struct literals, field
access. This extends naturally to `enum`, `union`, and `session` type constructors.

**Integer constants** — folded directly into use sites at codegen time:

```
X = 42
return X    // emits ret i32 42 — no load, no alloca
```

### Control Flow

**If with match arms** — equality-only arms compile to LLVM `switch`:

```
if (x) {
    = 1 =>
        return 10
    = 2 =>
        return 20
}
```

Comparison arms use `<`, `>`, `<=`, `>=`, `!=`:

```
if (x) {
    < 0  => return -1
    > 100 => return 1
}
```

**While** and **Loop**:

```
while (i < n) {
    i := i + 1
}

loop {
    // infinite loop
}
```

## Type System

| Rei     | LLVM   | Bits | Signed |
|---------|--------|------|--------|
| void    | void   | 0    | —      |
| bool    | i1     | 1    | —      |
| int8    | i8     | 8    | yes    |
| int16   | i16    | 16   | yes    |
| int32   | i32    | 32   | yes    |
| int64   | i64    | 64   | yes    |
| nat8    | i8     | 8    | no     |
| nat16   | i16    | 16   | no     |
| nat32   | i32    | 32   | no     |
| nat64   | i64    | 64   | no     |
| float32 | float  | 32   | yes    |
| float64 | double | 64   | yes    |

Pointer (`*T`), struct, enum, union, and session types are defined in the type
system but most are not yet wired into the parser or codegen.

## Build

Dependencies: C99/C11 compiler, [Meson](https://mesonbuild.com/), [Ninja](https://ninja-build.org/).
LLVM is **not** required at build time (the codegen emits IR text).

```
meson setup builddir
ninja -C builddir
```

Address/UB sanitizers (development only):

```
meson setup builddir -Dasan=enabled
ninja -C builddir
```

## Test

Tests live in `tests/` and cover all pipeline stages:

```
./builddir/tests/test_lexer
./builddir/tests/test_parser
./builddir/tests/test_sema
./builddir/tests/test_codegen
```

## What's Missing

- Module / multi-file support (currently a single embedded source string in `main.c`)
- Function calls
- Arrays, maps, slices
- Pointers and address-of / dereference syntax
- Enum and union types (defined but not parsed or codegen'd)
- Session types (defined but not implemented)
- For loops
- Real LLVM library binding (currently text emission)
- Standard library

## License

[GNU General Public License v3.0](LICENSE) or later.

Copyright (C) 2026  LLLichlet
