---
title: 'Array List'
weight: 1
---

```Rust
// Simple array list class implementation

pub extern printf(i8*, ...) i32;
extern malloc(i64) void*;
extern calloc(i64, i64) void*;
extern free(void*) void;
extern realloc(void*, i64) void*;

let SECTION_SIZE: i64 = 100;

pub class IntList {
    mut arr: i64*;
    pub mut len: i64;
    mut section_size: i64;

    pub IntList() {
        len = 0;
        section_size = SECTION_SIZE;
        arr = calloc(section_size, size_of(i64));
    }

    pub mut fn append(val: i64) void {
        if len != 0 and len % section_size == 0 {
            let num_iterations = len / section_size;
            let size: i64 = (num_iterations + 1) * section_size * size_of(i64);
            arr = realloc(arr, size);
        }
        arr[len++] = val;
    }

    pub fn print() void {
        loop i in len {
            printf("%d\n", arr[i]);
        }
    }

    pub mut fn free() void {
        free(arr);
    }

}

fn take_in_int_list(x: IntList) void {
	x.print();
}

fn main() void {
    let mut list = IntList();

    loop i in 500 {
        list.append(i);
    }

    take_in_int_list(list);

    list.free();

}
```