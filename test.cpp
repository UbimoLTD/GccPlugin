#include <stdio.h>

struct __attribute__((warn_unused_result)) A;
struct [[gnu::ubimo_warn_unused_result_type]]  A { int x; int y; int z; int a; int b[100]; A(int x, int y, int z, int a) : x(x), y(y), z(z), a(a) { for (int i = 0; i < 100; ++i) b[i] = i; } /*~A() { printf("hello\n"); } */ };

struct [[gnu::ubimo_warn_unused_result_type]] B {
  B(int x) : x(x) { printf("Constructing B\n"); }
  ~B() { printf("Destroying B\n"); }
  int x;
};

A f() __attribute__((warn_unused_result));
A f() {
  return A(1,2,3,4);
}

__attribute__((warn_unused_result)) A *g();
A* g()  {
  return new A(1,2,3,4);
}

extern A x;

int main() {
  f();
  g();
  A moshe = f();
  A *moshe2 = g();
}
