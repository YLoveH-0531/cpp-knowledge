# Static Data Members

> 范围：只讲类的静态数据成员。静态成员函数、函数内 `static`、`thread_local` 暂不展开。

## Why

类里有些数据属于整个类，而不属于某个对象：账户利率、对象计数器、类级常量。

没有静态成员时，这类数据只能放进全局变量。存储没问题（整个程序一份），但名字落在全局作用域，容易冲突；没有访问控制，谁都能改；从语法上也看不出它属于哪个类。

静态数据成员就是这个问题的答案：**存储不变，名字收进类里。**

## What

**静态数据成员 = 名字在类作用域里的全局变量。**

| 面向 | 像谁 | 表现 |
|---|---|---|
| 存储与生命周期 | 全局变量 | 静态存储期，整个程序一份；不是对象的子对象，不计入 `sizeof`；没有对象时也存在 |
| 名字与访问 | 类成员 | 类作用域，写成 `X::m`，不需要对象；受 `public`/`protected`/`private` 约束 |

> **Note:** 类内的 `static` 表示“不属于对象”，不改变链接：静态数据成员的链接跟随它的类。命名空间作用域的 `static` 是另一回事，它给名字内部链接，头文件里写 `static int x;` 会让每个翻译单元各得一份。同一个关键字，两种用途。

## How

### i. 声明与定义

**非 inline 的静态数据成员，类内是声明，不是定义；定义要写在类外，整个程序只能有一个。**

```cpp
// account.h
class Account {
public:
    static double rate;           // 声明
};

// account.cpp
double Account::rate = 0.04;      // 定义：整个程序只有这一处
```

原因有两层：

1. 它是一个真正的对象，要有自己的存储。非 inline 的变量，被 odr-use 时整个程序必须恰好有一个定义。
2. 类定义写在头文件里，会被很多翻译单元包含。如果类内就是定义，每个翻译单元都定义一次，链接时重复定义。Stroustrup 的说法：为了避免复杂的链接规则，C++ 要求每个对象有唯一的定义；若允许在类内定义需要存放在内存里的对象，这条规则就破了。

对照非静态成员：它没有这个问题，因为它只描述“每个对象里有这么一块”，存储随对象创建，不是一个独立存在的对象。

### ii. 类内初始化的演进

上面的规则带来一个麻烦：类级常量（数组长度、`case` 标签）只想在类里写个值，却要另外到 .cpp 里再定义一次。后面的演进，都围绕这件事。

| 版本 | 写法 | 类内是什么 | 取地址（odr-use）时 |
|---|---|---|---|
| enum 技巧（至今可用） | `enum { kMax = 100 };` | 枚举常量，不是变量 | 不能取地址 |
| C++98 | `static const int kMax = 100;`（只限整型、枚举） | 声明，附一个常量值 | 类外要有定义（不带初始化器） |
| C++11 | `static constexpr double kPi = 3.14;`（字面类型） | 同上 | 同上 |
| C++17 | `inline static int count = 0;`；`static constexpr` 自动 inline | 定义 | 不需要类外定义 |

（后三行已在 g++ 11.4、clang 14 上按 C++98–20 逐版本编译核对。）

- **绕开（前三行）**：只要用的是值，不需要对象本身（不取地址、不绑定引用，也就是没有 odr-use），编译器直接用这个常量，不需要存储，所以类内写值不违反“唯一定义”；一旦需要对象，还是要类外定义。能写在类内的，限于本质上是编译期常量值的东西：C++98 只有整型和枚举，C++11 的 `constexpr` 把编译期常量扩到字面类型，所以 `double` 才能写。
- **根治（C++17）**：inline 变量沿用 inline 函数的规则：同一个变量可以在多个翻译单元里定义，只要定义完全相同，它们就是同一个实体。类内因此可以直接定义，头文件里也能放变量定义了。此前想这样做，只能借助模板（类模板的静态成员允许多处定义）或函数内的 `static` 来绕。
- **两个别混的点**：`static const` 不会自动 inline（`const int` 取地址仍要类外定义）；C++11/14 风格的类外 `constexpr double S::kPi;` 在 C++17 起仍合法，但已弃用，可以删。

### iii. 初始化与销毁

静态数据成员按**非局部变量**的规则初始化和销毁，不是第一次访问时才初始化（函数内的 `static` 才是）：

1. 先做静态初始化，早于所有动态初始化：初始化器是常量表达式，就是常量初始化（上一节的类内常量属于这类，没有顺序问题）；定义里没写初始化器，就是零初始化，所以 `int Account::n;` 是 0，不像局部变量那样值不确定。
2. 其余是动态初始化：在程序启动阶段完成（实现可以推迟，细节见 [process-memory-layout-and-startup-review.md](../02-class-object-oop/process-memory-layout-and-startup-review.md) §2.3.2）。普通的非 inline 成员，同一翻译单元内按定义顺序，**跨翻译单元的顺序没有保证**；inline 成员是“部分有序”，类模板的静态成员是“无序”（见该文 §2.3.1 的三档）。
3. 销毁：程序退出时，与初始化完成的顺序相反。

## Relation

### i. Issues

- **误以为每个对象一份。** 静态数据成员整个程序只有一份，所有对象共用。
- **把 `static` 的两种意思混了，或把定义写错地方。** 两个 .cpp 各 include 同一个头文件，结果如下（g++ 11.4、clang 14，C++17 实测；第三列是 `nm` 在各自 .o 里看到的符号）：

| 头文件里的写法 | 结果 | 符号（`nm`） |
|---|---|---|
| `static int c;`（命名空间作用域） | 每个翻译单元各有一份：地址不同，一边改了另一边看不到 | 两个 .o 里各一个局部符号（`b`） |
| `struct S { static int c; }; int S::c = 0;`（定义写进头文件） | 链接报重复定义 | 两个 .o 里都是全局符号（`B`） |
| `struct S { static int c; };`，其中一个 .cpp 里写 `int S::c = 0;` | 整个程序一份 | 一个 .o 里是 `B`，另一个是 `U`（未定义，交给链接器去找） |
| `struct S { inline static int c = 0; };`（C++17） | 整个程序一份，不需要 .cpp | 两个 .o 里都放在 COMDAT 组里，是弱符号（clang `V`；g++ 是 GNU unique，显示 `u`），链接器只留一份 |

- **漏写类外定义。** 只有被 odr-use 时才会链接失败：取地址、绑定引用（如 `std::max(S::kMax, x)`）；只读取值不会。缺定义时报不报错还取决于优化（见 Example），不能靠它判断。
- **可变的静态成员就是全局变量**，带着全局变量的毛病：隐藏的耦合、跨翻译单元的初始化顺序、多线程下的数据竞争。

### ii. Connection

- 类级常量用 `static constexpr`（C++17 起自动 inline，类内写完即可）；需要可变的共享变量用 `inline static`（C++17），不用再到 .cpp 里补定义。
- 可变的共享状态，能放进对象或作为参数传入就不要用静态成员；非用不可时，初始化里别依赖别的翻译单元里的全局变量（Core Guidelines I.2、I.22）。

**See also:**

- [lifetime-and-initialization.md](../02-class-object-oop/lifetime-and-initialization.md)
  - 「storage class specifiers」「Storage duration」「Linkage」三节：`static` 是存储类说明符，同一个关键字在不同位置起不同作用：给变量静态存储期；在命名空间作用域还给内部链接；在类里表示不属于对象，链接跟随类（[dcl.stc]/4）。Linkage 里的另一条可以对照：命名空间作用域的 `const` 变量默认是内部链接，头文件里的 `const int kMax = 100;` 每个翻译单元各有一份（实测地址不同），不会重复定义；类内的 `static const` 链接随类（外部），取地址就要有唯一的类外定义。
  - 「declaration」一节说“一部分声明同时也是定义”：类内的非 inline 静态成员是“声明但不是定义”的典型（[basic.def]/2），C++17 的 inline 改的正是这一条。
  - 「scope」一节的 ③、④：类外定义 `double Account::rate = 0.04;` 是“target scope 不一定等于 inhabited scope”的例子：它 inhabit 命名空间作用域，target scope 却是类 `Account`（[dcl.meaning.general]：带限定名的 declarator 取被限定的作用域；这套术语是现行草案的，N4861 里没有）。它属于类作用域，存储期却是静态的，正好印证 ④：“属于”与存储期无关。
  - §1 和「类对象的初始化」一节：静态成员不是对象的子对象，不走构造函数和成员初始化列表（写进列表直接编译报错，g++、clang 实测）。示例里 `Account() { ++count; }` 是赋值，不是初始化；类内的 `inline static int w = 30;` 也不是默认成员初始化器：后者每个对象构造时执行，前者全程序只做一次。
- [process-memory-layout-and-startup-review.md](../02-class-object-oop/process-memory-layout-and-startup-review.md)
  - §2.3.3：ODR-use 是同一条判据，用在两处：本篇用它判断要不要类外定义，这里用它定被推迟的动态初始化的最晚期限。`inline static` 成员是 §2.3.2 表里的 inline 变量，只看它自己的首次非初始化 ODR-use。
  - §2.3.1、§2.3：三档排序里，普通静态成员“有序”，`inline static`“部分有序”，类模板的静态成员“无序”；阶段 1 里，定义没写初始化器的静态成员是零初始化。
  - §2.4：Issues 里“跨翻译单元的初始化顺序”的解法是函数内 `static`（构造时间由调用决定）；C++17 之前想把变量的定义放进头文件，也是借 inline 函数里的 `static` 同一对象这一条。
  - §3.2、§3.3：示例里三个静态成员落在三个节：`rate`（0.04）在 `.data`，`count`（常量初始化成 0）在 `.bss`，`kNameMax`（const 且被取地址）在 `.rodata`，与 §3.3 “按值分”一致（链接后的可执行文件上用 objdump 实测，g++ 11.4、clang 14）。Issues 表第三列的符号类型，是链接器合并多个 .o 的依据。
  - §4.1：静态数据成员的整条生命线就在这条时间线上：静态初始化 → 动态初始化 → `main` → 退出时析构（§4.3），与任何 `Account` 对象的生命周期无关。

## Example

```cpp
#include <cstdio>

struct Account {
    double balance = 0;               // 对象级：每个对象一份
    static double rate;               // 类级：声明，不是定义
    static const int kNameMax = 32;   // 类内常量初始化器，仍然只是声明
    inline static int count = 0;      // C++17：类内就是定义
    char name[kNameMax];              // 当数组长度：只用值，不需要 kNameMax 的定义
    Account() { ++count; }
};

struct Plain { double balance; char name[32]; };   // 同样的对象级成员，没有静态成员

double Account::rate = 0.04;          // rate 的唯一定义
const int Account::kNameMax;          // 只有取地址时才需要；const 不会自动 inline，所以仍要写

static_assert(sizeof(Account) == sizeof(Plain), "static members take no space in objects");

int main() {
    std::printf("no object yet: rate=%g count=%d\n", Account::rate, Account::count);
    Account a, b;
    double* pa = &a.rate;             // a.rate 和 b.rate 指的是同一个对象
    double* pb = &b.rate;
    *pa = 0.05;
    std::printf("b.rate=%g same=%d count=%d\n", b.rate, pa == pb, Account::count);
    const int* p = &Account::kNameMax;   // odr-use：需要上面类外的那行定义
    std::printf("kNameMax=%d\n", *p);
}
```

Output:

```text
no object yet: rate=0.04 count=0
b.rate=0.05 same=1 count=2
kNameMax=32
```

Verifies:

- 还没有对象时，`Account::rate` 和 `Account::count` 已经存在、可以用（第一行）。
- `a.rate` 与 `b.rate` 是同一个对象（`same=1`）；`count` 是两个对象共用的一份（`count=2`）。
- 静态成员不占对象的空间：`static_assert` 通过。
- `kNameMax` 当数组长度不需要定义，取地址才需要：删掉类外那一行，`-O0` 下链接报 `undefined reference to 'Account::kNameMax'`；`-O2` 下 `*p` 被折叠成常量，不再报错（标准对此是“无需诊断”），所以不能靠链接是否通过来判断定义写没写。

环境：g++ 11.4、clang++ 14，`-std=c++17` 与 `-std=c++20`，`-Wall -Wextra -pedantic-errors` 无警告。

## Reference

- cppreference：[Static members](https://en.cppreference.com/w/cpp/language/static)；[inline specifier](https://en.cppreference.com/w/cpp/language/inline)（inline 变量，header-only 库）
- Stroustrup FAQ：[How do I define an in-class constant?](https://www.stroustrup.com/bs_faq2.html#in-class)：类内常量只限 `const` 整型/枚举、enum 技巧，以及“每个对象要有唯一定义”的原因
- C++ standard (draft)，段号按 C++20（N4861），现行草案的编号有变：
  - [class.static.data](https://eel.is/c++draft/class.static.data)：/1 不是子对象、整个程序一份；/3 类内是声明、没有对象时也存在；/4 类内初始化器（`const` 整型/枚举、inline）；/6 链接（注）；/7 按非局部变量初始化和销毁
  - [class.static](https://eel.is/c++draft/class.static)：/1 用 `X::s` 访问，不需要对象
  - [basic.def](https://eel.is/c++draft/basic.def)：/2 非 inline 静态成员的类内声明不是定义
  - [basic.def.odr](https://eel.is/c++draft/basic.def.odr)：/10 被 odr-use 的非 inline 变量恰有一个定义，无需诊断；/13 inline 变量可以在多个翻译单元定义
  - [dcl.constexpr](https://eel.is/c++draft/dcl.constexpr)：/1 `constexpr` 静态成员隐含 inline（C++17 起）
  - [depr.static.constexpr](https://eel.is/c++draft/depr.static.constexpr)：类外重声明 `constexpr` 静态成员已弃用
  - [basic.start.static](https://eel.is/c++draft/basic.start.static)：静态初始化早于动态初始化
  - [basic.start.dynamic](https://eel.is/c++draft/basic.start.dynamic)：/1 动态初始化的三档排序；/5、/6 推迟初始化的期限（非 inline、inline）
  - [dcl.stc](https://eel.is/c++draft/dcl.stc)：/4 `static` 给变量静态存储期，用在类成员上的作用见 [class.static]
  - [basic.link](https://eel.is/c++draft/basic.link)：/3 命名空间作用域的 `static` 变量和 `const` 变量是内部链接
  - [dcl.meaning.general](https://eel.is/c++draft/dcl.meaning.general)、[basic.scope.scope](https://eel.is/c++draft/basic.scope.scope)（现行草案，N4861 没有 inhabit、target scope 这套术语）：带限定名的 declarator 的 target scope；inhabit、target scope、belong 的定义
  - 版本差异（N3337 = C++11，N4659 = C++17）：[class.static.data]/3（N3337）`static constexpr` 字面类型成员可在类内初始化；[basic.def.odr]/5（N3337）允许多处定义的清单里没有 inline 变量，/6（N4659）加上了；[dcl.fct.spec]/4（N3337）inline 函数内的 `static` 局部变量是同一个对象
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：[I.2](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global) Avoid non-const global variables；[I.22](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ri-global-init) Avoid complex initialization of global objects
