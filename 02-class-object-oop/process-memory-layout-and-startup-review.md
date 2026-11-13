# 初始化，与程序的内存分布、启动过程

> 整理于 2026-11-09，2026-11-11 重新整理。环境：Ubuntu 22.04（WSL2）、x86-64、g++ 11.4、glibc 2.35、binutils 2.38、gdb 12.1；C++ 标准以 C++20 为基线，版本差异单独写明。

## 1. 先看全貌

### 五个问题

| 问题 | 答案 | 在哪 |
|---|---|---|
| 初始化器怎么写，各算哪种初始化？ | 不写→默认，空 `()`→值，`= 表达式`→拷贝，`(实参)`→直接，`{ }`→列表 | §2.2 |
| 分哪两个阶段？ | 先静态初始化，再动态初始化 | §2.3 |
| 多个非局部变量谁先谁后？ | 按变量种类分三档 | §2.3.1 |
| 非局部变量何时初始化？ | 静态存储期的通常在 `main` 之前；标准允许提前（当作静态初始化来做）或推迟（到 `main` 之后），推迟的期限由 ODR-use 定。线程存储期的每个线程各一次，时机见 §2.3.2 | §2.3.2、§2.3.3 |
| 静态局部变量（函数内的 `static`、`thread_local`）呢？ | 零初始化、常量初始化仍在静态阶段；动态初始化等执行流第一次经过声明才做。抛异常下次重试，多线程同时经过只初始化一次 | §2.4、§2.5 |

### 一条时间线

```text
装载程序 → 静态初始化 → 动态初始化（通常在 main 之前，标准允许推迟，§2.3.2） → main → 退出时析构 → 内核回收
```

这是典型实现里**非局部的静态存储期**变量的顺序，其中动态初始化与 `main` 的先后不是语言保证的（§2.3.2）。线程存储期（`thread_local`）的非局部变量不在这条线上：每个线程各自初始化（时机见 §2.3.2）、各自销毁（§4.3）。静态局部变量也不在这条线上：它们的动态初始化要等执行流第一次经过声明（§2.4）。

标准保证：**所有静态初始化，都先于任何动态初始化。**

## 2. 语言层面：初始化

### 2.1 讨论对象

从 §2.3 起，讨论的是**静态存储期**或**线程存储期**（`thread_local`）的变量，按声明的位置分两类。

**非局部变量**（§2.3）：

- 命名空间作用域的变量（含全局变量）；
- 类的静态数据成员。

它们的存储期是**静态**的，或者是**线程**的（`thread_local`）。

**函数内的变量**（§2.4）：函数等块作用域里加了 `static`（静态存储期）或 `thread_local`（线程存储期）的变量；不加的是自动存储期，不在讨论范围内。

> 两个维度互相独立：**非局部/局部**说的是变量写在哪，**静态/线程存储期**说的是变量活多久。

### 2.2 初始化器的写法与初始化种类

初始化是对象创建时确定它初始状态的过程：按类型和写法，可能调用构造函数，也可能什么都不做（如局部的 `int x;`）。本节对所有变量通用，不限于静态或线程存储期的变量；从 §2.3 起才收窄到它们（§2.1）。

**初始化发生在哪**

- 声明符后面的**初始化器**，以及 `new` 表达式里的初始化器。
- 函数调用时，**形参**和**返回值**也会被初始化，它们按拷贝初始化处理（附录 B 的 [dcl.init]/1、/15）。

**初始化器的四种写法**

| 编号 | 写法 | 例子 | 版本 |
|---|---|---|---|
| ① | `= 表达式` | `int a = 1;` | |
| ② | `= { ... }` | `int d = {4};` | |
| ③ | `( 表达式列表 )` | `int b(2);` | |
| ④ | `{ ... }` | `int c{3};` | C++11 起 |

- 指派初始化：花括号里可以整个写成指派列表，如 `P q{.x = 5, .y = 6};`（`P` 是有成员 `x`、`y` 的聚合），C++20 起。
- C++11 之前没有 ④；② 的含义是：T 是聚合 → 聚合初始化；T 是标量类型 → `T x = {a};` 等价于 `T x = a;`；其他类型非法。C++11 起 ② 和 ④ 一起归入列表初始化（T 是聚合时，列表初始化走聚合初始化）。前半按 cppreference 的 until C++11 标注写，C++03 原文我没有核对。
- ③ 圆括号里的每一项，C++11 起也可以是花括号列表，如 `std::pair<int, std::vector<int>> pr(1, {2, 3});`；C++11 之前只能是表达式。
- 聚合大致是数组或简单的 struct，精确定义随版本变（见 guide）；标量类型是算术类型、枚举、指针等。

**花括号里装什么**

- 花括号里是用逗号隔开的**初始化子句**。一个子句可以是：表达式、`{}`、或另一个花括号列表（含 C++20 的指派列表），所以能嵌套：`int m[2][2] = {{1, 2}, {3, 4}};`。
- 这几种花括号写法合称**花括号初始化列表**（标准里叫 braced-init-list）。语法里的 initializer-list 是语法成分，不是标准库的 `std::initializer_list`。
- 子句里的表达式不能是没加括号的逗号表达式，逗号在这里是分隔符；要用逗号运算符就加括号：`int t = (1, 2);`，`t` 是 2。

**写法决定种类**

| 声明里怎么写 | 初始化的种类 |
|---|---|
| 不写初始化器 | 默认初始化 |
| 空的 `()`, `{}`(满足条件的) | 值初始化 |
| ① `= 表达式` | 拷贝初始化 |
| ③ `( 表达式列表 )` | 直接初始化 |
| ② ④ 花括号 | 列表初始化（C++11 起） |

- `()` 在变量声明里写不出来：`T x();` 会被当成函数声明。它只出现在 `new T()`、`T()`（函数式转换）、成员初始化器里（[dcl.init]/11）。
- 花括号写法在标准里还分两种：`T x{...}` 叫直接列表初始化，同时属于直接初始化；`T x = {...}` 叫拷贝列表初始化，同时属于拷贝初始化（[dcl.init.list]/1）。
- 被初始化的是引用时，写了初始化器的按"引用初始化"的规则处理，本文不展开；引用的定义必须有初始化器（`extern` 声明、类成员声明、形参、返回类型除外，[dcl.ref]/5）；不写初始化器（默认初始化）或写 `()`（值初始化），对引用都是非法的（[dcl.init]/9）。
- 拷贝、直接、列表这三种，以及各写法对聚合与非聚合类的差别，见 guide 第 3 问的表，这里不展开；默认、值、零三种放在下面。

**默认、值、零：各做什么**

| 种类 | 什么时候发生 | 做什么（按被初始化对象的类型 T） |
|---|---|---|
| 默认初始化 | 没写初始化器 | 类类型：调用选中的默认构造函数；数组：逐元素默认初始化；其他类型：**什么也不做** |
| 值初始化 | 初始化器是 `()`；花括号为空时：T 是聚合走聚合初始化，T 是有默认构造函数的非聚合类或标量才走值初始化 | 标量：零初始化；类类型：见下；数组：逐元素值初始化 |
| 零初始化 | 没有对应的写法，由别的初始化触发 | 标量：变成 0；类类型：每个非静态成员、基类子对象逐个零初始化，填充位置零；联合体：第一个具名成员；数组：逐元素；引用：什么也不做 |

类类型的值初始化，看默认构造函数是不是 user-provided。user-provided 的定义（[dcl.fct.def.default]/5）：用户声明的函数，且没有在**首次声明**时就写成 `= default` 或 `= delete`。所以隐式生成的、类内首次声明就写 `= default` 的都不算；类内只声明、类外才写 `T::T() = default;` 的算：

- 不是 user-provided（也没被删除）：先把整个对象零初始化，再（若默认构造函数不是平凡的）默认初始化。
- 是 user-provided、被删除、或根本没有默认构造函数：只做默认初始化。所以用户的构造函数没碰的成员不会被清零；没有默认构造函数或它被删除时，编译不过。

三者的关系：

- 值初始化是用另外两种拼出来的：标量直接零初始化；类类型按上面两条，默认构造函数不是 user-provided 时零初始化再加默认初始化，是 user-provided 时只做默认初始化。所以 `T()` 比不写初始化器多"清零"这一步，只对标量和默认构造函数不是 user-provided 的类成立；默认构造函数是 user-provided 时两者没有区别，都只调用它（下面例子里的 `U`）。
- 零初始化自己没有写法，主要在两处被用到：值初始化，以及静态/线程存储期变量没做常量初始化时（§2.3）。
- 默认初始化对标量什么也不做，所以局部的 `int x;` 值不确定；全局的 `int g;` 是 0，靠的是零初始化。

```cpp
struct P { int x; };           // 默认构造函数是隐式生成的，不是 user-provided
struct U { int x; U() {} };    // 默认构造函数是 user-provided，没碰 x

int g;                         // 静态存储期：先零初始化；默认初始化对 int 什么也不做 → 0
U gu;                          // 先零初始化，再调用 U()；U() 没碰 x → x 仍是 0

int main() {
    int* a = new int();        // 值初始化 → 0
    P*   b = new P();          // 值初始化：P 的默认构造函数不是 user-provided，先清零 → 0
    int* c = new int;          // 默认初始化 → 值不确定，不能读
    U*   d = new U();          // 值初始化：U() 是 user-provided，只调用它 → x 值不确定，不能读
}
```

标准保证 `g`、`gu.x`、`*a`、`b->x` 都是 0（g++ 11.4 和 clang 14，`-std=c++11` 与 `-std=c++20` 下实测都打印 0）；`*c` 和 `d->x` 不能读。

**和 §2.3 的阶段是两个独立的维度**

上表按**怎么写**分种类；§2.3 按**是否满足常量初始化的条件**分静态、动态阶段（零初始化也算静态）。同一个变量，两边各有一个答案：

| 全局声明 | 写法决定的种类 | 所处阶段（§2.3） |
|---|---|---|
| `int a = 10;` | 拷贝初始化 | 常量初始化 |
| `int b;` | 默认初始化 | 零初始化（没有动态初始化） |
| `int c = f();` | 拷贝初始化 | 先零初始化，再动态初始化 |
| `U gu;`（上面的 `U`，构造函数不是 `constexpr`） | 默认初始化 | 先零初始化，再动态初始化（调用 `U()`） |
| `C gc;`（`struct C { int x; constexpr C() : x(10) {} };`） | 默认初始化 | 常量初始化 |

- 全局的 `int b;` 是 0，**不是默认初始化给的**。按写法，它是默认初始化，而默认初始化对 `int` 什么也不做（[dcl.init]/7），没有运行期动作。这个 0 来自阶段那一边：`b` 是静态存储期的变量，又不满足常量初始化的条件（没有初始化器，默认初始化什么也不做），所以做零初始化（[basic.start.static]/2），它属于静态初始化、先于任何其他初始化（[dcl.init]/10 的注），没有动态初始化。典型实现里它只是 `.bss` 中一块全零内存，没有初始化代码（§3.3）。局部的 `int x;`（自动存储期）没有零初始化这一步，值不确定（[basic.indet]/1）。

### 2.3 非局部变量（Non-local variables）

非局部变量（§2.1）的初始化分两个阶段，先静态初始化，再动态初始化。这两个阶段的定义对静态局部变量（§2.4）也一样，这里先以非局部变量讲清楚。

**阶段 1：静态初始化**

对静态或线程存储期的变量：满足 constant-initialized 的条件就做**常量初始化**，否则做**零初始化**。两者合称静态初始化，其余的初始化都是动态初始化（附录 B 的 [basic.start.static]/2）。

| 情形 | 做法 |
|---|---|
| 满足 constant-initialized 的条件 | **常量初始化** |
| 其他 | **零初始化** |

- constant-initialized 的条件（附录 B 的 [expr.const]）：变量有初始化器（或它的默认初始化会执行某些初始化），并且整个初始化是常量表达式。常见情形：`int a = 10;` 满足；`int b;` 没有初始化器，对 `int` 的默认初始化什么也不做，所以不满足，做零初始化；没有初始化器、但默认构造函数是 `constexpr` 的类对象（§2.2 表里的 `gc`）也满足：它的默认初始化会执行初始化，且整个是常量表达式。
- 零初始化的结果按类型看（§2.2 的表，[dcl.init]/6）：标量变成 0，**引用不做任何事**。需要动态初始化的变量不满足常量初始化，所以也先被零初始化，再轮到动态初始化（实现若把它提前成静态初始化，§2.3.2，一开始就是最终值）。
- 所有静态初始化先于任何动态初始化。实现上它通常在编译期算好、随程序映像装载，不需要运行初始化代码；但标准保证的只是"先于动态初始化"，没有保证"绝不运行代码"（[expr.const] 的注里有"可能在运行期求值"的例子，见附录 B）。

**阶段 2：动态初始化**

其余的初始化。通常是初始化器要调用普通函数、非 constexpr 的构造函数等，需要运行代码。

```cpp
int a = 10;          // 常量初始化
int b;               // 零初始化
int c = f();         // 先零初始化为 0，再动态初始化为 f()
```

静态局部变量的动态初始化另有规则，见 §2.4。

#### 2.3.1 谁先谁后：动态初始化的三档排序

三档说的是**排序规则**，不是三种初始化操作。它们只针对**静态存储期**的非局部变量（[basic.start.dynamic]/1、/3 的原文就是这样限定的）。

| 档位 | 适用对象（均为静态存储期） | 顺序保证 |
|---|---|---|
| **有序** | 其他所有非局部变量（最常见） | 同一翻译单元内按**定义顺序**；跨翻译单元**不确定** |
| **部分有序**（C++17） | inline 变量（模板实例化的除外） | 若在每个定义了 W 的翻译单元里，V 都定义在 W 之前，则 V 先于 W；不满足条件就没有保证，所以叫"部分" |
| **无序** | 类模板的静态数据成员、变量模板（C++14）的实例化；显式特化除外（它有序） | 没有常规的顺序保证 |

**跨翻译单元：静态初始化顺序问题**

```cpp
// a.cpp
extern B b;
A a;            // 构造函数里用了 b

// b.cpp
B b;
```

`a` 和 `b` 在不同翻译单元，不能保证 `b` 先完成动态初始化，`a` 的构造函数可能读到只被零初始化、构造函数还没跑的 `b`。所以要避免跨翻译单元的全局对象互相依赖；常见的办法是改成静态局部变量（函数内的 `static`），用到时才构造（§2.4）。

#### 2.3.2 何时初始化：默认、提前、推迟

**默认**

| 变量 | 时机 |
|---|---|
| 静态存储期 | 程序启动时，通常在 `main` 之前 |
| 线程存储期 | 线程启动时；实现也可以推迟，最晚到本线程第一次用到同一翻译单元里带动态初始化的 `thread_local` 变量（见下面的推迟表）。每个线程各一次 |

`thread_local` 不是"程序启动时统一初始化"：带动态初始化的 `thread_local`，g++ 11.4 选的是**每个线程第一次使用时**才构造（实测，实验 3；标准原文见附录 B 的 [basic.start.dynamic]/7）。

**提前：Early dynamic initialization**

满足下面两个条件，编译器可以把本该动态的初始化**提前做成静态**（附录 B 的 [basic.start.static]/3）：

- 动态版本的初始化，不会改变其他静态或线程存储期的**对象**在它们自己初始化之前的值；
- 得到的值与动态初始化的结果相同。

```cpp
inline double fd() { return 1.0; }
extern double d1;
double d2 = d1;
double d1 = fd();
```

- 只有一种组合让 `d2` 得到 `1.0`：`d1` 被提前成静态初始化，而 `d2` 仍是动态初始化。
- 其余组合 `d2` 都是 `0.0`：两者都动态（`d2` 初始化时 `d1` 还只是被零初始化的 0）；或者 `d2` 自己被提前成静态。
- 所以 `d2` 是 `0.0` 还是 `1.0`，标准称 unspecified（两种结果都允许，实现不必说明选了哪种），不要依赖。

**推迟：Deferred dynamic initialization**

动态初始化也可以推迟到 `main`（或线程函数）的第一条语句之后，这一点由实现定义（附录 B 的 [basic.start.dynamic]/5）。推迟时有一个**最晚期限**，用"非初始化引起的 ODR-use"（non-initialization odr-use）来定，含义见 §2.3.3：

| 变量 | 必须在什么之前完成 |
|---|---|
| 非 inline、静态存储期 | 同一翻译单元里，**任何非 inline 函数或非 inline 变量**的第一次非初始化 ODR-use |
| inline、静态存储期（C++17） | **它自己**的第一次非初始化 ODR-use |
| 非 inline、线程存储期 | 本线程对同一翻译单元里**任何非 inline、有动态初始化的线程存储期变量**的第一次非初始化 ODR-use†（只数变量，不数函数） |


#### 2.3.3 ODR-use：推迟初始化的触发条件

§2.3.2 的期限是用 ODR-use 定的，先弄清它是什么，再看它怎么起作用。条文对照 C++20 草案（N4861）核对过，摘录见附录 B。

**ODR-use 是什么**

ODR 是 One Definition Rule（单一定义规则）。**ODR-use** 是其中的一个概念：对一个实体的某种使用，按语言规则要求这个实体必须有定义。不是"写了名字"就算；也和优化后有没有真的给它分配存储无关。一旦 ODR-use，程序里就必须有这个实体的定义（非 inline 的必须有且只有一个，[basic.def.odr]/10）。

| 实体 | 典型的 ODR-use 情形（不是完整的判定规则，正式规则见 [basic.def.odr]/4、/7） |
|---|---|
| 对象 | 读值、改值、取地址、绑定引用 |
| 函数 | 调用、取地址 |

有两类**不算**：

- 出现在**不求值语境**里的名字：`sizeof(x)`、`decltype(x)`。
- **只用到编译期常量值**的常量对象：`constexpr int n = 10; int x = n;` 不算（直接用常量 10 就行）；`&n` 算（取地址，用到的是这个对象本身，而不是它的值）。

```cpp
int global = 10;
constexpr int n = 10;

void use() {
    int a = global;           // ODR-use：读取
    int* p = &global;         // ODR-use：取地址
    int& r = global;          // ODR-use：绑定引用
    auto s = sizeof(global);  // 不算：不求值语境
    int x = n;                // 不算：只用常量值
    auto q = &n;              // ODR-use：取地址，用到对象本身
}
```

**为什么拿它当期限**

- C++ 要保证：**程序真正用到一个对象之前，它已经初始化完成**。
- 推迟只是允许把初始化放到 `main` 之后，**不是**允许晚于使用。
- ODR-use 正是"这次使用要求实体有定义"的概念，所以拿它当"最晚期限"。

**非 inline 变量：期限覆盖整个翻译单元**

```cpp
// service.cpp
int get_value() { return 100; }
int global = get_value();
void run() { std::cout << "run()\n"; }

// main.cpp
void run();
int main() { run(); }
```

假设 `global` 的动态初始化被推迟：

- `main` 没碰 `global`，只调用了 `run()`。
- 调用 `run()` 就是对它的 ODR-use；`run()` 是 `service.cpp` 里的非 inline 函数，与 `global` 在同一翻译单元。
- 规则看的是"同一翻译单元里的非 inline 函数或变量"，不要求使用 `global` 本身。所以在 `run()` 被调用之前，`global` 必须已经初始化为 100。

> cppreference 页面上这句写的是"任何静态/线程存储期的**变量**"，没有函数（2026-11-12 查看）；标准条文是"非 inline 函数或非 inline 变量"（附录 B 的 /5）。以标准为准，所以调用 `run()` 就能触发。

再在 `service.cpp` 里加一个没人用、构造函数有输出的对象：

```cpp
struct Logger { Logger() { std::cout << "Logger constructed\n"; } };
Logger logger;   // 有副作用的动态初始化
```

`main` 同样只调用 `run()`，`logger` 也会被构造。这里有两层依据，别混成一条触发规则：

- **什么时候**：触发规则本身就是翻译单元范围的（§2.3.2 的表），所以这个翻译单元里所有被推迟的非 inline 变量，最晚在 `run()` 被 ODR-use 之前初始化。
- **会不会被省掉**：这是标准脚注的补充保证，不是另一种触发条件：推迟时，静态存储期变量的初始化有副作用，即使它自己没被 ODR-use，也要执行。更一般的依据是 [basic.stc.static]/2：静态存储期变量的初始化或析构有副作用，即使看起来没被用到，也不能被消除（拷贝省略除外）。

反过来，如果这个翻译单元里的东西从头到尾没被 ODR-use，推迟的初始化就没有期限，可能一直不发生。cppreference 说这是在模拟"按需加载的动态库"。

**inline 变量：只看它自己（C++17）**

```cpp
// config.h
inline int config = get_config();
inline void run() {}
```

- 只调用 `run()`：不触发 `config` 的初始化，规则只看 `config` 自己。
- 读取 `config`：这次 ODR-use 之前，`config` 必须已经初始化。

**"非初始化引起的"这个限定**

标准原文写的是 **non-initialization odr-use**：不是由非局部（静态或线程存储期）变量的初始化，直接或间接引起的 ODR-use。上面说的"第一次 ODR-use"都指这种：运行阶段（`main`、线程函数里）的使用算，**初始化过程里的使用不算**。

为什么要排除？这是我按条文推的，标准没写理由：`int global = get_value();` 的初始化器本身就调用了同一翻译单元里的非 inline 函数 `get_value()`，这是一次 ODR-use。如果它也算，规则就成了"`global` 的初始化要先于这次调用"，而这次调用正是 `global` 初始化的一部分，自己先于自己，满足不了。

版本差异：C++14 的措辞是"第一次 ODR-use 任何函数或变量之前"，既没有这个限定，也没有 inline 变量；C++17 起写成现在的样子（附录 B）。

**小结**

- ODR-use 是描述"怎么用一个实体"的语言概念；推迟初始化借它定最晚期限，保证被推迟的对象在程序真正需要相关实体时，已经初始化完成。
- 非 inline 变量的期限覆盖整个翻译单元，inline 变量只看它自己。
- g++ 11.4 对非 inline 的静态存储期全局对象不推迟：构造都发生在 `main` 之前（实验 1、2、3）。所以本节的例子是标准允许的行为，在这台机器上观察不到推迟。

### 2.4 静态局部变量（Static local variables）

函数里的变量（块作用域变量）默认是自动存储期（[basic.stc.auto]/1）。写 `static`，就是静态存储期（[basic.stc.static]/3）；写 `thread_local`，就是线程存储期，块作用域里单独写 `thread_local` 会隐含 `static`（[dcl.stc]/3）。本节的静态局部变量，指这两种（§2.1 的"函数内的变量"）；标准叫它们块作用域变量（block-scope variable，现行草案缩写成 block variable）。它们是局部变量，存储期却和 §2.3 的非局部变量一样是静态或线程的。拿它们和非局部变量比：

- **静态初始化一样**：零初始化、常量初始化的规则同 §2.3 阶段 1（[basic.start.static]/2）；
- **动态初始化不一样**：非局部变量归 [basic.start.dynamic]（§2.3.1–§2.3.3 的顺序、提前、推迟）；静态局部变量归 [stmt.dcl]/4，由执行流触发（[basic.start.static]/2 的注把这两条路分开了）。

**规则**（[stmt.dcl]/4、/5，附录 B）

| 情形 | 规则 |
|---|---|
| 零初始化、常量初始化 | 在静态初始化阶段就完成，早于第一次进入所在的块，运行期没有动作 |
| 动态初始化 | 执行流**第一次经过它的声明**时才做；做完才算已初始化；之后再经过这个声明，直接跳过。实现也可以按 §2.3.2 的"提前"条件，把它提前成静态初始化 |
| 初始化抛异常 | 异常照常向外传播；这次初始化不算完成，下次再经过声明时重试（与非局部变量的对比见 §2.5） |
| 多个线程同时第一次经过声明（C++11 起） | 只初始化一次，其他线程等它完成 |
| 初始化期间，递归地再次经过这个声明 | 未定义行为 |
| 线程存储期（C++11 起） | 每个线程各有一个对象（[basic.stc.thread]/1），各自在本线程第一次经过声明时初始化；没经过声明的线程，既不构造也不析构 |
| 析构 | 只有构造完成了才析构：静态存储期的在程序退出时，线程存储期的在线程退出时；静态对象之间按初始化完成的先后反过来（§4.3） |

标 C++11 起的两行是按 cppreference 的版本标注写的；N3337（C++11）的 [stmt.dcl]/4 里已有这两条，C++03 原文我没有核对。

```cpp
void f() {
    static int k = 10;    // 常量初始化：f 第一次被调用之前，k 已经是 10
    static int d = g();   // 动态初始化：f 第一次执行到这一行才调用 g()，以后跳过
}
```

- **常量初始化**：`static std::string s = "hello";` 能不能常量初始化，取决于标准库实现和字符串长度，别当规则记。C++20 标准已把 `std::basic_string` 的成员函数声明为 constexpr（特性宏 `__cpp_lib_constexpr_string` 在 [version.syn] 里是 201907L）；但本机 g++ 11.4 的 libstdc++ 11 里这个宏是 201811L，还没实现，所以它是动态初始化。换成 libstdc++ 12（clang 14 实际用的就是它，宏是 201907L）：`constinit std::string s = "hello";` 能编译，说明短串做成了常量初始化；超出小字符串缓冲区的长串（实测用了 51 个字符）不行（实测，附录 B 的 [version.syn]）。
- **实测**（g++ 11.4、clang 14，`-std=c++20`，结果一致）：`g()` 在 `f()` 第一次被调用时才执行；构造抛异常后，下次调用重试（第三次才成功，析构只在退出时跑一次）；4 个线程同时第一次调用，构造只跑了 1 次，4 个线程读到的都是构造完成后的值；`thread_local` 在两个线程里各构造、各析构一次。递归经过声明是未定义行为，这两个编译器用的运行库（libstdc++）都是抛出 `__gnu_cxx::recursive_init_error` 后 `terminate`，这是实现的行为，标准不保证。
- **实现**：需要动态初始化的变量带 guard 变量，常量初始化的没有，见 §3.3。
- **`inline` 函数里的 `static`**：函数有外部链接时，不管它在多少个翻译单元里各被定义一次，里面的 `static` 变量全程序只有一个对象（[dcl.inline]/6 的注）。显式写 `inline` 的函数，和类里定义的成员函数（隐式 inline，[dcl.inline]/7 的注），都是这类；内部链接的（`static inline`）每个翻译单元有各自的函数，也就各有一份变量。实测（g++ 11.4、clang 14）：`inline` 函数、类里定义的成员函数里的 `static` 变量，在两个翻译单元里同址，`static inline` 函数里的不同址。

**用途：绕开静态初始化顺序问题**

§2.3.1 的问题出在跨翻译单元的全局对象没有顺序保证。静态局部变量的构造时间由调用决定，不靠翻译单元之间的排序，所以把 `b` 换成函数里的 `static`，谁用谁调用：

```cpp
// b.cpp
B& get_b() { static B b; return b; }   // 第一次调用时才构造
// a.cpp
A a;    // 构造函数里用 get_b()，此时 b 一定已经构造完
```

实测（g++ 11.4、clang 14）：`B` 的构造函数把成员 `ready` 置 1，`A` 的构造函数读它。

| `b` 的写法 | 链接顺序 `a.o b.o` | 链接顺序 `b.o a.o` |
|---|---|---|
| 全局变量（§2.3.1 的例子） | 读到 0：`b` 只被零初始化，构造函数还没跑 | 读到 1 |
| 静态局部变量，经 `get_b()` 取 | 读到 1 | 读到 1 |

全局变量那一行取决于链接器怎么排各翻译单元的初始化函数，标准不保证。

### 2.5 异常

| 变量 | 初始化抛异常的结果 |
|---|---|
| 非局部变量（静态或线程存储期） | 调用 `std::terminate`，无法在外面 catch（[basic.start.dynamic]/8） |
| 静态局部变量 | 异常正常传播；这次初始化不算完成，下次执行到声明时重试（[stmt.dcl]/4） |

## 3. 实现层面：Linux 可执行文件与内存布局

以下是典型的 Linux / ELF / x86-64 情形。内存分段是实现细节，C++ 标准没有规定。

### 3.1 节（section）和段（segment）

| | 作用 |
|---|---|
| **节** | 组织文件里的内容：`.text`、`.rodata`、`.data`、`.bss`、`.init_array`、`.tdata`/`.tbss` |
| **段** | 描述运行时怎么把文件内容映射进进程：由程序头的 `PT_LOAD` 等条目给出 |

主要的节：

| 节 | 内容 | 属性 |
|---|---|---|
| `.text` | 机器指令 | 可读、可执行 |
| `.rodata` | 只读数据，如字符串字面量 | 只读 |
| `.data` | 有非零初值的可写变量 | 可读写 |
| `.bss` | 值为 0 的可写变量；文件里不占空间 | 可读写 |
| `.init_array` | 函数指针数组，指向动态初始化函数 | 装载后只读（RELRO） |
| `.tdata` / `.tbss` | 线程局部变量的**初始映像** | — |

实测（实验 1，`readelf -lW`）：

- 4 个 `PT_LOAD`：只读的 ELF 头；`R E` 的 `.text`；只读的 `.rodata`；`RW` 的 `.tdata .init_array .fini_array .data.rel.ro .dynamic .got .data .bss`。
- 最后一个 `PT_LOAD` 的 `FileSiz` 小于 `MemSiz`，差值正是 `.bss` 的大小：`p_memsz > p_filesz` 时，多出来的部分装载时置零，所以 `.bss` 不用在文件里存零。
- `GNU_RELRO` 覆盖 `.init_array` 等：重定位完成后被设成只读。运行时这个程序自己的映射是 5 段（`r--p`、`r-xp`、`r--p`、RELRO 区 `r--p`、`rw-p`）。
- 小的 `.data` 和 `.bss` 落在同一页里，`.bss` 大到超出这一页时，多出来的才是匿名页。

> `.init_array` 容易被当成和 `.text`/`.rodata` 一样"装载时直接映射"的只读内容，其实它在可写的 `PT_LOAD` 里（因为要被重定位），装载后才变只读。

### 3.2 变量放哪

| 变量 | C++ 存储期 | 典型位置 |
|---|---|---|
| 普通局部变量 | 自动 | 栈（或寄存器；优化后可能被消掉） |
| 全局变量、类的静态数据成员、函数内 `static` | 静态 | `.data` / `.bss` |
| `thread_local` 变量 | 线程 | 每个线程自己的 TLS 块（模板来自 `.tdata`/`.tbss`） |
| `new` 出来的对象 | 动态 | 堆分配器管理，大块走 mmap |
| 字符串字面量 | 静态存储期的数组 | 通常 `.rodata` |

要点：

- **C++ 的存储期不等于物理存放位置**，这张表讲的是典型实现，不是标准规定。
- **`const` 不一定在 `.rodata`**：被取了地址的 `const int g_const = 10;` 在 `.rodata`；只当值用的 `const int g_const_folded = 12;`，`-O0` 下在 `.rodata`，`-O2` 下**符号整个消失**（实验 1）。
- **每个线程有自己的栈**，也有自己的 `thread_local` 实例。实测第二线程的 `t_data` 和它栈上的局部变量落在同一块匿名映射里，地址只差 2228 字节。
- **堆栈方向只是典型示意**：`new int` 在 `[heap]`，`malloc(1 MiB)` 走独立的 mmap 映射，不在 `[heap]`。

### 3.3 与初始化的三个连接点

| 位置 | 与初始化的关系 |
|---|---|
| `.data` | 常量初始化且值非 0：值随文件映射就位，不需要 C++ 的动态初始化函数；含地址的初值，PIE 里装载时由动态链接器重定位（见下） |
| `.bss` | 值为 0；最终仍走动态初始化的变量也先按 0 放在这里，所以动态初始化之前读到 0 |
| `.init_array` | 存的是动态初始化**函数的指针**，由启动代码遍历调用 |

三点易记错：

1. **`.data`/`.bss` 按值分，不按 C++ 的初始化类别分。** `int g_zero_const = 0;` 是常量初始化，GCC 照样放进 `.bss`。准确的说法：
   - 初值为 0（零初始化，或常量初始化成 0）→ `.bss`；
   - 非零的常量初始化 → `.data`（加 `const` 且被需要 → `.rodata`）；
   - 最终仍走动态初始化的 → 先按 0 放在 `.bss`，再由初始化函数赋值（被编译器提前成静态初始化的，直接按最终值放，没有初始化函数，§2.3.2）。
2. **`.init_array` 存函数指针，不存对象。** 对象的存储在 `.bss`，初始化函数的机器指令在 `.text`，`.init_array` 里是指向函数的指针，三者别混。
3. **`.init_array` 不是每个全局对象一项；本实验的 g++ 11.4 构建里是每个翻译单元一项。** 实验 2：两个翻译单元共 4 个有动态初始化的全局变量，`.init_array` 只有 3 项：crt 的 `frame_dummy`，加每个翻译单元的 `_GLOBAL__sub_I_*`。同一个翻译单元里的变量由同一个函数按定义顺序初始化，这就是"同一翻译单元内有序"在实现上的来源。这是没用优先级属性时的情形：给变量加 `init_priority` 后，每个优先级另有一个函数和一项（实测）。

另外：

- **静态局部变量的动态初始化不走 `.init_array`**，初始化代码就在函数体里（语言规则见 §2.4）：每个需要动态初始化的变量配一个 guard 变量（8 字节，在 `.bss`，实验 1），常量初始化的没有。g++ 11.4 x86-64、`-O0` 的汇编：先读 guard 的首字节，非 0 就跳过；为 0 则调 `__cxa_guard_acquire`（别的线程在这里等），构造对象，`__cxa_atexit` 登记析构，`__cxa_guard_release` 置标志；构造抛异常则调 `__cxa_guard_abort`，标志仍是 0，所以下次重试。已初始化之后每次经过，开销只是一次普通的首字节读取和判断。
- **需要在退出时析构的静态存储期对象（含函数内 static），构造完成后立刻用 `__cxa_atexit` 登记析构函数**，退出时按登记的逆序调用。这是"反序析构"和"只有构造完成的才析构"的来源。析构函数平凡的（如 `int g = f();`）没有析构要做，不登记（Itanium ABI 的条件是 "that will require destruction on exit"，g++ 11.4 实测一致）。`thread_local` 对象另用 `__cxa_thread_atexit`，在线程退出时调用。
- **PIE 构建里**，`.init_array` 每个槽都有一条 `R_X86_64_RELATIVE` 重定位，装载时由动态链接器加上基址；`.data` 里含地址的初值（如 `int* p = &x;`）同理。所以"静态初始化"不只是映射，装载时还要做重定位（实验 2）。
- **`.tdata`/`.tbss` 只是模板**：可执行文件里只有一份，每个线程启动时由运行库拷贝出自己的 TLS 块，活的对象在那块里。

## 4. 串起来：从加载到退出

### 4.1 时间线

以下是本机（g++ 11.4 / glibc 2.35 / Linux x86-64）实测到的典型启动路径，不是 C++ 标准规定的统一顺序：标准允许把部分动态初始化推迟到 `main` 或线程函数开始之后（§2.3.2）。对非 inline 的静态存储期对象，这台机器没有推迟（实验 1、2、3）。

| 步 | 谁做 | 做什么 | 对应的语言规则 |
|---|---|---|---|
| 1 加载 | 内核（`execve`） | 按 `PT_LOAD` 映射各段；`.bss` 置零；建栈，放 `argv` 和环境变量；动态链接的程序还会加载 `ld.so` | |
| 2 动态链接 | `ld.so` | 把 `libc.so` 等映射进来，做符号解析和重定位 | 典型实现下，**静态初始化需要的数据到此就位**：常量初始化的值在 `.data`/`.rodata`，零初始化的在全零的 `.bss`，重定位也做完了。这是实现静态初始化的方式，不是语言规定的一个阶段 |
| 3 启动代码 | `_start` → `__libc_start_main` | 初始化 libc；`call_init` 遍历 `.init_array` | **动态初始化**：全局对象的构造函数在这里执行 |
| 4 `main` | 用户代码 | 栈上的局部变量、`new` 的对象、函数内 `static`、线程和 `thread_local` 各按自己的规则 | 函数内 `static` 第一次经过声明时初始化（§2.4） |
| 5 退出 | 运行库 | 见 §4.3 | |
| 6 回收 | 内核 | 回收整个地址空间，包括没 `delete` 的堆内存 | |

第 3 步的调用栈（实测，gdb 断在构造函数里，`main` 还没被调用）：

```text
A::A ← __static_initialization_and_destruction_0 ← _GLOBAL__sub_I__Z1fv
     ← call_init (csu/libc-start.c:145) ← __libc_start_main_impl ← _start
```

共享库自己的 `.init_array` 由动态链接器调用，这部分没有实测。

### 4.2 `main` 运行阶段：对象在哪、何时消失

```cpp
int global = 10;
void func() {
    int x = 20;
    static int s = 30;
    int* p = new int(40);
}
int main() { func(); }
```

| 对象 | 位置 | `func()` 返回后 |
|---|---|---|
| `global`、`s` | `.data` | 还在（静态存储期） |
| `x`、指针 `p` | 栈 | 消失 |
| `*p` | 堆 | 还在，要靠 `delete` 或所有权管理释放 |

这也是 RAII 的意义：资源的释放要有明确的管理关系，不能指望进程退出时统一回收。

### 4.3 退出：与初始化相反的过程

`main` 返回，效果相当于调用 `std::exit`。顺序是：

1. `main` 里的自动对象（`main` 返回前就已销毁）；
2. **主线程的 `thread_local` 对象**；
3. **静态存储期对象的析构**，与 `atexit` 登记的函数交错执行：先构造完成（或先登记）的，后执行（[basic.start.term]/3、/5）。不是"先析构完所有静态对象，再统一调 `atexit` 函数"。标准是成对规定先后的（A 的初始化完成 strongly happens before B 的，则 B 先析构）；没有其他线程时，初始化完成和 `atexit` 登记的实际次序总有先后，所以可以当成一条时间线；
4. 进程终止，内核回收地址空间。

补充：

- 只有构造**完成**的对象才会析构。
- 其他线程的 `thread_local` 在各自线程退出时就已销毁。
- `std::exit` 不会析构调用处的自动对象。
- `_exit` 和 `std::abort` 不跑任何析构。`abort` 是被信号 SIGABRT（编号 6）终止的，进程没有退出码；shell 的 `$?` 把它报成 134（= 128 + 6），这是 shell 的约定，标准没有规定（本机 bash、zsh 实测都是 134）。

> 主线程的 `thread_local` **先于**静态对象销毁（[support.start.term]，附录 B），不是单纯按"构造完成的逆序"。实验 3 里 `thread_local` 的构造早于 `static local`，如果只是按构造完成的逆序，它应该排在 `static local` 之后；实测排在前面，正是 `exit` 的"先线程局部、后静态"。

## 5. 知识地图与掌握程度

```text
初始化
├─ 对象：静态 / 线程存储期的变量：非局部的（命名空间作用域变量、类的静态数据成员）、静态局部变量（函数内的 static、thread_local）
├─ 初始化器的写法 → 种类（所有变量通用，与下面的"阶段"互相独立）
│   ├─ 不写→默认；空 ()→值；= 表达式→拷贝；(实参)→直接；{ }→列表
│   ├─ 默认：类→默认构造函数；数组→逐元素；其他→什么也不做
│   ├─ 值：标量→零初始化；类→看默认构造函数是否 user-provided；数组→逐元素
│   └─ 零：没有写法，由值初始化、以及"静态/线程存储期没做常量初始化"触发
├─ 非局部变量（§2.3）
│   ├─ 怎么初始化（静态局部变量同样适用）
│   │   ├─ 静态初始化（先）：满足 constant-initialized 就常量初始化，否则零初始化
│   │   └─ 动态初始化（后）：其余的，通常需要运行代码
│   ├─ 谁先谁后（静态存储期变量的动态初始化）
│   │   ├─ 有序：同一翻译单元按定义顺序，跨翻译单元不确定（没提前启动线程时）
│   │   ├─ 部分有序（C++17）：inline 变量，满足定义顺序条件时有保证
│   │   ├─ 无序：模板实例化变量
│   │   └─ thread_local：标准没给三档分类，只规定时机
│   └─ 何时初始化
│       ├─ 默认：main 之前 / 线程启动或首次使用
│       ├─ Early：动态提前成静态
│       └─ Deferred：推迟到 main 之后，最晚期限由首次非初始化 ODR-use 定
│           ├─ 非 inline、静态存储期：同一翻译单元里任何非 inline 函数或变量的首次
│           ├─ inline、静态存储期：它自己的首次
│           └─ 非 inline、线程存储期：本线程对同一翻译单元里有动态初始化的 thread_local 变量的首次（CWG 2684）
├─ 静态局部变量（§2.4）
│   ├─ 零、常量初始化在静态阶段；动态初始化等第一次经过声明（实现也可提前成静态）
│   ├─ 多线程同时第一次经过：只初始化一次（C++11）；递归经过：未定义
│   └─ thread_local：每个线程一份，各自第一次经过时初始化
├─ 异常（§2.5）：非局部变量 → std::terminate；静态局部变量 → 传播、下次重试
├─ 落到可执行文件：.data / .bss / .init_array
└─ 退出：主线程 thread_local → 静态对象与 atexit（逆序）
```

**必须理解**

1. 静态初始化先于动态初始化；静态初始化是"常量初始化，否则零初始化"。
2. 需要动态初始化的变量，先被零初始化，再动态初始化：标量先是 0，类类型逐个成员置零，引用不做任何事。
3. 同一翻译单元按定义顺序，跨翻译单元不确定，由此产生静态初始化顺序问题。
4. 非局部变量初始化抛异常会 `std::terminate`，静态局部变量不会。
5. C++ 的存储期不等于物理位置，内存布局只是 Linux ELF 的典型实现。
6. 初始化器的写法决定初始化的种类：不写→默认，空 `()`→值，`= 表达式`→拷贝，`(实参)`→直接，`{ }`→列表；它和静态/动态初始化是两个独立的维度：同样是默认初始化，全局 `int b;` 只有零初始化（0 是零初始化给的，默认初始化什么也没做），构造函数不是 `constexpr` 的类还要在动态阶段调用它，构造函数是 `constexpr` 的类是常量初始化。
7. 静态局部变量（函数内的 `static`、`thread_local`）：零初始化、常量初始化仍在静态阶段，动态初始化等执行流第一次经过声明；抛异常下次重试，多线程同时经过只初始化一次，递归经过是未定义行为。

**理解规则即可，不必背措辞**

1. 三档排序各自适用什么对象。
2. Early 与 Deferred 的方向相反。
3. `thread_local` 是每线程各一次。
4. 节与段的区别，`.data`/`.bss` 按值分。
5. ODR-use 的典型情形（读、改、取地址、绑定引用、调用），以及它在推迟初始化里充当"最晚期限"。
6. 默认、值、零三种初始化各做什么；值初始化对类类型，看默认构造函数是否 user-provided。
7. 用静态局部变量绕开静态初始化顺序问题的写法；`inline` 函数里的静态局部变量全程序只有一份。

**知道有这回事即可**

`.init_array` 的重定位细节、线程相关的 happens-before 规则、内核装载的内部过程、"非初始化引起的"这个限定的条文细节、指派初始化（C++20）、圆括号里嵌花括号（C++11）、直接列表初始化与拷贝列表初始化的区分。

版本标签：变量模板的无序初始化是 C++14；inline 变量的部分有序，以及它单独的推迟规则是 C++17。

## 附录 A：实验（可复现）

环境同文件开头。源码在 A.0，输出为实际运行结果（地址相关的细节作了截取）。

### A.0 源码与编译

<details>
<summary>exp1.cpp：变量落在哪个节、哪个映射</summary>

```cpp
// 变量落在哪个节/映射：objdump -t 看节，/proc/self/maps 看运行时映射
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

struct A {
    const char* n;
    explicit A(const char* n) : n(n) { std::printf("  ctor %s\n", n); }
    ~A() { std::printf("  dtor %s\n", n); }
};

int f() { return 7; }

int g_bss;                          // 零初始化
int g_zero_const = 0;               // 常量初始化，值为 0
int g_data = 10;                    // 常量初始化，值非 0
const int g_const = 10;
const int* g_pconst = &g_const;     // 取地址，迫使 g_const 真正存在
const int g_const_folded = 12;      // 只当值用
int g_dyn = f();                    // 动态初始化
const char* g_str = "hello";
thread_local int t_data = 5;
thread_local int t_bss;
A g_obj1("g_obj1");
A g_obj2("g_obj2");

void fn() {
    static int s_data = 1;
    static int s_bss;
    static int s_dyn = f();         // 动态初始化的函数内 static，带 guard 变量
    s_data += s_bss + s_dyn;
}

static void where(const char* name, const void* p) {
    unsigned long a = reinterpret_cast<unsigned long>(p);
    std::FILE* fp = std::fopen("/proc/self/maps", "r");
    char line[512];
    while (std::fgets(line, sizeof line, fp)) {
        unsigned long lo = 0, hi = 0;
        char perms[8] = "";
        char path[400] = "";
        std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %399[^\n]", &lo, &hi, perms, path);
        if (a >= lo && a < hi) {
            const char* base = std::strrchr(path, '/');
            base = base ? base + 1 : path;
            std::printf("%-26s %s  %s\n", name, perms, base[0] ? base : "[anon]");
            break;
        }
    }
    std::fclose(fp);
}

int main() {
    fn();
    int local = 1;
    int* heap = new int(2);
    void* big = std::malloc(1 << 20);
    where("f (代码)", reinterpret_cast<void*>(&f));
    where("puts (libc 代码)", reinterpret_cast<void*>(&::puts));
    where("\"hello\" 字面量", "hello");
    where("g_const", &g_const);
    where("g_data", &g_data);
    where("g_zero_const", &g_zero_const);
    where("g_bss", &g_bss);
    where("g_dyn", &g_dyn);
    where("g_str(指针本身)", &g_str);
    where("local (栈)", &local);
    where("new int", heap);
    where("malloc(1MiB)", big);
    where("t_data (主线程)", &t_data);
    std::thread th([] {
        int x = 0;
        where("t_data (第二线程)", &t_data);
        where("x (第二线程的栈)", &x);
        std::printf("t_data 与 x 地址相差 %ld 字节\n",
                    static_cast<long>(reinterpret_cast<char*>(&t_data) - reinterpret_cast<char*>(&x)));
    });
    th.join();
    delete heap;
    std::free(big);
}
```

extra.cpp（第二个翻译单元，用来数 `.init_array` 的项数）：

```cpp
#include <cstdio>

struct B {
    B() { std::puts("  ctor g_obj3 (另一个翻译单元)"); }
};
B g_obj3;
```

</details>

<details>
<summary>exp2.cpp：退出顺序、thread_local 的初始化时机、exit / _exit / abort</summary>

```cpp
// 退出顺序、thread_local 的初始化时机、exit/_exit/abort 的区别（全部写 stderr，避免缓冲干扰）
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>

struct T {
    const char* n;
    explicit T(const char* n) : n(n) { std::fprintf(stderr, "  ctor %s\n", n); }
    ~T() { std::fprintf(stderr, "  dtor %s\n", n); }
};

T g_static("static global");
thread_local T g_tls("thread_local global");

static void bye(const char* how) {
    T x("local in bye()");
    if (!std::strcmp(how, "exit")) std::exit(0);
    if (!std::strcmp(how, "_exit")) _exit(0);
    if (!std::strcmp(how, "abort")) std::abort();
}

int main(int argc, char** argv) {
    const char* how = argc > 1 ? argv[1] : "return";
    std::fprintf(stderr, "main start (mode=%s)\n", how);
    T local("main local");
    std::fprintf(stderr, "main 第一次用 g_tls: %s\n", g_tls.n);
    static T s_local("static local");
    std::thread th([] {
        std::fprintf(stderr, "thread start\n");
        std::fprintf(stderr, "thread 第一次用 g_tls: %s\n", g_tls.n);
        std::fprintf(stderr, "thread end\n");
    });
    th.join();
    std::fprintf(stderr, "main end\n");
    if (std::strcmp(how, "return")) bye(how);
}
```

</details>

编译（本文用到的构建）：

```bash
g++ -std=c++20 -O0 -Wall -Wextra -pthread exp1.cpp extra.cpp -o exp1       # PIE（默认）
g++ -std=c++20 -O0 -pthread -no-pie exp1.cpp extra.cpp -o exp1np           # 非 PIE：直接读 .init_array 的值
g++ -std=c++20 -O2 -pthread exp1.cpp extra.cpp -o exp1o2                   # 对照 -O2
g++ -std=c++20 -O0 -g -pthread exp1.cpp extra.cpp -o exp1g                 # gdb 用
g++ -std=c++20 -O0 -Wall -Wextra -pthread exp2.cpp -o exp2
```

### A.1 实验 1：变量落在哪个节、哪个映射

**节表**（`readelf -SW exp1`，只留相关行）：

```text
  [Nr] Name              Type            Address          Off    Size   ES Flg
  [ 1] .interp           PROGBITS        0000000000000350 000350 00001c 00   A
  [16] .text             PROGBITS        00000000000012b0 0012b0 000ef4 00  AX
  [18] .rodata           PROGBITS        0000000000003000 003000 0001fe 00   A
  [20] .eh_frame         PROGBITS        0000000000003400 003400 00081c 00   A
  [22] .tdata            PROGBITS        0000000000005c5c 004c5c 000004 00 WAT
  [23] .tbss             NOBITS          0000000000005c60 004c60 000004 00 WAT
  [24] .init_array       INIT_ARRAY      0000000000005c60 004c60 000018 08  WA
  [25] .fini_array       FINI_ARRAY      0000000000005c78 004c78 000008 08  WA
  [27] .dynamic          DYNAMIC         0000000000005d00 004d00 000210 10  WA
  [28] .got              PROGBITS        0000000000005f10 004f10 0000e0 08  WA
  [29] .data             PROGBITS        0000000000006000 005000 000030 00  WA
  [30] .bss              NOBITS          0000000000006030 005030 000040 00  WA
```

**程序头**（`readelf -lW exp1`，截取；`Flg` 是段权限）：

```text
  Type           Offset   VirtAddr           FileSiz  MemSiz   Flg Align
  INTERP         0x000350 0x0000000000000350 0x00001c 0x00001c R   0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x000000 0x0000000000000000 0x000e78 0x000e78 R   0x1000
  LOAD           0x001000 0x0000000000001000 0x0011b1 0x0011b1 R E 0x1000
  LOAD           0x003000 0x0000000000003000 0x000c63 0x000c63 R   0x1000
  LOAD           0x004c5c 0x0000000000005c5c 0x0003d4 0x000414 RW  0x1000
  TLS            0x004c5c 0x0000000000005c5c 0x000004 0x000008 R   0x4
  GNU_RELRO      0x004c5c 0x0000000000005c5c 0x0003a4 0x0003a4 R   0x1

 Section to Segment mapping:
   03     .init .plt .plt.got .plt.sec .text .fini
   04     .rodata .eh_frame_hdr .eh_frame .gcc_except_table
   05     .tdata .init_array .fini_array .data.rel.ro .dynamic .got .data .bss
   09     .tdata .tbss                                                   (TLS)
   13     .tdata .init_array .fini_array .data.rel.ro .dynamic .got      (GNU_RELRO)
```

**符号 → 所在节**（`objdump -t --demangle exp1`，`-O0`）：

| 符号 | 所在节 | 说明 |
|---|---|---|
| `g_bss` | .bss | 零初始化 |
| `g_zero_const` | **.bss** | 常量初始化成 0，仍在 .bss |
| `g_data` | .data | 常量初始化，非 0 |
| `g_const` | .rodata | 被取了地址 |
| `g_const_folded` | .rodata（`-O2` 下**不存在**） | 只当值用 |
| `g_pconst`、`g_str` | .data | 指针变量本身 |
| `g_dyn` | .bss | 动态初始化，先按 0 放在这里 |
| `g_obj1`、`g_obj2`、`g_obj3` | .bss | 有动态初始化的对象 |
| `fn()::s_data` | .data | 函数内 static，非零常量初值 |
| `fn()::s_bss`、`fn()::s_dyn` | .bss | |
| `guard variable for fn()::s_dyn` | .bss（8 字节） | 动态初始化的函数内 static 的 guard |
| `t_data` | .tdata | 线程局部，初始映像 |
| `t_bss` | .tbss | 线程局部，零初始化 |

**运行输出**（`./exp1`；每行是"变量名 / 所在映射的权限 / 映射名"）：

```text
  ctor g_obj1
  ctor g_obj2
  ctor g_obj3 (另一个翻译单元)
f (代码)                 r-xp  exp1
puts (libc 代码)         r-xp  libc.so.6
"hello" 字面量          r--p  exp1
g_const                    r--p  exp1
g_data                     rw-p  exp1
g_zero_const               rw-p  exp1
g_bss                      rw-p  exp1
g_dyn                      rw-p  exp1
g_str(指针本身)        rw-p  exp1
local (栈)                rw-p  [stack]
new int                    rw-p  [heap]
malloc(1MiB)               rw-p  [anon]
t_data (主线程)         rw-p  [anon]
t_data (第二线程)      rw-p  [anon]
x (第二线程的栈)     rw-p  [anon]
t_data 与 x 地址相差 2228 字节
  dtor g_obj2
  dtor g_obj1
```

**运行中的映射**（gdb 断在 `main` 入口，读 `/proc/<pid>/maps`；关了 ASLR，地址固定；省略 libm、libgcc_s、libstdc++、ld-linux 和几段匿名映射）：

```text
555555554000-555555555000 r--p 00000000   exp1        ← ELF 头等
555555555000-555555557000 r-xp 00001000   exp1        ← .text
555555557000-555555558000 r--p 00003000   exp1        ← .rodata
555555559000-55555555a000 r--p 00004000   exp1        ← RELRO 区：.tdata/.init_array/.dynamic/.got
55555555a000-55555555b000 rw-p 00005000   exp1        ← .data + .bss
55555555b000-55555557c000 rw-p 00000000   [heap]
7ffff7b3c000-7ffff7b64000 r--p 00000000   libc.so.6
7ffff7b64000-7ffff7cf9000 r-xp 00028000   libc.so.6
7ffff7cf9000-7ffff7d51000 r--p 001bd000   libc.so.6
7ffff7d51000-7ffff7d52000 ---p 00215000   libc.so.6
7ffff7d52000-7ffff7d56000 r--p 00215000   libc.so.6
7ffff7d56000-7ffff7d58000 rw-p 00219000   libc.so.6
7ffff7fbd000-7ffff7fc1000 r--p 00000000   [vvar]
7ffff7fc1000-7ffff7fc3000 r-xp 00000000   [vdso]
7ffffffdd000-7ffffffff000 rw-p 00000000   [stack]
```

### A.2 实验 2：`.init_array` 里是什么、谁来调用

**非 PIE 构建里的 `.init_array`**（直接读文件里的值，再用 `nm` 查符号）：

```text
.init_array 共 3 项:
  [0] 0x401380  frame_dummy
  [1] 0x401ced  _GLOBAL__sub_I__Z1fv
  [2] 0x402182  _GLOBAL__sub_I_g_obj3
```

**PIE 构建**：槽值是未加基址的偏移，每槽一条 `RELATIVE` 重定位：

```text
文件里的槽值: ['0x1390', '0x1d01', '0x2196']
  0000000000005c60  0000000000000008 R_X86_64_RELATIVE   1390
  0000000000005c68  0000000000000008 R_X86_64_RELATIVE   1d01
  0000000000005c70  0000000000000008 R_X86_64_RELATIVE   2196
RELATIVE 重定位总数: 12
```

**`exp1.cpp` 这个翻译单元的初始化函数里依次调用了什么**（`objdump -d -C` 反汇编，只留 `call`）：

```text
call   f()                    ← g_dyn = f()
call   A::A(char const*)      ← g_obj1
call   __cxa_atexit@plt       ← 登记 g_obj1 的析构
call   A::A(char const*)      ← g_obj2
call   __cxa_atexit@plt       ← 登记 g_obj2 的析构
```

**谁调用**（`gdb -batch -ex 'break A::A' -ex run -ex bt`）：

```text
#0  A::A (this=0x55555555a048 <g_obj1>, n=... "g_obj1") at exp1.cpp:9
#1  __static_initialization_and_destruction_0 (__initialize_p=1, __priority=65535) at exp1.cpp:25
#2  _GLOBAL__sub_I__Z1fv () at exp1.cpp:82
#3  call_init (env=<optimized out>, argv=..., argc=1) at ../csu/libc-start.c:145
#4  __libc_start_main_impl (main=0x555555555695 <main()>, argc=1, argv=..., ...)
#5  _start ()
```

### A.3 实验 3：退出顺序、`thread_local` 初始化时机、`exit`/`_exit`/`abort`

`./exp2`（正常 `return`）：

```text
  ctor static global
main start (mode=return)
  ctor main local
  ctor thread_local global           ← 主线程第一次用时才构造
main 第一次用 g_tls: thread_local global
  ctor static local
thread start
  ctor thread_local global           ← 第二线程第一次用时才构造（各线程各一份）
thread 第一次用 g_tls: thread_local global
thread end
  dtor thread_local global           ← 第二线程退出时析构
main end
  dtor main local
  dtor thread_local global           ← 主线程的 thread_local，先于静态对象
  dtor static local
  dtor static global
```

在 `bye()`（栈上有 `local in bye()`）里分别调用三种退出方式，`main end` 之后的输出：

| 调用 | `main end` 之后的输出 | shell 的 `$?` |
|---|---|---|
| `std::exit(0)` | `ctor local in bye()`；`dtor thread_local global`；`dtor static local`；`dtor static global`（**没有** `dtor local in bye()`，也**没有** `dtor main local`） | 0 |
| `_exit(0)` | `ctor local in bye()`（之后什么都没有） | 0 |
| `std::abort()` | `ctor local in bye()`（之后什么都没有） | 134（被 SIGABRT 终止，128 + 6） |

## 附录 B：标准与规范原文摘录（核对用，均为短摘）

标准条文取自 C++ 工作草案（eel.is/c++draft）和旧版草案（timsong-cpp.github.io/cppwp），ELF 部分取自 gABI 与 `elf(5)`。ODR-use、推迟初始化与初始化器写法的条文（11-11 增补），函数内 `static` 的条文（11-13 增补），用 cplusplus/draft 仓库的 n3337（C++11）、n4140（C++14）、n4659（C++17）、n4861（C++20）几个标签核对过，段号按 N4861。

来源链接：N3337（C++11）<https://github.com/cplusplus/draft/tree/n3337>、N4140（C++14）<https://github.com/cplusplus/draft/tree/n4140>、N4659（C++17）<https://github.com/cplusplus/draft/tree/n4659>、N4861（C++20）<https://github.com/cplusplus/draft/tree/n4861>、现行草案 <https://eel.is/c++draft/>（源文件 <https://github.com/cplusplus/draft/tree/main>）、CWG 2684 <https://cplusplus.github.io/CWG/issues/2684.html>。下面各条里，没特别标注的都是 N4861 原文；现行草案措辞和缺陷报告单独标明。

- **[support.start.term]（`std::exit` 的 Effects）**："First, objects with thread storage duration and associated with the current thread are destroyed. Next, objects with static storage duration are destroyed and functions registered by calling atexit are called. … (Objects with automatic storage duration are not destroyed as a result of calling exit().)"
- **[basic.start.term]/1、/2、/3、/5**（节选）：/1 "Constructed objects with static storage duration are destroyed and functions registered with std::atexit are called as part of a call to std::exit."；/2 "The destruction of all constructed objects with thread storage duration within that thread strongly happens before destroying any object with static storage duration."；/3 "If the completion of the constructor or dynamic initialization of an object with static storage duration strongly happens before that of another, the completion of the destructor of the second is sequenced before the initiation of the destructor of the first."；/5（与 `atexit` 的交错）"If the completion of the initialization of an object with static storage duration strongly happens before a call to std::atexit, the call to the function passed to std::atexit is sequenced before the call to the destructor for the object. If a call to std::atexit strongly happens before the completion of the initialization of an object with static storage duration, the call to the destructor for the object is sequenced before the call to the function passed to std::atexit. If a call to std::atexit strongly happens before another call to std::atexit, the call to the function passed to the second std::atexit call is sequenced before the call to the function passed to the first std::atexit call."
- **[stmt.dcl]/4、/5**（函数内的 `static`、`thread_local`；现行草案里是 /3、/4：goto 那一段并入 /2 后前移了一位，措辞里的 block-scope variable 也改成了 block variable）：/4 "Dynamic initialization of a block-scope variable with static storage duration or thread storage duration is performed the first time control passes through its declaration; such a variable is considered initialized upon the completion of its initialization. If the initialization exits by throwing an exception, the initialization is not complete, so it will be tried again the next time control enters the declaration. If control enters the declaration concurrently while the variable is being initialized, the concurrent execution shall wait for completion of the initialization. If control re-enters the declaration recursively while the variable is being initialized, the behavior is undefined."；/5 "A block-scope object with static or thread storage duration will be destroyed if and only if it was constructed."（后面的注指向 [basic.start.term]，销毁的先后在那里规定。）
- **函数内变量的存储期与静态初始化**：[basic.stc.auto]/1 "Block-scope variables not explicitly declared static, thread_local, or extern have automatic storage duration."；[basic.stc.static]/3 "The keyword static can be used to declare a local variable with static storage duration."；[dcl.stc]/3 "When thread_local is applied to a variable of block scope the storage-class-specifier static is implied if no other storage-class-specifier appears in the decl-specifier-seq."；[basic.stc.thread]/1 "All variables declared with the thread_local keyword have thread storage duration. … There is a distinct object or reference per thread, and use of the declared name refers to the entity associated with the current thread."；[basic.start.static]/2 的注 "The dynamic initialization of non-local variables is described in [basic.start.dynamic]; that of local static variables is described in [stmt.dcl]."（现行草案写作 non-block variables 与 static block variables。）
- **[dcl.inline]/6、/7 的注**（`inline` 函数里的 `static`）：/6 "An inline function or variable with external or module linkage has the same address in all translation units. A static local variable in an inline function with external or module linkage always refers to the same object. …"；/7 "A constexpr function is implicitly inline. In the global module, a function defined within a class definition is implicitly inline."（现行草案把 /6 那句改写后放进 [dcl.inline]/2 的注："An inline function or variable with external or module linkage can be defined in multiple translation units, but is one entity with one address. A type or static variable defined in the body of such a function is therefore a single entity."）
- **函数内 static 初始化的措辞演变**：
  - C++11（N3337 [stmt.dcl]/4）和 C++14（N4140 /4）：同一段里还写着 "The zero-initialization of all block-scope variables with static storage duration or thread storage duration is performed before any other initialization takes place. Constant initialization of a block-scope entity with static storage duration, if applicable, is performed before its block is first entered. An implementation is permitted to perform early initialization of other block-scope variables with static or thread storage duration under the same conditions that an implementation is permitted to statically initialize a variable with static or thread storage duration in namespace scope. Otherwise such a variable is initialized the first time control passes through its declaration; …"，后面是同样的异常重试、并发等待（脚注 "The implementation must not introduce any deadlock around execution of the initializer."）和递归未定义。
  - C++17（N4659 [stmt.dcl]/4）起：前面几句并入 [basic.start.static]，[stmt.dcl]/4 从 "Dynamic initialization of a block-scope variable …" 写起，与 N4861 相同。
- **[dcl.init]/1**（初始化器的适用范围与文法）："The process of initialization described in this subclause applies to all initializations regardless of syntactic context, including the initialization of a function parameter, the initialization of a return value, or when an initializer follows a declarator." 文法：`initializer: brace-or-equal-initializer | ( expression-list )`；`brace-or-equal-initializer: = initializer-clause | braced-init-list`；`initializer-clause: assignment-expression | braced-init-list`；`braced-init-list: { initializer-list ,opt } | { designated-initializer-list ,opt } | { }`；`expression-list: initializer-list`（在 [expr.post]）。
- **[dcl.init]/6**（零初始化）："To zero-initialize an object or reference of type T means: if T is a scalar type, the object is initialized to the value obtained by converting the integer literal 0 (zero) to T; if T is a (possibly cv-qualified) non-union class type, its padding bits are initialized to zero bits and each non-static data member, each non-virtual base class subobject, and, if the object is not a base class subobject, each virtual base class subobject is zero-initialized; … if T is an array type, each element is zero-initialized; if T is a reference type, no initialization is performed."（联合体一条从略。）
- **[dcl.init]/7**（默认初始化）：类类型调用默认构造函数；数组逐元素；"Otherwise, no initialization is performed."
- **[dcl.init]/8**（值初始化）："To value-initialize an object of type T means: if T is a (possibly cv-qualified) class type, then if T has either no default constructor or a default constructor that is user-provided or deleted, then the object is default-initialized; otherwise, the object is zero-initialized and the semantic constraints for default-initialization are checked, and if T has a non-trivial default constructor, the object is default-initialized; if T is an array type, then each element is value-initialized; otherwise, the object is zero-initialized."
- **[dcl.init]/9**："A program that calls for default-initialization or value-initialization of an entity of reference type is ill-formed."
- **[dcl.init]/10**（注）："For every object of static storage duration, static initialization is performed at program startup before any other initialization takes place. In some cases, additional initialization is done later."
- **[basic.indet]/1**："When storage for an object with automatic or dynamic storage duration is obtained, the object has an indeterminate value, and if no initialization is performed for the object, that object retains an indeterminate value until that value is replaced." 注："Objects with static or thread storage duration are zero-initialized, see [basic.start.static]."
- **[dcl.init]/11**（值初始化与 `()`）："An object whose initializer is an empty set of parentheses, i.e., (), shall be value-initialized." 注："Since () is not permitted by the syntax for initializer, `X a();` is not the declaration of an object of class X, but the declaration of a function taking no argument and returning an X. The form () is permitted in certain other initialization contexts ([expr.new], [expr.type.conv], [class.base.init])."
- **[dcl.init]/12**："If no initializer is specified for an object, the object is default-initialized."
- **[dcl.init]/15**（拷贝初始化）："The initialization that occurs in the = form of a brace-or-equal-initializer or condition, as well as in argument passing, function return, throwing an exception, handling an exception, and aggregate member initialization, is called copy-initialization."
- **[dcl.init]/16**（直接初始化，原文是五个并列项）："The initialization that occurs for an initializer that is a parenthesized expression-list or a braced-init-list, for a new-initializer, in a static_cast expression, in a functional notation type conversion, and in the braced-init-list form of a condition is called direct-initialization."
- **[dcl.init]/17**（初始化器的语义，节选）："If the initializer is a (non-parenthesized) braced-init-list or is = braced-init-list, the object or reference is list-initialized. If the destination type is a reference type, see [dcl.init.ref]. … If the initializer is (), the object is value-initialized. …"（标准先判花括号，再判引用，再判 `()`；cppreference 把引用放在最前，是简化的写法。）
- **[dcl.init.list]/1**："List-initialization can occur in direct-initialization or copy-initialization contexts; list-initialization in a direct-initialization context is called direct-list-initialization and list-initialization in a copy-initialization context is called copy-list-initialization."
- **[dcl.init.list]/3**（节选，空花括号相关）："Otherwise, if T is an aggregate, aggregate initialization is performed. … Otherwise, if the initializer list has no elements and T is a class type with a default constructor, the object is value-initialized. … Otherwise, if the initializer list has no elements, the object is value-initialized."（例：`int** pp {};` "initialized to null pointer"。聚合那一条排在值初始化之前，所以聚合的空花括号走聚合初始化。）
- **[dcl.ref]/5**："The declaration of a reference shall contain an initializer except when the declaration contains an explicit extern specifier, is a class member declaration within a class definition, or is the declaration of a parameter or a return type."
- **值初始化的措辞演变**：
  - C++11（N3337 [dcl.init]/7）："if T is a (possibly cv-qualified) class type with a user-provided constructor, then the default constructor for T is called (and the initialization is ill-formed if T has no accessible default constructor); if T is a (possibly cv-qualified) non-union class type without a user-provided constructor, then the object is zero-initialized and, if T's implicitly-declared default constructor is non-trivial, that constructor is called."
  - C++14（N4140 [dcl.init]/8）起：改成 N4861 /8 的写法，按"默认构造函数是否 user-provided 或被删除"判断。这是 CWG 1301「Value initialization of union」的解决方案（2012 年 10 月成为 DR）；CWG 1368「Value initialization and defaulted constructors (part 2)」提出的是 `struct A { int i; A() = default; A(int i): i(i) { } };` 的值初始化把 `A::i` 留成未初始化，"This issue is resolved by the resolution of issue 1301."（同月成为 DR）。
- **初始化器文法的版本演变**：C++11（N3337）起有 `braced-init-list`，`expression-list` 也是 `initializer-list`；指派初始化（`designated-initializer-list`）只在 C++20（N4861）里出现，N3337、N4140、N4659 里都没有（g++ 11.4 在 `-std=c++17 -pedantic` 下对指派初始化警告"只有 C++20 才有"）。C++03 的 `= { ... }`（聚合与标量）按 cppreference 的 until C++11 标注，没有核对原文。
- **[expr.const]（constant-initialized 的定义）**："A variable or temporary object o is constant-initialized if either it has an initializer or its default-initialization results in some initialization being performed, and the full-expression of its initialization is a constant expression when interpreted as a constant-expression, except that if o is an object, that full-expression may also invoke constexpr constructors for o and its subobjects even if those objects are of non-literal class types."
- **[expr.const] 里关于求值时机的注**：常量表达式不一定在翻译期求值。注里的例子：`char array[1 + int(1 + 0.2 - 0.1 - 0.1)];` 是 "Must be evaluated during translation"，而 `int size = 1 + int(1 + 0.2 - 0.1 - 0.1);` 是 "May be evaluated at runtime"。我没有找到标准里说静态初始化"不运行代码"的条文。
- **[basic.def.odr]/2**（什么叫"可能被求值"）："An expression or conversion is potentially evaluated unless it is an unevaluated operand, a subexpression thereof, or a conversion in an initialization or conversion sequence in such a context."（`sizeof`、`decltype` 的操作数是不求值操作数，所以里面的名字不算 ODR-use。）
- **[basic.def.odr]/4**（变量何时算 odr-used）："A variable x whose name appears as a potentially-evaluated expression E is odr-used by E unless …"。例外里与正文相关的一条：x 是非引用类型、能用于常量表达式、没有 mutable 子对象，并且 E 是"被施加了左值到右值转换的、非 volatile 非类类型表达式"的潜在结果之一。所以 `constexpr int n = 10; int x = n;` 不算，`&n` 算。
- **[basic.def.odr]/7**（函数何时算 odr-used）："A function is odr-used if it is named by a potentially-evaluated expression or conversion."（调用和取地址都属于"被命名"。）
- **[basic.def.odr]/10**："Every program shall contain exactly one definition of every non-inline function or variable that is odr-used in that program outside of a discarded statement; no diagnostic required."
- **[basic.start.dynamic]/1**（只针对静态存储期）："Dynamic initialization of a non-local variable with static storage duration is unordered if the variable is an implicitly or explicitly instantiated specialization, is partially-ordered if the variable is an inline variable that is not an implicitly or explicitly instantiated specialization, and otherwise is ordered."
- **[basic.start.dynamic]/2、/3**（节选，同样只针对静态存储期）：/2 "A declaration D is appearance-ordered before a declaration E if D appears in the same translation unit as E, or the translation unit containing E has an interface dependency on the translation unit containing D, in either case prior to E."；/3 "Dynamic initialization of non-local variables V and W with static storage duration are ordered as follows:" ① V、W 都是有序且 V 的定义 appearance-ordered 在 W 之前（或 V 部分有序、W 不是无序、且 W 的每个定义之前都有 V 的定义）：没有启动其他线程，或两者有序且在同一翻译单元 → V 的初始化 sequenced before W 的；否则 strongly happens before；② "Otherwise, if the program starts a thread other than the main thread before either V or W is initialized, it is unspecified in which threads the initializations of V and W occur; the initializations are unsequenced if they occur in the same thread."；③ "Otherwise, the initializations of V and W are indeterminately sequenced."（N4861 和现行草案的 basic.tex 里，与初始化排序有关的 unsequenced / indeterminately sequenced 只出现在这一条，没有针对线程存储期变量的对应条文。）
- **[basic.start.dynamic]/4**（非初始化引起的 ODR-use，N4861；最新草案把 non-local 改成 non-block）："A non-initialization odr-use is an odr-use not caused directly or indirectly by the initialization of a non-local static or thread storage duration variable."
- **[basic.start.dynamic]/5**（静态存储期、非 inline 的非局部变量）："It is implementation-defined whether the dynamic initialization … is sequenced before the first statement of main or is deferred. If it is deferred, it strongly happens before any non-initialization odr-use of any non-inline function or non-inline variable defined in the same translation unit as the variable to be initialized." 脚注："A non-local variable with static storage duration having initialization with side effects is initialized in this case, even if it is not itself odr-used." 后面还有一句："It is implementation-defined in which threads and at which points in the program such deferred dynamic initialization occurs."
- **[basic.start.dynamic]/6**（inline 变量，C++17 起）："It is implementation-defined whether the dynamic initialization of a non-local inline variable with static storage duration is sequenced before the first statement of main or is deferred. If it is deferred, it strongly happens before any non-initialization odr-use of that variable."
- **[basic.start.dynamic]/7**（线程存储期、非 inline 的非局部变量）："It is implementation-defined whether the dynamic initialization … is sequenced before the first statement of the initial function of a thread or is deferred. If it is deferred, the initialization associated with the entity for thread t is sequenced before the first non-initialization odr-use by t of any non-inline variable with thread storage duration defined in the same translation unit as the variable to be initialized."（N4861 原文；只数变量，没有函数。CWG 2684 修正后的写法见下一条。）
- **CWG 2684「thread_local dynamic initialization」**（缺陷报告）：状态 DR，2026 年 6 月会议接受，CWG 于 2026-05-29 通过解决方案。提出者的问题：`extern thread_local constinit int x;` 能不能触发同一翻译单元里其他 `thread_local` 的延迟初始化，原措辞没说清，而 clang 和 gcc 不生成相应的包装代码（按原措辞算不符合标准）。解决方案是在 /7 的 "any non-inline variable with thread storage duration" 后加 "and dynamic initialization"，已并入现行草案（draft 仓库提交 89713372c8，2026-07-11，标题 "CWG2684 thread_local dynamic initialization"）。**现行草案措辞**的 /7：" … the first non-initialization odr-use by t of any non-inline variable with thread storage duration and dynamic initialization defined in the same translation unit as the variable to be initialized."
- **[basic.start.dynamic]/8**："If the initialization of a non-block variable with static or thread storage duration exits via an exception, the function std::terminate is called."
- **[basic.stc.static]/2**："If a variable with static storage duration has initialization or a destructor with side effects, it shall not be eliminated even if it appears to be unused, except that a class object or its copy/move may be eliminated as specified in [class.copy.elision]."
- **[basic.start.static]/3**（C++17 起）："An implementation is permitted to perform the initialization of a variable with static or thread storage duration as a static initialization even if such initialization is not required to be done statically, provided that …"（条件：动态版本不改变其他静态/线程存储期对象初始化前的值；静态版本得到的值与动态版本相同。）该条的注里有同一个 `d1`/`d2` 例子，原文："d2 // unspecified: may be statically initialized to 0.0 or dynamically initialized to 0.0 if d1 is dynamically initialized, or 1.0 otherwise"。
- **静态初始化的措辞演变**：
  - C++14（N4140 [basic.start.init]/2）："Variables with static storage duration or thread storage duration shall be zero-initialized before any other initialization takes place. … Constant initialization is performed: …"
  - C++17（N4659 [basic.start.static]/2）和 C++20（N4861）："Constant initialization is performed if … If constant initialization is not performed, a variable with static storage duration or thread storage duration is zero-initialized. Together, zero-initialization and constant initialization are called static initialization; … All static initialization strongly happens before any dynamic initialization."
- **推迟初始化的措辞演变**：
  - C++14（N4140 [basic.start.init]/4）："If the initialization is deferred to some point in time after the first statement of main, it shall occur before the first odr-use of any function or variable defined in the same translation unit as the variable to be initialized."（没有"非初始化引起的"限定，也没有 inline 变量；脚注同样写明有副作用的初始化即使没被 odr-use 也必须进行。）
  - C++17（N4659 [basic.start.dynamic]）和 C++20（N4861）：改成上面 /4 到 /7 的样子，即加了 non-initialization 限定，并把静态存储期的非 inline 变量、inline 变量、线程存储期变量分成三段。
- **动态初始化排序的措辞演变**：
  - C++14（N4140 [basic.start.init]/2）："If a program starts a thread, the subsequent initialization of a variable is unsequenced with respect to the initialization of a variable defined in a different translation unit. Otherwise, the initialization of a variable is indeterminately sequenced with respect to the initialization of a variable defined in a different translation unit."（语境是静态存储期的非局部变量，同样没有针对线程存储期变量的排序条文。）
  - C++17（N4659 [basic.start.dynamic]）起：写成 N4861 /3 的三条，其中有 "if the program starts a thread other than the main thread before either V or W is initialized, it is unspecified in which threads the initializations … occur"。
- **[dcl.fct.def.default]/5**（user-provided 的定义）："A function is user-provided if it is user-declared and not explicitly defaulted or deleted on its first declaration."
- **[version.syn]**（N4861）：`__cpp_lib_constexpr_string` 是 201907L，`__cpp_lib_constexpr_string_view` 是 201811L。本机实测 `<string>` 里的值：g++ 11.4 是 201811L，clang 14（实际使用 libstdc++ 12）是 201907L。
- **gABI（System V ABI，节头一章）**：`.bss` "holds uninitialized data that contribute to the program's memory image. By definition, the system initializes the data with zeros when the program begins to run. The section occupies no file space, as indicated by the section type, SHT_NOBITS."；`SHT_INIT_ARRAY` "contains an array of pointers to initialization functions … Each pointer in the array is taken as a parameterless procedure with a void return."；`.tdata` 的节标志是 `SHF_ALLOC+SHF_WRITE+SHF_TLS`，`.tbss` 是 `SHT_NOBITS` 加同样的标志。
- **`elf(5)`**：`.rodata` "holds read-only data that typically contributes to a nonwritable segment in the process image."
