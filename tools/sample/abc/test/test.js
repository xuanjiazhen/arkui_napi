export class A {
  static x = 100
  //static hack_x = 200
  y = 100
  hack_y = 200
  foo() {
    print(this.x)
  }
  //hack_foo() {}
  static s_foo() {
    print(this.x)
  }
  //static hack_s_foo() {}
}

export function bar() {}

export let dd = 100
