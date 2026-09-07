export class A {
    static x = 100
    y = 100
    hack_y = 200
    foo() {
        print(this.x)
    }
    static s_foo() {
        print(this.x)
    }
}

export function bar() {}

export let dd = 100