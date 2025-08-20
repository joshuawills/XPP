+++
title = 'X and X++ Programming Languages'
date = 2024-08-14T11:49:05+10:00
+++

# X++ Programming Language

```Rust
extern printf(i8*, ...) i32;

fn main() void {
    printf("hello world!\n");
}
```

X++ is a simple low-level programming language ideal for *type-safety*, *fast-runtimes* and an 
*opinionated compiler*. The compiler itself is written in C++, and generates the intermediate representation
for LLVM, enabling the binary output to achieve similar performance to C++ and Rust. This also has the 
added benefit of making the compiler cross-platform.

To get started with X++, see the [getting started](/X++-Lang/X++-Docs/getting-started/) information here.

For a look at the formal CFG of the language, see the [grammar page](/X++-Lang/X++-Docs/grammar)


# X Programming Language

**The 'X Programming Language is no longer under development but the docs remain online. Use of X++ is suggested as an alternative.**

```Rust
using io;

fn main() -> void {
    println("hello, world!");
}
```

X is a simple low-level programming language ideal for *type-safety*, *fast-runtimes* and an 
*opinionated compiler*. The compiler itself runs on the JVM, and generates the intermediate representation
for LLVM, enabling the binary output to achieve similar performance to C++ and Rust. This also has the 
added benefit of making the compiler cross-platform.

To get started with X, see the [getting started](/X-Lang/X-Docs/getting-started/) information here.

For a look at the formal CFG of the language, see the [grammar page](/X-Lang/X-Docs/grammar)

For an intro to language documentation, check out the [language docs](/X-Lang/X-Docs/language/your-first-program)