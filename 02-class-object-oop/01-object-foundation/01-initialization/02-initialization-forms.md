# Initialization Forms

> 范围：拷贝初始化、列表初始化，以及各写法对 `explicit` 构造函数的不同待遇（`explicit` 本身的规则见 [constructors-and-destructors.md](../../02-class-and-oop/constructors-and-destructors.md) §5）；直接初始化作对照。默认、值、零初始化见 [01-initialization-basics.md](01-initialization-basics.md) §2.2。聚合只讲列表初始化涉及的判定和元素初始化规则（C++20 的圆括号聚合初始化只提一句），拷贝省略只讲与拷贝初始化直接相关的规则；引用初始化、`auto` 推导、类模板实参推导不讲。条文以 C++20（N4861）为基线，版本差异见 Note；示例在 g++ 11.4、clang 14 的 C++11–20 下实测，另注除外。

## Why

同一个类对象有四种常见的初始化写法：

```cpp
T a = x;    // 拷贝初始化
T b(x);     // 直接初始化
T c{x};     // 直接列表初始化
T d = {x};  // 拷贝列表初始化
```

写法不同，候选的构造函数和适用的规则也不同：

```cpp
std::vector<int> a = 5;     // 错误：count 构造函数是 explicit
std::vector<int> b(5);      // 5 个元素，值为 0
std::vector<int> c{10, 5};  // 2 个元素：10、5
std::vector<int> d(10, 5);  // 10 个元素，值都是 5
```

`explicit` 构造函数在四种写法里的待遇也不同：

```cpp
struct E { explicit E(int); };

E a = 1;    // 错误：拷贝初始化不考虑 explicit 构造函数
E b(1);     // 正确
E c{1};     // 正确
E d = {1};  // 错误：explicit 构造函数参与选择，选中则 ill-formed
```

弄清这些现象要三件事：拷贝与直接初始化怎样选构造函数（i）；列表初始化怎样选构造函数（ii）；`explicit` 把哪些构造函数挡在哪些写法之外（见 constructors-and-destructors.md §5）。

## What

**写法和场合决定这是哪一种初始化；再结合目标类型和初始化器，决定候选的构造函数、`explicit` 构造函数的待遇、窄化转换是否允许，以及 initializer-list 构造函数是否优先。** 下表以 T 是非聚合类、初始化器是一个表达式 x 为前提：

| 写法 | 标准术语 | 候选的构造函数 | `explicit` 构造函数 | 窄化转换 | initializer-list 构造函数优先 |
|---|---|---|---|---|---|
| `T t = x;` | copy-initialization | 转换构造函数（源类型不是 T 或其派生类时，还有源类的转换函数） | 不是候选 | 允许 | 无 |
| `T t(x);` | direct-initialization | 全部构造函数 | 是候选 | 允许 | 无 |
| `T t{x};` | direct-list-initialization | 两步选择（ii.3） | 是候选 | 禁止 | 有 |
| `T t = {x};` | copy-list-initialization | 两步选择（ii.3） | 是候选，选中则 ill-formed | 禁止 | 有 |

- **转换构造函数**（converting constructor）：非 `explicit` 的构造函数（[class.conv.ctor]/1；`explicit(false)` 的也算），C++11 起不限于单参数。
- **initializer-list 构造函数**：第一个形参是 `std::initializer_list<E>`，或指向 cv 限定的 `std::initializer_list<E>` 的引用；没有其余形参，或其余形参都有默认实参（[dcl.init.list]/2）。

## How

### i. 拷贝初始化与直接初始化

#### 1. 发生场合

**拷贝初始化**（copy-initialization，[dcl.init]/15）：

- 变量定义里的 `=` 形式：`T x = a;`
- 按值传实参、按值返回
- 抛出和处理异常
- 聚合成员的初始化：普通聚合初始化里，显式初始化的元素由对应子句拷贝初始化；指定初始化（C++20）按指定初始化器的形式（ii.4）
- condition 里声明变量的 `=` 形式：`if (T x = a)`、`while (T x = a)`
- 默认成员初始化器按写法分：`T m = x;` 是拷贝初始化

**直接初始化**（direct-initialization，[dcl.init]/16）：

- `T x(a);`、`T x{a};`
- `new T(a)`、`new T{a}`
- `static_cast<T>(a)`、函数式转换 `T(a)`、`T{a}`
- condition 里声明变量的花括号形式：`if (T x{a}) …`:
- 基类和成员的 mem-initializer（`C() : base(a), m{b} {}`，[class.base.init]/7）。默认成员初始化器按写法分：`T m{x};` 是直接列表初始化，不能写圆括号。

#### 2. 类类型：构造函数的选择

目标类型 T 是类类型时，标准按顺序判断三种情形，第一个满足的生效（[dcl.init]/17.6，三行依次是 17.6.1、17.6.2、17.6.3）：

| 条件 | 做法 |
|---|---|
| **情形 1**：源是 T 的纯右值（忽略 cv 限定）  | 直接用它初始化目标对象，不再查构造函数（C++17 起）；之前归入情形 2，拷贝或移动可以省略，见 i.3 |
| **情形 2**：直接初始化；或拷贝初始化且源类型是 T 或 T 的派生类 | 对 T 的构造函数做重载决议。**直接初始化的候选是全部构造函数，拷贝初始化的是转换构造函数**。 |
| **情形 3**：其余的拷贝初始化 | 选一个用户定义转换，候选是 T 的转换构造函数，以及源是类类型时，该类及其基类中能产生 T 或其派生类的非 `explicit` 转换函数。两种情形都用调用的结果直接初始化目标：结果是 T 的纯右值时走情形 1（C++17 起），其余（左值、将亡值、派生类的值）走情形 2 |

情形 2 的候选集见 [over.match.ctor]/1，情形 3 的见 [over.match.copy]/1。

```cpp
std::string s1 = "abc";                  // 情形 3：const char[4] → std::string，转换构造函数 string(const char*)
std::string s2 = s1;                     // 情形 2：同类左值，拷贝构造函数
std::string s3 = std::move(s1);          // 情形 2：同类右值，移动构造函数
std::string s4 = std::string("abc");     // 情形 1（C++17 起）：s4 就是这个纯右值的结果对象

struct W { operator std::string() const { return "w"; } };
std::string s5 = W();                    // 情形 3：源类型 W 的转换函数，结果再初始化 s5
```

所以拷贝初始化不一定调用拷贝构造函数：可能调用转换构造函数、移动构造函数，也可能一个都不调用。

#### 3. C++17 纯右值语义与可选拷贝省略

- **C++17 之前**，`T t = T(1);`、`T t = 1;` 语义上先有临时对象，再拷贝或移动构造 `t`；实现可以省略这一步，但重载决议选中的拷贝或移动构造函数必须可访问、未被删除。
- **C++17 起，纯右值语义（保证）**：类类型的纯右值用来初始化同类型的对象时，不再先产生临时对象，而是直接初始化最终的目标对象，拷贝或移动构造函数不必存在。这是语言规则，不是优化（cppreference：prvalue semantics，又称 “guaranteed copy elision”）。
- **仍是可选的省略**：满足条件时，返回具名局部对象（NRVO）、throw 表达式的操作数、异常处理器的形参可以省略。实现可以省略，但选中的拷贝或移动构造函数仍须可访问、未被删除。


#### 4. 用户定义转换的次数限制

一个值至多隐式施加一次用户定义转换（构造函数或转换函数，[class.conv]/4）。拷贝初始化选出的用户定义转换序列由三部分组成（[over.ics.user]/1、/2）：

1. 初始标准转换序列：通过构造函数时，把源转成构造函数实参所需的类型；通过转换函数时，把源转成转换函数的隐含对象形参。
2. 一次用户定义转换：调用转换构造函数或转换函数。
3. 第二标准转换序列：把转换的结果转成目标类型（引用绑定也算在内）。

前后两段标准转换序列都可以是恒等转换。用户定义转换本身只有一次，前后可以各有标准转换。直接初始化是对构造函数的重载决议，每个实参可以各自做一次隐式转换（含一次用户定义转换）。

```cpp
struct T { T(std::string); };
T a("abc");                  // 直接初始化：const char[4] → std::string 一次转换，再调用 T(string)
T b = "abc";                 // 拷贝初始化：const char[4] → std::string → T，两次用户定义转换，错误
T c = std::string("abc");    // 拷贝初始化：一次转换，正确
```

### ii. 列表初始化

#### 1. 定义与两种形式

**列表初始化**（list-initialization）是用花括号初始化列表（braced-init-list）初始化对象或引用（[dcl.init.list]/1）。列表可以为空；元素按出现顺序求值。

出现在直接初始化语境里的叫 **direct-list-initialization**，出现在拷贝初始化语境里的叫 **copy-list-initialization**：

- direct-list：`T x{a}`、`new T{a}`、`T{a}`、`: m{a}`、默认成员初始化器 `T m{a};`
- copy-list：`T x = {a}`、`f({a})`、`return {a};`、默认成员初始化器 `T m = {a};`

#### 2. 判定顺序

对类型 T 的对象，下表各项按顺序判断，第一项满足的生效（C++20 [dcl.init.list]/3，左列是项号）：

| 项 | 条件 | 做法 |
|---|---|---|
| 3.1 | T 是聚合类，列表包含指定初始化列表（designated-initializer-list） | 聚合初始化（C++20） |
| 3.2 | T 是聚合类，列表只有一个元素，类型是 U（忽略 cv 限定），U 是 T 或 T 的派生类 | 用该元素初始化：直接列表初始化按直接初始化，拷贝列表初始化按拷贝初始化 |
| 3.4 | T 是聚合 | 聚合初始化 |
| 3.5 | 列表为空，T 是有默认构造函数的类 | 值初始化 |
| 3.6 | T 是 `std::initializer_list` 的特化 | 构造该对象（示例见 ii.3） |
| 3.7 | T 是类类型 | 构造函数重载决议（ii.3） |
| 3.9 | T 不是类，列表只有一个元素（T 是引用时，被引用类型要与元素类型 reference-related） | 用该元素直接（或拷贝）初始化，禁止窄化转换 |
| 3.11 | 列表为空 | 值初始化 |

表中略去 3.3 字符数组、3.8 固定底层类型的枚举（C++17，只适用于直接列表初始化）、3.10 其余的引用。

表按顺序匹配：到 3.11 时，T 只剩非类、非引用的类型（如 `int`、指针、枚举），空列表一律值初始化；类和数组的空列表在 3.4、3.5、3.7 已处理，引用的在 3.10 已处理。

#### 3. 非聚合类：initializer-list 构造函数优先

T 是非聚合类（3.7）时，构造函数分两步选（[over.match.list]/1）：

1. 列表非空，或 T 没有默认构造函数：先只考虑 initializer-list 构造函数，**整个列表作一个实参**。
2. 否则，或第 1 步没有可行函数：考虑全部构造函数，列表的**各元素作实参**。

`std::initializer_list<X>` 形参可行，当且仅当列表的每个元素都能隐式转换成 X（[over.ics.list]/5）。**所以“优先”的准确含义是：第 1 步只考虑 initializer-list 构造函数，其中只要有可行的，就不进入第 2 步，哪怕别的构造函数更匹配；第 1 步内仍要选出唯一的最佳函数，有歧义则 ill-formed；只有第 1 步没有可行函数，才进入第 2 步。**

| 写法 | 选中的构造函数 | 结果 |
|---|---|---|
| `std::vector<int> v{10, 5};` | `vector(initializer_list<int>)` | 2 个元素：10、5 |
| `std::vector<int> v(10, 5);` | `vector(size_type, const int&)` | 10 个元素，都是 5 |
| `std::vector<int> v{10};` | `vector(initializer_list<int>)` | 1 个元素：10 |
| `std::vector<int> v(10);` | `explicit vector(size_type)` | 10 个元素，都是 0 |
| `std::vector<int> v{};` | 默认构造函数（3.5） | 空 |
| `std::vector<std::string> v{3, "x"};` | `vector(size_type, const string&)` | 3 个 `"x"`：`3` 不能转成 `std::string`，第 1 步不可行 |

**窄化转换不影响重载决议，选定函数之后才检查**（[dcl.init.list]/3.7；窄化转换的定义见 /7）：判断函数是否可行，只看实参有没有隐式转换序列，需要窄化转换的函数照样可行；函数选定后（无论在第 1 步还是第 2 步），若所需转换里有窄化转换，则 ill-formed，不会改选其他函数。

```cpp
struct N { N(std::initializer_list<int>); N(double, double); };
N a{1.5, 2.5};   // 错误：initializer_list<int> 构造函数可行，被选中，随后发现要窄化
N b(1.5, 2.5);   // 正确：圆括号是直接初始化，选 N(double, double)
std::vector<char> v{300};   // 错误：initializer_list<char> 构造函数可行，被选中，300 转 char 是窄化；不会改选 vector(size_type) 造出 300 个元素
```

选定 initializer-list 构造函数后，形参 `initializer_list<E>` 还要由列表初始化出来（3.6）：每个元素拷贝初始化出 `const E` 数组的对应元素。窄化转换和 `explicit` 都在这一步失败，同样不会改选其他函数。底层数组的细节见 [function.md](../../../01-basic/function.md)。

```cpp
std::initializer_list<int> il = {1, 2, 3};  // 3.6：T 本身是 initializer_list：元素拷贝初始化进 const int[3]，il 指向它，数组与 il 同寿
std::vector<int> v{1, 2, 3};                // 3.7：第 1 步选中 vector(initializer_list<int>)；形参按 3.6 初始化，数组是函数实参，活到完整表达式结束

struct T { T() = default; explicit T(const T&) = default; };
T a;
std::vector<T> w;
w.push_back(a);       // 正确：容器内部直接初始化新元素
T b = a;              // 错误：拷贝初始化不能用 explicit 拷贝构造函数
std::vector<T> u{a};  // 错误：同类型实参的转换序列是恒等转换（不看 explicit），第 1 步选中 initializer_list<T> 构造函数，
                      // 再拷贝初始化数组元素时失败；不会进入第 2 步
```

#### 4. 聚合初始化在列表初始化中的位置

T 是聚合时（3.4），列表初始化按元素的顺序初始化聚合，不调用聚合类自己的构造函数（[dcl.init.aggr]）：

- **聚合**是数组，或同时满足下面四条的类：没有用户声明的或继承来的构造函数；没有 private 或 protected 的直接非静态数据成员；没有虚函数；没有虚基类、private 基类或 protected 基类（/1，C++20；各版本的定义见 Note）。
- **元素**：数组是各下标的元素；类是直接基类（按声明顺序）后接直接非静态数据成员（按声明顺序，不含匿名联合的成员）（/2）。
- 每个显式初始化的元素，由对应的子句**拷贝初始化**；子句本身是花括号列表时，该元素按列表初始化，规则递归套用（/4）。指定初始化（C++20）里，元素由指派符后的初始化器初始化，初始化的种类看它的形式：`.m = x` 是拷贝初始化，`.m{x}` 是直接列表初始化（CWG 2619）。
- 没有子句的元素（非联合体聚合）：有默认成员初始化器就用它，否则从空列表拷贝初始化，引用成员则 ill-formed；子句比元素多：ill-formed（/11）。

```cpp
struct Point { int x; int y = 0; };     // C++14 起是聚合（有默认成员初始化器）
Point a{1, 2};                          // x = 1，y = 2
Point b{1};                             // x = 1，y 用默认成员初始化器：0
Point c{};                              // x 从 {} 拷贝初始化：0，y：0
Point d = {1.5};                        // 错误：double → int 是窄化转换
```

C++20 起，`Point p(1, 2);` 也做聚合初始化。

指定初始化的两种形式（CWG 2619；g++ 11.4 拒绝 `a1`，clang 14 接受）：

```cpp
struct S { explicit S(int); };
struct A { S s; };
A a1{.s{1}};    // 直接列表初始化 s，可用 explicit 构造函数
A a2{.s = 1};   // 拷贝初始化 s，错误
```

## Relation

### i. Issues

- **以为拷贝初始化一定写 `=`，或一定调用拷贝构造函数。** 见 i.1–i.3。
- **以为列表初始化总调用 initializer-list 构造函数。** 聚合和空列表各有自己的路径（ii.2）。
- **以为聚合的花括号总是聚合初始化。** 列表只有一个 T 或其派生类的元素时，是用该元素初始化整个对象（ii.2 的 3.2）：A 是聚合、a0 是 A 的对象时，`A a{a0};` 拷贝 a0，不是用 a0 初始化第一个成员。
- **以为 `explicit` 只管单参数构造函数。** 转换构造函数自 C++11 起不限于单参数：`struct M { explicit M(int, int); };` 挡住 `M m = {1, 2};`，不挡 `M m{1, 2};`。
- **以为聚合与构造函数互斥。** C++11 到 C++17，类内写 `A() = delete;` 的类仍是聚合，`A a{1};` 绕过被删除的构造函数；C++20 起它不再是聚合。
- **以为容器里的元素都是拷贝初始化出来的。** `push_back(a)` 经分配器直接初始化新元素；`std::vector<T>{a}` 的底层数组元素才是拷贝初始化。见 ii.3。

### ii. Connection

C++ Core Guidelines 把这些规则写成建议：C.46 默认把单参数构造函数声明为 `explicit`（例子是 `String s = 10;`，与 Why 里的 `std::vector<int> a = 5;` 同类）；C.164 避免隐式转换运算符；ES.23 优先用 `{}` 初始化，理由包括禁止窄化转换、可用于 `explicit` 构造函数（指直接列表初始化；拷贝列表初始化选中 `explicit` 构造函数是 ill-formed）；例外是 `vector` 这类容器：`vector<int> v1(10)` 是 10 个 0，`v2{10}` 是 1 个元素。

所以，对 `vector` 这类容器，通常用圆括号表示“个数”，用花括号表示“元素列表”（使用习惯，不是语言规则）；要禁止窄化转换用花括号；要初始化 `explicit` 的对象，用直接初始化或直接列表初始化。

**See also:**

- [01-initialization-basics.md](01-initialization-basics.md)：§2.1 形式与种类的关系；§2.2 默认、值、零初始化。
- [constructors-and-destructors.md](../../02-class-and-oop/constructors-and-destructors.md)：构造函数与成员初始化列表（mem-initializer 是直接初始化）；§5 `explicit` 对各种初始化写法的影响。
- [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md)：§2.2 静态初始化与动态初始化。
- [new-delete.md](../../../03-memory-resource/new-delete.md)：new-initializer 的 `()` 与 `{}`。
- [copy-and-move-draft.md](../../../03-memory-resource/copy-and-move-draft.md)：特殊成员函数；拷贝省略与移动的后续内容。

## Note

### i. 空列表：3.5 与 [over.match.list] 的分工

正文 ii.2 的 3.5（列表为空且 T 有默认构造函数→值初始化）与 ii.3 第 2 步里的“否则”（列表为空且 T 有默认构造函数→不做第 1 步），条件相同。本节说明两处为什么都有，各自什么时候用得上。条文按 C++20（N4861）。

#### 1. 3.7 的选择规则由 [over.match.list] 规定

3.7 只写了：T 是类类型时，枚举适用的构造函数，由重载决议选出最佳的一个（[over.match]、[over.match.list]）；任一实参的转换需要窄化转换，则 ill-formed。怎样枚举、怎样分阶段，是 [over.match.list]/1 的内容：

- **适用范围**：对非聚合类 T 做列表初始化，且 [dcl.init.list] 指明按本节做重载决议；**或者**为 [over.ics.list] 形成列表初始化的转换序列。
- **两个阶段**（phase；正文 ii.3 称“第 1 步、第 2 步”，本节沿用）：
  - (1.1) 列表非空，或 T 没有默认构造函数：先做一次重载决议，候选是 T 的 initializer-list 构造函数，实参是整个列表（作为一个实参）。
  - (1.2) 否则（即列表为空且 T 有默认构造函数），或没有可行的 initializer-list 构造函数：再做一次重载决议，候选是 T 的全部构造函数，实参是列表的各个元素。
- **explicit**：拷贝列表初始化中选中 explicit 构造函数，则 ill-formed。
- **“默认构造函数”**（[class.default.ctor]/1）：每个形参（函数形参包除外）都有默认实参的构造函数，包括没有形参的构造函数。所以一个类可以有多个默认构造函数，被 `= delete` 的也算。

#### 2. 两个调用方，以及 3.5 造成的差别

[over.match.list] 主要有两个调用方（类模板实参推导 [over.match.class.deduct]/4 也引用它，本节不展开）：

| 调用方 | 用途 | 列表为空且 T 有默认构造函数时 |
|---|---|---|
| [dcl.init.list]/3.7 | 初始化对象 | 前面有 3.5（来自 CWG 990）：先按值初始化处理，到不了 [over.match.list] |
| [over.ics.list]/7 | 重载决议中，为类类型的形参形成隐式转换序列，不构造对象 | 没有对应 3.5 的规则：由 [over.match.list]/1 自己跳过第 1 步，选出默认构造函数 |

#### 3. 例子

g++ 11.4、clang 14 各跑 c++11/14/17/20，共 8 种组合，输出（或错误）一致。语句都放在 `main` 里，省略了 `#include`。

**例一：有默认构造函数的类，两条路都得到 `A()`。**

```cpp
struct A {                                         // 有默认构造函数，另有两个 initializer-list 构造函数
    A()                              { std::puts("  A()"); }
    A(std::initializer_list<int>)    { std::puts("  A(initializer_list<int>)"); }
    A(std::initializer_list<double>) { std::puts("  A(initializer_list<double>)"); }
};
void f(A) { std::puts("  f(A) 的函数体"); }

A a{};    // 经 [dcl.init.list]：初始化对象。3.5：列表为空、A 有默认构造函数 → 值初始化，调 A()。
          // 到不了 3.7，也不进 [over.match.list]
f({});    // 经 [over.ics.list]：先在重载决议里为形参 A 形成转换序列。这里没有 3.5，
          // 由 [over.match.list]/1 自己跳过第 1 步，选出 A()，f 可行（此时不构造对象）。
          // 选定 f 之后才初始化形参：这是初始化，走 [dcl.init.list]，3.5 生效，调 A()
```

```text
A a{};
  A()
f({});
  A()
  f(A) 的函数体
```

`f({})` 里 `A()` 只调用了一次，发生在形参初始化时。

**例二：去掉 `A()`，两条路都二义。**

```cpp
struct A2 {                                        // 没有默认构造函数
    A2(std::initializer_list<int>)    {}
    A2(std::initializer_list<double>) {}
};
void f2(A2) {}

A2 a2{};  // 错误：二义。3.5 不适用 → 3.7 → 第 1 步：空列表转 initializer_list<int> 和
          // initializer_list<double> 都是恒等转换（[over.ics.list]/5），两个构造函数不分高下
f2({});   // 错误：二义，原因相同，发生在 f2 的重载决议里
```

例一的 `A` 去掉 `A()` 就是 `A2`。没有 3.5 和“跳过第 1 步”这两处补丁，`A` 的两条路也会是这个结果；这正是 CWG 990（`A a{}`）和 CWG 1229（`f({})`）各自要避免的。

**例三：没有默认构造函数、只有一个 initializer-list 构造函数：空列表由第 1 步处理。**

```cpp
struct N {                                         // 没有默认构造函数
    N(std::initializer_list<int> il) { std::printf("  N(initializer_list<int>)，size = %zu\n", il.size()); }
    N(int, int)                      { std::puts("  N(int, int)"); }
};
void fn(N) { std::puts("  fn(N) 的函数体"); }

N n{};      // 3.5 不适用 → 3.7 → (1.1) 的条件“T 没有默认构造函数”成立，做第 1 步：
            // 空列表转 initializer_list<int> 是恒等转换，可行，选中；size = 0
N m{1, 2};  // 非空：第 1 步可行，选中 initializer-list 构造函数，不看 N(int, int)
N k(1, 2);  // 圆括号不是列表初始化，选 N(int, int)
fn({});     // 经 [over.ics.list]：第 1 步选中 initializer-list 构造函数，fn 可行；形参初始化走 3.7，同样选它

struct P { P(int); };
P p{};      // 错误：没有默认构造函数，3.5 不适用；第 1 步没有 initializer-list 构造函数；
            // 第 2 步的实参个数为零，没有构造函数接得了
```

```text
N n{};
  N(initializer_list<int>)，size = 0
N m{1, 2};
  N(initializer_list<int>)，size = 2
N k(1, 2);
  N(int, int)
fn({});
  N(initializer_list<int>)，size = 0
  fn(N) 的函数体
```

**例四：被删除的默认构造函数也是默认构造函数。**

```cpp
struct D {
    D() = delete;                                  // 被删除，但仍是默认构造函数
    D(std::initializer_list<int>) {}
};
void fd(D) {}

D d{};    // 错误：3.5 适用（“有默认构造函数”不看能不能用）→ 值初始化 → 选中被删除的 D()；
          // initializer-list 构造函数没有机会参与，不会改选它
fd({});   // 错误：同样选中被删除的 D()
```

对比例三的 `N n{}`：有没有默认构造函数（不论能否使用），决定 `{}` 走 3.5 还是第 1 步。

**例五：3.5 是值初始化，不只是“选默认构造函数”。**

```cpp
struct Z {
    int x;
    Z() = default;                                 // 默认构造函数不是用户提供的
    Z(std::initializer_list<int>) {}
};

alignas(Z) unsigned char buf[sizeof(Z)];           // 每条语句前先把 buf 填成 0xFF，再在 buf 上构造，看 x 的字节
new (buf) Z{};   // 3.5 → 值初始化：先零初始化 → 00 00 00 00
new (buf) Z();   // 同样是值初始化 → 00 00 00 00
new (buf) Z;     // 默认初始化，平凡的默认构造函数什么也不做 → ff ff ff ff
                 // （标准只说 x 的值不确定；读到 ff 是这两个编译器的实测）
```

```text
Z{}   x 的四个字节：00 00 00 00
Z()   x 的四个字节：00 00 00 00
Z     x 的四个字节：ff ff ff ff
```

[over.match.list] 只规定选哪个构造函数，不含零初始化；`Z{}` 的 x 一定是 0，靠的是 3.5 指定的值初始化。

**例六：花括号里的 `{}` 不一定是空列表。**

```cpp
struct S {
    S()                           { std::puts("  S()"); }
    S(std::initializer_list<int>) { std::puts("  S(initializer_list<int>)"); }
};

S a{};     // 列表没有元素 → 3.5 → S()
S b{{}};   // 列表有一个元素（内层的 {}），不是空列表 → 3.7 → 第 1 步选中 S(initializer_list<int>)
S c({});   // 圆括号是直接初始化，{} 是构造函数的实参，不是初始化 c 的列表。
           // {} 转 initializer_list<int> 是恒等转换，转成 S（拷贝、移动构造函数的形参）是用户定义转换，
           // 前者更好 → S(initializer_list<int>)
```

```text
S a{};
  S()
S b{{}};
  S(initializer_list<int>)
S c({});
  S(initializer_list<int>)
```

**把例子对回两条路：**

| 类型 | `T x{}`（经 [dcl.init.list]） | `f({})`（经 [over.ics.list]） |
|---|---|---|
| `A`：有默认构造函数，两个 initializer-list 构造函数 | 3.5 → `A()` | 跳过第 1 步 → `A()`，f 可行 |
| `A2`：没有默认构造函数，两个 initializer-list 构造函数 | 错误：第 1 步二义 | 错误：第 1 步二义 |
| `N`：没有默认构造函数，一个 initializer-list 构造函数 | 第 1 步选中它，size = 0 | 第 1 步选中它，f 可行 |
| `D`：默认构造函数被删除，一个 initializer-list 构造函数 | 错误：3.5 → 被删除的 `D()` | 错误：同样选中被删除的 `D()` |

本节另引的出处（其余条文见 Reference）：[class.default.ctor](https://eel.is/c++draft/class.default.ctor)、[over.match.class.deduct](https://eel.is/c++draft/over.match.class.deduct)、[CWG 990](https://cplusplus.github.io/CWG/issues/990.html)、[CWG 1229](https://cplusplus.github.io/CWG/issues/1229.html)、[N3262](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3262.htm)。

## Reference

- cppreference：[Copy-initialization](https://en.cppreference.com/w/cpp/language/copy_initialization)、[Direct-initialization](https://en.cppreference.com/w/cpp/language/direct_initialization)、[List-initialization](https://en.cppreference.com/w/cpp/language/list_initialization)、[Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization)、[explicit specifier](https://en.cppreference.com/w/cpp/language/explicit)、[Converting constructor](https://en.cppreference.com/w/cpp/language/converting_constructor)、[Copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)、[std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)、[std::allocator::construct](https://en.cppreference.com/w/cpp/memory/allocator/construct)
- C++ standard (draft)，段号按 C++20（N4861），现行草案的编号不同处已在正文注明：
  - [dcl.init](https://eel.is/c++draft/dcl.init)：/15 拷贝初始化，/16 直接初始化，/17.6 类类型的语义；[dcl.init.list](https://eel.is/c++draft/dcl.init.list)：/1 定义，/2 initializer-list 构造函数，/3 判定顺序，/4 求值顺序，/5、/6 底层数组，/7 窄化转换；[dcl.init.aggr](https://eel.is/c++draft/dcl.init.aggr)：/1 聚合，/2 元素，/4、/5 元素的初始化，/11 子句过多，/15、/16 花括号省略
  - [over.match.ctor](https://eel.is/c++draft/over.match.ctor)、[over.match.copy](https://eel.is/c++draft/over.match.copy)、[over.match.list](https://eel.is/c++draft/over.match.list)、[over.ics.list](https://eel.is/c++draft/over.ics.list)、[over.ics.user](https://eel.is/c++draft/over.ics.user)
  - [class.conv](https://eel.is/c++draft/class.conv)：/4、[class.conv.ctor](https://eel.is/c++draft/class.conv.ctor)
  - [class.copy.elision](https://eel.is/c++draft/class.copy.elision)、[class.base.init](https://eel.is/c++draft/class.base.init)：/7、[expr.prim.lambda.capture](https://eel.is/c++draft/expr.prim.lambda.capture)：/15
  - [initializer.list.syn](https://eel.is/c++draft/initializer.list.syn)：/1 的注、[container.requirements.general](https://eel.is/c++draft/container.requirements.general)：/3（现行草案在 [container.requirements.pre]/3）、[allocator.traits.members](https://eel.is/c++draft/allocator.traits.members)
- 缺陷报告：[CWG 1270](https://cplusplus.github.io/CWG/issues/1270.html)、[CWG 1301](https://cplusplus.github.io/CWG/issues/1301.html)、[CWG 1324](https://cplusplus.github.io/CWG/issues/1324.html)、[CWG 1467](https://cplusplus.github.io/CWG/issues/1467.html)、[CWG 1518](https://cplusplus.github.io/CWG/issues/1518.html)、[CWG 2137](https://cplusplus.github.io/CWG/issues/2137.html)、[CWG 2610](https://cplusplus.github.io/CWG/issues/2610.html)、[CWG 2619](https://cplusplus.github.io/CWG/issues/2619.html)
- 提案：[N3653](https://wg21.link/N3653)、[P0017R1](https://wg21.link/P0017R1)、[P1008R1](https://wg21.link/P1008R1)、[P0960R3](https://wg21.link/P0960R3)、[P0329R4](https://wg21.link/P0329R4)、[P0398R0](https://wg21.link/P0398R0)、[P0135R1](https://wg21.link/P0135R1)、[P0138R2](https://wg21.link/P0138R2)、[P0892R2](https://wg21.link/P0892R2)、[P1957R2](https://wg21.link/P1957R2)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：[C.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-explicit)、[C.164](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conversion)、[ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list)
