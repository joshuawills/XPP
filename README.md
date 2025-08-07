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

```Rust
extern printf(i8*, ...) i32;

fn len(str: i8*) i64 {
	let mut p: i8* = str;
	let mut count = 0;
	while *p != '\0' {
		count = count + 1;
		p = p + 1;
	}
	return count;
}

fn main() void {
	let mut my_str = "hello";
	printf("%d\n", len(my_str));

	let ref_one = &my_str;
	let ref_two = &my_str;
	if ref_one == ref_two {
		printf("The refs are equal!");
	} else {
		printf("The refs are not equal!");
	}

}

```