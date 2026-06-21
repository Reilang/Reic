# ReiC

ReiC is the reference compiler for the [Rei](https://codeberg.org/reilang) language,
compiling Rei source code into native executables.

> **Status: conceptual design phase.** The project is in very early development.
> Syntax, semantics, and the compilation pipeline are all subject to significant
> change. Not yet suitable for production or educational use.

## Pipeline Overview

```
Source  ->  Lexer  ->  Parser  ->  AST  ->  HIR Lower  ->  HIR  ->  Sema  ->  Codegen (LLVM)
```

- **Lexer** — state-machine-driven tokenizer, emits a token stream
- **Parser** — state-stack-driven recursive-descent parser, emits a flat AST
- **HIR Lower** — translates AST into a high-level intermediate representation
- **Sema** — semantic analysis: symbol resolution and type checking
- **Codegen** — translates HIR into LLVM IR for native code generation

## Syntax Example

```
fn main(a: int32, b: int32) -> int32 {
    var x: int32 := 1
    var y := x + 2
    x := x + y

    if (x) {
        = 10 =>
            var z := 42
            x := z
        < 20 =>
            return y
    }

    return x
}
```

## Build

Dependencies: C11 compiler, [Meson](https://mesonbuild.com/), [Ninja](https://ninja-build.org/), [LLVM](https://llvm.org/).

```
meson setup builddir
ninja -C builddir
```

## Type System

| Rei    | LLVM | Bits | Signed |
|--------|------|------|--------|
| void   | void | 0    | -      |
| int8   | i8   | 8    | yes    |
| int16  | i16  | 16   | yes    |
| int32  | i32  | 32   | yes    |
| int64  | i64  | 64   | yes    |
| nat8   | i8   | 8    | no     |
| nat16  | i16  | 16   | no     |
| nat32  | i32  | 32   | no     |
| nat64  | i64  | 64   | no     |

## License

[GNU General Public License v3.0](LICENSE) or later.

Copyright (C) 2026  LLLichlet
