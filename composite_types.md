# Hypothetical implementation for composite types

```Rust
using str;

class IntBox {
    mut val: i64;

    pub IntBox() {
        this.val = 0;
    }

    pub IntBox(v: i64) {
        this.val = v;
    }

    pub fn get_val() i64 { return val; }
    // pub mut fn set_val(v: i64) void { this.val = v; }
}

class Person {
    name: str;
    mut age: u8;
    pub mut likes_chicken: bool;

    pub Person() {
        this.name = str();
        this.age = 0u;
        this.likes_chicken = false;
    }

    pub Person(name_: str&, age_: u8) {
        this.name = str(name_);
        this.age = age_;
    }

    static fn get_bob() Person {
        let n = str("Bob James");
        return Person(n, 21u);
    }
}

```