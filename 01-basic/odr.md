# One Definition Rule (ODR)

> 范围：只讲主线：一个实体有几个定义，有多个时要怎样一致。ODR-use 的形式化判定，lambda、默认实参、隐式特殊成员的一致性细则，显式特化，模块的细节，暂不展开。

## Why

C++ 程序由多个翻译单元（TU：一个 .cpp 加上它 `#include` 进来的全部内容）组成。每个 TU 单独编译，最后由链接器拼在一起。标准明确允许这样做：程序不必一次翻译完。

这个模型同时有两个需要：

- **多数实体只能有一份。** 两个 TU 里的同名函数如果是同一个实体，却各有一个定义，程序就不知道听谁的。C 的做法是全程序恰好一个定义，重复的由链接器发现（N0369 的描述）。
- **少数实体必须在每个 TU 里都有一份。** 编译 `sizeof(X)` 和 `x.member` 要知道类的完整定义，内联一个函数要看到它的函数体，实例化一个模板要看到模板的定义。共享定义的办法是写进头文件，让 `#include` 把同一段文本复制进每个 TU。

ODR 是这两头之间的约定：哪些实体只能定义一次，哪些可以每个 TU 各定义一次，以及“各一次”时这些定义要满足什么条件。条件满足，程序的行为**如同只有一个定义**。

条件没人替你检查。典型工具链里，编译器一次只看一个 TU，链接器只按符号名拼接，没有谁能把两个 TU 里的定义拿来比较。标准的选择是不要求诊断，把责任交给程序员：WG21 1994 年的提案 N0369 说得很直白，这样实现可以假设程序员遵守了 ODR，不用去检查；1995 年的 N0716 补了理由：理论上能做出检查 ODR 的系统，但当时多数系统都不做，不想把这笔实现负担压给厂商。

> **Note:** 名字有点误导，规则里既有“只有一个定义”，也有“多个定义必须相同”。N0716 的作者自己就说，叫“多重定义规则”（MDR）更贴切，只是 ODR 已经成了惯用名。

## What

**ODR：一个实体默认只有一个定义；少数必须在每个 TU 里出现的实体例外，可以每个 TU 各定义一次，条件是各份相同。**

拆成三条规则：

1. **规则 1：** 同一个 TU 里，同一个变量、函数、类、枚举、模板……至多一个定义。
2. **规则 2：** 被 ODR-use 的非 inline 函数和变量，整个程序里恰好一个定义。
3. **规则 3：** 类、枚举、inline 函数和变量、模板可以在每个 TU 里各定义一次，但各份必须相同。

判断一段代码守不守约，按顺序问三件事：

```text
这两个名字，是不是同一个实体？              ← 链接（linkage）
        │ 是
        ▼
这个实体有几个定义？                        ← 规则 1（TU 内）、规则 2（全程序）
        │ 不同 TU 里各有一个
        ▼
属于可以重复的那几类吗？各份相同吗？         ← 规则 3
```

| 实体 | 定义放哪 | 全程序里的定义数 |
|---|---|---|
| 非 inline 函数和变量（外部链接） | 一个 .cpp | 恰好 1 个（被 ODR-use 时） |
| 类、枚举、inline 函数和变量（C++17）、模板 | 头文件 | 每个需要它的 TU 各 1 个，各份相同，是同一个实体 |
| 内部链接的名字（`static`、匿名命名空间、命名空间作用域的 `const`） | 头文件或 .cpp | 每个 TU 各有一个**不同**的实体，互不相干 |

## How

### i. 先看是不是同一个实体

链接决定不同 TU 里的同名声明指的是不是同一个实体：

- 外部链接：是同一个实体。
- 内部链接和无链接：不是。内部链接的实体只属于它所在的 TU，别的 TU 里的名字指不到它。

所以“全程序有几个定义”只对外部链接的名字有意义。头文件里写 `static int x = 0;`，每个 TU 得到自己的一个 `x`，两个 .cpp 里各有一个定义也不冲突；写 `int x = 0;`，就是同一个实体被定义了两次。

### ii. 规则 1 和规则 2：定义的数量

**规则 1：** 违反是编译错误。头文件保护宏（`#ifndef`、`#pragma once`）解决的正是它：防止同一个头文件在一个 TU 里展开两次。

**规则 2：** 没有定义，链接通常报 undefined reference；定义多了，通常报 multiple definition。头文件保护宏管不了它：保护宏只管一个 TU，管不了每个 TU 各展开一次。inline 函数和变量不在这条之内，它们要求每个 ODR-use 它的 TU 里都能看到定义。

只有被 ODR-use 的实体才需要定义。**ODR-use：这次使用需要的是实体本身，而不只是它的值。** 调用函数、取地址、绑定引用、读写变量都是；`sizeof(x)`、`decltype(x)` 这类不求值的写法，以及只取编译期常量的值，不是。典型实现里有个简单的判断办法：ODR-use 会让目标文件出现对该实体的符号引用，定义不在本 TU 时，在 `nm` 里就是 `U`，要靠链接去找它的定义。

```cpp
struct A { static const int value = 10; };
int read()        { return A::value; }   // 只取值：不是 ODR-use，目标文件里没有 A::value
const int& bind() { return A::value; }   // 绑定引用：ODR-use，nm 里出现 U A::value，需要类外定义
```

（g++ 11.4、clang 14，`-O0` 实测。ODR-use 在推迟初始化里的用法见 [process-memory-layout-and-startup-review.md](../02-class-object-oop/process-memory-layout-and-startup-review.md) §2.3.3，静态数据成员的定义见 [static.md](static.md)。）

### iii. 规则 3：重复的定义必须相同

可以每个 TU 各有一个定义的实体：类、枚举、inline 函数和变量、模板化实体（另有默认实参、默认模板实参）。它们的共同点是，**编译时必须在使用的地方看到完整定义**，所以每个用到它们的 TU 都得有一份：

| 实体 | 使用点为什么要看到定义 |
|---|---|
| 类、枚举 | `sizeof`、成员访问、继承都要知道布局和成员 |
| inline 函数 | 内联展开要看到函数体。N0369 就是这样解释函数为什么允许多处定义的 |
| 模板 | 实例化要看到模板定义（按条文推的，标准没把它写成理由） |
| inline 变量（C++17） | 把 inline 函数的这项权利给了变量，头文件里才能放变量定义 |

> **Note:** 类内定义的成员函数、`constexpr` 函数、`constexpr` 静态数据成员（C++17）隐含 inline，同样享有这项许可。函数模板不是隐含 inline，它凭“模板化实体”的身份在清单上。

“各份相同”的判据是机械的，不比较行为：

1. **记号序列相同**，在预处理之后比较。空白、注释不影响；宏展开后不同，就是不同。
2. **名字所指相同**：每个名字查找之后，必须指向同一个实体。记号一样，名字却指向各 TU 里不同的实体，也算违约。

```cpp
// common.h
inline int get_value() { return helper(); }

// a.cpp
namespace { int helper() { return 10; } }   // 匿名命名空间：内部链接，每个 TU 各有一个 helper
#include "common.h"                          // 记号相同，但 helper 指向 a.cpp 里的那个

// b.cpp
namespace { int helper() { return 20; } }   // 另一个 helper
#include "common.h"                          // 名字所指不同：违反规则 3
```

不比较“行为是否等价”，是因为语义等价没法机械判断：N0716 说越定义越陷进去（quagmire），N0369 说它本质上不可判定。

满足条件后，程序的行为**如同只有一个实体、一个定义**：inline 函数的地址在所有 TU 相同，其中的 `static` 局部变量只有一份。N0716 对这条规则用意的说法是：让系统能从任何一份现有的定义里得到“真正的”定义。典型实现靠弱符号做到：这些实体在每个 .o 里各有一份，符号是弱的（`nm` 里的 `W`、`V`；g++ 的 inline 变量是 `u`），链接器只留一份，不比较内容，留哪份都行。所以各份必须相同：链接器留哪份，程序都得是对的。

### iv. 违约的后果

| 违反 | 标准怎么说 | 通常看到 |
|---|---|---|
| 规则 1 | 必须诊断 | 编译错误 |
| 规则 2 | ill-formed，无需诊断 | 链接报 undefined reference 或 multiple definition；`-O2` 下常量被折叠时可能什么都不报（见 [static.md](static.md)） |
| 规则 3 | C++11 到 C++17：行为未定义。C++20 起：ill-formed，无需诊断（CWG 2300；附属于命名模块的实体除外） | 编译、链接都通过；结果取决于链接器留了哪份、编译器内联了哪份（见 Example） |

“无需诊断”不等于没事：标准规定，违反了一条无需诊断的规则，标准对实现处理这个程序**不再有任何要求**。所以规则 3 的措辞从“未定义行为”改成 ill-formed，并没有变轻。

工具只能抓一部分：g++ 开启链接时优化（`-flto`）后能报类定义不一致（`-Wodr`，g++ 11.4 实测）；普通编译链接，以及 clang 14 的 `-flto`，都没有提示。

## Relation

### i. Issues

两个 .cpp 都 include 同一个头文件，头文件里不同写法的结果（g++ 11.4、clang 14，C++17 实测）：

| 头文件里的写法 | 结果 |
|---|---|
| `int g = 1;`、`int f() { return 1; }` | 链接报 multiple definition |
| `struct S { int f(); };` 加 `int S::f() { return 1; }` | 链接报 multiple definition |
| `inline int g = 1;`、`inline int f() { return 1; }`、`inline int S::f() { return 1; }` | 通过；全程序一个实体 |
| `static int g = 1;`、`const int g = 1;`、匿名命名空间里的 `int g = 1;` | 通过；每个 TU 各一个，地址不同 |
| `extern int g;`，加某个 .cpp 里的 `int g = 1;` | 通过；全程序一个实体 |
| 类、枚举、模板的定义 | 通过 |

- **以为头文件保护宏能防重复定义。** 它只防规则 1，防不了规则 2。
- **以为 `inline` 只是“请求内联”。** 编译器是否内联不由它决定，它现在主要意味着允许多处定义（见 Note）。
- **同一个头文件，在不同 TU 里展开成不同的记号。** 宏、条件编译都能做到：头文件相同不等于定义相同（见 Example）。
- **inline 函数引用了 TU 私有的名字。** 匿名命名空间或 `static` 的函数、变量，被头文件里的 inline 函数用到，名字所指就因 TU 而异。

### ii. Connection

- 普通函数和变量：头文件里只放声明，定义放进一个 .cpp；C++17 起也可以在头文件里用 `inline` 变量。Core Guidelines SF.2 把它写成了规则：头文件不得包含对象定义或非 inline 函数定义，能放的是类定义、模板、inline 函数定义、`constexpr` 和 `const` 定义、声明。
- 类、模板、inline 函数：定义放头文件，并保证每个 TU 展开出的记号一致，不要让宏或条件编译改变它。
- 只想给一个 TU 用的东西：放进匿名命名空间（或 `static`），让它成为各 TU 自己的实体，不进入跨 TU 的 ODR 约束。

**See also:**

- [lifetime-and-initialization.md](../02-class-object-oop/lifetime-and-initialization.md)：“some concepts”里的 declaration、definition 和 Linkage。
- [process-memory-layout-and-startup-review.md](../02-class-object-oop/process-memory-layout-and-startup-review.md)：ODR-use 在推迟初始化里怎么用（§2.3.3）。
- [static.md](static.md)：类的静态数据成员的定义，`inline static`。

## Example

```cpp
// common.h
inline int scale(int x) { return x * FACTOR; }

// a.cpp
#define FACTOR 2
#include "common.h"
int from_a() { return scale(10); }

// b.cpp
#define FACTOR 3
#include "common.h"         // 同一个头文件，预处理后 scale 的记号不同：违反规则 3
int from_b() { return scale(10); }

// main.cpp
#include <cstdio>
int from_a();
int from_b();
int main() { std::printf("from_a=%d from_b=%d\n", from_a(), from_b()); }
```

构建：`g++ -std=c++17 -Wall -Wextra -pedantic-errors a.cpp b.cpp main.cpp`，分别用 `-O0`、`-O2`，每种再把 `a.cpp b.cpp` 换成 `b.cpp a.cpp`（改变链接顺序）。编译和链接都没有任何提示。

Output:

```text
-O0, a.cpp b.cpp:  from_a=20 from_b=20
-O0, b.cpp a.cpp:  from_a=30 from_b=30
-O2, 两种顺序:     from_a=20 from_b=30
```

Verifies:

- 编译器和链接器都不提示：规则 3 没人检查。
- `-O0` 下两个 TU 调用的是同一个 `scale`（两个 .o 里都是弱符号，链接器只留先出现的那份），结果由链接顺序决定：同一份源码，换个顺序结果就变。
- `-O2` 下各 TU 把自己的那份内联掉，结果又变成 20/30：优化级别也会改变结果。所以不能靠“跑出来对不对”判断有没有违约。
- 违约来自预处理之后的记号不同，不是头文件本身有问题：把两个 .cpp 里的 `FACTOR` 定义成同一个值，程序就合法了，两边都得 20。

环境：g++ 11.4、clang++ 14，`-std=c++17` 与 `-std=c++20`，输出相同。

## Note

### i. 演进

| 时间 | 变化 |
|---|---|
| 标准成文前（1994–1995） | WG21 的文件已在专门讨论 ODR：N0369 说明 C++ 为什么要在 C 的“一个定义”之外允许重复定义；N0716 提出“如同只有一个定义”、“记号相同加名字相同”两条判据、违约不要求诊断 |
| C++11 | 移除模板的 `export`（C++03 里有效，允许同一个模板在别的 TU 里只声明、不定义；附录 C 给的理由是 No implementation consensus），模板只剩“定义放头文件”一条路；`constexpr` 函数隐含 inline |
| C++17 | inline 变量，允许多处定义的清单加上它；`constexpr` 静态数据成员隐含 inline |
| C++20 | 模块：附属于命名模块的定义不在许可之内，重复定义时只要先前的定义可达就必须诊断；规则 3 违约的措辞由“行为未定义”改为“ill-formed，无需诊断”（CWG 2300） |
| 现行草案 | 把“可以被定义的东西”统称 definable item；“恰好一个定义”拆成“被 ODR-use 的至少一个”加“非 inline 非模板的函数或变量有多处定义则 ill-formed” |

更早的来历（ARM 时代，1990 年前后）没有查到可核对的原文，这里不写。

### ii. inline 含义的变迁

`inline` 原本是对优化的请求。N0369 解释了 C++ 为什么要允许函数多处定义：函数的定义在 TU 里可见时才可能内联；要在多个 TU 里内联，每个 TU 就得有定义。所以“允许多处定义”原本是为内联服务的。

后来内联与否由编译器自己判断。条文从 C++11 到现行草案都写着同一段话：`inline` 表示倾向内联替换，但实现不要求这样做，即使不做，inline 函数的其他规则仍须遵守。标准语义里可观察的，只剩 ODR 这一层。cppreference 的概括是：从 C++98 起，函数的 `inline` 就意味着“允许多处定义”，而不是“倾向内联”；C++17 把这层意思原样给了变量，变量谈不上内联展开，`inline` 对它就只有这一个意思。

## Reference

- cppreference：[Definitions and ODR](https://en.cppreference.com/w/cpp/language/definition)；[inline specifier](https://en.cppreference.com/w/cpp/language/inline)（`inline` 含义的变迁、inline 变量）；[export](https://en.cppreference.com/w/cpp/keyword/export)（C++11 前的模板 `export`）
- WG21 早期文件：
  - [N0369](https://open-std.org/JTC1/sc22/wg21/docs/papers/1993/N0369.asc)（1994-01-27，John Max Skaller，The “ONE DEFINITION RULE”）：C 与 C++ 的对照、为什么允许重复定义、哪些部分无法诊断
  - [N0716](https://open-std.org/Jtc1/sc22/wg21/docs/papers/1995/N0716.htm)（1995，Jerry Schwarz，The One Definition Rule）：ODR 的意图、“如同只有一个定义”、记号相同和名字相同、违约不要求诊断
- [CWG 2300](https://cplusplus.github.io/CWG/issues/2300.html)：Lambdas in multiple definitions，2019 年 7 月作为缺陷报告接受，同时把规则 3 违约的后果由“行为未定义”改为“ill-formed，无需诊断”
- C++ standard (draft)，段号按 C++20（N4861），现行草案的编号有变：
  - [lex.separate](https://eel.is/c++draft/lex.separate)：/1 的注，程序不必一次翻译完
  - [basic.link](https://eel.is/c++draft/basic.link)：/2 外部链接、内部链接、无链接各自能指到哪
  - [basic.def.odr](https://eel.is/c++draft/basic.def.odr)：/1 规则 1；/10 规则 2（被 odr-use 的非 inline 函数和变量恰有一个定义，无需诊断）；/11 inline 函数和变量的定义要可达；/13 允许多处定义的清单和各份要满足的要求；/14 如同只有一个实体；/15 不满足时 ill-formed
  - [dcl.inline](https://eel.is/c++draft/dcl.inline)：/2 内联替换只是倾向、其他规则仍须遵守；/6 的注：inline 函数和变量的地址、`static` 局部变量、类型在各 TU 相同
  - [dcl.constexpr](https://eel.is/c++draft/dcl.constexpr)：/1 `constexpr` 函数和静态数据成员隐含 inline
  - [intro.compliance](https://eel.is/c++draft/intro.compliance)：/2 的第三条，违反无需诊断的规则时，标准对实现不再有要求
  - [diff.cpp03.temp](https://eel.is/c++draft/diff.cpp03.temp)：移除 `export`
  - 版本差异（N3337 = C++11，N4659 = C++17，现行草案 2026-11 取）：N3337 [basic.def.odr]/3 规则 2、/5 允许多处定义的清单（没有 inline 变量，末尾写“行为未定义”）；N4659 /6 清单加上 inline 变量，仍写“行为未定义”；现行草案 /1 definable item、/12 至少一个定义、/16 非 inline 非模板的函数或变量有多处定义则 ill-formed
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：[SF.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-inline) A header file must not contain object definitions or non-inline function definitions；[SF.8](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rs-guards) Use `#include` guards for all header files
