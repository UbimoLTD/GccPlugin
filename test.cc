#include <stdio.h>

struct A { int x; int y; int z; int a; int b[100]; A(int x, int y, int z, int a) : x(x), y(y), z(z), a(a) { for (int i = 0; i < 100; ++i) b[i] = i; } /*~A() { printf("hello\n"); } */ };

struct [[warn_unused_result_type]] B {
  B(int x) : x(x) { printf("Constructing B\n"); }
  ~B() { printf("Destroying B\n"); }
  int x;
};

__attribute__((warn_unused_result)) A f();
A f() {
  return A(1,2,3,4);
}

A *g() [[warn_unused_result]] {
  return new A(1,2,3,4);
}

extern A x;

int main() {
  f();
  g();
}

