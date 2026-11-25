# Constructors and Destructors

> 范围：类对象的构造——构造函数、成员和基类的初始化、初始化顺序、委托构造函数、`explicit`。析构只写了销毁顺序，析构函数本身、拷贝与移动待写。初始化的概念和分类见 [01-initialization-basics.md](../01-object-foundation/01-initialization/01-initialization-basics.md) §1、§2，拷贝、直接、列表初始化的规则见 [02-initialization-forms.md](../01-object-foundation/01-initialization/02-initialization-forms.md)。条文号以 C++20（N4861）为准，现行草案不同的另注；示例在 g++ 11.4、clang 14 的 C++11–20 下实测，另注除外。

## 1. 构造函数

**构造函数**（constructor）用来初始化它所属类的对象（[class.ctor]/2）。构造一个类对象，先按固定顺序（第 3 节）初始化基类和非静态数据成员（初始化器从哪来，见第 2 节），再执行函数体。委托构造函数（第 4 节）把这些交给另一个构造函数；`explicit`（第 5 节）决定哪些初始化写法能选中某个构造函数。

- 非聚合类的默认、值、直接、拷贝、列表初始化，通常都靠选中并调用一个构造函数完成：写法决定候选集合，重载决议选出被调用的那个，规则见 forms。构造函数调用本身不是一种初始化种类。
- 聚合类用花括号列表初始化时做聚合初始化（C++20 起，没有可行构造函数时圆括号列表也可以）：按聚合元素（成员，C++17 起还有基类）的顺序逐个初始化，不调用该类自己的构造函数；元素是类类型时，仍调用元素自己的构造函数。

```cpp
struct Member { Member(int); };
struct A { Member m; };
A a{10};    // 聚合初始化：m 由 10 拷贝初始化，调用 Member(int)
```

## 2. 成员和基类的初始化

### 2.1 默认成员初始化器

**默认成员初始化器**（default member initializer，C++11）是非静态数据成员声明处的 brace-or-equal-initializer，写成 `= 初始化子句` 或花括号列表：`int z = 30;`、`int z{30};`（[class.mem]/10，现行草案 [class.mem.general]/15）。没有圆括号形式：`int z(30);` 会被当成函数声明而出错。静态数据成员的类内初始化是另一回事，见 [static-data-members.md](static-data-members.md)。

### 2.2 成员初始化列表

**成员初始化列表**（mem-initializer-list）是构造函数定义里 `:` 后的部分，可以为基类和非静态数据成员各写一个 mem-initializer：`名字(表达式列表)` 或 `名字{...}`，按直接初始化（花括号形式是直接列表初始化）进行（[class.base.init]/1、/7）。

非委托构造函数里，每个基类和成员按下表初始化（[class.base.init]/9、/10）；没有成员初始化列表，等于每一项都“没写”：

| 情形 | 初始化方式 |
|---|---|
| 列表里写了 | 按列表里的初始化器；成员的默认成员初始化器被忽略 |
| 没写，成员有默认成员初始化器 | 用默认成员初始化器（联合体里，只在没有别的变体成员被指定时） |
| 其余没写的 | 默认初始化：内置类型什么也不做，值不确定；联合体和匿名联合的变体成员完全不初始化 |

- 默认初始化行不通的，必须在列表里写：引用成员；不是 const-default-constructible 的 `const` 成员（如 `const int`）；没有可用默认构造函数的成员和基类。成员也可以改写默认成员初始化器，基类不行。`const std::string s;` 这样 const-default-constructible 的成员可以不写（见 [basics](../01-object-foundation/01-initialization/01-initialization-basics.md) §2.2）。
- 在函数体里给成员赋值不是初始化：成员先默认初始化，再赋值，两步；写在列表里一步完成。
- `x(n)` 是直接初始化，`x{n}` 是直接列表初始化，后者禁止窄化转换（[dcl.init.list]/7）；n 不是常量表达式时，clang 报错，g++ 只警告。

## 3. 初始化顺序与销毁顺序

非委托构造函数按下面的顺序初始化，只看声明，不看列表的书写顺序（[class.base.init]/13，现行草案 /15）：

1. 虚基类：只有最派生类的构造函数初始化它们，按基类图的深度优先、从左到右；
2. 直接基类：按基类列表里的顺序；
3. 非静态数据成员：按类里的声明顺序；
4. 构造函数体。

析构时先执行析构函数体，再按构造完成的相反顺序销毁成员和基类（[class.dtor]/14，现行草案 /13）。声明顺序固定，就是为了让销毁顺序恰是初始化顺序的逆序（[class.base.init]/13 的注）。所以成员之间有依赖时，被依赖的先声明；写反了编译器会警告（g++：`-Wreorder`，clang：`-Wreorder-ctor`）。

```cpp
struct A {
    int x;
    const int y;                       // const int 成员：必须在列表里初始化
    int z = 30;                        // 默认成员初始化器
    A(int n) : z(n), y(20), x(10) {}   // 书写顺序 z、y、x，实际顺序 x、y、z，然后函数体；z 的默认值被忽略
};
```

## 4. 委托构造函数

**委托构造函数**（delegating constructor，C++11）：成员初始化列表里写本类名，且只能写这一项。它选中的构造函数叫**目标构造函数**（target constructor），由重载决议选出；先执行目标构造函数，返回后才执行委托者的函数体。直接或间接委托给自己，ill-formed，不要求诊断（[class.base.init]/6）。

抛出异常时：

- 非委托构造函数（包括被委托的目标构造函数）抛出：对象自己的析构函数不调用，只按构造的相反顺序销毁已完成初始化的子对象（[except.ctor]/3）。
- 目标构造函数已返回，委托者的函数体抛出：调用对象的析构函数（[except.ctor]/4）。

与此一致，对象的初始化在非委托构造函数返回时就算完成（CWG 2756，DR；现行草案 [class.base.init]/11 写明，C++20 原文没有这一句），所以对象的生命周期可以在所有委托构造函数完成之前开始（[basic.life]）。两条异常规则在 g++ 11.4、clang 14 的 C++11、C++20 下实测一致。

## 5. explicit

### 5.1 含义

**转换构造函数**（converting constructor）：不是 `explicit` 的构造函数（`explicit(false)` 也算，见 5.5），它指定从形参类型到本类类型的转换（[class.conv.ctor]/1）。C++11 起不限于单参数；不是 `explicit` 的拷贝和移动构造函数也是转换构造函数（/3）。

`explicit` 只能写在类内的构造函数和转换函数（转换函数 C++11 起）的声明上，使它不能用于隐式转换（[dcl.fct.spec]/3；推导指引也可以带，本篇不讲）。依据是隐式转换的定义：表达式 e 能隐式转换成 T，当且仅当 `T t = e;` 良构（[conv]/3）；而 `static_cast<T>(e)` 这样的显式转换，结果对象按直接初始化从 e 构造（[expr.static.cast]/4）。所以拷贝初始化不考虑 `explicit` 的构造函数和转换函数，它们只在直接初始化和显式转换里能用（[class.conv.ctor]/2 的注、[class.conv.fct]/2）；拷贝列表初始化考虑 `explicit` 构造函数，选中则 ill-formed（5.2）。

### 5.2 对各种写法的影响

下表的 E 是 `struct E { explicit E(int); };`，A 是 `struct A { E e; };`：

| 写法 | `explicit` 构造函数 | 依据 |
|---|---|---|
| 直接初始化 `E x(1);`、`new E(1)`、mem-initializer `: m(1)` | 是候选 | [over.match.ctor]/1 |
| 函数式转换 `E(1)`、`static_cast<E>(1)`、`(E)1` | 是候选（按直接初始化） | [expr.type.conv]/2、[expr.static.cast]/4、[expr.cast]/4 |
| 直接列表初始化 `E x{1};` | 是候选 | [over.match.list]/1 |
| 拷贝初始化 `E x = 1;`、传参、返回、默认成员初始化器 `E m = 1;` | 不是候选 | [over.match.ctor]/1、[over.match.copy]/1 |
| 拷贝列表初始化 `E x = {1};`、`g({1})`、`return {1};` | 是候选，选中则 ill-formed | [over.match.list]/1 |
| 聚合的元素（`A a{1};`）、数组元素、`std::initializer_list` 的元素 | 这些都是拷贝初始化：不是候选 | [dcl.init.aggr]/4、[dcl.init.list]/5 |
| C++20 的圆括号聚合初始化 `A a(1);`（g++ 11.4 实测，clang 14 不支持） | 元素是拷贝初始化：不是候选 | [dcl.init]/17.6.2 |

“不是候选”与“选中则 ill-formed”机制不同，再加一个非 `explicit` 的重载就看得出来（[over.match.list]/1 的注）：

```cpp
struct S { explicit S(int); S(long); };
S s1 = 1;      // 拷贝初始化：候选只有转换构造函数 S(long)，被选中
S s2(1);       // 直接初始化：候选是全部构造函数，S(int) 更匹配
S s3{1};       // 直接列表初始化：同上，选中 S(int)
S s4 = {1};    // 拷贝列表初始化：候选也是全部构造函数，选中 S(int)；它是 explicit，ill-formed，不会改选 S(long)
```

### 5.3 explicit 的拷贝、移动与默认构造函数

**拷贝、移动构造函数**可以是 `explicit`（它因此不是转换构造函数）：直接初始化仍可用，拷贝初始化不能。同类纯右值的规则见 [forms](../01-object-foundation/01-initialization/02-initialization-forms.md) How i.3。

```cpp
struct X { X(); explicit X(const X&); };
X a;
X b(a);        // 直接初始化：可以
X c = a;       // 拷贝初始化，源是同类左值：候选只有转换构造函数，错误
void h(X);     // h(a)、按值返回 return a; 同样错误
X d = X();     // C++17 起可以（同类纯右值，不查构造函数）；C++11/14 错误
```

**默认构造函数**可以是 `explicit`，仍用于默认初始化和值初始化（[class.conv.ctor]/2 的注）。但 `= {}` 是拷贝列表初始化，空列表要做值初始化（[dcl.init.list]/3.5），其中的默认初始化只考虑转换构造函数（N4861 [over.match.ctor]/1；现行草案改为考虑全部构造函数、选中 `explicit` 的才 ill-formed，此例结果相同），所以：

```cpp
struct Z { explicit Z() = default; int a; };
Z z1;                 // 默认初始化：可以
Z z2{};               // 直接列表初始化，值初始化：可以
Z z3 = Z();           // 可以：Z() 是直接初始化，z3 由同类纯右值初始化
Z z4 = {};            // 错误：拷贝列表初始化
struct W { Z z; };    // W 是聚合
W w{};                // 错误：没有显式初始化的元素 z 要从 {} 拷贝初始化（[dcl.init.aggr]/5）
```

出处是 CWG 1518（P0398R0，N4659 起；cppreference 标为追溯适用于 C++11）。

### 5.4 explicit 的转换函数

**转换函数**（conversion function）是形如 `operator T()` 的成员函数，把类对象转换成 T（[class.conv.fct]/1）。它也可以是 `explicit`（C++11 起），此时**只在直接初始化里作为用户定义转换考虑**（/2）：

```cpp
struct B { explicit operator bool() const; };
B b;
bool x = b;                      // 拷贝初始化：错误
bool y(b);                       // 直接初始化：可以
bool z{b};                       // 直接列表初始化：可以
bool v = {b};                    // 拷贝列表初始化：错误
bool w = static_cast<bool>(b);   // static_cast 按直接初始化：可以
if (b) {}                        // 上下文转换为 bool：可以
int n(b);                        // 错误：explicit operator bool 返回 bool，不是 int
```

- **上下文转换为 `bool`**：e 能被这样转换，当且仅当 `bool t(e);` 良构（[conv]/4）。所以 `if`、`while` 的条件，`!`、`&&`、`||` 的操作数，`?:` 的第一个操作数，`explicit operator bool` 都能用。
- **直接初始化非类类型**：`explicit` 转换函数只有“返回目标类型，或能经限定转换变成目标类型”才是候选（[over.match.conv]/1），所以 `int n(b);` 不行：`bool` 到 `int` 是整型提升。
- **目标是类类型 C**：转换函数是 `explicit operator C() const` 时，`C c(d);` 可以，`C c = d;` 不行（[over.match.copy]/1 特别把这种情形里的 `explicit` 转换函数也列为候选；标准的例子在 [class.conv.fct]/2）。

### 5.5 条件 explicit 与标准库

C++20 起 `explicit` 可以带常量表达式：`explicit(expr)`，值为 false 时函数不是 `explicit`；不带表达式的 `explicit` 等价于 `explicit(true)`（[dcl.fct.spec]/4，P0892R2）。标准库用它表达“能隐式转换才隐式”，如 `std::pair` 的构造函数是 `explicit(see below)`（[pairs.pair]）。库里常见的 `explicit` 构造函数（E 同 5.2）：

```cpp
std::unique_ptr<int> p1 = new int;      // 错误：unique_ptr(pointer) 是 explicit（[unique.ptr.single.ctor]）
std::unique_ptr<int> p2(new int);       // 正确
std::pair<E, int> pr1 = {1, 2};         // 错误：int 不能隐式转换成 E，pair 的这个构造函数是 explicit
std::pair<E, int> pr2{1, 2};            // 正确
```

## 6. 验证与关联

- 验证：[lifetime-and-initialization-exp.cpp](https://github.com/YLoveH-0531/cpp-knowledge/blob/13499b4/02-class-object-oop/lifetime-and-initialization-exp.cpp) 实验 1 验证初始化与销毁的顺序（故意触发 `-Wreorder`）。该文件已从仓库删除，链接指向删除前的提交 13499b4。
- 关联：
  - [lifetime.md](../01-object-foundation/lifetime.md)：对象的生命周期从初始化完成时开始；委托构造函数的情形见第 4 节。
  - [static-data-members.md](static-data-members.md)：静态数据成员不是对象的子对象，不走构造函数和成员初始化列表。
  - [03-static-initialization-and-program-startup.md](../01-object-foundation/01-initialization/03-static-initialization-and-program-startup.md)：全局对象的构造函数是 `constexpr`、且整个初始化是常量表达式时，做常量初始化，不必在运行时调用构造函数，见 §2.2 里的 `gc` 例子。
