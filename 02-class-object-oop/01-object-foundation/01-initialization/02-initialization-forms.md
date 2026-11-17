# Initialization Forms

> 范围：只讲拷贝初始化、列表初始化和 `explicit` 怎样决定“选哪个构造函数、能不能选”。默认、值、零初始化见 [01-initialization-basics.md](01-initialization-basics.md) §3.1；聚合只讲它在列表初始化里的位置；拷贝省略只讲拷贝初始化用得上的部分，其余留给 [copy-and-move-draft.md](../../../03-memory-resource/copy-and-move-draft.md)。引用的初始化、`auto` 推导、类模板实参推导不讲。

## Why

同一个对象有四种常见的初始化写法：`T t = x;`、`T t(x);`、`T t{x};`、`T t = {x};`。它们看上去只是书写不同，规则却不一样，常见的现象都由此而来：

- `std::vector<int> v = 5;` 编译不过，`std::vector<int> v(5);` 可以。
- `std::vector<int> v{10, 5};` 有 2 个元素，`std::vector<int> v(10, 5);` 有 10 个元素。
- 设 `E` 的构造函数是 `explicit E(int)`：`E e = 1;` 和 `E e = {1};` 都编译不过，`E e(1);` 可以，而前两者编译不过的原因并不相同。

这些现象由三件事决定：拷贝初始化和直接初始化各自考虑哪些构造函数（i）；花括号初始化怎样选构造函数，initializer-list 构造函数为什么优先，窄化转换为什么被禁止（ii）；`explicit` 把哪些构造函数挡在哪些写法之外（iii）。三件事前后相扣，下面按这个顺序讲。

## What

**初始化的写法决定标准怎样称呼它，称呼决定了候选的构造函数、`explicit` 构造函数的待遇、窄化转换是否允许，以及 initializer-list 构造函数是否优先。**

| 写法 | 标准术语 | 候选的构造函数 | explicit 构造函数 | 窄化转换 | initializer-list 构造函数优先 |
|---|---|---|---|---|---|
| `T t = x;` | copy-initialization | 转换构造函数 | 不是候选 | 允许 | 无 |
| `T t(x);` | direct-initialization | 全部构造函数 | 是候选 | 允许 | 无 |
| `T t{x};` | direct-list-initialization | 全部构造函数 | 是候选 | 禁止 | 有 |
| `T t = {x};` | copy-list-initialization | 全部构造函数 | 是候选，选中则 ill-formed | 禁止 | 有 |

表中 T 是类类型。转换构造函数（converting constructor）指没有 `explicit` 的构造函数。源类型是别的类类型时，拷贝初始化还会考虑源类型的非 `explicit` 转换函数（见 i）。T 是聚合时，花括号不选构造函数，而是做聚合初始化（见 ii）。

## How

### i. 拷贝初始化与直接初始化

#### 1. 什么时候发生

先看这两个术语在标准里怎样定义。**拷贝初始化（copy-initialization）** 是下面这些场合发生的初始化（[dcl.init]/15）：

- 变量定义里的 `=` 形式：`T t = x;`
- 实参初始化形参：按值传参
- 函数返回：按值返回
- 抛出异常、处理异常
- 聚合成员初始化：花括号里的子句初始化聚合的各个元素（[dcl.init.aggr]/4）

**直接初始化（direct-initialization）** 的形式是：初始化器为圆括号表达式列表或花括号初始化列表，与拷贝初始化的 `=` 形式相对。下面这些场合发生的初始化是直接初始化（[dcl.init]/16）：

- 变量定义里的圆括号或花括号形式：`T t(x);`、`T t{x};`
- new 表达式的初始化器：`new T(x)`、`new T{x}`
- `static_cast` 表达式：`static_cast<T>(x)`
- 函数式转换：`T(x)`、`T{x}`
- condition 里声明变量的花括号形式：`if (T t{x}) …`

另有两处由别的条款规定为直接初始化：基类和成员的 mem-initializer（`D() : B(x), m{y} {}`，[class.base.init]/7），以及 lambda 表达式把按复制捕获的实体用于初始化闭包对象的成员（[expr.prim.lambda.capture]/15）。

condition 里声明的变量和类内的默认成员初始化器，也按初始化器的形式归类：`if (T t = x) …`、`T m = x;` 是拷贝初始化，`T m{x};` 是直接列表初始化，默认成员初始化器不能写圆括号（[dcl.init]/15、/16）。

> **Note:** [dcl.init] 里这份清单的写法随版本变过。C++11/14 写成“在 `T x(a);`、`T x{a};` 这两种形式里，以及 new 表达式、static_cast 表达式、函数式转换、基类和成员的初始化器里”；C++17 把“基类和成员的初始化器”改称 mem-initializer，并加上 condition 的花括号形式；C++20 起第一项改按文法写成“初始化器是圆括号表达式列表或花括号初始化列表”，不再单列 mem-initializer。cppreference 的 Direct-initialization 页也是先列语法形式，再列场合。

C++ Primer 列出的前四种拷贝初始化场合（用 `=` 定义变量；传给非引用形参；从返回类型非引用的函数返回；用花括号初始化数组元素或聚合类的成员）都在上面拷贝初始化的清单里。

> **Note:** Primer 的第五种场合是“库容器在初始化容器、调用 insert 或 push 时拷贝初始化它们的元素”，严格说不对：容器是用直接初始化来构造元素的：放进去的那一份副本如此，扩容时把已有元素搬进新内存也如此。标准库不直接写 `new`，而是通过分配器的 `construct` 构造元素（[container.requirements.general]/3、[allocator.traits.members]），对 `std::allocator` 它就是 `::new (p) T(args...)`，带圆括号，是直接初始化（cppreference 的 `std::allocator::construct`、`std::construct_at` 两页）。
>
> 两者的区别只有在 T 的拷贝构造函数是 `explicit` 时才看得出来：拷贝初始化不能用 `explicit` 构造函数，直接初始化可以。设 `struct T { T() = default; explicit T(const T&) = default; };`、`T a;`、`std::vector<T> v;`：
>
> - `v.push_back(a);` 通过：元素由容器直接初始化，相当于 `::new (p) T(a)`。`emplace_back(a)`、`std::vector<T> w(v);` 也通过。
> - `T b = a;` 编译失败：这是拷贝初始化。
> - `std::vector<T> w = {a};` 编译失败，写成 `w{a}` 也一样：花括号要先造出 `std::initializer_list<T>` 的底层数组，数组的元素是拷贝初始化的（见 ii.6），这一步就失败了，还没轮到 vector 自己构造元素。
>
> 以上在 g++ 11.4、clang 14 的 C++11–20 下实测。

#### 2. 对类类型的语义

目标类型 T 是类类型时，标准按顺序判断三种情形（[dcl.init]/17.6）：

1. 初始化表达式是 T 的纯右值（忽略 cv 限定）：这个表达式直接初始化目标对象（17.6.1，C++17 起）。
2. 直接初始化，或者拷贝初始化且源类型是 T 或 T 的派生类：考虑 T 的构造函数，用重载决议选一个，以初始化表达式或表达式列表为实参调用它（17.6.2）。候选的构造函数由 [over.match.ctor]/1 规定：**直接初始化是全部构造函数，拷贝初始化只是转换构造函数。**
3. 其余的拷贝初始化，也就是源类型与 T 无关：列出能把源类型转换成 T 的用户定义转换序列，候选是 T 的转换构造函数和源类型的非 `explicit` 转换函数（[over.match.copy]/1），用重载决议选一个；选中的函数以初始化表达式为实参调用，如果它是构造函数，这次调用的结果是 T 的纯右值，再用它直接初始化目标对象（17.6.3）。

这与 Primer 的说法对得上：拷贝初始化“要求编译器把右操作数拷贝进正在创建的对象，必要时先转换这个操作数”。“拷贝”是情形 2：源是左值时调用拷贝构造函数，是右值时调用移动构造函数，[dcl.init]/15 的注写着 “Copy-initialization may invoke a move”，所以 Primer 说“有时用移动构造函数”；“必要时先转换”是情形 3。

```cpp
std::string s1 = "abc";                  // 情形 3：const char[4] → std::string，选中转换构造函数 string(const char*)
std::string s2 = s1;                     // 情形 2：源是同类左值，调用拷贝构造函数
std::string s3 = std::move(s1);          // 情形 2：源是同类右值，调用移动构造函数
std::string s4 = std::string("abc");     // 情形 1（C++17 起）：s4 就是这个纯右值的结果对象
```

#### 3. 拷贝省略

情形 3 里构造函数的调用结果是一个纯右值，情形 1 又是对纯右值的处理，所以拷贝省略（copy elision）在这里最容易看出版本差别：

- **C++17 之前**，拷贝省略是允许而非强制的（[class.copy.elision]；N3337 是 [class.copy]/31）。`T t = 1;`、`T t = T(1);` 在语义上先有一个临时对象，再由它拷贝或移动构造 t，实现可以省略这一步；省略与否，拷贝或移动构造函数都必须可访问、未被删除。
- **C++17 起**，纯右值不再先物化成临时对象，而是直接构造在它最终的目的地，拷贝或移动构造函数不必存在。cppreference 称这条规则为 prvalue semantics（常说 guaranteed copy elision），并特别指出标准不把它称为 copy elision，因为没有东西被省略。[class.copy.elision]/1 里剩下的，是仍然允许而非强制的几种情形：返回具名局部对象、throw 表达式、异常处理器的形参、协程形参。

| 初始化 | C++11/14 | C++17 起 |
|---|---|---|
| `T t = T(1);`（同类纯右值） | 可以省略，但拷贝或移动构造函数必须可访问、未被删除 | 不拷贝、不移动，不要求它们存在 |
| `T t = 1;`（转换构造函数） | 同上 | 同上 |
| `T t = a;`（同类左值） | 调用拷贝构造函数 | 同左 |
| `T t = D();`（D 派生自 T） | 调用 T 的移动构造函数（切片） | 同左，不在保证之内 |

最后一行里源类型是 D，不是 T，不满足情形 1，按情形 2 选 T 的移动构造函数，用 D 对象里的 T 子对象初始化 t。所以“继承自 T 的纯右值也被省略”的说法不对，C++17 的保证只针对同一个类的纯右值。拷贝或移动构造函数被删除或 private 时，`T t = 1;`、`T t = T(1);` 在 C++11/14 编译失败（`use of deleted function`），C++17 起通过；`T t = a;`（a 是左值）任何版本都失败（g++ 11.4、clang 14 实测）。

`explicit` 与拷贝省略是两件事。若 T 的单参数构造函数是 `explicit`，`T t = "abc";` 在任何版本都是 ill-formed：拷贝初始化的候选里没有它（见 iii）。`T t = T("abc");` 则合法：右边的 `T("abc")` 是函数式转换，属于直接初始化，可以用 `explicit` 构造函数，得到 T 的纯右值，再由它初始化 t（情形 1）；C++11/14 里这最后一步是可以省略的移动或拷贝构造，C++17 起是直接构造。

#### 4. 只能有一个用户定义转换

情形 3 选出的是一个用户定义转换序列，它由一个初始的标准转换序列、一个用户定义转换、一个第二标准转换序列组成，只含一个用户定义转换；如果用户定义转换是构造函数，初始的标准转换序列只能把源类型转换成构造函数的形参类型（[over.ics.user]/1）。直接初始化没有这个限制：它是对构造函数的重载决议，每个实参都可以各自做一次用户定义转换。cppreference 的说法是：拷贝初始化里的隐式转换必须直接从初始化表达式产生 T，而直接初始化期望的是从初始化表达式到构造函数实参的隐式转换。

```cpp
struct T { T(std::string); };           // 转换构造函数，形参类型是 std::string
T t1("abc");                            // 直接初始化：实参 const char[4] → std::string 是一次用户定义转换，通过
T t2 = "abc";                           // 拷贝初始化：const char[4] → std::string → T 要两次用户定义转换，错误
T t3 = std::string("abc");              // 拷贝初始化：源类型 std::string，一次转换就到 T，通过
```

下面的列表初始化在拷贝与直接两种初始化之上多出两件事：候选构造函数的选法（initializer-list 构造函数优先）和对窄化转换的禁止。

### ii. 列表初始化

#### 1. 定义和两种形式

列表初始化（list-initialization）是用花括号初始化列表（braced-init-list）初始化对象或引用（[dcl.init.list]/1）。花括号里用逗号隔开的子句叫这个初始化列表（initializer list）的元素，列表可以为空；元素按出现顺序求值，前一个元素的值计算和副作用先于后一个（/4），这一点与函数调用的实参不同。

列表初始化出现在直接初始化的语境里，叫 **direct-list-initialization**；出现在拷贝初始化的语境里，叫 **copy-list-initialization**。判断标准是所在的语境：

| 语境 | 例子 | 术语 |
|---|---|---|
| 变量定义 | `T t{x};` | direct-list |
| 变量定义，带 `=` | `T t = {x};` | copy-list |
| new 表达式、函数式转换 | `new T{x}`、`T{x}` | direct-list |
| 成员初始化器，默认成员初始化器（不带 `=`） | `: m{x}`、`T m{x};` | direct-list |
| 默认成员初始化器（带 `=`） | `T m = {x};` | copy-list |
| 函数实参、return 语句 | `f({x})`、`return {x};` | copy-list |

两种形式的规则相同，差别只有一处：`explicit` 构造函数（见 iii）。

#### 2. 判定顺序

对类型 T 的对象，下表各项按顺序判断，第一项满足的生效（[dcl.init.list]/3，C++20；左列是标准的项号）：

| 项 | 条件 | 初始化方式 |
|---|---|---|
| 3.1 | 列表含指派符 | T 必须是聚合类，聚合初始化（C++20） |
| 3.2 | T 是聚合类，列表只有一个元素，类型是 T 或 T 的派生类 | 用该元素初始化 |
| 3.3 | T 是字符数组，列表只有一个合适类型的字符串字面量 | 按 [dcl.init.string] 初始化 |
| 3.4 | T 是聚合 | 聚合初始化 |
| 3.5 | 列表为空，T 是有默认构造函数的类类型 | 值初始化 |
| 3.6 | T 是 `std::initializer_list` 的特化 | 构造该对象（见 ii.6） |
| 3.7 | T 是类类型 | 构造函数的重载决议（见 ii.4） |
| 3.8 | T 是固定底层类型的枚举，只有一个元素，直接列表初始化（C++17） | 用该元素转换成 T |
| 3.9 | 列表只有一个元素 | 用该元素初始化 |
| 3.11 | 列表为空 | 值初始化 |

（3.10 是引用类型，3.12 是 ill-formed，略。）

这是 C++20 的顺序，早期版本不同：C++11 原文（N3337）没有 3.1、3.2、3.3、3.8，并且 3.5 排在 3.4 之前；C++14（N4140）起聚合在前，这是 CWG 1301 的结果（CWG 1324 由它解决，追溯适用）；3.2、3.3 出自 CWG 1467，CWG 2137 之后 3.2 只限聚合类；3.8 是 C++17 新增，3.1 是 C++20 新增。

#### 3. 聚合：聚合初始化

3.4 里的聚合（aggregate）是数组，或同时满足下面四条的类（[dcl.init.aggr]/1，C++20）：没有用户声明的或继承来的构造函数；没有 private 或 protected 的直接非静态数据成员；没有虚函数；没有虚基类、private 基类或 protected 基类。这个定义随版本变，本笔记取 C++20：

| 版本 | 相对上一版的变化 | 来源 |
|---|---|---|
| C++11 | 起点：无 user-provided 构造函数、无默认成员初始化器、无 private 或 protected 非静态数据成员、无基类、无虚函数 | N3337 |
| C++14 | 默认成员初始化器不再使类失格 | N3653 |
| C++17 | 允许 public 非虚基类，基类成为聚合的元素 | P0017R1 |
| C++20 | 构造函数一条由“无 user-provided、explicit 或继承来的”收紧为“无用户声明的或继承来的” | P1008R1 |

另有两个缺陷报告追溯适用，所以各版本实际生效的规则比原文多：CWG 1518（P0398R0，追溯 C++11）使有 `explicit` 构造函数或继承来的构造函数的类不再是聚合；CWG 2610（追溯 C++17）使只有 private 或 protected 的直接基类才使类失格。

聚合初始化的规则（[dcl.init.aggr]）：

- 元素：数组是各下标的元素；类是直接基类（按声明顺序）后接直接非静态数据成员（按声明顺序，不含匿名联合的成员）（/2）。
- 每个显式初始化的元素（explicitly initialized element），由对应的子句**拷贝初始化**；窄化转换是 ill-formed（/4）。子句本身是花括号列表时，该元素按列表初始化，规则递归套用。
- 子句比元素少：没有显式初始化的元素，有默认成员初始化器就用它，否则从空初始化列表拷贝初始化；是引用则 ill-formed（/5）。子句比元素多：ill-formed（/11）。
- 花括号可以省略（brace elision，/15–/16）：子句是表达式、不能初始化当前元素，而当前元素本身是子聚合时，就当作省略了子聚合的花括号，依次取子句初始化它的元素。CWG 1270 之后，不带 `=` 的 `T x{...}` 也可以省略。

```cpp
struct Point { int x; int y = 0; };     // C++14 起是聚合（有默认成员初始化器）
Point a{1, 2};                          // x = 1，y = 2
Point b{1};                             // x = 1，y 用默认成员初始化器：0
Point c{};                              // x 从 {} 拷贝初始化：0，y：0
Point d = {1.5};                        // 错误：double → int 是窄化转换
```

C++20 起，圆括号形式 `Point p(1, 2);` 也做聚合初始化（P0960R3）：元素同样拷贝初始化，但允许窄化转换，也没有花括号省略（[dcl.init]/17.6.2）。实测 g++ 11.4 在 `-std=c++20` 下通过，clang 14 不支持。聚合的每个元素都是拷贝初始化，这一点在 iii 里会用到。

#### 4. 非聚合类：initializer-list 构造函数优先

T 是非聚合的类类型时（3.7），用重载决议选构造函数。**initializer-list 构造函数（initializer-list constructor）：第一个形参是 `std::initializer_list<E>` 或它的 cv 限定引用，其余形参都有默认实参（[dcl.init.list]/2）。** 构造函数模板 `template<class U> C(U)` 不算，因为 initializer list 实参会让对应的形参成为非推导语境。

重载决议分两步（[over.match.list]/1；措辞自 C++11 起有过改动，规则不变）：

1. 如果初始化列表非空，或者 T 没有默认构造函数：先只把 T 的 initializer-list 构造函数作为候选函数，实参列表是**整个初始化列表作为一个实参**。
2. 否则，或者上一步没有找到可行函数（viable function）：再做一次重载决议，候选函数是 T 的全部构造函数，实参列表是初始化列表的各个元素。

形参 `std::initializer_list<X>` 可行的条件是：初始化列表的每个元素都能隐式转换成 X；隐式转换序列取各元素中最差的那个转换，列表为空则是恒等转换（[over.ics.list]/5）。**所以“initializer-list 构造函数优先”的准确含义是：只要有一个 initializer-list 构造函数可行，它就被选中，哪怕别的构造函数更匹配；一个都不可行，才进入第 2 步。** [dcl.init.list]/2 的注把它概括为：Initializer-list constructors are favored over other constructors in list-initialization。

列表为空且 T 有默认构造函数时，第 1 步被跳过：直接初始化 `T t{};` 在到达这里之前，就由 3.5 做了值初始化。T 没有默认构造函数时，空列表进入第 1 步，调用 initializer-list 构造函数（实测）。

| 写法 | 选中的构造函数 | 结果 |
|---|---|---|
| `std::vector<int> v{10, 5};` | `vector(initializer_list<int>)` | 2 个元素：10、5 |
| `std::vector<int> v(10, 5);` | `vector(size_type, const int&)` | 10 个元素，都是 5 |
| `std::vector<int> v{10};` | `vector(initializer_list<int>)` | 1 个元素：10 |
| `std::vector<int> v(10);` | `explicit vector(size_type)` | 10 个元素，都是 0 |
| `std::vector<int> v{};` | 默认构造函数（3.5） | 空 |
| `std::vector<std::string> v{3, "x"};` | `vector(size_type, const string&)` | 3 个 "x" |

第 1、3 行是列表初始化，initializer-list 构造函数可行，直接选中；第 2、4 行是圆括号，是直接初始化，没有“优先”这条规则，按普通的重载决议选构造函数（`vector<int> v(10, 5)` 不会选迭代器区间的构造函数模板，它只在 `InputIt` 满足 LegacyInputIterator 时参与重载决议，见 cppreference 的 `std::vector::vector`）。最后一行里，`std::initializer_list<std::string>` 要求每个元素都能隐式转换成 `std::string`，`3` 不能，所以不可行，进入第 2 步，实参是 `3` 和 `"x"`，选中 `vector(size_type, const string&)`。以上结果在 g++ 11.4、clang 14、C++11 到 C++20 下一致。

**窄化转换在选定构造函数之后才检查，不会触发第 2 步**（[dcl.init.list]/3.7 的末句）。第 1 步判断可行性只看隐式转换，窄化转换也是隐式转换：

```cpp
struct N { N(std::initializer_list<int>); N(double, double); };
N n1{1.5, 2.5};   // 错误：第 1 步中 double → int 是隐式转换，initializer-list 构造函数可行，被选中；随后发现需要窄化转换，ill-formed
N n2(1.5, 2.5);   // 圆括号：直接初始化，选 N(double, double)
```

`std::vector<int> v{1, 2.5};` 同理报错（g++ 11.4、clang 14，C++11 与 C++20）。给一个已有的类加上 initializer-list 构造函数，会改变它所有现有 `{...}` 初始化的含义：原来选别的构造函数的，现在可能改选它。

#### 5. 窄化转换

窄化转换（narrowing conversion）是下列隐式转换（[dcl.init.list]/7）：

- 浮点类型到整数类型；
- `long double` 到 `double` 或 `float`，`double` 到 `float`，除非源是常量表达式，且转换后的值在目标类型的取值范围内（即使不能精确表示）；
- 整数类型或无作用域枚举类型到浮点类型，除非源是常量表达式，且转换后的值能容纳在目标类型里，并能转回原类型得到原值；
- 整数类型或无作用域枚举类型到不能表示原类型全部值的整数类型，除非源是常量表达式，且整型提升后的值能容纳；
- 指针类型或成员指针类型到 `bool`（P1957R2，作为缺陷报告追溯适用）。

列表初始化里需要窄化转换就是 ill-formed：聚合的元素、选中的构造函数的实参、`std::initializer_list` 的元素、单元素的初始化（3.9）都是如此。圆括号和 `=` 形式不检查：

```cpp
int a(3.14);    // 直接初始化：允许，a 是 3
int b = 3.14;   // 拷贝初始化：允许，b 是 3
int c{3.14};    // 列表初始化：错误，浮点到整数总是窄化转换
char d{300};    // 错误：常量 300 放不进 char
char e{65};     // 通过：常量 65 放得进 char
```

实测（g++ 11.4、clang 14，C++11 与 C++20）：以上各行的结果一致；非常量表达式的窄化（`int n = 300; char g{n};`、`int* p = nullptr; bool h{p};`），clang 14 报错，g++ 11.4 默认只给 `-Wnarrowing` 警告，加 `-pedantic-errors` 才是错误。

#### 6. std::initializer_list 对象

对应 3.6：`std::initializer_list<E>` 对象由初始化列表构造。实现生成并物化（materialize）一个“N 个 `const E` 的数组”的纯右值，N 是初始化列表的元素个数；数组的每个元素由初始化列表的对应元素**拷贝初始化**，`std::initializer_list<E>` 对象指向这个数组；需要窄化转换则 ill-formed（[dcl.init.list]/5）。这个数组的生命周期与其他临时对象相同，但用它初始化 `std::initializer_list` 对象时，会像绑定引用一样延长它的生命周期（/6）。cppreference 说得更直白：`std::initializer_list` 是对这个数组的轻量代理对象，拷贝它不拷贝底层数组。

由此有两个后果：元素是拷贝初始化，所以元素类型的 `explicit` 构造函数在这里用不上（见 iii）；`std::initializer_list` 只是指向数组，不能比数组活得久。

列表初始化里，直接与拷贝两种形式的差别只剩下 `explicit` 构造函数这一处，下面专门讲它。

### iii. explicit

#### 1. 它限制什么

`explicit` 说明符（explicit-specifier）用在类定义内的构造函数和转换函数的声明上（[dcl.fct.spec]/3）；C++17 起推导指引也可以用，本笔记不讲。没有 `explicit` 的构造函数叫转换构造函数（converting constructor）：它指定了从它的各形参类型到本类类型的转换（[class.conv.ctor]/1；C++11 起不限于单参数）。

`explicit` 的效果由标准对“隐式转换”的定义推出：**表达式 E 能隐式转换成 T，当且仅当声明 `T t = E;` 良构**（[conv]/3）。显式转换则按直接初始化处理，比如 `static_cast<T>(E)` 对应 `T t(E);`（[expr.static.cast]/4）。隐式转换就是拷贝初始化，所以一个构造函数能不能参与隐式转换，就看拷贝初始化能不能用它。标准在 [class.conv.ctor]/2 的注里说得最直接：`explicit` 构造函数和非 `explicit` 的一样构造对象，但只在使用直接初始化语法、或显式使用强制转换的地方。

#### 2. 对各种写法的影响

下面的 E 是 `struct E { explicit E(int); };`：

| 写法 | explicit 构造函数 | 依据 |
|---|---|---|
| 直接初始化 `E x(1);`、`new E(1)`、mem-initializer `: m(1)` | 是候选 | [over.match.ctor]/1 |
| 函数式转换 `E(1)`、`static_cast<E>(1)`、`(E)1` | 是候选（按直接初始化） | [expr.type.conv]、[expr.static.cast]/4 |
| 直接列表初始化 `E x{1};` | 是候选 | [over.match.list]/1 |
| 拷贝初始化 `E x = 1;`、传参、返回、默认成员初始化器 `E m = 1;` | 不是候选 | [over.match.ctor]/1、[over.match.copy]/1 |
| 拷贝列表初始化 `E x = {1};`、`g({1})`、`return {1};` | 是候选，选中则 ill-formed | [over.match.list]/1 |
| 聚合的元素、数组元素、`std::initializer_list` 的元素 | 它们是拷贝初始化：不是候选 | [dcl.init.aggr]/4、[dcl.init.list]/5 |
| C++20 的圆括号聚合初始化 `A a(1);` | 元素是拷贝初始化：不是候选 | [dcl.init]/17.6.2 |

#### 3. “不是候选”与“选中则 ill-formed”

表里第 4 行和第 5 行的 `explicit` 构造函数都不能用，但机制不同，Why 里的 `E e = 1;` 与 `E e = {1};` 就是这两种。再加一个非 `explicit` 的重载，两者的差别立刻显现：

```cpp
struct S { explicit S(int); S(long); };
S s1 = 1;      // 拷贝初始化：候选只有转换构造函数 S(long)，被选中
S s2(1);       // 直接初始化：候选是全部构造函数，S(int) 更匹配
S s3{1};       // 直接列表初始化：同上，选中 S(int)
S s4 = {1};    // 拷贝列表初始化：候选也是全部构造函数，S(int) 更匹配而被选中；它是 explicit，ill-formed
```

实测（g++ 11.4、clang 14）：`s1` 选中 `S(long)`，`s2`、`s3` 选中 `S(int)`，`s4` 编译失败。

`s1` 与 `s4` 的差别在候选函数的集合：拷贝初始化里 `explicit S(int)` 根本不在候选里（[over.match.ctor]/1），重载决议只能在 `S(long)` 里选；拷贝列表初始化把全部构造函数都放进候选（[over.match.list]/1），`S(int)` 最匹配，选中之后才因为它是 `explicit` 而 ill-formed，不会改选 `S(long)`。标准在 [over.match.list]/1 的注里专门指出了这一点：这与别处（[over.match.ctor]、[over.match.copy]）拷贝初始化只考虑转换构造函数不同，并且这条限制只在这次初始化属于重载决议的最终结果时适用。

#### 4. explicit 的拷贝、移动构造函数和默认构造函数

**拷贝、移动构造函数。** 非 `explicit` 的拷贝或移动构造函数是转换构造函数；隐式声明的拷贝或移动构造函数不是 `explicit`（[class.conv.ctor]/3）。把拷贝构造函数声明成 `explicit` 以后：

```cpp
struct X { X(); explicit X(const X&); };
X a;
X b(a);        // 直接初始化：可以
X c = a;       // 拷贝初始化，源是同类左值：候选只有转换构造函数，explicit 的拷贝构造函数不在其中，错误
void h(X);     // h(a)、按值返回 return a; 都是拷贝初始化，同样错误
X d = X();     // C++17 起可以（情形 1：同类纯右值，不查构造函数）；C++11/14 错误
```

（g++ 11.4、clang 14，C++11–20 实测。）

**默认构造函数。** 默认构造函数可以是 `explicit`，它仍用于默认初始化和值初始化（[class.conv.ctor]/2 的注）。但 `= {}` 是拷贝列表初始化：空列表对有默认构造函数的类做值初始化（3.5），而 [over.match.ctor]/1 规定拷贝初始化（包括拷贝初始化语境里的默认初始化）的候选只有转换构造函数，所以：

```cpp
struct Z { explicit Z() = default; int a; };
Z z1;                 // 默认初始化：可以
Z z2{};               // 直接列表初始化，值初始化：可以
Z z3 = Z();           // 可以：Z() 是直接初始化，z3 由同类纯右值初始化
Z z4 = {};            // 错误：拷贝列表初始化
void g(Z);            // g({}) 同样错误
struct W { Z z; };    // W 是聚合
W w{};                // 错误：没有显式初始化的元素 z 要从 {} 拷贝初始化（[dcl.init.aggr]/5）
```

出处是 CWG 1518（P0398R0，2016 年 7 月并入草案，N4659 起；cppreference 标为追溯适用于 C++11）。“空列表的值初始化也落在拷贝初始化的语境里”这一步是按条文推的，两个编译器的结果一致（C++11–20 实测）。

#### 5. explicit 的转换函数

转换函数也可以是 `explicit`（C++11 起）。**`explicit` 转换函数只在直接初始化里作为用户定义转换考虑**（[class.conv.fct]/2）：

```cpp
struct B { explicit operator bool() const; };
B b;
bool x = b;                      // 拷贝初始化：错误
bool y(b);                       // 直接初始化：可以
bool z{b};                       // 直接列表初始化：可以
bool w = static_cast<bool>(b);   // static_cast 按直接初始化：可以
if (b) {}                        // 上下文转换为 bool：可以
```

“上下文转换为 bool”（contextually converted to `bool`）的定义是：E 能被这样转换，当且仅当 `bool t(E);` 良构（[conv]/4），所以 `if`、`while` 的条件、`!`、`&&`、`||`、`?:` 的条件操作数里，`explicit` 的 `operator bool` 都能用。

两个细节：

- 直接初始化一个非类类型的对象时，`explicit` 转换函数只有“返回目标类型，或能经限定转换（qualification conversion）变成目标类型”才是候选（[over.match.conv]/1），所以 `int j(b);` 和 `static_cast<int>(b)` 都是错误：`bool` 到 `int` 是整型提升，不在其内。
- 目标是类类型 C、转换函数是 `explicit operator C() const` 时，`C c = d;` 错误，`C c(d);` 可以：[over.match.copy]/1 为“以单个实参直接初始化 C，初始化绑定到构造函数第一个引用形参的临时对象”这种情形，特别把 `explicit` 转换函数也列为候选。

以上在 g++ 11.4、clang 14、C++11–20 下一致。

#### 6. 条件 explicit 与标准库

C++20 起 `explicit` 可以带常量表达式：`explicit(expr)`，`expr` 是 `bool` 类型的 contextually converted constant expression；不带表达式的 `explicit` 等价于 `explicit(true)`，值为 false 时函数不是 `explicit`（[dcl.fct.spec]/4，P0892R2）。标准库用它表达“能隐式转换才隐式”：`std::pair` 的构造函数是 `explicit(see below)`，`pair(U1&&, U2&&)` 的条件是 `!is_convertible_v<U1, first_type> || !is_convertible_v<U2, second_type>`（[pairs.pair]）。库里常见的 `explicit` 构造函数：

```cpp
std::vector<int> v1 = 5;                // 错误：vector(size_type) 是 explicit（[vector.cons]）
std::vector<int> v2(5);                 // 可以
std::unique_ptr<int> p1 = new int;      // 错误：unique_ptr(pointer) 是 explicit（[unique.ptr.single.ctor]）
std::unique_ptr<int> p2(new int);       // 可以
std::pair<E, int> pr1 = {1, 2};         // 错误：int 不能隐式转换成 E，pair 的这个构造函数是 explicit
std::pair<E, int> pr2{1, 2};            // 可以
```

（g++ 11.4、clang 14，C++11–20 实测；这里的 E 同上。）

## Relation

### i. Issues

**以为 `=` 与圆括号、花括号只是书写差别。** 同一个类 `T`，十四种写法的结果（g++ 11.4、clang 14，`-std=c++11` 到 `c++20`，`-pedantic-errors`，每格在八种配置下完全一致）：

| 写法 | `explicit T(int)` | `T(int)` |
|---|---|---|
| `T x(1);` | ✓ | ✓ |
| `T x{1};` | ✓ | ✓ |
| `T x = 1;` | ✗ | ✓ |
| `T x = {1};` | ✗ | ✓ |
| `void g(T);` 之后 `g(1);` | ✗ | ✓ |
| `void g(T);` 之后 `g({1});` | ✗ | ✓ |
| `T f() { return 1; }` | ✗ | ✓ |
| `T f() { return {1}; }` | ✗ | ✓ |
| `struct A { T m = 1; };` | ✗ | ✓ |
| `struct A { T m{1}; };` | ✓ | ✓ |
| `struct A { T m; }; A a{1};` | ✗ | ✓ |
| `struct A { T m; }; A a{T(1)};` | ✓ | ✓ |
| `static_cast<T>(1)` | ✓ | ✓ |
| `T x = T(1);` | ✓ | ✓ |

- **以为 `explicit` 只管单参数构造函数。** 转换构造函数自 C++11 起不限于单参数：`struct M2 { explicit M2(int, int); };` 挡住 `M2 m = {1, 2};`，`M2 m{1, 2};` 可以（实测）；`explicit Z() = default;` 挡住 `Z z = {};`（iii.4）。
- **以为花括号总是调用 initializer-list 构造函数。** 只有 T 是非聚合类、且有可行的 initializer-list 构造函数时才是（ii.4）。T 是聚合或标量时不涉及构造函数：`Point p{1, 2};`、`int x{5};` 都不是；`std::vector<int> v{};` 是值初始化，调用默认构造函数。
- **以为聚合与构造函数互斥。** 聚合也有隐式声明的拷贝、移动构造函数。C++11 到 C++17，类内写 `A() = default;` 或 `A() = delete;` 的类仍是聚合，`A a{1};` 做聚合初始化，绕过了被删除的构造函数：`struct A { A() = delete; int x; }; A a{1};` 在 C++17 通过，C++20 报错（实测）。C++20 把定义收紧为“无用户声明的构造函数”，就是为了堵上这个口子。
- **以为拷贝构造函数被删除或不可访问，拷贝初始化就一定不行。** 只有真的需要调用它才报错：C++17 之前，同类纯右值和转换构造函数产生的临时对象也要有可访问的拷贝或移动构造函数，C++17 起不需要，同类左值始终需要（i.3）。

### ii. Connection

C++ Core Guidelines 把这些规则写成了建议：C.46 默认把单参数构造函数声明为 `explicit`，理由是避免意料之外的转换，例子就是 `String s = 10;` 得到一个长 10 的字符串，与 `std::vector<int> v = 5;` 同类；C.164 避免隐式转换运算符，不要为了一点方便引入隐式转换（无论是转换运算符，还是非 `explicit` 的构造函数）；ES.23 优先用 `{}` 初始化，指出 `{}` 初始化器禁止窄化转换，也允许 `explicit` 构造函数；它的例外正是 `vector` 这类类型：`vector<int> v1(10)` 是 10 个 0，`v2{10}` 是 1 个元素 10，`v3(1, 2)` 是 1 个元素 2，`v4{1, 2}` 是 2 个元素。

所以，表示“个数”用圆括号，表示“元素列表”用花括号；要禁止窄化转换用花括号；要初始化 `explicit` 的对象，用直接初始化或直接列表初始化。

**See also:**

- [01-initialization-basics.md](01-initialization-basics.md)：§3.1 默认、值、零初始化；§3.2 构造函数与成员初始化列表（mem-initializer 是直接初始化）。
- [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md)：§2.2 初始化形式与初始化阶段的区别。
- [new-delete.md](../../../03-memory-resource/new-delete.md)：new-initializer 的 `()` 与 `{}`。
- [copy-and-move-draft.md](../../../03-memory-resource/copy-and-move-draft.md)：特殊成员函数；拷贝省略与移动的后续内容。

## Example

```cpp
#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

struct Tracer {
    Tracer(int)           { std::puts("    Tracer(int)"); }
    Tracer(const Tracer&) { std::puts("    Tracer(const Tracer&)"); }
    Tracer(Tracer&&)      { std::puts("    Tracer(Tracer&&)"); }
};
struct Derived : Tracer { Derived() : Tracer(0) {} };

struct S {
    explicit S(int) { std::puts("    explicit S(int)"); }
    S(long)         { std::puts("    S(long)"); }
};

struct W {
    W(int, int)                   { std::puts("    W(int, int)"); }
    W(std::initializer_list<int>) { std::puts("    W(initializer_list<int>)"); }
};

static void show(const char* what, const std::vector<int>& v) {
    std::printf("  %-22s size=%zu\n", what, v.size());
}

int main() {
    std::puts("[拷贝初始化]");
    std::puts("  Tracer a = 1;");             Tracer a = 1;              // 转换构造函数
    std::puts("  Tracer b = Tracer(2);");     Tracer b = Tracer(2);      // 同类纯右值
    std::puts("  Tracer c = a;");             Tracer c = a;              // 左值：拷贝构造函数
    std::puts("  Tracer d = std::move(a);");  Tracer d = std::move(a);   // 右值：移动构造函数
    std::puts("  Tracer e = Derived();");     Tracer e = Derived();      // 派生类纯右值：不是同类，走构造函数

    std::puts("[explicit 与候选构造函数]");
    std::puts("  S s1 = 1;");                 S s1 = 1;                  // 拷贝初始化
    std::puts("  S s2(1);");                  S s2(1);                   // 直接初始化
    std::puts("  S s3{1};");                  S s3{1};                   // 直接列表初始化

    std::puts("[initializer-list 构造函数优先]");
    show("vector<int> v{10, 5}",  std::vector<int>{10, 5});
    show("vector<int> v(10, 5)",  std::vector<int>(10, 5));
    show("vector<int> v{10}",     std::vector<int>{10});
    show("vector<int> v(10)",     std::vector<int>(10));
    show("vector<int> v{}",       std::vector<int>{});
    std::vector<std::string> sv{3, "x"};                                // initializer_list<string> 不可行
    std::printf("  %-22s size=%zu\n", "vector<string> v{3,\"x\"}", sv.size());
    std::puts("  W w1{1, 2};");               W w1{1, 2};
    std::puts("  W w2(1, 2);");               W w2(1, 2);
    (void)b; (void)c; (void)d; (void)e; (void)s1; (void)s2; (void)s3; (void)w1; (void)w2;
}
```

构建：`g++ -std=c++17 -fno-elide-constructors -Wall -Wextra -pedantic-errors init_forms.cpp`。`-fno-elide-constructors` 关掉实现自己的拷贝省略，这样允许而非强制的省略就看不到了，剩下的都是标准要求的。

Output:

```text
[拷贝初始化]
  Tracer a = 1;
    Tracer(int)
  Tracer b = Tracer(2);
    Tracer(int)
  Tracer c = a;
    Tracer(const Tracer&)
  Tracer d = std::move(a);
    Tracer(Tracer&&)
  Tracer e = Derived();
    Tracer(int)
    Tracer(Tracer&&)
[explicit 与候选构造函数]
  S s1 = 1;
    S(long)
  S s2(1);
    explicit S(int)
  S s3{1};
    explicit S(int)
[initializer-list 构造函数优先]
  vector<int> v{10, 5}   size=2
  vector<int> v(10, 5)   size=10
  vector<int> v{10}      size=1
  vector<int> v(10)      size=10
  vector<int> v{}        size=0
  vector<string> v{3,"x"} size=3
  W w1{1, 2};
    W(initializer_list<int>)
  W w2(1, 2);
    W(int, int)
```

改用 `-std=c++14 -fno-elide-constructors`，只有 `a` 和 `b` 两处不同，各多一行 `Tracer(Tracer&&)`：

```text
  Tracer a = 1;
    Tracer(int)
    Tracer(Tracer&&)
  Tracer b = Tracer(2);
    Tracer(int)
    Tracer(Tracer&&)
```

Verifies:

- `Tracer a = 1;` 与 `Tracer b = Tracer(2);` 在 C++17 只调用 `Tracer(int)`，关掉省略的选项也不能让它们多出移动构造：这是标准的保证，不是优化。C++14 里同一个选项使两处各多一次 `Tracer(Tracer&&)`，也就是先有临时对象、再移动构造（i.3）。
- 同类左值调用拷贝构造函数，右值调用移动构造函数（i.2 的情形 2）。
- `Tracer e = Derived();` 在 C++14 和 C++17 里都是 `Tracer(int)`（`Derived` 的基类）加一次 `Tracer(Tracer&&)`：源类型是 `Derived`，不是 `Tracer`，不在保证之内（i.3 表的最后一行）。
- `S s1 = 1;` 选中 `S(long)`，`S s2(1);`、`S s3{1};` 选中 `explicit S(int)`：拷贝初始化的候选里没有 `explicit` 构造函数（iii.3）。
- `vector<int> v{10, 5}` 是 2 个元素，`v(10, 5)` 是 10 个元素；`{10}` 是 1 个元素，`(10)` 是 10 个；`{}` 是空；`vector<string> v{3, "x"}` 因为 `initializer_list<string>` 不可行，选了 `(size_type, const string&)`，得 3 个元素（ii.4）。
- `W w1{1, 2};` 选 initializer-list 构造函数，`W w2(1, 2);` 选 `W(int, int)`：同一组实参，花括号与圆括号选的构造函数不同（ii.4）。

环境：g++ 11.4、clang++ 14，`-std=c++17` 与 `-std=c++14`，`-fno-elide-constructors -Wall -Wextra -pedantic-errors` 无警告，两个编译器的输出相同。

## Note

### i. 演进

| 版本 | 变化 |
|---|---|
| C++11 | 列表初始化、`std::initializer_list`、窄化转换的限制；`explicit` 可用于转换函数；转换构造函数不再限于单参数 |
| C++11 的缺陷报告（追溯适用） | CWG 1301（CWG 1324 由它解决）：聚合初始化先于空列表的值初始化；CWG 1270：花括号省略不再限于 `=` 形式；CWG 1467：同类型的单元素列表直接初始化（CWG 2137 又把它限定为聚合类）；CWG 1518（P0398R0）：`explicit` 默认构造函数不能用于拷贝初始化的语境，有 `explicit` 构造函数的类不是聚合；P1957R2：指针到 `bool` 是窄化转换 |
| C++14 | 聚合允许默认成员初始化器（N3653） |
| C++17 | 保证的拷贝省略（P0135R1）；聚合允许 public 非虚基类（P0017R1）；固定底层类型枚举的直接列表初始化（P0138R2） |
| C++20 | 聚合改为“无用户声明的构造函数”（P1008R1）；圆括号聚合初始化（P0960R3）；指派初始化（P0329R4）；`explicit(bool)`（P0892R2） |

### ii. 边角：`T{t}` 与 initializer-list 构造函数

用同类型的对象 `t` 做 `T{t}`：T 是聚合类时，按 3.2 用 `t` 初始化，不涉及构造函数；T 是非聚合类时，initializer-list 构造函数先参与（CWG 2137，追溯适用于 C++11），不会因为实参与 T 同类型就直接调用拷贝构造函数。cppreference 在 list-initialization 页的 Notes 里给了例子：

```cpp
struct Q {
    Q() = default;
    Q(Q const&) = default;
    Q(std::initializer_list<Q>) {}       // 非聚合类的 initializer-list 构造函数
};
Q q;
Q q2 = Q{q};                             // 选 initializer-list 构造函数，不是拷贝构造函数（标准要求）
```

实测 g++ 11.4 按标准选 initializer-list 构造函数，clang 14 选了拷贝构造函数，与标准不符。

### iii. cppreference 与标准的措辞差异

- cppreference 的 explicit specifier 页，示例里有一行注释 `B b4 = {4, 5}; // error: copy-list-initialization does not consider B::B(int, int)`，不准确：[over.match.list]/1 和 cppreference 自己的 list-initialization 页（“both explicit and non-explicit constructors are considered, but only non-explicit constructors may be called”）都说是考虑之后，选中才 ill-formed，iii.3 的实验证实了这一点。
- cppreference 的 aggregate initialization 页把“no user-provided, inherited, or explicit constructors”标为 C++11 起、C++20 前，而 N3337 与 N4140 的原文只有 “no user-provided constructors”：多出的 `explicit` 与继承来的构造函数，来自追溯适用的 CWG 1518。本笔记按实际生效的规则写，`explicit T() = default;` 的类在 `-std=c++11` 下也不是聚合（两个编译器实测）。

## Reference

- cppreference：[Copy-initialization](https://en.cppreference.com/w/cpp/language/copy_initialization)（场合、拷贝初始化的规则、`S s = "abc"` 的例子）；[Direct-initialization](https://en.cppreference.com/w/cpp/language/direct_initialization)；[List-initialization](https://en.cppreference.com/w/cpp/language/list_initialization)（判定顺序、窄化转换、缺陷报告表）；[Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization)（聚合的定义随版本变）；[explicit specifier](https://en.cppreference.com/w/cpp/language/explicit)；[Converting constructor](https://en.cppreference.com/w/cpp/language/converting_constructor)；[Copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)（prvalue semantics）；[std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)；[std::vector::vector](https://en.cppreference.com/w/cpp/container/vector/vector)；[std::allocator::construct](https://en.cppreference.com/w/cpp/memory/allocator/construct)、[std::construct_at](https://en.cppreference.com/w/cpp/memory/construct_at)
- 缺陷报告：[CWG 1270](https://cplusplus.github.io/CWG/issues/1270.html)、[CWG 1301](https://cplusplus.github.io/CWG/issues/1301.html)、[CWG 1324](https://cplusplus.github.io/CWG/issues/1324.html)、[CWG 1467](https://cplusplus.github.io/CWG/issues/1467.html)、[CWG 1518](https://cplusplus.github.io/CWG/issues/1518.html)、[CWG 2137](https://cplusplus.github.io/CWG/issues/2137.html)、[CWG 2610](https://cplusplus.github.io/CWG/issues/2610.html)
- C++ standard (draft)，段号按 C++20（N4861），现行草案的编号有变（[dcl.init] 在现行草案里拆成 [dcl.init.general] 等：拷贝初始化与直接初始化的定义在 /13、/14，语义在 /15）：
  - [dcl.init](https://eel.is/c++draft/dcl.init)：/15 拷贝初始化的场合，及其注（可能调用移动）；/16 直接初始化的场合；/17.6 类类型的语义（17.6.1 同类纯右值，17.6.2 构造函数，17.6.3 用户定义转换）
  - [class.base.init](https://eel.is/c++draft/class.base.init)：/7 mem-initializer 按直接初始化；[expr.prim.lambda.capture](https://eel.is/c++draft/expr.prim.lambda.capture)：/15 按复制捕获的实体直接初始化闭包对象的成员
  - [dcl.init.list](https://eel.is/c++draft/dcl.init.list)：/1 定义和两种形式；/2 initializer-list 构造函数的定义；/3 各项的先后；/4 元素的求值顺序；/5、/6 `std::initializer_list` 对象和底层数组；/7 窄化转换
  - [dcl.init.aggr](https://eel.is/c++draft/dcl.init.aggr)：/1 聚合的定义；/2 元素；/4 元素的拷贝初始化与窄化；/5 没有显式初始化的元素；/11 子句过多；/15、/16 花括号省略
  - [over.match.ctor](https://eel.is/c++draft/over.match.ctor)：/1 直接初始化是全部构造函数，拷贝初始化只是转换构造函数；[over.match.copy](https://eel.is/c++draft/over.match.copy)：/1 拷贝初始化类类型时的用户定义转换；[over.match.conv](https://eel.is/c++draft/over.match.conv)：/1 非类类型的转换函数，直接初始化里 `explicit` 转换函数的限制；[over.match.list](https://eel.is/c++draft/over.match.list)：/1 两步选择、拷贝列表初始化选中 `explicit` 则 ill-formed、及其注
  - [over.ics.list](https://eel.is/c++draft/over.ics.list)：/5 `std::initializer_list<X>` 形参的隐式转换序列；[over.ics.user](https://eel.is/c++draft/over.ics.user)：/1 用户定义转换序列
  - [class.conv.ctor](https://eel.is/c++draft/class.conv.ctor)：/1 转换构造函数，/2 的注（`explicit` 只在直接初始化和强制转换里用；默认构造函数可以是 `explicit`），/3 拷贝和移动构造函数；[class.conv.fct](https://eel.is/c++draft/class.conv.fct)：/2 `explicit` 转换函数只在直接初始化里考虑
  - [conv](https://eel.is/c++draft/conv)：/3 隐式转换 ⟺ `T t = E;` 良构，/4 上下文转换为 `bool` ⟺ `bool t(E);` 良构；[expr.static.cast](https://eel.is/c++draft/expr.static.cast)：/4 显式转换按直接初始化；[expr.type.conv](https://eel.is/c++draft/expr.type.conv)：函数式转换
  - [dcl.fct.spec](https://eel.is/c++draft/dcl.fct.spec)：/1 `explicit-specifier` 的语法，/3 用在类定义内的构造函数和转换函数的声明上，/4 `explicit(bool)`
  - [class.copy.elision](https://eel.is/c++draft/class.copy.elision)：/1 仍然允许而非强制的拷贝省略；[allocator.traits.members](https://eel.is/c++draft/allocator.traits.members)：`construct`；[pairs.pair](https://eel.is/c++draft/pairs.pair)：`pair` 构造函数的 `explicit(see below)`；[vector.cons](https://eel.is/c++draft/vector.cons)；[unique.ptr.single.ctor](https://eel.is/c++draft/unique.ptr.single.ctor)
  - [container.requirements.general](https://eel.is/c++draft/container.requirements.general)：/3 容器用 `allocator_traits<allocator_type>::construct` 构造元素（现行草案在 [container.requirements.pre]/3）；/15（N3337 为 /13）CopyInsertable、MoveInsertable、EmplaceConstructible 的定义，及其注“默认的 `construct` 调用 `::new((void*)p) T(args)`”；[sequence.reqmts](https://eel.is/c++draft/sequence.reqmts)：`push_back`、`insert` 的 “a copy of t”；[vector.modifiers](https://eel.is/c++draft/vector.modifiers)：/1 重新分配与异常保证；[specialized.construct](https://eel.is/c++draft/specialized.construct)：/2 `construct_at` 等价于 `::new (voidify(*location)) T(std::forward<Args>(args)...)`
  - 版本差异（N3337 = C++11，N4140 = C++14，N4659 = C++17）：N3337 [dcl.init.aggr]/1 聚合的定义（含“无默认成员初始化器”“无基类”），[dcl.init.list]/3 空列表在聚合之前，[class.copy]/31 拷贝省略；N4140 [dcl.init.aggr]/1 去掉对默认成员初始化器的限制，[dcl.init.list]/3 聚合在前，[class.copy]/31；N4659 [dcl.init.aggr]/1 允许基类、构造函数一条写作“no user-provided, explicit, or inherited constructors”，[over.match.ctor]/1 写成“直接初始化，或不在拷贝初始化语境里的默认初始化，候选是全部构造函数”（CWG 1518）；直接初始化的定义文字：N3337 [dcl.init]/15（拷贝初始化是 /14），N4140 与 N4659 [dcl.init]/16（拷贝初始化是 /15），写法见 How i.1 的 Note
- 提案（合入时间都在草案仓库的提交信息里核对过）：[N3653](https://wg21.link/N3653)、[P0017R1](https://wg21.link/P0017R1)、[P1008R1](https://wg21.link/P1008R1)、[P0960R3](https://wg21.link/P0960R3)、[P0329R4](https://wg21.link/P0329R4)、[P0398R0](https://wg21.link/P0398R0)、[P0135R1](https://wg21.link/P0135R1)、[P0138R2](https://wg21.link/P0138R2)、[P0892R2](https://wg21.link/P0892R2)、[P1957R2](https://wg21.link/P1957R2)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：[C.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-explicit) By default, declare single-argument constructors explicit；[C.164](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conversion) Avoid implicit conversion operators；[ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list) Prefer the `{}`-initializer syntax

## 附录一：Primer 的四种拷贝初始化场合

先抓住一个关键点：**拷贝初始化（copy-initialization）描述的是一种初始化规则，不代表一定调用拷贝构造函数，也不代表代码里必须出现&#x20;**`=`**。**

C++ Primer 列出的四种场合，实际上是在说明：除了我们熟悉的 `T a = x;`，函数传参、函数返回，以及聚合类型的成员初始化，也会采用拷贝初始化规则。

我逐个解释，并且会特别区分两个容易混淆的地方：**初始化的是谁，以及拷贝初始化在这个场合具体意味着什么。**

### **1. 用&#x20;**`=`**&#x20;定义变量**

这是最直观的一种：

```cpp
int x = 10;

std::string s = "hello";
```

变量 `x` 和 `s` 都采用拷贝初始化。

对 `int` 来说，初始化过程很简单：用 `10` 初始化 `x`。

对 `std::string` 来说，过程稍微复杂一些：

```cpp
std::string s = "hello";
```

大致可以理解为：

1. 字符串字面量 `"hello"` 不能直接作为 `std::string` 对象。
2. `std::string` 提供了一个接受 `const char*` 的非 `explicit` 构造函数。
3. 拷贝初始化允许使用这种隐式转换。
4. 最终构造出 `s`。

对比：

```cpp
std::string s1 = "hello";  // 拷贝初始化
std::string s2("hello");   // 直接初始化
```

两者都能成功，但构造函数的选择规则不同。尤其是拷贝初始化不能使用 `explicit` 构造函数，而直接初始化可以。

注意：这里的“拷贝”不是说先创建一个临时 `string`，再把临时对象复制给 `s`。它只是初始化形式的名称。

### **2. 按值传递函数参数**

看这个例子：

```cpp
void print(std::string s)
{
    // ...
}

int main()
{
    std::string name = "Alice";

    print(name);
}
```

这里有两个不同的对象：

- `name`：调用函数之前就存在的对象。
- `print` 的形参 `s`：函数调用时创建的对象。

调用：

```cpp
print(name);
```

相当于使用实参 `name` 初始化形参 `s`。

因为形参是按值传递的，所以函数得到的是一个独立的 `std::string` 对象，而不是 `name` 本身。这个初始化采用拷贝初始化规则。

由于 `name` 是一个左值，通常会调用 `std::string` 的拷贝构造函数来初始化 `s`。

再看：

```cpp
void func(std::string s);

func("Alice");
```

这个调用也涉及形参的拷贝初始化。因为实参是字符串字面量，编译器需要先考虑能否通过隐式转换得到 `std::string`，再用它初始化形参。

因此，`func("Alice")` 可以调用非 `explicit` 的转换构造函数；如果唯一可用的转换构造函数是 `explicit` 的，就不能这样调用。

**这里要记住：按值传递时，发生拷贝初始化的是形参，不是函数调用表达式本身。**

另外，引用形参不同：

```cpp
void func(const std::string& s);
```

这里的 `s` 是引用，不会创建一个独立的 `std::string` 对象来接收实参，因此不属于上面说的按值传参场景。

### **3. 从非引用返回类型的函数返回对象**

看这个例子：

```cpp
std::string make_string()
{
    std::string s = "hello";
    return s;
}

int main()
{
    std::string result = make_string();
}
```

这里需要区分两个初始化过程：

#### 第一处：函数返回对象的初始化

```cpp
return s;
```

函数的返回类型是 `std::string`，不是引用类型。因此，函数需要产生一个 `std::string` 类型的返回结果对象。

这个返回结果对象由 `return s` 中的表达式初始化，采用拷贝初始化规则。

在这个例子里，编译器通常可以进行 NRVO（具名返回值优化），直接在返回结果对象的位置构造 `s`，避免额外复制或移动。

即使没有进行 NRVO，现代 C++ 也可能在适用条件下将这里的拷贝转为移动。

**第二处：用函数调用结果初始化&#x20;**`result`

```cpp
std::string result = make_string();
```

这一行从语法形式看又是拷贝初始化。但在 C++17 及之后，函数调用产生的同类型纯右值可以直接初始化 `result`，不需要再创建一个临时对象并复制给它。

所以，虽然这里出现了两处拷贝初始化，实际执行时却不一定发生任何拷贝。

为什么要强调“非引用返回类型”？

```cpp
std::string& get_string();
```

如果函数返回的是引用，函数返回的是一个已有对象的引用，而不是创建一个新的 `std::string` 返回结果对象。此时不能把它理解为上面所说的按值返回对象的初始化。

### **4. 用花括号初始化数组元素或聚合类成员**

这是最容易困惑的一种，因为代码里没有 `=`，但仍然涉及拷贝初始化。

先看数组：

```cpp
int arr[3] = {1, 2, 3};
```

数组的三个元素分别是：

```cpp
arr[0]  // 用 1 初始化
arr[1]  // 用 2 初始化
arr[2]  // 用 3 初始化
```

根据聚合初始化规则，花括号中的每个初始化子句分别用于初始化对应的数组元素。这些元素采用拷贝初始化。

再看聚合类：

```cpp
struct Person
{
    std::string name;
    int age;
};

Person p{"Alice", 20};
```

`Person` 是聚合类。初始化时：

```cpp
name  // 用 "Alice" 初始化
age   // 用 20 初始化
```

对应的聚合元素（成员）分别由花括号中的初始化子句进行拷贝初始化。

这里有一个很重要的区别：

```cpp
Person p{"Alice", 20};
```

从整个变量 `p` 的初始化形式看，它是**直接列表初始化**；但从聚合初始化规则看，`p` 的各个成员是由对应的初始化子句**拷贝初始化**的。

也就是说，初始化规则可以分层理解：外层是直接列表初始化，内层的聚合元素初始化却采用拷贝初始化。

### **5. 用一个&#x20;**`explicit`**&#x20;构造函数，把四种场合理解透**

前面四种场合有一个共同点：**它们都采用拷贝初始化规则，因此在需要选择目标类的转换构造函数时，不能使用&#x20;**`explicit`**&#x20;构造函数。**

我们用一个简单的类型验证：

```cpp
struct X
{
    explicit X(int) {}
};
```

看看四种情况：

#### ① 定义变量

```cpp
X x = 10;  // 错误：拷贝初始化，不能使用 explicit X(int)
X y{10};   // 正确：直接列表初始化，可以使用 explicit X(int)
```

#### ② 按值传递参数

```cpp
void func(X x) {}

func(10);     // 错误：形参采用拷贝初始化
func(X{10});  // 正确：先直接列表初始化出 X，再用它初始化形参
```

#### ③ 按值返回对象

```cpp
X make()
{
    return 10;     // 错误：返回结果对象采用拷贝初始化
}

X make2()
{
    return X{10};  // 正确：先构造 X 类型的纯右值
}
```

这里 `return X{10}` 可以成立，是因为 C++17 起，同类型纯右值可以直接初始化函数返回结果对象，不需要调用拷贝或移动构造函数。

#### ④ 初始化聚合类的成员

```cpp
struct Data
{
    X value;
};

Data a{10};     // 错误：value 从 10 拷贝初始化
Data b{X{10}};  // 正确：value 从 X 类型的纯右值初始化
```

注意 `Data b{X{10}}`：

- 外层：`Data` 采用直接列表初始化。
- 内层：成员 `value` 采用拷贝初始化。
- 但由于初始化表达式已经是 `X` 类型的纯右值，C++17 起可以直接初始化成员，不需要调用拷贝构造函数。

这个例子尤其重要，因为它说明：

**拷贝初始化不仅取决于外层变量声明使用什么语法，还要看当前正在初始化的具体对象及其初始化规则。**

### **6. 把四种场合放在一起理解**

| 场合                           | 被初始化的对象   | 拷贝初始化发生在哪里 |
| ---------------------------- | --------- | ---------- |
| `T a = x;`                   | 变量 `a`    | `a` 的初始化   |
| `f(x)`，形参为 `T`               | 函数形参      | 形参的初始化     |
| `T f() { return x; }`        | 函数返回结果对象  | 返回结果对象的初始化 |
| `T a[] = {x,y}`              | 数组元素      | 每个元素的初始化   |
| `struct A { T x; }; A a{x};` | 聚合类成员 `x` | 成员的初始化     |

这几种情况虽然语法不同，但本质上都在回答同一个问题：

**当一个目标对象需要从某个表达式或初始化子句获得初始值时，采用什么规则来完成初始化？**

标准把上述场合归入拷贝初始化。特别是函数传参、函数返回，它们虽然不是显式写在变量声明中的初始化器，也属于初始化过程。

### **7. 最后，回到 Claude 提到的&#x20;**`explicit`**&#x20;拷贝构造函数**

你原文中还有一个容易混淆的地方：`explicit` 不仅可以修饰普通转换构造函数，也可以修饰拷贝构造函数。

例如：

```cpp
struct T
{
    T() = default;
    explicit T(const T&) = default;
};

T a;

T b = a;  // 错误：拷贝初始化不能使用 explicit 拷贝构造函数

void func(T x);

func(a);  // 错误：形参采用拷贝初始化
```

而：

```cpp
std::vector<T> v;
v.push_back(a);  // 可以
```

为什么？

因为 `push_back` 的形参是引用，容器内部再通过分配器构造一个新的元素。对于 `std::allocator`，其构造过程使用类似下面的形式：

```cpp
::new (p) T(a);
```

这里是直接初始化，可以使用 `explicit` 拷贝构造函数。

这也正是为什么：

- 普通的按值传参可能因为 `explicit` 拷贝构造函数而失败。
- `vector::push_back` 却可以成功。

两者看起来都是把一个已有对象放到另一个地方，但采用的初始化规则不同。

### **总结：你真正需要掌握的内容**

建议你把 C++ Primer 的这四种情况理解为一张初始化场景表，而不是四种独立的语法。

1. **变量定义**：`T a = x`，这是最常见的拷贝初始化。
2. **函数传参**：按值传递时，实参用于初始化形参，采用拷贝初始化。
3. **函数返回**：按值返回时，返回表达式用于初始化返回结果对象，采用拷贝初始化。
4. **聚合初始化**：数组元素和聚合类成员由初始化子句初始化，采用拷贝初始化。

最后记住三个容易混淆的概念：

- **拷贝初始化 ≠ 调用拷贝构造函数**：可能调用转换构造函数、移动构造函数，也可能因为复制消除而不调用构造函数。
- **拷贝初始化 ≠ 必须使用&#x20;**`=`：函数传参、返回和聚合成员初始化都没有显式的 `=`，但仍然是拷贝初始化。
- **直接初始化 ≠ 一定不发生拷贝**：直接初始化也可以调用拷贝构造函数，例如 `T b(a);`。

标准也明确说明，拷贝初始化可以调用移动操作。

**对你整理 C++ 初始化笔记来说，掌握这三点和四种场景就够了，不必把它们记成一堆孤立的语法规则。**

## 附录二：拷贝初始化为什么不能用 explicit 构造函数

核心原因是：`explicit`**&#x20;的设计目的，就是禁止构造函数被隐式使用；而拷贝初始化属于允许隐式转换的初始化形式。**

所以，拷贝初始化和 `explicit` 在语义上存在冲突。

我们从一个例子理解。

### **1. 先看普通转换构造函数**

```cpp
struct MyInt
{
    MyInt(int x) {}
};
```

这里的 `MyInt(int)` 是一个非 `explicit` 构造函数。

因此，编译器可以把 `int` 隐式转换成 `MyInt`：

```cpp
MyInt a = 10;  // 可以
MyInt b(10);   // 可以
```

第一种是拷贝初始化，第二种是直接初始化。

再看函数调用：

```cpp
void func(MyInt x) {}

func(10);  // 可以
```

这里为什么也可以？

因为函数形参 `x` 的类型是 `MyInt`，而传入的实参是 `int`。

编译器可以使用 `MyInt(int)`，隐式地将 `10` 转换成 `MyInt`，然后初始化形参。

这就是隐式转换的典型场景。

### **2. 加上 explicit 会怎样？**

现在修改构造函数：

```cpp
struct MyInt
{
    explicit MyInt(int x) {}
};
```

再看：

```cpp
MyInt a = 10;  // 错误
MyInt b(10);   // 正确
MyInt c{10};   // 正确
```

为什么？

因为 `explicit` 表示：

**这个构造函数只能在显式构造对象时使用，不能参与隐式转换。**

对应到上面的三种写法：

| 初始化形式           | 是否允许使用 explicit 构造函数 |
| --------------- | -------------------- |
| `MyInt a = 10;` | 不允许                  |
| `MyInt b(10);`  | 允许                   |
| `MyInt c{10};`  | 允许                   |

函数调用也是一样：

```cpp
void func(MyInt x) {}

func(10);          // 错误
func(MyInt{10});   // 正确
```

第一种需要编译器隐式地把 `int` 转换成 `MyInt`，但 `explicit` 禁止这样做。

第二种是我们先显式构造出 `MyInt` 对象，再把它传给函数。

### **3. 为什么连 explicit 拷贝构造函数也不行？**

再看你之前遇到的例子：

```cpp
struct T
{
    T() = default;
    explicit T(const T&) = default;
};

T a;

T b = a;  // 错误
T c(a);   // 正确
```

这里可能产生一个疑问：

`a` 本身就是 `T` 类型，为什么还涉及隐式转换？

关键在于：**拷贝初始化并不只表示源对象和目标对象类型不一样，它规定的是一种初始化方式。**

`T b = a` 虽然两边都是 `T`，但它仍然是拷贝初始化。根据拷贝初始化的规则，`explicit` 构造函数不参与这种初始化的构造函数选择。

而：

```cpp
T c(a);
```

属于直接初始化，可以使用 `explicit` 拷贝构造函数。

按值传参也是同样的道理：

```cpp
void func(T x);

T a;
func(a);  // 错误：形参采用拷贝初始化
```

这里不是说 `a` 不能被复制，而是 `explicit` 拷贝构造函数不能用于这种初始化形式。

### **4. explicit 到底解决什么问题？**

它主要解决的是：**避免不期望的隐式类型转换。**

例如：

```cpp
struct Distance
{
    Distance(double meters) {}
};

void travel(Distance d) {}
```

如果没有 `explicit`：

```cpp
travel(100.0);
```

可以直接通过编译，编译器隐式地把 `double` 转换成 `Distance`。

但如果加上 `explicit`：

```cpp
struct Distance
{
    explicit Distance(double meters) {}
};
```

就必须明确表达转换意图：

```cpp
travel(100.0);             // 错误
travel(Distance{100.0});   // 正确
```

这样可以避免一些不够直观、甚至可能引发错误的隐式转换。

**最后记住一个关系就够了：**

- 拷贝初始化：允许编译器使用隐式转换规则，但不允许使用 `explicit` 构造函数。
- 直接初始化：显式指定要构造对象的初始化方式，因此可以使用 `explicit` 构造函数。
- `explicit`：控制构造函数能否参与隐式转换，而不是禁止构造函数被调用。

这也是为什么 C++ 允许拷贝构造函数声明为 `explicit`：它可以限制对象被隐式复制初始化，但仍允许调用者通过直接初始化明确地完成复制。

## 附录三：容器元素的初始化形式

这里真正需要弄清楚的是三个问题：

1. 容器接收一个对象时，是否发生了复制？
2. 容器内部创建新元素时，采用什么初始化形式？
3. 为什么有些容器初始化方式又确实会触发拷贝初始化？

我们用你给出的 explicit 拷贝构造函数作为贯穿例子。

### 一、先看 push_back：复制元素不等于拷贝初始化

定义：

```cpp
struct T
{
    T() = default;
    explicit T(const T&) = default;
};

T a;
std::vector<T> v;

v.push_back(a);
```

这段代码可以编译通过。

要理解原因，需要把 push_back 拆成两个阶段。

#### 1. 接收实参

vector 有这样的成员函数：

```cpp
void push_back(const T& value);
```

调用：

```cpp
v.push_back(a);
```

这里发生的是：

```cpp
const T& value = a;
```

value 是引用，只是绑定到 a，没有创建新的 T 对象。

所以这里根本不存在一个需要拷贝初始化的按值形参。

#### 2. 在容器内部创建新元素

现在 vector 需要在自己的存储空间中创建一个新的 T 对象。

容器需要先准备好存储空间，然后在这块尚未构造对象的存储空间上创建元素。

标准规定，allocator-aware container 通过：

```cpp
std::allocator_traits<Allocator>::construct(
    allocator, p, args...
);
```

来构造元素。

对于默认分配器 std::allocator：

C++11～C++17：

核心实现相当于：

```cpp
::new (static_cast<void*>(p)) T(value);
```

C++20 起：

通过 std::construct_at，效果相当于：

```cpp
std::construct_at(p, value);
```

而 construct_at 本身的等价形式也是：

```cpp
::new (voidify(*p)) T(value);
```

这里真正决定初始化形式的是：

```cpp
T(value)
```

它使用圆括号，是直接初始化。

因此，可以把容器内部的过程简化为：

```cpp
// 假设 p 指向已经分配好的存储空间
T* new_element = ::new (p) T(value);
```

这就解释了为什么：

```cpp
explicit T(const T&) = default;
```

不会阻止 push_back(a)。

因为容器构造新元素采用直接初始化，而直接初始化允许使用 explicit 拷贝构造函数。

标准和 cppreference 对这个过程都有明确描述。

补充：以上是 std::allocator 的行为。标准允许特殊分配器提供不同的 construct 定义，因此不应把这个实现形式无条件推广到任意自定义分配器。

### 二、为什么容器拷贝构造时也是直接初始化？

再看：

```cpp
T a;

std::vector<T> v;
v.push_back(a);

std::vector<T> w(v);
```

这里容易产生一个误解：

既然 w 是通过 v 拷贝构造出来的，那么 w 里面的每个元素是不是也采用拷贝初始化？

不是。

这里实际上有两个层次的构造。

#### 第一层：创建容器对象

```cpp
std::vector<T> w(v);
```

这是对 `vector<T>` 对象 w 的直接初始化。

它调用 vector 自己的拷贝构造函数。

#### 第二层：创建容器内部的元素

vector 拷贝构造函数需要为 w 申请自己的存储空间，然后把 v 中的元素复制到新存储空间。

对于每一个元素，构造过程相当于：

```cpp
::new (p) T(*it);
```

其中：

- `p`：新容器中元素的存储位置。
- `*it`：原容器中的元素。

仍然是圆括号形式，因此是直接初始化。

所以：

```cpp
std::vector<T> w(v);
```

可以通过编译，即使 T 的拷贝构造函数是 explicit。

这里还有一个值得注意的细节：

```cpp
std::vector<T> w = v;
```

这一行对外层的 vector 对象采用拷贝初始化，但它内部元素的构造依然通过 allocator 的 construct 完成。

也就是说：

外层容器对象采用什么初始化形式，不决定内部元素采用什么初始化形式。

这和之前讨论的聚合初始化非常相似：需要区分不同层次、不同对象的初始化。

### 三、为什么 initializer_list 反而会失败？

这是整个问题中最关键的反例。

看下面两种写法：

```cpp
std::vector<T> w{a};

std::vector<T> x = {a};
```

两者都不能通过编译。

为什么？因为这里和 push_back 不一样。

#### 1. 花括号触发 initializer_list 构造

vector 有一个构造函数：

```cpp
vector(std::initializer_list<T> init);
```

因此：

```cpp
std::vector<T> w{a};
```

会优先考虑 initializer_list 构造函数。

但 `std::initializer_list<T>` 并不是直接引用原来的对象 a。

编译器需要先生成一个底层数组，保存初始化列表中的元素。

对于：

```cpp
std::vector<T> w{a};
```

可以将过程粗略理解为：

```cpp
const T backing_array[1] = { a };

std::initializer_list<T> init(
    backing_array,
    backing_array + 1
);

std::vector<T> w(init);
```

#### 2. 底层数组的元素采用什么初始化？

这里正好涉及 C++ 初始化规则的一个特殊规定。

根据标准的列表初始化规则：

`std::initializer_list<E>` 的底层数组中的每个元素，都使用对应的初始化子句进行拷贝初始化。

因此，上面的：

```cpp
const T backing_array[1] = { a };
```

其中的数组元素相当于：

```cpp
const T element = a;
```

注意，这里才是真正的拷贝初始化。

而我们的拷贝构造函数是：

```cpp
explicit T(const T&) = default;
```

拷贝初始化不能使用它。

因此底层数组构造失败。

甚至还没有轮到 vector 自己构造内部元素，程序就已经无法通过编译了。

#### 3. 为什么 w = {a} 和 w{a} 都失败？

因为虽然外层初始化形式不同：

```cpp
std::vector<T> w{a};      // 直接列表初始化
std::vector<T> x = {a};  // 拷贝列表初始化
```

但是，两者在构造 `std::initializer_list<T>` 时，都需要创建底层数组。

底层数组的元素都采用拷贝初始化。

所以两种形式都会在这一步失败。

这也说明了一个重要问题：

直接列表初始化不意味着它内部涉及的所有对象都采用直接初始化。

### 四、扩容时，把已有元素搬进新内存也是这样吗？

是的，但这里要区分两个概念：元素的构造形式和实际调用的构造函数。

例如：

```cpp
std::vector<T> v;
v.push_back(a);
v.push_back(a);
```

假设第二次 push_back 导致扩容。

原来的元素不能直接留在旧存储空间里，于是 vector 需要：

1. 分配新的存储空间。
2. 在新存储空间中重新构造已有元素。
3. 构造新插入的元素。
4. 销毁旧存储空间中的元素。
5. 释放旧存储空间。

其中，第二步仍然通过 allocator 的 construct 在新位置构造对象。

对于默认分配器，构造形式仍然相当于：

```cpp
::new (new_p) T(std::move_if_noexcept(*old_p));
```

这里有一个细节：

- 如果选择移动，通常调用移动构造函数。
- 如果选择复制，调用拷贝构造函数。
- 具体选择还与类型的移动构造能力、异常保证等有关。

无论最终选择复制还是移动，在新存储空间中创建对象时，采用的仍然是直接初始化形式。

注意，这不意味着 vector 所有插入操作的所有步骤都是直接初始化。例如，在原有存储空间内移动元素时，也可能使用移动赋值，而赋值本身不是初始化。

### 五、把几个例子放在一起对比

现在回到你的例子：

```cpp
struct T
{
    T() = default;
    explicit T(const T&) = default;
};

T a;
std::vector<T> v;
```

| 操作 | 新对象是谁 | 内部初始化形式 | 结果 |
| --- | --- | --- | --- |
| `T b = a;` | `T b` | 拷贝初始化 | 失败 |
| `T b(a);` | `T b` | 直接初始化 | 通过 |
| `v.push_back(a);` | vector 的新元素 | 直接初始化 | 通过 |
| `v.emplace_back(a);` | vector 的新元素 | 直接初始化 | 通过 |
| `std::vector<T> w(v);` | vector 的新元素 | 直接初始化 | 通过 |
| `std::vector<T> w{a};` | initializer_list 底层数组元素 | 拷贝初始化 | 失败 |
| `std::vector<T> w = {a};` | initializer_list 底层数组元素 | 拷贝初始化 | 失败 |

这里还有一个标准术语值得认识：CopyInsertable（可复制插入）。

标准对这个要求的定义，并不是：

```cpp
T b = a;
```

而是要求下面这个表达式合法：

```cpp
std::allocator_traits<A>::construct(m, p, a);
```

并且构造完成后，新元素的值与原对象等价，原对象的值不变。

它描述的是：一个类型能否通过指定分配器，将已有对象复制构造到容器的存储空间中。

因此，CopyInsertable 中的 Copy 指的是复制构造意义上的复制，而不是语言层面的 copy-initialization。

### 六、为什么 Primer 的第五种场合不严谨？

现在可以准确地解释了。

Primer 想表达的事实是：

容器在接收元素时，可能需要根据已有对象创建一个新的元素副本。

这个事实没有问题。

但它进一步把这种行为称为：

拷贝初始化容器的元素。

这就不严谨了。

因为：

- 元素确实可能调用拷贝构造函数。
- 但容器通常通过 allocator 的 construct 在存储空间中构造元素。
- 对于 std::allocator，其构造形式相当于 T(args...)，属于直接初始化。
- 只有某些特定场合，例如 initializer_list 底层数组的元素初始化，才确实采用拷贝初始化。

我建议你在自己的 C++ 笔记中这样记：

容器在插入、复制元素时，可能需要通过拷贝构造函数创建新元素。但这不意味着元素采用拷贝初始化。对于默认分配器，容器通过 allocator_traits::construct 构造元素，其构造形式相当于 placement new 中的 T(args...)，属于直接初始化。initializer_list 是需要单独注意的情况，其底层数组的元素采用拷贝初始化。

这样既保留了 Primer 想表达的实际场景，也不会把两个不同的 C++ 术语混为一谈。

最值得记住的一句话：复制是构造行为，拷贝初始化是初始化形式。两者不是同一个概念。
