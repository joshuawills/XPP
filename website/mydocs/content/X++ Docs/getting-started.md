---
title: "Getting Started"
weight: 1
---

*All development so far has been on Manjaro Linux. I cannot confirm yet whether or not this compiler
works correctly on other operating systems/environments*

## Pre-Requisites

- CMake (min version 3.1)
- LLVM 
- Git
- Clang (supporting C++ version 17>)

## Setup

Clone the `X++ Compiler` repository onto your local machine

```bash
# HTTPS
git clone https://github.com/joshuawills/X-PL-2.0.git

# SSH
git clone git@github.com:joshuawills/X-PL-2.0.git
```

Build the project using `cmake -B build; cd build; make`

You should now have an executable located at `build/compiler`

I recommend invoking the program with the `-h` flag to see all the options you can run the compiler
with. For a simple example, write the following content to a file called `hello.xpp`

```Rust
extern printf(i8*, ...) i32;

fn main() void {
    printf("hello, world!\n");
}
```

Compile the program with the command `./compiler hello.xpp -o hello` and run the executable `hello` that is 
generated. Alternately, run the compiler with the additional `-r` flag to build and run the program
in succession.