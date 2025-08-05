# X++ Programming Language

Ideally: 

```Rust
using io;

fn print_t<T>(v: mut T&) void {
    println(v);
}

fn main() void {
    print_t(21 as u8);
}
```

Currently:

```Rust
extern printf(i8*, ...) i32;

fn main() void {
    let x = 1;
	printf("my favourite number is: %d\n", x);
}
```