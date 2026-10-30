# 对象生命周期、存储期与初始化：学习指南

> 主线第 1/4 篇。排期：W1 第 1 篇，3h，标准档（正文 ≤ 100 行）。笔记正文由你自己写；这份指南只给承接关系、资料、过程和实验设计，实验的参考观察放在最后一节。

## 在主线上的位置

[RAII 笔记](../03-memory-resource/RAII_summary.md) → **① 构造（本篇）** → [② 移动](../03-memory-resource/copy-and-move-guide.md) → [③ 智能指针](../03-memory-resource/smart-pointers-guide.md) → [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)

### 承接：RAII 笔记用了、但没有展开的前提

[RAII 笔记](../03-memory-resource/RAII_summary.md)的 How ii 给出构造失败时的三条规则：非委托构造函数抛出时，对象自己的析构函数不会被调用；已经构造完成的基类和成员子对象按构造的逆序销毁；`new` 表达式里还会释放已分配的存储。这三条背后是同一个前提：**对象从初始化完成的那一刻才算存在。** 构造函数还没返回，对象就还不存在，所以没有析构；已经构造完的子对象是存在的，所以要销毁。RAII 笔记在使用这个前提，没有展开它。

[new-delete 笔记](../03-memory-resource/new-delete.md)把"对象"和"存储"分成两件事：`new` 表达式 = 分配存储 + 初始化对象。本篇补上缺的另一半：存储从哪来、能活多久（**存储期**）；对象何时开始、何时结束（**生命周期**）；**初始化**怎样把一块存储变成一个对象。

你已经在 [temp.md](../01-basic/temp.md) 写下了这篇的起点：初始化是比"调用构造函数"更广的过程，基类和成员子对象在构造函数体执行之前完成初始化，整个对象在非委托构造函数返回时初始化完成。本篇要把这段话变成一张能查、能验证的模型。

### 引出：用另一个对象来初始化新对象

初始化里有一类特殊的形式：用同类型的另一个对象初始化新对象——`T a = b;`、按值传参、按值返回，走的都是拷贝构造或移动构造。当对象是一份资源的管理者时，这一步决定资源怎么办：复制、转移，还是禁止。这是下一篇 [② 移动](../03-memory-resource/copy-and-move-guide.md)的问题。

## 读完应能回答

1. 对象的生命周期从哪一刻开始、到哪一刻结束？它和存储期（自动、静态、线程、动态）是一回事吗？存储为什么能先于对象存在、晚于对象结束？
2. `T x;`、`T x{};`、`T x = T();`、`new T`、`new T()` 分别是哪种初始化？对内置类型、对带默认成员初始化器的类、对有用户提供构造函数的类，各自得到什么？
3. 构造一个派生类对象时，基类、成员、构造函数体的先后顺序由什么决定？成员的顺序由声明顺序还是初始化列表顺序决定？销毁的顺序呢？
4. 全局、静态局部、自动、动态四种存储期的对象，各在什么时候构造、什么时候销毁？

## 本篇不写什么

- 构造失败时的清理：RAII 笔记 How ii 和 new-delete 笔记已经写了，这里只放链接。
- placement new 里"存储先于对象、晚于对象"的时间差：new-delete 的 Example 在验证，不重复。
- 继承体系里的构造细节（虚基类、构造期间调用虚函数）：留给 W4 的继承与多态。
- 拷贝构造、移动构造本身的规则：下一篇。

## 资料

### 必读（第 2 步，共约 45 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| cppreference：[Initialization](https://en.cppreference.com/w/cpp/language/initialization) | 开头的语法总览；Default initialization、Value initialization、Zero initialization 三节；Non-local variables 里静态初始化与动态初始化的区分 | 第 2、4 问 | 20 min |
| cppreference：[Constructors and member initializer lists](https://en.cppreference.com/w/cpp/language/constructor) | Initialization order 一节（基类、成员、构造函数体的顺序）；顺带扫一眼 Delegating constructor | 第 3 问 | 10 min |
| cppreference：[Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)、[Lifetime](https://en.cppreference.com/w/cpp/language/lifetime) | 前者读四种存储期的定义；后者读对象生命周期的开始与结束，跳过 linkage 和 implicit-lifetime 类型 | 第 1、4 问 | 15 min |

### 写完后对照（第 4 步，不占读的时间）

- C++ Primer 5e：2.2.1（初始化、列表初始化、默认初始化）、7.5.1（构造函数初始值列表，含成员初始化顺序）。
- *Effective Modern C++* Item 7：`()` 与 `{}` 的区别——窄化转换、最令人烦恼的解析、`initializer_list` 构造函数优先。
- C++ Core Guidelines：C.41、C.45、C.47、C.48、C.49、ES.20、ES.23，每条读标题和 Reason。

### 备查（不读）

- 标准：[basic.life](https://eel.is/c++draft/basic.life)、[basic.stc](https://eel.is/c++draft/basic.stc)、[dcl.init](https://eel.is/c++draft/dcl.init)、[class.base.init](https://eel.is/c++draft/class.base.init)。
- Nicolai Josuttis，*The Nightmare of Initialization in C++*（CppCon 2018）。
- C++ FAQ 里的 "static initialization order fiasco"：跨翻译单元的静态初始化顺序没有保证。

## 学习过程

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 1 先写 | 合上资料，用自己的话写 Why / What 各 3–5 句：对象什么时候开始存在？初始化到底是什么？起点是 temp.md 那段话。写不出来的地方，就是第 2 步要读的地方 | 0.25 | 两小段话，加一串"不确定"的问题 |
| 2 读 | 只读上面的"必读"三项；边读边填一张表：存储期（自动 / 静态 / 线程 / 动态）× 何时初始化 × 何时销毁 | 0.75 | 填好的表 |
| 3 动手 | 做下面两个实验，每个先写预测再跑 | 0.75 | 两份预测，两份实际输出 |
| 4 写 | How：初始化的几种形式（各配一个小例子）→ 构造与销毁的顺序规则（用实验 1 的输出）→ 存储期表。Relation：链到 RAII How ii、new-delete、下一篇。Example：用实验 1，Verifies 对应三条结论 | 1 | 笔记初稿 |
| 5 封板 | 对照下面的四条合格线自检；周五审一次，只修 P0/P1；commit | 0.25 | 封板 |

## 实验

### 实验 1：构造与销毁的顺序

要写的程序：

- 一个会打印的 `Tracer`：构造、析构各打印一行，带名字；一个同样会打印的基类 `Base`。
- 派生类 `D`：两个 `Tracer` 成员，**声明顺序是 `a`、`b`，构造函数的初始化列表却写成 `b`、`a`**；构造函数体和析构函数体各打印一行。
- 一个全局的 `Tracer`；一个函数，里面有静态局部 `Tracer`，`main` 里调用它两次；一个块作用域里的 `D`；一个 `new` 出来再 `delete` 的 `Tracer`。

先预测（写在代码旁边的注释里）：

1. `D` 构造时，`Base`、`a`、`b`、构造函数体谁先谁后？销毁的顺序呢？
2. 编译器会不会提醒你初始化列表的顺序有问题？
3. 全局对象在 `main` 之前还是之后构造？静态局部对象呢？函数调用两次，它构造几次？
4. `main` 结束后，全局对象和静态局部对象谁先销毁？

```bash
g++ -std=c++11 -Wall -Wextra exp1.cpp -o exp1 && ./exp1
```

它验证：成员按声明顺序初始化，和初始化列表的书写顺序无关；销毁与构造相反；静态存储期对象的构造与销毁时机。

### 实验 2：几种初始化各得到什么

要写的程序：

- `struct P { int x; };`、`struct R { int x = 7; int y; };`，一个静态 `int`，一个静态 `P`。
- 依次做 `new P()`、`new P{}`、`R r{}`、`new R`、`new R()`，打印**能读的**成员。

**注意**：默认初始化的内置类型成员，值是不确定的，读它是未定义行为。`new R` 的 `y`、`new P` 的 `x` 都不要打印。

先预测：每一行打印什么？为什么 `new R` 只能打印 `x`？分别用 `-std=c++11` 和 `-std=c++17` 编译，输出一样吗？

它验证：静态存储期的对象先被零初始化；值初始化（`()` 或 `{}`）把没有初始化器的成员清零；有默认成员初始化器的成员总是用它。

### 复用：对象与存储的时间差

placement new 里"存储先于对象存在、晚于对象结束"，new-delete 的 Example（`test_new_delete.cpp`）已经在验证。写 How 时直接引用那个 Example，不另写实验。

## 封板自检

- 能解释：不看资料，用两分钟讲清"生命周期和存储期的区别"，以及"一个派生类对象是怎样按顺序构造出来的"。
- 有模型：那张"存储期 × 构造时机 × 销毁时机"的表，加一条顺序规则：基类 → 成员（声明顺序）→ 构造函数体，销毁时相反。
- 有联系：Relation 链到 RAII How ii（构造失败）、new-delete（存储与对象）和下一篇；不重复它们的内容。
- 能验证：实验 1 的输出贴进 Example，Verifies 每条对应一个结论。

## 跑完之后再读

以下是 g++ 11.4、`-std=c++11 -Wall -Wextra` 的实际结果，先自己跑、先写预测，再对照。

### 实验 1 的结果

编译器的提醒：

```text
warning: ‘D::b’ will be initialized after [-Wreorder]
warning:   ‘Tracer D::a’ [-Wreorder]
warning:   when initialized here [-Wreorder]
```

运行输出：

```text
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
```

怎么读：

- `ctor global` 出现在 `main start` 之前：全局对象在 `main` 之前构造。
- `D` 的构造顺序是基类 → `a` → `b` → 构造函数体，初始化列表写的是 `b`、`a`，编译器用 `-Wreorder` 提醒这两者不一致；销毁完全相反。
- 静态局部对象在第一次调用 `f` 时构造，只构造一次。
- `main` 返回之后，销毁顺序是构造完成顺序的逆序：静态局部对象先于全局对象。

### 实验 2 的结果

`-std=c++11` 与 `-std=c++17` 输出相同：

```text
static int / static P.x: 0 0
new P(): x=0   new P{}: x=0
R r1{}:  x=7 y=0
new R:   x=7 (y indeterminate, not read)
new R(): x=7 y=0
```

怎么读：

- 静态存储期的对象在任何其他初始化之前先被零初始化，所以是 0。
- `new P()` 和 `new P{}` 都是值初始化，`P` 没有用户提供的构造函数，所以成员被清零。
- `R` 的 `x` 有默认成员初始化器，三种写法下都是 7；`y` 没有，值初始化（`R r1{}`、`new R()`）把它清零，默认初始化（`new R`）不管它，所以只有这一行不能读 `y`。
