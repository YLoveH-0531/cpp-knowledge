# 表达式、值类别与引用：GPT 问答的整理

整理自 11-26 贴来的 GPT 回答：先四段，后补三段（其中一段和回答 4 完全相同，没有重复整理），最后又补两段，共八段。八段的问题依次是：引用与“剥掉引用”；值类别怎么确定；`int&& rr = 20;`、复合表达式、类型描述什么；`int&` 与 `int&&` 的绑定、`int i = 10;` 算不算表达式；变量和函数的类型、`int&` 与 `int&&` 为什么这样设计；`const int&` 与 `int&&` 都能绑右值时重载决议选谁；三个值类别各表示什么、glvalue 和 rvalue 为什么存在；右值引用变量能不能初始化 `int&`、是不是“引用的引用”。11-27 又贴来七段引用初始化的回答，整理成第九节。

- 核对口径是 N3337（C++11），和 C++17、C++20 措辞不同的地方单独注明。
- 文中的结果都用 g++ 11.4 和 clang 14（`-std=c++11 -Wall -Wextra -pedantic`）实测过，两家一致。
- 带【更正 n】的地方，GPT 的原话不准或不严；正文写的是按标准改过的说法，理由集中在第八节（更正 1–11）和 9.11（更正 12–18）。

## 先记住这十句话

1. 名字、实体、表达式、声明是四个不同的概念，不是上下级关系。类型属于实体（变量、函数）和表达式，值类别只属于表达式。`int i = 10;` 是声明，不是表达式，里面的 `10` 才是（第一节）。
2. 引用是已有对象的另一个名字，不是对象。“受限制的指针”只能当实现上的类比（第二节）。也没有“引用的引用”：折叠只发生在 typedef、模板参数、`decltype` 形成的类型上（第六节）。
3. 表达式的类型：先按各类表达式自己的规则给出初始类型，是 “reference to T” 的，由 [expr]/5 调整成 `T`。这是标准分析表达式的规则，不是运行时转换（第三节）。
4. 三个值类别区分两件事：表达式有没有“身份”（能不能确定是哪个对象），以及对象的资源能不能被复用。glvalue（有身份的）和 rvalue（可以按右值处理的）是它们的两个合并名称，xvalue 同时属于两边（第三节）。
5. 表达式的值类别：由表达式的形式对应的规则规定（变量名是 lvalue，调用看返回类型，转换看目标类型），不是从“能不能取地址、能不能移动”反推的（第三节）。
6. 类型调整不改类别。所以用 `int`、`int&`、`int&&` 声明的变量，名字作为表达式都是 `int` 类型的 lvalue（第三节）。
7. 值类别影响取地址、引用绑定和重载决议，这些是类别带来的后果，不是类别的定义；类别本身不代表对象已经被移动（第四节）。
8. 复合表达式自底向上分析：先子表达式，再用最外层运算符的规则（第五节）。
9. 引用初始化按 [dcl.init.ref]/5 判断。对同类型的普通对象：`int&` 只绑 lvalue，`int&&` 不能直接绑 lvalue，右值引用变量的名字 `x` 本身是 lvalue，所以 `int& y = x;` 合法，`int&& z = x;` 不合法；类型不同、类类型转换函数这些情况另有规则（第六节）。`const int&` 和 `int&&` 在不能直接绑定时创建临时对象。临时对象活多久是另一条规则（[class.temporary]/5）：绑定到引用变量时一般延长到该引用的生命期，引用形参、`return`、`new` 的初始化器等语境例外（第六节）。
10. `const T&` 和 `T&&` 两个重载都能绑右值实参，其他条件相当时选 `T&&`（[over.ics.rank]/3 的排名规则）；同类型的 lvalue 实参绑不了 `T&&`，只能选 `T&` 或 `const T&`（第七节）。

## 一、先分清：名字、实体、表达式、声明

这几个词常被混着用，先分开，后面的问题大多就清楚了。它们不是上下级关系：声明引入名字，并给变量、函数指定类型；名字出现在表达式的位置，才构成有类型、有值类别的表达式；表达式又能出现在声明里（比如初始化器）。

| 概念 | 例子 | 有类型吗 | 有值类别吗 |
|---|---|---|---|
| 名字（name） | `i`、`r`、`A`（类名）、`N`（命名空间名） | 没有（见下） | 没有 |
| 实体（entity） | 对象 `i`、引用 `r`、函数、类型、命名空间 | 对象、引用、函数等有（声明类型）【更正 1】 | 没有 |
| 表达式（expression） | `r`、`&r`、`r + 1`、`42` | 有（表达式的类型） | 有 |
| 声明、语句 | `int i = 10;`、`i = 10;` | 整条没有 | 整条没有，但里面含有表达式 |

- 实体的清单在 [basic]/3：value、object、reference、function、enumerator、type、class member、template、namespace 等。名字是标识符、运算符函数名、转换函数名、模板标识（template-id）等的一次使用，它指代某个实体或标签（[basic]/4）。表达式是“指定一次计算的运算符和操作数的序列”（[expr]/1 的 Note）。
- 名字本身不等于表达式，要看它出现在哪、指代什么。变量名、函数名、枚举项名出现在表达式的位置，标准叫 id-expression，这时才有类型和值类别：变量、函数、数据成员的名字得 lvalue，枚举项得 prvalue（[expr.prim.general]/8）；指代函数模板特化的 template-id（如 `f<int>`）也能构成表达式。类名 `A`、命名空间名 `N` 在通常的类型和命名空间语境里不是表达式。
- 标准里说的 “the type of the identifier”，指的是名字所指代的实体的类型（[dcl.ref]/1、[expr.prim.general]/8）。
- 表达式不一定是一个名字：`42` 里没有名字；`i + 1`、`&i` 里有名字 `i`，但它只是子表达式，整个表达式的类型和值类别要按运算符的规则另算（第五节）。

### 变量和函数的类型

变量和函数的类型也来自声明：声明里的 specifier 给出基础类型，declarator 再用 `*`（pointer to）、`()`（function returning）等修改它（[dcl.decl]/2）；引用类型由 `&`、`&&` 形成，规则在 [dcl.ref]/1。【更正 2】所以 `int i`、`int& r`、`int&& rr` 声明的变量，类型分别是 `int`、`int&`、`int&&`。这些变量名作为表达式的类型，见 3.1 和 3.4。

函数的类型包含返回类型和参数类型（[dcl.fct]/1）。一个函数涉及三种不同的“类型”，不要混：

| 声明 | 函数的类型 | 返回类型 | 调用表达式 |
|---|---|---|---|
| `double f(int);` | `double(int)` | `double` | `f(10)`：`double`，prvalue |
| `int& g();` | `int&()` | `int&` | `g()`：`int`，lvalue |
| `int&& h();` | `int&&()` | `int&&` | `h()`：`int`，xvalue |

函数名本身作为表达式也有类型和类别：`g` 的类型是 `int&()`，是 lvalue（[expr.prim.general]/8：实体是函数）。下面的 `static_assert` 都实测通过；`decltype(f)` 取实体的类型，`decltype((e))` 读表达式的类型和类别（读法见 3.4）：

```cpp
#include <type_traits>

double f(int);
int&   g();
int&&  h();

static_assert(std::is_same<decltype(f), double(int)>::value, "the type of f");
static_assert(std::is_same<decltype(g), int&()>::value, "the type of g");
static_assert(std::is_same<decltype(h), int&&()>::value, "the type of h");
static_assert(std::is_same<decltype((f(10))), double>::value, "f(10): double, prvalue");
static_assert(std::is_same<decltype((g())), int&>::value, "g(): int, lvalue");
static_assert(std::is_same<decltype((h())), int&&>::value, "h(): int, xvalue");
static_assert(std::is_same<decltype((g)), int& (&)()>::value, "the name g: an lvalue of type int&()");
```

### `int i = 10;` 和 `i = 10;`

| | `int i = 10;` | `i = 10;`（`int i;` 之后） |
|---|---|---|
| 语法 | 简单声明（simple-declaration）；在函数体里同时是声明语句（declaration-statement） | 表达式语句（expression-statement） |
| 整条是表达式吗 | 否。里面的 `10` 是表达式：`int`，prvalue | 否（整条是语句）。里面的 `i = 10` 是表达式：`int`，lvalue |
| `=` 是什么 | 初始化器的语法（copy-initialization），不是赋值运算符 | 赋值运算符 |
| 作用 | 定义对象并初始化 | 修改已有对象 |

依据：[dcl.dcl]/1（简单声明的语法）、[dcl.init]/1（初始化器的语法）、[dcl.init]/14（`T x = a;` 叫 copy-initialization）、[stmt.dcl]/1、[stmt.expr]/1、[expr.ass]/1。

`int i = 10;` 既是声明也是定义，而 `extern int j;` 只是声明（[basic.def]/2）。copy-initialization 也不是调用赋值运算符：内置类型直接初始化，类类型走构造函数或转换函数（[dcl.init]/16）。

## 二、引用是什么

引用是已有对象（或函数）的另一个名字。[dcl.ref]/1 的 Note 原话是 “A reference can be thought of as a name of an object.”

```cpp
#include <iostream>

int main() {
    int i = 10;
    int& r = i;          // r is another name for i; i is not copied
    r = 20;              // same as i = 20
    int* p = &r;         // &r is the address of i
    std::cout << i << ' ' << (p == &i) << '\n';
}
```

```text
20 1
```

和指针对照着记：

| 目的 | 指针 | 引用 |
|---|---|---|
| 绑定到对象 | `int* p = &i;` | `int& r = i;` |
| 访问对象 | `*p` | `r` |
| 修改对象 | `*p = 20;` | `r = 20;` |
| 取对象的地址 | `p` | `&r` |

两个容易出现的误解：

- **“引用就是有限制的指针。”** 当作实现上的类比可以（编译器常用地址来实现引用），但不能当语义定义。语义上引用不是对象：[expr.new]/1 的 Note 明说 “because references are not objects, references cannot be created by new-expressions”，[basic]/3 也把 object 和 reference 分开列。标准没有规定引用占不占存储（[dcl.ref]/4），编译器可以不给它留任何空间。所以 `&r` 取的是被引用对象的地址，没有“引用自己的地址”可取。
- **“标准把引用剥掉，是编译器把引用换成对象或指针。”** 不是。剥掉引用是标准分析表达式的规则，不是运行时的步骤，既不生成指针也不生成新对象，见第三节。

## 三、表达式的类型和值类别：是什么，怎么确定

表达式有两个属性：类型（type）和值类别（value category）。后面的规则（`+` 的操作数、内置 `&` 的操作数、赋值的左操作数、重载决议）都要用到它们，所以标准要先规定，每种表达式是什么类型、什么类别。

### 3.1 类型：为什么说表达式的类型从不是引用类型

[expr]/5：如果表达式一开始的类型是 “reference to T”，在做进一步分析之前，类型先调整为 `T`；表达式指代引用所指的对象或函数，并且是 lvalue 或 xvalue，取决于表达式。

对 `int& r = i;` 里的表达式 `r`，分三步：

1. 变量 `r` 的声明类型是 `int&`（[dcl.ref]/1）。
2. `r` 是变量名，构成 id-expression，表达式的初始类型就是标识符的类型 `int&`（[expr.prim.general]/8）。
3. [expr]/5 把它调整为 `int`，并说明这个表达式指代的是 `i`。

初始类型是引用的表达式，常见的有三种：引用变量的名字；返回引用的函数调用（类型是返回类型，[expr.call]/3）；转换成引用类型的 `static_cast<int&&>(i)`。

为什么选 `T` 而不保留 `int&`，[expr]/5 没有给理由。我的理解（不是标准原话）：这样后面的每条规则只要写“类型为 `T` 的表达式”，不必再加“或 `T` 的引用”。一个看得见的后果是，[expr.ref]/2 要求 `.` 左边的表达式有完整类类型，`S& rs` 的 `rs.m` 之所以合法，是因为表达式 `rs` 的类型是 `S`：

```cpp
struct S { int m; };
S s = {7};
S& rs = s;
int v = rs.m;    // OK: the expression rs has type S, a class type
```

### 3.2 三个类别区分什么

值类别有三种：lvalue、xvalue、prvalue（[basic.lval]/1）。按 C++17 的说法，它们回答两个问题（“身份”一词是 C++17 才用的，N3337 的写法见下）：

1. 表达式有没有“身份”（identity）：求值能不能确定是哪个对象、位域或函数？有的叫 glvalue。
2. 有身份的，能不能按右值处理，也就是对象的资源允许被复用？不能的是 lvalue，能的是 xvalue。

没有身份的是 prvalue：它只是算出一个值，或者按所在的上下文去初始化一个对象。

| 类别 | 有身份 | 可以按右值处理 | 例 |
|---|---|---|---|
| lvalue | 是 | 否 | `i`、`r`、`*p` |
| xvalue | 是 | 是 | `static_cast<int&&>(i)`、`std::move(i)` |
| prvalue | 否 | 是 | `42`、`i + 1` |

“有身份”，cppreference 的 Value categories 页的说法是：能判断两个表达式是不是指同一个实体（比较所指对象的地址只是直观的例子：位域有身份，却没有自己的地址）。`i` 和 `static_cast<int&&>(i)` 都能确定是变量 `i`，所以都有身份；`42` 没有对象可指，它只是个值。“可以按右值处理”是说，移动构造函数、移动赋值运算符这类重载可以绑到它（同一页的说法）。

这是 C++17 的措辞（用了“身份”和“初始化”）。N3337 的 [basic.lval]/1 没有“身份”这个词：lvalue “指代一个对象或函数”，xvalue “也指代一个对象，通常快要结束生命期”，rvalue 是 xvalue、临时对象或“不与对象关联的值”，prvalue 是不是 xvalue 的 rvalue。完整的定义和 N3337 的原文见 [pointer-reference-const.md](pointer-reference-const.md) 的 What i 和 Note。三个类别的划分没有变。N3337 里 class 类型的 prvalue 可以就是一个临时对象，C++17 才改成“用来初始化对象”、临时对象在需要时才物化（第六节），所以不要把“prvalue 没有身份”直接读成“prvalue 不对应任何对象”。

glvalue 和 rvalue 是这三个类别的两个合并名称，不是另外两种类别：

- glvalue = lvalue + xvalue，有身份的。
- rvalue = xvalue + prvalue，可以按右值处理的。

xvalue 同时属于两边。规则里要说“有身份的”就写 glvalue，要说“可以按右值处理的”就写 rvalue，比如能绑到 `T&&` 的是 rvalue（第六节）。这两个集合关系在 N3337 和 C++17 里都成立。

xvalue 的定义里写的是“资源可以被复用（通常因为它快要结束生命期）”，“通常”不是条件：`static_cast<int&&>(i)` 之后 `i` 照常能用，生命期没有变。xvalue 只是告诉后面的规则“这个对象可以按右值处理”，不是对象快没了的证明。可以按右值处理只是语言允许，不保证后面真的会调用移动构造函数，更不保证对象已经被移动。

**为什么要分三类，而不是只分 lvalue 和 rvalue**（设计意图，不是标准原文；cppreference 同一页也是用 “has identity”“can be moved from” 两条独立的性质来讲）：

- 只分两类，`static_cast<int&&>(i)` 和 `42` 会落在同一类，可它们不一样：前者指向已有的 `i`，后者只是个值。同样绑到 `int&&`，前者直接绑到 `i`，后者要先造一个临时对象，第六节的 `f` 和 `d` 就是这两种情况。
- 反过来，lvalue 和 xvalue 都有身份，但有的规则只给 lvalue：内置 `&` 要求 lvalue，所以 `&static_cast<int&&>(i)` 非法（第四节）。有身份不等于什么要求 lvalue 的操作都能用。

所以“有没有身份”和“能不能按右值处理”是两件独立的事，三个类别刚好把它们分开。

### 3.3 值类别：由表达式的形式规定

上一节的两个问题讲的是类别的含义，不是判断办法。判断一个具体的表达式属于哪一类，要看标准对这种形式的表达式是怎么规定的，不是从“能不能取地址、能不能移动”反推。[basic.lval]/1 的 Note 说，每个内置运算符的条文会指明它产生的值的类别和它要求的操作数的类别；自定义运算符是函数，由返回类型和形参类型决定。

常见表达式（N3337）。表里的运算符都指内置运算符；重载的运算符是函数调用，类别看返回类型（返回 `A&` 的 `operator=` 得 lvalue，返回 `A` 的得 prvalue）：

| 表达式 | 类别 | 依据 |
|---|---|---|
| 变量名、函数名 | lvalue | [expr.prim.general]/8 |
| 字面量 `42` | prvalue（字符串字面量是 lvalue） | [expr.prim.general]/1 |
| `*p` | lvalue | [expr.unary.op]/1 |
| `&e` | prvalue | [expr.unary.op]/3 |
| 函数调用 | 返回 `T&`：lvalue；返回 `T&&`（对象）：xvalue；返回 `T`：prvalue | [expr.call]/10 |
| `static_cast<T&&>(e)`，`T` 是对象类型 | xvalue（目标是 `T&` 或函数类型的右值引用：lvalue；目标是非引用的 `T`：prvalue） | [expr.static.cast]/1 |
| `std::move(x)`，`x` 是对象 | xvalue（返回对象类型的右值引用的函数调用） | [expr.call]/10；`std::move` 见 [forward] |
| `x = 1`、`x += 1` | lvalue | [expr.ass]/1 |
| `++x`、`x++` | lvalue、prvalue | [expr.pre.incr]/1、[expr.post.incr]/1 |
| `(e)` | 和 `e` 相同【更正 3】 | [expr.prim.general]/6 |
| `(a, b)` | 和 `b` 相同 | [expr.comma]/1 |
| `a + b`（内置） | prvalue，条文没有单独写，由定义推出【更正 4】 | [basic.lval]/1 |

[expr]/5 里的 “depending on the expression” 不是“没有规则”，而是要按表达式的形式，去用上表里对应的那一条。

### 3.4 类型调整不改类别

[expr]/5 只改类型，不改值类别（这条规则从 N4861 起单列在 [expr.type]；现行草案在那里直接补了 “the value category of the expression is not altered”）。下表是实测，后两列用 `decltype((e))` 读出：lvalue 得 `T&`，xvalue 得 `T&&`，prvalue 得 `T`，这个 `T` 就是表达式的类型。

```cpp
#include <type_traits>

int i = 0;
int&  lref() { return i; }                       // returns int&
int&& rref() { return static_cast<int&&>(i); }   // returns int&&
int   val()  { return 42; }                      // returns int

int&  r = i;           // declared type int&
int&& x = val();       // declared type int&&

static_assert(std::is_same<decltype(x), int&&>::value, "decltype(x): the declared type of x");
static_assert(std::is_same<decltype((x)), int&>::value, "decltype((x)): the expression x is an lvalue");
```

| 表达式 | 声明类型、返回类型或转换目标类型 | 表达式的类型 | 值类别 |
|---|---|---|---|
| `i` | `int` | `int` | lvalue |
| `r` | `int&` | `int` | lvalue |
| `x` | `int&&` | `int` | lvalue |
| `lref()` | `int&` | `int` | lvalue |
| `rref()` | `int&&` | `int` | xvalue |
| `val()` | `int` | `int` | prvalue |
| `static_cast<int&&>(i)` | `int&&`（目标类型） | `int` | xvalue |

怎么读这张表：

- 第二列各不相同，第三列全是 `int`：引用在表达式的类型里被剥掉了。
- 第四列由表达式的形式决定：变量名一律是 lvalue（`r`、`x` 两行一样），调用看返回类型，转换看目标类型。
- 同一个名字 `x`：`decltype(x)` 取变量的声明类型，是 `int&&`（第二列）；`decltype((x))` 读表达式，`x` 是 lvalue，得 `int&`（第四列）。不带括号和带括号读到的不是一个东西（[dcl.type.simple]/4）。
- 类别来自上一节的规则，不是“因为它指代一个已经存在的对象”：`static_cast<int&&>(i)` 也指代已存在的对象，却是 xvalue。【更正 5】
- 所以右值引用变量的名字是 lvalue。这是变量名的规则，不是引用折叠的特殊规定（引用折叠出现在哪里，见第六节）。重载决议看的就是表达式的类别：

```cpp
#include <iostream>

void take(int&)  { std::cout << "int&\n"; }
void take(int&&) { std::cout << "int&&\n"; }

int main() {
    int&& x = 10;
    take(x);                       // the expression x is an lvalue
    take(static_cast<int&&>(x));   // an xvalue
}
```

```text
int&
int&&
```

## 四、值类别有什么用，又不是什么

- “左值 = 能取地址、能（或不能）移动的标签”这种理解不对。类别是对表达式的分类；取地址、引用绑定、重载决议（包括会不会选到移动构造函数）受它影响，这些是后果，不是定义。
- 类别描述表达式，不描述对象：同一个对象可以通过不同类别的表达式访问。`i` 是 lvalue，`static_cast<int&&>(i)` 是 xvalue，指的都是 `i`。C++17 的 [basic.lval]/1 有一句原话：“Despite their names, these terms classify expressions, not values.”
- 类别带来的规则：
  - 内置 `&` 要求操作数是 lvalue：`&r` 合法，`&static_cast<int&&>(i)` 非法。`&r` 整个表达式是 `int*` 的 prvalue，lvalue 的是它的操作数 `r`。位域的 lvalue 是例外，不能取地址（[class.bit]/3）。【更正 6】
  - xvalue 和 prvalue 可以绑定到 `T&&`（第六节）。
  - 重载决议要看实参的类别：同类型时，lvalue 实参绑不了 `T&&`，rvalue 实参在其他条件相当时优先选 `T&&`（第七节）。
  - `std::move(x)` 只是把表达式转成 xvalue，它的 Returns 是 `static_cast<typename remove_reference<T>::type&&>(t)`（[forward]）。xvalue 不等于已经移动，真正的移动发生在随后调用移动构造函数或移动赋值运算符的时候；lvalue 也能被移动，写成 `std::move(s)` 就行。

## 五、怎么分析一个表达式

本节只把前面的规则用起来，概念和依据见各节。

### 分析路线

遇到一段代码，可以按这条路线想。这是帮自己分析的思路，不是标准规定的执行流程；遇到重载运算符，操作数的转换和候选函数的选择要放在一起看。

1. 声明与实体：声明了什么变量或函数，声明类型是什么（第一节）。
2. 表达式：名字出现在哪个表达式里，这个表达式是什么结构（第一节）。
3. 类型与值类别：表达式的类型是什么，初始类型是引用的先按 [expr]/5 调整；值类别按表达式形式的规则判断（第三节）。
4. 运算、转换与重载决议：按运算符的规则、必要的类型转换和重载决议，得出整个表达式的类型和类别（下面的步骤和第七节）。
5. 引用初始化与绑定：引用能不能绑，要不要创建临时对象，临时对象活多久（第六节）。

### 一个表达式：自底向上

路线的第 2 到 4 步，对一个表达式这样做：

1. 按语法确定结构：优先级、结合性和显式括号一起决定怎么分组。括号可以改变分组；单纯把一个表达式 `e` 括起来得到的 `(e)`，类型和值类别都和 `e` 相同（[expr.prim.general]/6）。个别场合括号还影响含义，`decltype(e)` 和 `decltype((e))` 就不同（[dcl.type.simple]/4）。
2. 先分析各操作数的类型和类别，初始类型是引用的先调整。
3. 用最外层运算符的规则，得出整个表达式的类型和类别。
4. 涉及类型转换、重载运算符时再用相应规则：重载运算符是函数调用，按返回类型定类别。

例，`int i = 10; int& r = i;`：

| 表达式 | 分析 | 类型 | 类别 |
|---|---|---|---|
| `r` | 变量名；`int&` 调整为 `int` | `int` | lvalue |
| `&r` | 内置 `&`：操作数是 lvalue，结果是它的地址 | `int*` | prvalue |
| `(&r)` | 括号不改变类型和类别 | `int*` | prvalue |
| `*(&r)` | 内置 `*`：结果是指针所指对象的 lvalue | `int` | lvalue |
| `r + 1` | 先对 `r` 做 lvalue-to-rvalue 转换取出值；`1` 是 prvalue；结果是不对应对象的值 | `int` | prvalue |
| `r = 5` | 内置赋值：结果是指代左操作数的 lvalue | `int` | lvalue |

- `(r = 5) = 6` 合法，因为 `r = 5` 是 lvalue。上表都实测过。
- 整个表达式的类型和类别由最外层运算符的规则决定，一般不是继承某个操作数。但有些规则恰好是“同某个操作数”：括号、逗号运算符，以及 `?:` 在两个操作数是同类别同类型的 glvalue 时（[expr.cond]/4），所以不能说成“从不继承”。【更正 7】

## 六、引用初始化：绑定什么，临时对象怎么来

```cpp
#include <utility>

int n = 10;
int&       a = n;              // OK
// int&    b = 20;             // error: a non-const lvalue reference, and 20 is not an lvalue
const int& c = 20;             // OK: bound to a temporary
int&&      d = 20;             // OK: bound to a temporary
// int&&   e = n;              // error: n is an lvalue
int&&      f = std::move(n);   // OK: bound directly to n
```

[dcl.init.ref]/5 的判断顺序，只列常用的几条路径（GPT 引成 “/5.2.1”“/5.2.2”，那是网页的编号，见【更正 8】）：

1. 引用是 lvalue 引用，初始化器是 lvalue，类型兼容（reference-compatible）：**直接绑定**到这个对象。例：`a`。
2. 否则，引用必须是指向非 volatile 的 const 类型的 lvalue 引用（`const T&`），或者是 rvalue 引用（`T&&`），不然非法。例：`b` 在这里被拒。
3. 初始化器是 xvalue、class prvalue、array prvalue 或 function lvalue，且类型兼容：**直接绑定**到它指代的对象（或它的结果）。例：`f`。
4. 否则，创建一个临时对象，用初始化器按非引用的拷贝初始化来初始化它，引用绑定到临时对象。例：`c`、`d`。例外：引用是 `T&&`，而初始化器是相关类型（reference-related）的 lvalue，非法。例：`e` 在这里被拒。

“直接绑定”（bind directly）是标准术语，意思是引用绑到初始化器本身（第 1、3 条）。第 4 条创建临时对象，不算直接绑定：`d` 绑的是新建的临时对象，不是 `20`。【更正 9】

`int&& rr = 20;` 的全过程：

1. `20` 是 `int` 的 prvalue，不能直接绑定。
2. 走第 4 条：创建一个值为 20 的 `int` 临时对象，`rr` 绑定到它。
3. 临时对象本来在完整表达式结束时销毁；绑定到引用变量后，一般延长到该引用的生命期结束（[class.temporary]/5，例外见下）。创建临时对象和延长它的生命期是两条规则。
4. `rr` 是变量名，表达式 `rr` 是 lvalue，指代那个临时对象。延长生命期并没有把它变成普通局部变量，它还是临时对象。

```cpp
#include <iostream>

int main() {
    int&& rr = 20;       // a temporary int holding 20; rr is bound to it
    rr = 30;             // modifies the temporary
    int* p = &rr;        // the address of the temporary
    std::cout << *p << '\n';
}
```

```text
30
```

延长生命期的规则有四种例外，都不延长到引用的生命期（[class.temporary]/5）：

- 构造函数的 ctor-initializer 里绑定到引用成员：活到构造函数结束。
- 函数调用的引用形参：活到包含这次调用的完整表达式结束。形参的生命期在函数返回时结束（[expr.call]/4），所以临时对象比形参活得久。
- `return` 语句里绑定到返回值：不延长，在 `return` 语句的完整表达式结束时销毁。
- `new` 的初始化器（new-initializer）里绑定到引用：活到包含这个 new-initializer 的完整表达式结束。

注：以上是 N3337 原文的四种例外。第一种后来被 CWG 1696 取消（这个缺陷报告追溯适用于旧版本）：构造函数的 mem-initializer 里把临时对象绑到引用成员，现在是非法程序（C++17 的 [class.base.init]：“A temporary expression bound to a reference member in a mem-initializer is ill-formed.”），C++17 的 [class.temporary] 只剩另外三种。C++20 又加了一种：聚合类用圆括号初始化时，绑到引用成员的临时对象只活到包含这个表达式列表的完整表达式结束（N4861 [class.temporary]/6.10）。

版本差别：C++17 起，prvalue 不再先变成临时对象，而是在需要 glvalue 的地方（比如绑定引用）才经 temporary materialization conversion（[conv.rval]）物化成指代临时对象的 xvalue。这是 C++17 对 prvalue 的整体调整，不只是换了说法。例如拷贝、移动构造函数都 `= delete` 的类 `N`，`N f() { return N(); } N x = f();` 在 `-std=c++11` 下两家编译器都报错，在 `-std=c++17` 下通过（保证的拷贝消除）。对 `int&& rr = 20;` 这样的引用绑定，结果和 C++11 一样：引用绑到临时对象，生命期延长。

### 用右值引用变量初始化另一个引用

`int&& x = fun();` 之后再写 `int& y = x;`，这是“引用的引用”吗？不是。

`int& y = x;` 里，`int&` 是 `y` 的声明类型，`x` 是初始化表达式，不是类型。编译器不会把 `int&` 和 `int&&` 拼在一起，而是看表达式 `x`：它是变量名，是 lvalue（第三节）。`y` 是 lvalue 引用，lvalue 引用绑 lvalue，走判断顺序里的第 1 条，直接绑定到 `x` 指代的对象，也就是那个临时对象。所以 `x` 和 `y` 是同一个对象的两个名字：

```cpp
#include <iostream>

int fun() { return 10; }

int main() {
    int&& x = fun();   // x is bound to a temporary int holding 10
    int& y = x;        // the expression x is an lvalue, so an lvalue reference can bind to it
    // int&& z = x;    // error: the expression x is an lvalue
    y = 20;
    std::cout << x << '\n';
}
```

```text
20
```

`int&& z = x;` 不行：`x` 虽然声明成 `int&&`，表达式 `x` 还是 lvalue，`int&&` 不能绑 lvalue，和开头代码里的 `e` 一样。

临时对象的生命期仍然跟着 `x`：`y` 是用变量名 `x` 初始化的，不会再延长它。C++20 的 [class.temporary]/6 把规则写清楚了：只有绑定的 glvalue 是经过 temporary materialization、括号、数组下标、成员访问、类型转换、`?:`、逗号运算符等途径得到的，才延长；变量名不在其中。

引用折叠出现在别处。C++ 不允许直接写引用的引用（[dcl.ref]/5；`int& & q = x;` 两家编译器都报错）。只有通过 typedef 或 `using`、模板类型参数、`decltype` 间接形成时才会遇到，这时 [dcl.ref]/6 规定折叠：只要有一个是 lvalue 引用，结果就是 lvalue 引用；两个都是 rvalue 引用才得 rvalue 引用。C++17 的原文在这条后面加了一句 “This rule is known as reference collapsing.”，N3337 这一条没有。

```cpp
#include <type_traits>

int&& x = 10;
using R = int&&;

static_assert(std::is_same<R&, int&>::value, "R& collapses to int&");
static_assert(std::is_same<R&&, int&&>::value, "R&& stays int&&");
R& w = x;    // R& is int&: an lvalue reference bound to the lvalue x
```

这里 `R` 是 `int&&`，加上 `&` 形成引用的引用，折叠成 `int&`。`int& y = x;` 里没有任何类型组合，不涉及折叠。

### 为什么这样规定（设计意图，不是标准原文）

标准只规定绑定条件，没有给理由。Stroustrup 的说明和 `show`、`incr` 的例子在 [pointer-reference-const.md](pointer-reference-const.md) 的 How ii.1，这里不重复。三种引用各表达什么意图，可以这样记（GPT 的概括；cppreference 的 Reference declaration 页有相同的用法说明，但两者都不是标准原文）。下表只讲同类型的普通对象，类型不同、类类型的转换函数、函数类型等情况按 [dcl.init.ref]/5 的完整规则（见上面的判断顺序和更正 8）：

| 引用类型 | 能绑定 | 通常能否通过它修改 | 要表达的意图 |
|---|---|---|---|
| `T&` | lvalue | 能 | 要改调用者提供的对象 |
| `const T&` | lvalue 和 rvalue（含临时对象） | 不能 | 只读，不拷贝，左值右值都收 |
| `T&&` | rvalue（xvalue、prvalue） | 能 | 收下右值，可以改它、搬走它的资源 |

- 例：`int i; double&& r = i;` 合法。`int` 和 `double` 不是 reference-related，`i` 被转成一个 `double` 临时对象，`r` 绑到这个临时对象；`const double&` 同理，`double&` 不行。
- `T&` 不绑临时对象，是因为通过它改，改到的是马上消失的临时对象，调用者看不到。`T&&` 同样能改临时对象，并不矛盾：它的用途本来就是收下右值、改它、搬走它的资源，支撑移动构造和移动赋值，而且类型上明说了这一点。
- GPT 还说，`T&` 绑临时对象“也会破坏左值和右值的重载区分”。这是它的推断，没有出处，不要当结论记。【更正 10】
- 右值引用不保证绑的是临时对象，也不保证别处不再访问这个对象。`std::move(n)` 是 xvalue，`int&&` 可以直接绑到 `n`（上面的 `f`），通过它修改，改到的就是 `n`。右值引用只是调用者和函数之间的约定：“这个对象你可以按右值处理”，不是编译器保证的独占访问。
- `const T&` 只保证不能通过这个引用改对象的非 `mutable` 部分（[basic.lval]/8），不保证对象不会被别的途径改。它收的是类型兼容的实参；需要类型转换时（比如第七节的 `show("hello")`），绑的是转换出来的临时对象，不是原来的实参。

## 七、`const T&` 和 `T&&` 都能绑右值时，重载决议选谁

第三节的 `take(int&)`、`take(int&&)` 只用到了“lvalue 只能选 `int&`”。这里补上 `const T&` 也在场的情形。

重载决议分两步（[over.match.viable]、[over.match.best]）：先筛出可行函数（实参能初始化形参），再比较各可行函数的隐式转换序列，选最好的。能绑定，不等于会被选中。有多个形参时，要求每个实参的转换序列都不比对方差，并且至少一个更好（[over.match.best]/1）；下面的例子都只有一个形参，所以只比这一个。

用到的两条排名规则，都在 [over.ics.rank]/3 “标准转换序列 S1 比 S2 好”那一组里：

- 两个都是引用绑定，S1 把 rvalue 绑到 rvalue 引用，S2 把 rvalue 绑到 lvalue 引用：S1 更好（N3337 网页上的编号是 3.1.4）。【更正 11】这条排在“一个是另一个的子序列”“转换等级更好”等几条之后（原文逐条用 “or, if not that” 往下接），前面已经分出高下，就轮不到它。例：`f(const int&)` 和 `f(long&&)`，实参 `5`，`const int&` 是恒等转换，`long&&` 要先把 `int` 转成 `long`，前者等级更好，所以选 `const int&`。
- 两个都是引用绑定，所引用的类型只差顶层 cv，被引用类型 cv 限定更多的那个更差（3.1.6）。

例：`process` 有 `const int&`、`int&`、`int&&` 三个重载：

| 调用 | 实参 | `const int&` | `int&` | `int&&` | 选中 | 为什么 |
|---|---|---|---|---|---|---|
| `process(n)` | lvalue `int` | 可行 | 可行 | 不可行 | `int&` | 3.1.6：`int&` 引用的类型 cv 限定更少 |
| `process(c)`（`const int c`） | lvalue `const int` | 可行 | 不可行 | 不可行 | `const int&` | 只有一个可行 |
| `process(10)` | prvalue | 可行 | 不可行 | 可行 | `int&&` | 3.1.4 |
| `process(std::move(n))` | xvalue | 可行 | 不可行 | 可行 | `int&&` | 3.1.4，xvalue 也是 rvalue |

- 绑定的对象不同：`process(10)` 的 `int&&` 绑到由 `10` 创建的临时对象；`process(std::move(n))` 的 `int&&` 直接绑到 `n`，没有临时对象。`std::move` 只是转换，没有移动任何东西。
- 实际意义：只有 `const T&` 时，函数分不出实参是 lvalue 还是 rvalue。加上 `T&&` 重载，lvalue（调用者可能还要用）和 rvalue（调用者允许按右值处理，可以复用资源）就能分别处理。cppreference 的 Reference declaration 页写的是 “This allows move constructors, move assignment operators, and other move-aware functions … to be automatically selected when suitable.” `const T&` 仍然有用：一个只读接口，左值右值都收，不用写两套。

```cpp
#include <iostream>
#include <string>
#include <utility>

void process(const int&) { std::cout << "const int&\n"; }
void process(int&)       { std::cout << "int&\n"; }
void process(int&&)      { std::cout << "int&&\n"; }

void show(const std::string&) { std::cout << "const std::string&\n"; }
void show(std::string&&)      { std::cout << "std::string&&\n"; }

int main() {
    int n = 10;
    const int c = 10;
    process(n);              // int&
    process(c);              // const int&
    process(10);             // int&&
    process(std::move(n));   // int&&

    std::string s = "hello";
    show(s);                 // const std::string&
    show("hello");           // std::string&&: a temporary std::string is created from the literal
    show(std::move(s));      // std::string&&
}
```

```text
int&
const int&
int&&
int&&
const std::string&
std::string&&
std::string&&
```

`show("hello")` 的两个候选都要先用转换构造函数造出一个临时 `std::string`，用的是同一个构造函数（[over.ics.ref]/2）。比较分两层：先按 [over.ics.rank]/3 里用户定义转换序列那一条（网页编号 3.2），同一个构造函数就比较各自的第二标准转换序列；这两个第二标准转换序列都是引用绑定，再适用 3.1.4（右值引用绑右值更好），选出 `std::string&&`。

## 八、GPT 回答里需要更正的地方

GPT 的核心结论都成立，下面是措辞不准或不严的地方，编号和正文里的【更正 n】对应。

| # | GPT 的说法（出处） | 问题 | 依据 |
|---|---|---|---|
| 1 | “类型和值类别描述的是表达式，不是名字”（回答 3） | 类型也属于对象、引用、函数（GPT 随后讲的声明类型就是这个意思）；只有值类别专属于表达式。GPT 在回答 5 开头已自己改正 | [intro.object]/1、[dcl.ref]/1、[dcl.fct]/1 |
| 2 | “[dcl.decl]/2 … declarator 通过 `*`、`&`、`()` 等修改类型”（回答 5） | 条文对，但原文的例子只有 `*`（pointer to）和 `()`（function returning）；引用的 `&` 由 [dcl.ref]/1 规定 | [dcl.decl]/2、[dcl.ref]/1 |
| 3 | 回答 2 说“按 C++11”，表里却用 C++17 的措辞给 prvalue 下定义；回答 7 给 glvalue、prvalue 的定义（求值确定身份、求值初始化对象）也是 C++17 的措辞；回答 3 说括号表达式的“类型和值类别”相同 | 前两者是 C++17 的措辞：N3337 里 prvalue 是“不是 xvalue 的 rvalue”，glvalue 是“lvalue 或 xvalue”，没有“身份”一词（3.2 已把两种写法对照过）。最后一个在 N3337 写的是“类型和值相同，括号不影响是不是 lvalue”，明写 value category 是 C++17 起（N4140 仍是 C++11 的写法）。结论一致，引用时要注明版本 | [basic.lval]/1、[expr.prim.general]/6、N4659 [expr.prim.paren]/1 |
| 4 | “标准在各表达式的条款里明确规定了值类别”（回答 2） | 多数如此，但不是每个运算符都有明文：N3337 的 [expr.add] 全文没有出现 lvalue、rvalue、prvalue，`a + b` 是 prvalue 靠 [basic.lval]/1 的定义推出（不对应对象的值是 rvalue，又不是 xvalue） | [expr.add]、[basic.lval]/1 |
| 5 | “`r` 指代一个已经存在的对象，因此这个表达式是左值”（回答 1） | 理由不对：`static_cast<int&&>(i)` 也指代已存在的对象，却是 xvalue。`r` 是 lvalue，是因为变量名的规则。GPT 在回答 2 里已自己改正 | [expr.prim.general]/8 |
| 6 | “左值通常可以通过内置 `&` 取地址”（回答 2） | “通常”没错，但没给例外：位域的 lvalue 不能取地址 | [class.bit]/3 |
| 7 | “复合表达式的类型和值类别，并不是简单继承某个操作数”（回答 3） | 多数运算符如此，但括号、逗号运算符、`?:` 的规则恰好是同操作数 | [expr.prim.general]/6、[expr.comma]/1、[expr.cond]/4 |
| 8 | “T& 要求 lvalue；T&& 绑 xvalue 和 prvalue”，条文引成 “/5.2.1”“/5.2.2”（回答 4） | 是简化：类类型经转换函数得到 lvalue 或 xvalue 的情形、function lvalue 绑 `T&&` 的情形都略去了。“5.2.1”“5.2.2” 和 N3337 的网页（timsong-cpp）对得上，但那是网页渲染时加的编号：N3337 自己的源文件里这几条是无编号的嵌套列表，N4659 里对应的那一条（先转成 prvalue 再做 temporary materialization）编号又不同，在 5.2.2.2。引用时写明版本，或者说 “[dcl.init.ref]/5 里 xvalue、class prvalue 那一条”更稳 | [dcl.init.ref]/5 |
| 9 | “局部引用直接绑定临时对象”（回答 3、4） | “bind directly” 是术语，指绑定到初始化器本身；创建临时对象那一条不算直接绑定（[dcl.init.ref]/5 末句）。同一回答前面还说 `20` “不符合直接绑定的条件”，前后术语不一致 | [dcl.init.ref]/5 |
| 10 | “非 const 左值引用绑临时对象……也会破坏左值和右值的重载区分”（回答 5） | GPT 的推断，我没找到出处：标准只有绑定条件，没有给理由；Stroustrup 的原话（[pointer-reference-const.md](pointer-reference-const.md) 引的）只说了“对变量的赋值会变成对快消失的临时对象的赋值”这一条，没提重载区分。不要当结论记 | [dcl.init.ref]/5；TC++PL 4th §7.7.1 |
| 11 | “[over.ics.rank]/3.1.4 … 右值引用绑定右值更好”（回答 6） | 规则本身对。“3.1.4” 是网页对 N3337 的编号（第 3 段里“标准转换序列 S1 比 S2 好”那一组的第 4 项；N4659 前面多了 list-initialization 一组，同一条变成 3.2.3）。GPT 的转述还漏了限制条件：两个绑定都不能是“没有 ref-qualifier 的非静态成员函数的隐含对象形参” | [over.ics.rank]/3 |

出处补充：回答 1 说“C++ 标准明确规定引用不是对象”，是对的，只是没给出处。原话在 [expr.new]/1 的 Note（N3337、N4861、现行草案都有）：“because references are not objects, references cannot be created by new-expressions”。

其余说法逐条核对过，没有发现问题（它们对的结论都已经写在正文里）。要留意的有三处：

- 回答 7：“为什么不能只分左值和右值”是设计意图，不是标准原文，正文已标明。
- 回答 8：`static_assert` 只写一个参数是 C++17 的写法，C++11 要带消息；“`R` 展开后相当于 `int&& & y = x;`”只是示意，这种写法本身非法（[dcl.ref]/5），折叠发生在 `R&` 这个类型形成的时候；物化的说法是 C++17 的，N3337 的说法在第六节。
- 没核对：回答 5 提到 isocpp.org 和 Microsoft Learn 的页面，没有给地址，没法核对；对应的事实我用了标准条文和 cppreference 页的原文。

3.2 的例子、3.4 和第五节的两张表，类型和值类别都是对每个表达式用 `decltype((e))` 读出来的（读法见 3.4）；文中的代码块都编译运行过。

## 九、引用初始化的完整判断阶梯：[dcl.init.ref]/5 逐条读

11-27 又贴来七段 GPT 回答（先五段，后补两段，下面称回答 1 到 7）。问题依次是：①四步判断的先后为什么不能乱，“第三条”到底绑什么；②第二条（5.2）的原文是什么，C++14 和 C++17 各条怎么走；③reference-compatible 是什么，`const int& r = n` 属于哪一条，为什么先查引用的种类；④“lvalue/rvalue × 四种引用”的表，以及“第一条先选中、排除 lvalue 那一行，不满足再进 rvalue 那一行”的理解对不对；⑤`double&& r = i` 是不是“先生成 xvalue 再绑定”；⑥`double&& r1 = d` 违反哪一条，5.2.2 末尾的限制是不是只针对 5.2.2.2；⑦“右值引用不能绑 lvalue”这个记法要不要加限定。第六节只给了简版四步，这里补完整：原文、每一条的例子、版本差别。

- 块引用是原文，前一行写出处；“译”是我的翻译，其余是说明。引文里的交叉引用号和脚注略去。
- 核对口径仍是 N3337（C++11）。N4140（C++14）、N4659（C++17）的差别集中在 9.10。条文小号（5.1.1 等）取自 timsong-cpp 网页，N3337、N4140、N4659 三个版本的编号都对过；N3337 源文件里这些条没有编号（更正 8）。
- 例子和表里的写法都编译过：`// error` 的行取消注释后都报错，其余都通过（g++ 11.4 和 clang 14，c++11/14/17/20，共 8 种组合，结果一致）；9.9 的程序还运行过，输出与文中一致。

### 9.1 先备：两组术语

引用指向的类型写成 “cv1 T1”，初始化器的类型写成 “cv2 T2”。`const double& r = i;` 里，“cv1 T1” 是 `const double`，“cv2 T2” 是 `int`。

**原文**（N3337 [dcl.init.ref]/4）：

> Given types “cv1 T1” and “cv2 T2,” “cv1 T1” is reference-related to “cv2 T2” if T1 is the same type as T2, or T1 is a base class of T2. “cv1 T1” is reference-compatible with “cv2 T2” if T1 is reference-related to T2 and cv1 is the same cv-qualification as, or greater cv-qualification than, cv2.

**译**：T1 与 T2 是同一类型，或 T1 是 T2 的基类，叫 reference-related（引用相关）。在此基础上，cv1 的 cv 限定和 cv2 相同或更多，才叫 reference-compatible（引用兼容）。只许加 cv，不许丢 cv。

| cv1 T1（引用指向） | cv2 T2（初始化器） | related | compatible |
|---|---|---|---|
| `int` | `int` | 是 | 是 |
| `const int` | `int` | 是 | 是（加了 const） |
| `int` | `const int` | 是 | 否（丢了 const） |
| `A`（基类） | `B`（派生类） | 是 | 是 |
| `B` | `A` | 否 | 否 |
| `double` | `int` | 否 | 否 |

同一段的末句还规定：T1 是 T2 的基类时，如果这个基类不可访问或有歧义，需要这个绑定的程序非法。

“bind directly”（直接绑定）也是术语，在第 5 段末句：

> In all cases except the last (i.e., creating and initializing a temporary from the initializer expression), the reference is said to bind directly to the initializer expression.

**译**：除了最后一种情形（创建临时对象并用初始化器初始化它），其余都叫直接绑定。

也就是说，直接绑定是引用绑到初始化器的值，或转换函数的结果上，没有再按 T1 造一个新的临时对象来拷贝初始化。初始化器本身可以就是个临时对象（比如 `f()` 返回的类对象），绑它仍是直接绑定。

### 9.2 阶梯总览

“Otherwise” 把各条连成一条链：前面所有条都不成立，才看下一条。顺序是标准定死的，不是几种方式任选。

| 条 | 看什么 | 成立时 | cppreference 里的位置 |
|---|---|---|---|
| 5.1 | 引用是 lvalue 引用；初始化器是 lvalue（不是位域）且 reference-compatible，或类类型经转换函数得到 lvalue | 直接绑定，结束 | Direct binding 的前两项 |
| 5.2 | 否则，引用本身有没有资格：non-volatile 的 `const T&`，或 `T&&` | 有资格才往下看，没有就非法 | Direct binding 里夹着的 “…the program is ill-formed” 那一项 |
| 5.2.1 | 初始化器是 xvalue、class prvalue、array prvalue 或 function lvalue 且 compatible，或类类型经转换函数得到这些 | 直接绑定，结束 | Direct binding 的后两项（按 until C++17、since C++17 分开写） |
| 5.2.2 | 否则 | 造临时对象，引用绑到它（不算直接绑定）；末尾有两条限制 | Indirect binding，第二句 “In this case, T cannot be reference-related to U.” |

5.1 和 5.2.1 各有两种：初始化器本身就是（5.1.1、5.2.1.1），或类类型经转换函数得到（5.1.2、5.2.1.2）。

通俗地说是四道关：①能不能直接绑 lvalue；②不能的话，这种引用有没有资格去接“不是现成 lvalue 的东西”（`const T&` 和 `T&&` 有，`T&` 没有）；③能不能直接绑右值一侧；④都不行才造临时对象，造的时候对 `T&&` 另有一道限制。cppreference 的 Reference initialization 页开头也是这个次序：“Direct binding is considered first, followed by indirect binding, if neither binding is available, the program is ill-formed.” GPT 说的“间接绑定”就是这页的 Indirect binding；[dcl.init.ref] 里没有这个词，只有 bind directly【更正 15】。

### 9.3 5.1：lvalue 引用绑 lvalue

**原文**（N3337 [dcl.init.ref]/5.1）：

> If the reference is an lvalue reference and the initializer expression
>
> - is an lvalue (but is not a bit-field), and “cv1 T1” is reference-compatible with “cv2 T2,” or
> - has a class type (i.e., T2 is a class type), where T1 is not reference-related to T2, and can be implicitly converted to an lvalue of type “cv3 T3,” where “cv1 T1” is reference-compatible with “cv3 T3” (this conversion is selected by enumerating the applicable conversion functions and choosing the best one through overload resolution),
>
> then the reference is bound to the initializer expression lvalue in the first case and to the lvalue result of the conversion in the second case (or, in either case, to the appropriate base class subobject of the object).

**译**：引用是 lvalue 引用，并且初始化器 ①是 lvalue（不能是位域）且 reference-compatible；或者 ②是类类型，T1 与 T2 不 reference-related，能经转换函数得到一个 lvalue（原文脚注：这要求转换函数返回引用类型），且与它 reference-compatible。① 绑到初始化器本身，② 绑到转换结果；是基类的话，绑到基类子对象。

`rd`、`rcd`、`ra`、`ir` 四行是标准 5.1 的原例（`B` 的转换函数我补了函数体），其余几行是我加的：

```cpp
double d = 2.0;
double& rd = d;                 // rd refers to d
const double& rcd = d;          // rcd refers to d
const int cn = 0;
const int& rcc = cn;            // OK: same cv-qualification
// int& rc = cn;                // error: would drop const, not reference-compatible

struct A { };
int g;
struct B : A { operator int&() { return g; } } b;
A& ra = b;                      // ra refers to A subobject in b
int& ir = B();                  // ir refers to the result of B::operator int&
```

- 5.1 只接住，不排除：能直接绑的 lvalue 它接住，接不住的（包括所有 rvalue 引用）都往下掉。把 “lvalue → `T&&`” 挡住的不是它，是 5.2.2 的末句（9.6）。
- 第二种不要求初始化器本身是 lvalue：`B()` 是 prvalue，也能绑 `int&`，绑的是转换函数返回的 lvalue。

### 9.4 5.2：先查引用本身有没有资格

**原文**（N3337 [dcl.init.ref]/5.2）：

> Otherwise, the reference shall be an lvalue reference to a non-volatile const type (i.e., cv1 shall be const), or the reference shall be an rvalue reference.

**译**：否则，引用必须是指向 non-volatile const 类型的 lvalue 引用（即 cv1 必须是 const），或者必须是 rvalue 引用。

三点：

1. 它查的是**引用的种类**，不看初始化器，也不是一种绑定方式。通过只表示可以接着看 5.2.1、5.2.2，后面仍可能失败（`int&& r = n;`）。GPT 回答 1 的流程图把它画成独立的“第二步”，回答 2 自己改了口，说它是“前置资格条件”，以后者为准【更正 12】。
2. “Otherwise” 接的是 5.1 没接住的**一切**，包括 lvalue 初始化器（`const double& r = i;`、`double&& r = i;`）。所以“5.2 以后只处理 rvalue”不对。
3. 放行的是 `const T&`、`T&&`、`const T&&`。`T&`、`volatile T&`、`const volatile T&` 走到这里（5.1 没接住）就非法，没有别的办法了。

前两行是标准的原例，注释也是原文：

```cpp
int i = 2;
// double& rd2 = 2.0;           // error: not an lvalue and reference not const
// double& rd3 = i;             // error: type mismatch and reference not const
// const volatile int& rv = 10; // error: the referenced type is volatile
const double& rcd = i;          // OK: passes 5.2, then 5.2.2 creates a temporary double
```

### 9.5 5.2.1：直接绑到右值一侧

**原文**（N3337 [dcl.init.ref]/5.2.1）：

> If the initializer expression
>
> - is an xvalue, class prvalue, array prvalue or function lvalue and “cv1 T1” is reference-compatible with “cv2 T2”, or
> - has a class type (i.e., T2 is a class type), where T1 is not reference-related to T2, and can be implicitly converted to an xvalue, class prvalue, or function lvalue of type “cv3 T3”, where “cv1 T1” is reference-compatible with “cv3 T3”,
>
> then the reference is bound to the value of the initializer expression in the first case and to the result of the conversion in the second case (or, in either case, to an appropriate base class subobject). In the second case, if the reference is an rvalue reference and the second standard conversion sequence of the user-defined conversion sequence includes an lvalue-to-rvalue conversion, the program is ill-formed.

**译**：初始化器 ①本身是 xvalue、class prvalue、array prvalue 或 function lvalue，且 reference-compatible；或者 ②是类类型，T1 与 T2 不 reference-related，能经转换函数得到 xvalue、class prvalue 或 function lvalue，且与它 reference-compatible。① 绑到初始化器的值，② 绑到转换结果；是基类的话，绑到基类子对象。在 ② 里，如果引用是 rvalue 引用，而转换序列的第二个标准转换含 lvalue-to-rvalue 转换，程序非法。

四类初始化器各绑到哪：

| 初始化器 | 绑到 | 例 |
|---|---|---|
| xvalue | 它指代的那个对象，不再新建 | `static_cast<int&&>(i2)` 绑到 `i2` |
| class prvalue | 这个类对象；引用类型是基类时，绑到其中的基类子对象 | `B f()` 返回的对象，`const A& rca2 = f();` 绑到其中的 `A` 子对象，没有另造一个 `A` |
| array prvalue | 这个数组对象，少见 | `Arr{1, 2, 3}`（`Arr` 是 `int[3]`） |
| function lvalue | 这个函数 | `void (&&rf)() = func;` |

到 `rri2` 为止是标准的原例，注释也是原文（`f` 和 `X` 的转换函数我补了函数体），后面的 `Arr`、`func` 是我加的：

```cpp
struct A { };
struct B : A { };
B f() { return B(); }
const A& rca2 = f();                // bound to the A subobject of the B rvalue.
A&& rra = f();                      // same as above
struct X {
    operator B() { return B(); }
    operator int&() { static int v = 0; return v; }
} x;
const A& r = x;                     // bound to the A subobject of the result of the conversion
int i2 = 42;
int&& rri = static_cast<int&&>(i2); // bound directly to i2
B&& rrb = x;                        // bound directly to the result of operator B
// int&& rri2 = X();                // error: lvalue-to-rvalue conversion applied to the result of operator int&

typedef int Arr[3];
Arr&& ra2 = Arr{1, 2, 3};           // array prvalue
void func() { }
void (&&rf)() = func;               // function lvalue binds an rvalue reference
```

- **C++11/14 的这一条不含非类 prvalue**：`const int& r = 10;`、`int&& r = 10;` 不在这里，走 5.2.2；C++17 起才改成 “an rvalue”，并进来（9.10）。GPT 把这一条说成“右值一侧的直接绑定”（回答 1 的流程图、回答 4），在 C++11/14 里不能理解成“所有 rvalue”【更正 14】。
- function lvalue 能绑 `T&&` 是这一条的特例，标准只在列表里写了它，没写理由。GPT 的解释是“因为函数不是对象，不存在普通对象意义上的移动、临时对象创建等问题”，这是它的推断，不要当结论记。它举的 array prvalue 例子 `int arr[3]; int (&r)[3] = arr;` 其实是 array lvalue，走 5.1，上面的 `Arr{1, 2, 3}` 才是 array prvalue【更正 16】。
- 绑 class prvalue 时，那个类对象本身是临时对象（`f()` 的结果），引用会延长它的生命期。所以“直接绑定”不等于“没有临时对象”，只是没有再按 T1 造一个新的临时对象来拷贝初始化。

### 9.6 5.2.2：造临时对象，和末尾的两条限制

**原文**（N3337 [dcl.init.ref]/5.2.2）：

> Otherwise, a temporary of type “cv1 T1” is created and initialized from the initializer expression using the rules for a non-reference copy-initialization. The reference is then bound to the temporary. If T1 is reference-related to T2, cv1 shall be the same cv-qualification as, or greater cv-qualification than, cv2. If T1 is reference-related to T2 and the reference is an rvalue reference, the initializer expression shall not be an lvalue.

**译**：否则，创建一个 “cv1 T1” 类型的临时对象，用初始化器按非引用的拷贝初始化规则初始化它，引用绑到这个临时对象。如果 T1 与 T2 reference-related，cv1 的 cv 限定必须和 cv2 相同或更多。如果 T1 与 T2 reference-related，并且引用是 rvalue 引用，初始化器不能是 lvalue。

最后一句就是“lvalue 不能绑 `T&&`”的出处（N4140、N4659 里是 5.2.2.4）。它只拦 related 的类型，不相关的（`int` → `double`）不拦。标准的例子成对给，注释也是原文（`rcv`、`rrd4`、`rrd5` 三行是我加的）：

```cpp
const double& rcd2 = 2;         // rcd2 refers to temporary with value 2.0
double&& rrd = 2;               // rrd refers to temporary with value 2.0
const volatile int cvi = 1;
const volatile int& rcv = cvi;  // OK: same cv-qualification
// const int& r2 = cvi;         // error: type qualifiers dropped
double d2 = 1.0;
// double&& rrd2 = d2;          // error: copying lvalue of related type
int i3 = 2;
double&& rrd3 = i3;             // rrd3 refers to temporary with value 2.0

double&& rrd4 = 2.0;                        // OK: a prvalue, not an lvalue
double&& rrd5 = static_cast<double&&>(d2);  // OK: an xvalue, bound directly to d2
```

- `rrd2` 被拒：`double` 与 `double` related，`d2` 是 lvalue。同一个 `d2` 换成 xvalue（`rrd5`）或 prvalue（`rrd4`）就通过：末句只拦 lvalue，`rrd5` 直接绑到 `d2`，`rrd4` 绑到临时对象。`rrd3` 通过：`double` 与 `int` 不 related。
- 基类也算 related：`struct B : A {}` 时，`B b; A&& r = b;` 非法，`A&& r = B();` 合法（走 5.2.1）。
- 编号随版本变：N3337 的 5.2.2 不分小条；N4140、N4659 拆成 5.2.2.1 到 5.2.2.4（两个版本的差别见 9.10 的表）。以 N4140 为例：5.2.2.1（T1 是类类型：用户定义转换）和 5.2.2.2（T1 不是类类型：造临时对象）是两种做法，其后 “If T1 is reference-related to T2:” 引出 5.2.2.3（cv1 不少于 cv2）和 5.2.2.4（`T&&` 不能配 lvalue）。后两条管整个 5.2.2，不只属于 5.2.2.2，两种做法都要过这一关：T1 是类类型时也一样，上面基类那条的 `B b; A&& r = b;` 就被拒。GPT 引的 “5.2.2.2”“5.2.2.4” 是 N4140 的编号【更正 17】。
- 末句是对程序的静态要求（shall）：`double&& r = d2;` 整句非法，不是造出临时对象之后才发现错。标准对 `rrd2` 的说明只有注释里的 “copying lvalue of related type”，没有再写理由；GPT 回答 6 说它是“为了防止在间接绑定过程中，通过创建临时对象，绕过原本的引用绑定限制”，这是设计意图，没有出处，不要当结论记【更正 18】。

### 9.7 用阶梯走几个例子

下面 `n`、`i` 是 `int` 变量，`struct B : A { };`；按 C++11/14 的走法，C++17 的差别见 9.10。

| 代码 | 5.1 | 5.2 | 5.2.1 | 5.2.2 | 结果 |
|---|---|---|---|---|---|
| `int& r = n;` | 成立 | | | | 直接绑定到 `n` |
| `int& r = 10;` | 不成立：不是 lvalue | 不成立：`int&` 既不是 `const T&`，也不是 `T&&` | | | 非法 |
| `const int& r = 10;` | 不成立 | 放行 | 不成立：`int` 的 prvalue 不在列表里 | 成立：造临时 `int` | 合法，绑到临时对象 |
| `int&& r = std::move(n);` | 不成立：不是 lvalue 引用 | 放行 | 成立：xvalue | | 直接绑定到 `n` |
| `int&& r = n;` | 不成立 | 放行 | 不成立：`n` 是非类类型的 lvalue | 被末句拦住：`int` 与 `int` related，`n` 是 lvalue | 非法 |
| `double&& r = i;` | 不成立 | 放行 | 不成立：`i` 是非类类型的 lvalue | 成立：造临时 `double`；与 `int` 不 related，末句不拦 | 合法，绑到临时 `double` |
| `A&& r = B();` | 不成立 | 放行 | 成立：class prvalue | | 直接绑定到 `A` 子对象 |
| `B b; A&& r = b;` | 不成立 | 放行 | 不成立：`b` 是 lvalue | 被末句拦住：`A` 是 `B` 的基类，related，`b` 是 lvalue | 非法 |

回答 6 用 N4140 的小号把 `double&& r1 = d;` 的路径写细了，`int&& r = n;` 同理：5.1 不成立（不是 lvalue 引用）；5.2 通过（是 rvalue 引用）；5.2.1.1 不成立（`n` 是 lvalue，不在列表里）；5.2.1.2 不成立（`int` 不是类类型）；5.2.2.2 进入（T1 不是类类型，要造临时对象）；5.2.2.3 通过（cv 相同）；5.2.2.4 违反（引用是 rvalue 引用，初始化器是 lvalue），非法。

### 9.8 “lvalue/rvalue × 四种引用”的表：只在 related 时成立

常见的那张表（lvalue 一行 yes、yes、no、no，rvalue 一行 no、yes、yes、yes）是对的，但它只是 related 这一类的结果。补上不 related 的，是这样（每格都实测过）：

| 初始化器 | `T&` | `const T&` | `T&&` | `const T&&` |
|---|---|---|---|---|
| related（同类型或基类）的 lvalue | 可以（5.1） | 可以（5.1） | 不行（5.2.2 末句） | 不行（5.2.2 末句） |
| related 的 rvalue | 不行（5.2） | 可以 | 可以 | 可以 |
| 不 related（`int` → `double`）的 lvalue 或 rvalue | 不行（5.2） | 可以，临时对象 | 可以，临时对象 | 可以，临时对象 |

- 5.1 没有“排除”任何东西。第一行左边两格是 5.1 接住的；右边两格的“不行”，是 9.6 里那句 “…the initializer expression shall not be an lvalue” 拦的。
- “lvalue 一行不满足，就进 rvalue 一行”也不准：5.2 的 “Otherwise” 接的是 5.1 没接住的一切，lvalue 也会走到后面。对不 related 的类型，lvalue 和 rvalue 没有差别，都造临时对象，这就是 `double&& r = i;` 合法的原因。
- 所以“右值引用不能绑 lvalue”要加限定。回答 7 的说法是：对于普通对象，右值引用不能直接绑 lvalue，但 lvalue 可以经类型转换得到右值或临时对象，再初始化右值引用，function lvalue 是例外。方向对，写具体些。不 related 的类型，lvalue 有两条路绑 `T&&`：①造临时对象（`double&& r = i;`，5.2.2）；②类类型的 lvalue 经转换函数得到 xvalue、class prvalue 或 function lvalue（`B&& rrb = x;`，5.2.1 的第二种，代码在 9.5；转换函数返回 lvalue 的不行，`int&& rri2 = X();`）。related 的类型只有一个例外：③function lvalue 本身（`void (&&rf)() = func;`，5.2.1 的第一种）。其余 related 的 lvalue 都不行（9.6 的末句），就是表第一行右边两格。
- 表的前提是 cv1 不少于 cv2。`const T` 的 class prvalue、xvalue 绑 `T&&` 要丢 const，非法（`A&& r = fca();`，`fca` 返回 `const A`）；非类 prvalue 的 cv 在表达式层面就被丢掉了，所以 `const int fci(); int&& r = fci();` 合法（N3337 [basic.lval]/4；N4140、N4659 是 [expr]/6）。GPT 回答 4 说“`const T` 类型的右值不能绑 `T&&`”说宽了【更正 13】。

一句话的模型：标准先试最直接的绑定（5.1）；不行，就按引用的种类筛一遍（5.2）；再试直接绑右值一侧（5.2.1）；最后才造临时对象（5.2.2），造的时候拦住“related 的 lvalue 绑 `T&&`”。

### 9.9 `double&& r = i`：初始化来源和绑到的对象

按 C++11 的阶梯走（9.7 表里那一行）：`r` 绑到一个新的临时 `double`（值是 `2.0`），不是 `i`。要分清两件事：引用是用什么表达式初始化的，和它最后绑到哪个对象。`double&& r = i;` 用 lvalue `i` 初始化，绑到的是新临时对象；`int&& s = std::move(j);` 用 xvalue 初始化，绑到的就是 `j`。程序里的 `a`、`b` 用 `const` 引用演示同一件事（`const int&` 直接绑 `n`，`const double&` 造临时对象）：

```cpp
#include <iostream>
#include <utility>

int main() {
    int n = 1;
    const int& a = n;       // bound directly to n
    const double& b = n;    // bound to a temporary double
    n = 5;
    std::cout << a << ' ' << b << '\n';

    int i = 2;
    double&& r = i;         // a temporary double, not i
    r = 3.5;
    std::cout << i << ' ' << r << '\n';

    int j = 2;
    int&& s = std::move(j); // bound directly to j
    s = 7;
    std::cout << j << '\n';
}
```

```text
5 1
2 3.5
7
```

`a` 直接绑到 `n`，`n` 改成 5 后读到 5；`b` 绑的是临时 `double`，还是 1。`r = 3.5` 改的是临时对象，`i` 不变；`s = 7` 改的就是 `j`。

回答 5 的问题是：是不是“先生成一个 xvalue，再绑到这个 xvalue 上”。这是 C++17 的说法：

**原文**（N4659 [dcl.init.ref]/5.2.2.2）：

> Otherwise, the initializer expression is implicitly converted to a prvalue of type “cv1 T1”. The temporary materialization conversion is applied and the reference is bound to the result.

**原文**（N4659 [conv.rval]/1）：

> A prvalue of type T can be converted to an xvalue of type T. This conversion initializes a temporary object of type T from the prvalue by evaluating the prvalue with the temporary object as its result object, and produces an xvalue denoting the temporary object.

**译**：先把初始化器隐式转换成 “cv1 T1” 类型的 prvalue（`i` 转成值为 `2.0` 的 `double` prvalue），再做 temporary materialization：用这个 prvalue 初始化一个临时对象，并得到指代这个临时对象的 xvalue，`r` 绑到这个 xvalue。C++11/14 的原文（9.6）没有提 xvalue，只说创建临时对象再绑定，两种写法的结果相同。

### 9.10 C++17 起有什么变化

**原文**（N4659 [dcl.init.ref]/5.2.1：第一项，以及两项之后的一段）：

> is an rvalue (but not a bit-field) or function lvalue and “cv1 T1” is reference-compatible with “cv2 T2”, or
>
> …
>
> then the value of the initializer expression in the first case and the result of the conversion in the second case is called the converted initializer. If the converted initializer is a prvalue, its type T4 is adjusted to type “cv1 T4” and the temporary materialization conversion is applied. In any case, the reference is bound to the resulting glvalue (or to an appropriate base class subobject).

**译**：5.2.1.1 的初始化器类别改成 “rvalue（非位域）或 function lvalue”；符合条件的初始化器（或转换结果）叫 converted initializer，它如果是 prvalue，先按引用的 cv 调整类型，再做 temporary materialization，引用绑到得到的 glvalue 上。

| 项 | C++11（N3337） | C++14（N4140） | C++17（N4659） |
|---|---|---|---|
| 5.1、5.2 | 实质相同 | 同左 | 同左 |
| 5.2.1 初始化器类别 | xvalue、class prvalue、array prvalue、function lvalue | 同左，xvalue 加 “but not a bit-field” | rvalue（非位域）或 function lvalue |
| 5.2.1 怎么绑 | 绑到初始化器的值 | 同左 | prvalue 先 temporary materialization，再绑到得到的 glvalue |
| 5.2.2 | 一条：造临时对象并拷贝初始化，后接两条限制 | 拆成 5.2.2.1（T1 是类类型）、5.2.2.2（T1 不是类类型）、5.2.2.3、5.2.2.4（两条限制） | 5.2.2.1：T1 或 T2 是类类型且不 related，用户定义转换；5.2.2.2：否则转成 prvalue 再实质化；5.2.2.3、5.2.2.4 同左 |
| `int&& r = 10;` | 5.2.2，造临时对象，不算直接绑定 | 同左 | 5.2.1，直接绑定（经实质化） |
| 末句 “bind directly” 里排除的“最后一种” | creating and initializing a temporary from the initializer expression | 同左 | implicitly converting the initializer expression to the underlying type of the reference |

同一段代码在 C++11 和 C++17 下都合法，临时对象的生命期也一样（延长到引用的生命期），变的是标准的描述方式，和 “是不是直接绑定” 的分类。第六节的“版本差别”讲了 prvalue 整体的调整。

- N4659 [dcl.init.ref]/4 还给 reference-compatible 加了一种：T2 是 “noexcept function”、T1 是 “function”，且函数类型其余相同。对象类型不受影响。
- C++20 及以后：N4861 起 reference-related 改用 “similar”，5.2 改写成 “Otherwise, if the reference is an lvalue reference to a type that is not const-qualified or is volatile-qualified, the program is ill-formed.”；现行草案把 5.2.2 末尾的两条限制换成 5.4 开头的一句：“Otherwise, T1 shall not be reference-related to T2.” cppreference 的 Indirect binding 里也有这一句：“In this case, T cannot be reference-related to U.”

### 9.11 GPT 回答里需要更正的地方（接第八节的编号）

| # | GPT 的说法（出处） | 问题 | 依据 |
|---|---|---|---|
| 12 | 回答 1 的流程图把 5.2 画成独立的“第二步：非 const 或 volatile 的 lvalue 引用 → 非法”；回答 2 自己改口，说它是“前置资格条件” | 以改口后的为准。5.2 查引用的种类，不看初始化器，不是绑定方式。“非 const 或 volatile 的 lvalue 引用”也含糊，原文是 “lvalue reference to a non-volatile const type” 以外的 lvalue 引用 | [dcl.init.ref]/5.2 |
| 13 | “`const T` 类型的右值不能绑 `T&&`”（回答 4） | 只对 class prvalue 和 xvalue 成立；非类 prvalue 的 cv 被丢掉，`const int fci(); int&& r = fci();` 合法 | N3337 [basic.lval]/4；N4140、N4659 [expr]/6 |
| 14 | 5.2.1 是“rvalue 一侧的直接绑定”（回答 1 流程图的第③步；回答 4：“5.2.1 确实主要处理右值一侧的绑定”） | 在 C++11/14 里，“右值一侧”不等于所有 rvalue：5.2.1 的列表不含非类 prvalue，`int&& r = 10;`、`const int& r = 10;` 走 5.2.2。C++17 起才改成 “an rvalue” | 5.2.1.1 的各版原文（9.10） |
| 15 | “间接绑定”（回答 1、3、5） | [dcl.init.ref] 里没有这个词，只有 bind directly，创建临时对象那一种“不算”。cppreference 的 Reference initialization 页用 Direct binding / Indirect binding 作小节名，所以这个说法有出处，出处是 cppreference，不是标准；笔记里用这个词要写明这一点 | [dcl.init.ref]/5 末句；cppreference |
| 16 | 回答 1 举 `int arr[3]; int (&r)[3] = arr;` 当 array prvalue 的例子；说 function lvalue 能绑 `T&&` 是“因为函数不是对象，不存在普通对象意义上的移动、临时对象创建等问题” | 前者是 array lvalue，走 5.1（GPT 随后也这么说）。后者是 GPT 的推断，标准只在 5.2.1 的列表里写了 function lvalue，没写理由，不要当结论记 | [dcl.init.ref]/5.2.1 |
| 17 | 各回答引的小号（5.1.1、5.2.1.1、5.2.2.2、5.2.2.4 等） | 对得上 N4140 的网页编号，不是错；但 N3337 的 5.2.2 不分小条，N4659 的 5.2.2.2 内容已变，引用时要写版本 | 9.6、9.10；更正 8 |
| 18 | 5.2.2.3、5.2.2.4 “是为了防止在间接绑定过程中，通过创建临时对象，绕过原本的引用绑定限制”（回答 6） | 效果的概括没错，但“为了……”是设计意图，标准没写；标准只在例子的注释里说 “copying lvalue of related type”。不要当结论记 | [dcl.init.ref]/5 的例子注释；9.6 |

其余说法逐条核对过，没有发现问题：5.1、5.2、5.2.1、5.2.2 的原文引用；5.2.1 里 “lvalue-to-rvalue” 的那条限制；function lvalue 绑 `T&&`；N4659 里 reference-compatible 增加 `noexcept` 函数那一种；C++17 里 prvalue 转换再 temporary materialization 的描述；`double&& r = i` 修改 `r` 不影响 `i`；回答 6 里 `double&& r1 = d` 违反 N4140 的 5.2.2.4、两条限制只在 T1 与 T2 related 时检查、`double&& r3 = static_cast<double&&>(d)` 直接绑到 `d`；回答 7 里 function lvalue 可以绑 `T&&`。回答 1 说“cppreference 对这两个时期分别列出了规则”也对：该页 5.2.1 对应的那一条分 until C++17 和 since C++17 两种写法。
