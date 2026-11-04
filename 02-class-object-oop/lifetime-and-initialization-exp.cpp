// 指南第三步的三个实验。用 -DEXP=1、2、3 选一个编译，三个互不干扰；不写 -DEXP 就是实验 1：
//   g++ -std=c++11 -Wall -Wextra -DEXP=1 lifetime-and-initialization-exp.cpp -o /tmp/exp1 && /tmp/exp1
// 每个实验后面是“答案”。实际输出来自 g++ 11.4；clang++ 14，以及 -std=c++11 到 c++20，输出都一样。

#include <cstdio>

#ifndef EXP
#define EXP 1
#endif

#if EXP == 1
// ============================================================
// 实验 1：构造与销毁的顺序
// 编译时会有一条 -Wreorder 警告，这是故意的（问题 2）。
// ============================================================

struct Tracer {
    const char* n;
    explicit Tracer(const char* n_) : n(n_) { std::printf("  ctor %s\n", n); }
    ~Tracer() { std::printf("  dtor %s\n", n); }
};

struct Base {
    Base() { std::puts("  Base ctor"); }
    ~Base() { std::puts("  Base dtor"); }
};

struct D : Base {
    Tracer a;
    Tracer b;
    D() : b("b"), a("a") { std::puts("  D body"); }  // 声明顺序是 a、b，初始化列表写的是 b、a
    ~D() { std::puts("  D dtor body"); }
};

Tracer g("global");

void f() {
    static Tracer s("static local");
    std::puts("  f: after static local");
}

int main() {
    std::puts("main start");
    {
        std::puts("scope: D");
        D d;
    }
    f();
    f();
    Tracer* p = new Tracer("dynamic");
    delete p;
    std::puts("main end");
}

/* 答案

   1. 构造：Base → a → b → D 的构造函数体；销毁相反：D 的析构函数体 → b → a → Base。
      初始化列表里写的是 b、a，没有用，成员按类里的声明顺序（a、b）初始化。
   2. 会。-Wall 里的 -Wreorder：“‘D::b’ will be initialized after ‘Tracer D::a’”。
      clang++ 的措辞是 “field 'b' will be initialized after field 'a'”（-Wreorder-ctor）。
   3. 全局对象在 main 之前构造（“ctor global” 在 “main start” 前面）。
      静态局部对象在第一次执行到它的声明时构造，也就是第一次调用 f()；f() 调了两次，只构造一次。
   4. 静态局部对象先销毁，全局对象后销毁：静态存储期的对象按构造完成的逆序销毁，全局对象先构造完成。

   实际输出：
        ctor global
      main start
      scope: D
        Base ctor
        ctor a
        ctor b
        D body
        D dtor body
        dtor b
        dtor a
        Base dtor
        ctor static local
        f: after static local
        f: after static local
        ctor dynamic
        dtor dynamic
      main end
        dtor static local
        dtor global
*/

#elif EXP == 2
// ============================================================
// 实验 2：几种初始化各得到什么
// 只做这五种，不要为了覆盖更多情形再加测试。
// ============================================================

struct P { int x; };
struct R { int x = 7; int y; };

static int g_i;
static P g_p;

int main() {
    std::printf("static int / static P.x: %d %d\n", g_i, g_p.x);
    P* a = new P();
    P* b = new P{};
    std::printf("new P(): x=%d   new P{}: x=%d\n", a->x, b->x);
    R r1{};
    R* r2 = new R;
    R* r3 = new R();
    std::printf("R r1{}:  x=%d y=%d\n", r1.x, r1.y);
    std::printf("new R:   x=%d (y indeterminate, not read)\n", r2->x);
    std::printf("new R(): x=%d y=%d\n", r3->x, r3->y);
    delete a;
    delete b;
    delete r2;
    delete r3;
}

/* 答案

   实际输出：
      static int / static P.x: 0 0
      new P(): x=0   new P{}: x=0
      R r1{}:  x=7 y=0
      new R:   x=7 (y indeterminate, not read)
      new R(): x=7 y=0

   1. 静态 int、静态 P：静态存储期的对象在任何其他初始化之前先零初始化，所以是 0。
   2. new P()：值初始化。P 没有用户提供的构造函数，成员先被清零，所以 x=0。
      new P{}：P 是聚合，{} 走聚合初始化，x 从空列表初始化，也是 0。结果相同，种类不同。
   3. new R()：值初始化，同样先清零再默认初始化：x 用默认成员初始化器得 7，y 被清零得 0。
   4. R r1{}：x=7、y=0。C++11 里 R 不是聚合（有默认成员初始化器），{} 走值初始化，同第 3 条；
      C++14 起 R 是聚合，{} 走聚合初始化：x 用默认成员初始化器，y 从空列表初始化得 0。路径不同，结果相同。
   5. new R：默认初始化。x 有默认成员初始化器，总是用它，是 7；y 没有，值不确定，读它是未定义行为，
      所以只打印 x。
   6. -std=c++11 和 -std=c++17 的输出一样（原因见第 4 条）。
*/

#elif EXP == 3
// ============================================================
// 实验 3：作用域结束，对象会怎样
// ============================================================

struct Tracer {
    const char* n;
    Tracer(const char* s) : n(s) { std::printf("  ctor %s\n", n); }
    ~Tracer() { std::printf("  dtor %s\n", n); }
};

Tracer* leak;

int main() {
    std::puts("enter block");
    {
        Tracer a("auto");
        leak = new Tracer("dynamic");
        static Tracer s("static");
        std::puts("leave block");
    }
    std::puts("after block");
    std::printf("  dynamic object still alive: %s\n", leak->n);
    delete leak;
    std::puts("main end");
}

/* 答案

   1. 块结束时只有 auto 被销毁（“dtor auto” 紧跟在 “leave block” 后面）；dynamic 和 static 还活着。
   2. 安全。动态对象不随作用域结束而销毁，全局指针一直指着活着的对象，直到 delete。
   3. 静态局部对象的名字出了块就看不见了，对象却一直活到程序结束：main 返回之后才销毁
      （“dtor static” 在 “main end” 后面）。
   一句话：作用域结束只影响名字；只有自动对象的生命周期跟着作用域结束。

   实际输出：
      enter block
        ctor auto
        ctor dynamic
        ctor static
      leave block
        dtor auto
      after block
        dynamic object still alive: dynamic
        dtor dynamic
      main end
        dtor static
*/

#else
#error "用 -DEXP=1、-DEXP=2 或 -DEXP=3 选一个实验"
#endif
