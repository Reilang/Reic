Rei is a simple but powerful system programming language.

```
fn main(a: int32, b: int32) -> int32 {
    var x: int32 := 1
    var y := x + 2
    x := x + y

    if (x) {
        = 10 =>
            var z := 42
            x := z
        = 20 =>
            return y
    }

    return x
}
```

## Build

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
