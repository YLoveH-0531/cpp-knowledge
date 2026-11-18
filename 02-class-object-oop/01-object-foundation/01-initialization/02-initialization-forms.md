# Initialization Forms

> 范围：拷贝初始化、列表初始化，以及各写法对 `explicit` 构造函数的不同待遇（`explicit` 本身的规则见 [constructors-and-destructors.md](../../02-class-and-oop/constructors-and-destructors.md) §5）；直接初始化作对照。默认、值、零初始化见 [01-initialization-basics.md](01-initialization-basics.md) §2.2。聚合只讲它在列表初始化里的位置，拷贝省略只讲与拷贝初始化直接相关的规则；引用初始化、`auto` 推导、类模板实参推导不讲。条文以 C++20（N4861）为基线，版本差异见 Note；示例在 g++ 11.4、clang 14 的 C++11–20 下实测，另注除外。

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

弄清这些现象要三件事：拷贝与直接初始化怎样选构造函数（i）；列表初始化怎样选构造函数（ii、iii）；`explicit` 把哪些构造函数挡在哪些写法之外（见 constructors-and-destructors.md §5）。

## What

**写法和场合决定这是哪一种初始化（标准术语）；再结合目标类型和初始化器，决定候选的构造函数、`explicit` 构造函数的待遇、窄化转换是否允许，以及 initializer-list 构造函数是否优先。** 下表以 T 是非聚合类、初始化器是一个表达式 x 为前提：

| 写法 | 标准术语 | 候选的构造函数 | `explicit` 构造函数 | 窄化转换 | initializer-list 构造函数优先 |
|---|---|---|---|---|---|
| `T t = x;` | copy-initialization | 转换构造函数（源类型无关时还有源类的转换函数） | 不是候选 | 允许 | 无 |
| `T t(x);` | direct-initialization | 全部构造函数 | 是候选 | 允许 | 无 |
| `T t{x};` | direct-list-initialization | 两步选择（ii.3） | 是候选 | 禁止 | 有 |
| `T t = {x};` | copy-list-initialization | 两步选择（ii.3） | 是候选，选中则 ill-formed | 禁止 | 有 |

表中是一般情形的构造函数选择。C++17 起，x 是 T 类型的纯右值时，`T t = x;` 和 `T t(x);` 不查构造函数，直接用 x 初始化 t（i.2 情形 1）。

- **转换构造函数**（converting constructor）：没有声明为 `explicit` 的构造函数（[class.conv.ctor]/1），C++11 起不限于单参数。
- **initializer-list 构造函数**：第一个形参是 `std::initializer_list<E>` 或它的 cv 限定引用，其余形参都有默认实参（[dcl.init.list]/2）。

T 是聚合时，花括号通常做聚合初始化（ii.4），不选构造函数；列表只有一个 T 或其派生类的元素时例外，直接用该元素初始化（ii.2 的 3.2）。

## How

### i. 拷贝初始化与直接初始化

#### 1. 发生场合

**拷贝初始化**（copy-initialization，[dcl.init]/15）：

- 变量定义里的 `=` 形式：`T x = a;`
- 按值传实参、按值返回
- 抛出和处理异常
- 聚合成员的初始化：花括号里的子句初始化各元素

**直接初始化**（direct-initialization，[dcl.init]/16）：

- `T x(a);`、`T x{a};`
- `new T(a)`、`new T{a}`
- `static_cast<T>(a)`、函数式转换 `T(a)`、`T{a}`
- condition 里声明变量的花括号形式：`if (T x{a}) …`

另有两处由别的条款规定为直接初始化：基类和成员的 mem-initializer（`C() : base(a), m{b} {}`，[class.base.init]/7），以及 lambda 用复制捕获的实体初始化闭包对象的成员（[expr.prim.lambda.capture]/15）。默认成员初始化器按写法分：`T m = x;` 是拷贝初始化，`T m{x};` 是直接列表初始化，不能写圆括号。

#### 2. 类类型：构造函数的选择

目标类型 T 是类类型时，标准按顺序判断三种情形（[dcl.init]/17.6）：

1. 源是 T 的纯右值（忽略 cv 限定）：直接用它初始化目标对象，不再查构造函数（17.6.1，C++17 起）。
2. 直接初始化；或拷贝初始化且源类型是 T 或 T 的派生类：对 T 的构造函数做重载决议（17.6.2）。**直接初始化的候选是全部构造函数，拷贝初始化的只是转换构造函数**（[over.match.ctor]/1）。
3. 其余的拷贝初始化（源类型无关）：选一个用户定义转换，候选是 T 的转换构造函数和源类型的非 `explicit` 转换函数（17.6.3，[over.match.copy]/1）；选中构造函数时，调用的结果是 T 的纯右值，再用它直接初始化目标。The last step is usually optimized out and the result of the conversion is constructed directly in the memory allocated for the target object, but the appropriate constructor (move or copy) is required to be accessible even though it's not used.(until C++17)

```cpp
std::string s1 = "abc";                  // 情形 3：const char[4] → std::string，转换构造函数 string(const char*)
std::string s2 = s1;                     // 情形 2：同类左值，拷贝构造函数
std::string s3 = std::move(s1);          // 情形 2：同类右值，移动构造函数
std::string s4 = std::string("abc");     // 情形 1（C++17 起）：s4 就是这个纯右值的结果对象
```

所以拷贝初始化不一定调用拷贝构造函数：可能调用转换构造函数、移动构造函数，也可能一个都不调用。

#### 3. C++17 纯右值语义与可选拷贝省略

- **C++17 之前**，`T t = T(1);`、`T t = 1;` 语义上先有临时对象，再拷贝或移动构造 `t`；实现可以省略这一步，但拷贝或移动构造函数必须可访问、未被删除。
- **C++17 起，纯右值语义（保证）**：纯右值不再物化成临时对象，而是直接构造在最终目的地，拷贝或移动构造函数不必存在。这是语言规则，不是优化（cppreference：prvalue semantics，又称 “guaranteed copy elision”）。
- **仍是可选的省略**（各版本相同）：返回具名局部对象（NRVO）、throw 表达式、异常处理器的形参、协程形参（[class.copy.elision]/1）。实现可以省略，但拷贝或移动构造函数仍须可访问、未被删除。

| 初始化 | C++11/14 | C++17 起 |
|---|---|---|
| `T t = T(1);`（同类纯右值） | 可以省略，但拷贝或移动构造函数必须可访问 | 不拷贝、不移动，它们不必存在 |
| `T t = 1;`（转换构造函数） | 同上 | 同上 |
| `T t = a;`（同类左值） | 调用拷贝构造函数 | 同左 |
| `T t = D();`（D 派生自 T） | 调用 T 的移动构造函数 | 同左，不在保证之内 |

最后一行的源类型是 D，不满足情形 1，按情形 2 选 T 的移动构造函数。拷贝或移动构造函数被删除时，`T t = 1;`、`T t = T(1);` 在 C++11/14 编译失败，C++17 起通过；`T t = a;` 任何版本都失败。

#### 4. 用户定义转换的次数限制

一个值至多隐式施加一次用户定义转换（构造函数或转换函数，[class.conv]/4）：拷贝初始化选出的用户定义转换序列，由初始标准转换、一次用户定义转换、第二标准转换组成（[over.ics.user]/1）。直接初始化是对构造函数的重载决议，每个实参可以各做一次。

```cpp
struct T { T(std::string); };
T a("abc");                  // 直接初始化：const char[4] → std::string 一次转换，再调用 T(string)
T b = "abc";                 // 拷贝初始化：const char[4] → std::string → T，两次用户定义转换，错误
T c = std::string("abc");    // 拷贝初始化：一次转换，正确
```

### ii. 列表初始化

#### 1. 定义与两种形式

**列表初始化**（list-initialization）是用花括号初始化列表（braced-init-list）初始化对象或引用（[dcl.init.list]/1）。列表里逗号隔开的子句叫元素，列表可以为空；元素按出现顺序求值，前一个的值计算和副作用先于后一个（/4），与函数实参不同。

出现在直接初始化语境里的叫 **direct-list-initialization**，出现在拷贝初始化语境里的叫 **copy-list-initialization**：

- direct-list：`T x{a}`、`new T{a}`、`T{a}`、`: m{a}`、默认成员初始化器 `T m{a};`
- copy-list：`T x = {a}`、`f({a})`、`return {a};`、默认成员初始化器 `T m = {a};`

对非聚合类，两者的构造函数选择规则相同，只差一处：拷贝列表初始化选中 `explicit` 构造函数时 ill-formed（constructors-and-destructors.md §5.2）。单元素列表的两条规则按语境区分：聚合类的 3.2 和非类类型的 3.9，在拷贝列表初始化里按拷贝初始化、在直接列表初始化里按直接初始化，3.9 里因此 `explicit` 转换函数只在直接列表初始化里可用（constructors-and-destructors.md §5.4）。固定底层类型的枚举（3.8）也只适用于直接列表初始化。

#### 2. 判定顺序

对类型 T 的对象，下表各项按顺序判断，第一项满足的生效（C++20 [dcl.init.list]/3，左列是项号）：

| 项 | 条件 | 做法 |
|---|---|---|
| 3.1 | 列表含指派符 | T 必须是聚合类，聚合初始化（C++20） |
| 3.2 | T 是聚合类，列表只有一个元素，类型是 T 或 T 的派生类 | 用该元素初始化：直接列表初始化按直接初始化，拷贝列表初始化按拷贝初始化 |
| 3.4 | T 是聚合 | 聚合初始化 |
| 3.5 | 列表为空，T 是有默认构造函数的类 | 值初始化 |
| 3.6 | T 是 `std::initializer_list` 的特化 | 构造该对象（iii） |
| 3.7 | T 是类类型 | 构造函数重载决议（ii.3） |
| 3.9 | T 不是类，列表只有一个元素（T 是引用时，被引用类型要与元素类型 reference-related） | 用该元素直接（或拷贝）初始化，禁止窄化转换 |
| 3.11 | 列表为空 | 值初始化 |

表中略去 3.3 字符数组、3.8 固定底层类型的枚举（C++17，只适用于直接列表初始化）、3.10 其余的引用。

#### 3. 非聚合类：initializer-list 构造函数优先

T 是非聚合类（3.7）时，构造函数分两步选（[over.match.list]/1）：

1. 列表非空，或 T 没有默认构造函数：先只考虑 initializer-list 构造函数，**整个列表作一个实参**。
2. 否则，或第 1 步没有可行函数：考虑全部构造函数，列表的**各元素作实参**。

`std::initializer_list<X>` 形参可行，当且仅当列表的每个元素都能隐式转换成 X（[over.ics.list]/5）。**所以“优先”的准确含义是：只要有一个 initializer-list 构造函数可行就选它，哪怕别的构造函数更匹配。**

| 写法 | 选中的构造函数 | 结果 |
|---|---|---|
| `std::vector<int> v{10, 5};` | `vector(initializer_list<int>)` | 2 个元素：10、5 |
| `std::vector<int> v(10, 5);` | `vector(size_type, const int&)` | 10 个元素，都是 5 |
| `std::vector<int> v{10};` | `vector(initializer_list<int>)` | 1 个元素：10 |
| `std::vector<int> v(10);` | `explicit vector(size_type)` | 10 个元素，都是 0 |
| `std::vector<int> v{};` | 默认构造函数（3.5） | 空 |
| `std::vector<std::string> v{3, "x"};` | `vector(size_type, const string&)` | 3 个 `"x"`：`3` 不能转成 `std::string`，第 1 步不可行 |

**窄化转换在选定构造函数之后才检查，不会回退到第 2 步**（[dcl.init.list]/3.7；窄化转换的定义见 /7）：第 1 步判断可行性只看隐式转换，窄化转换也是隐式转换。

```cpp
struct N { N(std::initializer_list<int>); N(double, double); };
N a{1.5, 2.5};   // 错误：initializer_list<int> 构造函数可行，被选中，随后发现要窄化
N b(1.5, 2.5);   // 正确：圆括号是直接初始化，选 N(double, double)
```

#### 4. 聚合初始化在列表初始化中的位置

T 是聚合时（3.4），列表初始化按元素的顺序初始化聚合，不调用聚合类自己的构造函数（[dcl.init.aggr]）：

- **聚合**是数组，或同时满足下面四条的类：没有用户声明的或继承来的构造函数；没有 private 或 protected 的直接非静态数据成员；没有虚函数；没有虚基类、private 基类或 protected 基类（/1，C++20；各版本的定义见 Note）。
- **元素**：数组是各下标的元素；类是直接基类（按声明顺序）后接直接非静态数据成员（按声明顺序，不含匿名联合的成员）（/2）。
- 每个显式初始化的元素，由对应的子句**拷贝初始化**，窄化转换是 ill-formed；子句本身是花括号列表时，该元素按列表初始化，规则递归套用（/4）。
- 没有子句的元素：有默认成员初始化器就用它，否则从空列表拷贝初始化，引用成员则 ill-formed（/5）。子句比元素多：ill-formed（/11）。
- 子句是表达式、不能初始化当前元素，而当前元素是子聚合时，省略子聚合的花括号（brace elision，/15–/16）。

```cpp
struct Point { int x; int y = 0; };     // C++14 起是聚合（有默认成员初始化器）
Point a{1, 2};                          // x = 1，y = 2
Point b{1};                             // x = 1，y 用默认成员初始化器：0
Point c{};                              // x 从 {} 拷贝初始化：0，y：0
Point d = {1.5};                        // 错误：double → int 是窄化转换
```

C++20 起，`Point p(1, 2);` 也做聚合初始化。

### iii. std::initializer_list 的底层数组

对应 3.6：`std::initializer_list<E>` 对象由列表构造。实现生成并物化一个“N 个 `const E` 的数组”（N 是元素个数）；数组的每个元素由列表的对应元素**拷贝初始化**，`initializer_list` 对象指向这个数组、不拥有它；需要窄化转换则 ill-formed（[dcl.init.list]/5）。现行草案和 cppreference 把这个数组叫 backing array。

数组的生命周期同其他临时对象，用它初始化 `initializer_list` 对象时才像绑定引用一样被延长（/6）。拷贝 `initializer_list` 不拷贝数组（[initializer.list.syn]/1 的注），所以 `initializer_list` 不能比数组活得久：

```cpp
std::vector<int> v{1, 2, 3};                                     // 数组是函数实参，活到完整表达式结束
std::initializer_list<int> il = {1, 2, 3};                       // 数组与 il 同寿
struct A { std::initializer_list<int> m; A() : m{1, 2, 3} {} };  // ill-formed：m 悬空（clang 14 报错，g++ 11.4 只在 -Wextra 下警告）
```

元素是拷贝初始化，所以元素类型的 `explicit` 构造函数在这里用不上。容器自己构造元素用的是直接初始化：对 `std::allocator`，`allocator_traits::construct` 在已分配的存储上写 `::new (p) T(args...)`（C++20 起经 `std::construct_at`，效果相同；[container.requirements.general]/3、[allocator.traits.members]）。两者的差别：

```cpp
struct T { T() = default; explicit T(const T&) = default; };
T a;
std::vector<T> v;
v.push_back(a);       // 正确：容器内部直接初始化新元素
T b = a;              // 错误：拷贝初始化不能用 explicit 拷贝构造函数
std::vector<T> w{a};  // 错误：失败在底层数组，元素要从 a 拷贝初始化，还没轮到 vector 构造元素
```

自定义分配器的 `construct` 可以不同，以上针对 `std::allocator`。

## Relation

### i. Issues

- **以为拷贝初始化一定写 `=`，或一定调用拷贝构造函数。** 见 i.1–i.3。
- **以为列表初始化总调用 initializer-list 构造函数。** 聚合和空列表各有自己的路径（ii.2）。
- **以为聚合的花括号总是聚合初始化。** 列表只有一个 T 或其派生类的元素时，是用该元素初始化整个对象（ii.2 的 3.2）：A 是聚合、a0 是 A 的对象时，`A a{a0};` 拷贝 a0，不是用 a0 初始化第一个成员。
- **以为 `explicit` 只管单参数构造函数。** 转换构造函数自 C++11 起不限于单参数：`struct M { explicit M(int, int); };` 挡住 `M m = {1, 2};`，不挡 `M m{1, 2};`。
- **以为聚合与构造函数互斥。** C++11 到 C++17，类内写 `A() = delete;` 的类仍是聚合，`A a{1};` 绕过被删除的构造函数；C++20 起它不再是聚合。
- **以为复制元素就是拷贝初始化。** 见 iii。

### ii. Connection

C++ Core Guidelines 把这些规则写成建议：C.46 默认把单参数构造函数声明为 `explicit`（例子是 `String s = 10;`，与 Why 里的 `std::vector<int> a = 5;` 同类）；C.164 避免隐式转换运算符；ES.23 优先用 `{}` 初始化，因为它禁止窄化转换，也允许 `explicit` 构造函数；例外是 `vector` 这类容器：`vector<int> v1(10)` 是 10 个 0，`v2{10}` 是 1 个元素。

所以，通常用圆括号表示“个数”，用花括号表示“元素列表”；要禁止窄化转换用花括号；要初始化 `explicit` 的对象，用直接初始化或直接列表初始化。

**See also:**

- [01-initialization-basics.md](01-initialization-basics.md)：§2.1 形式与种类的关系；§2.2 默认、值、零初始化。
- [constructors-and-destructors.md](../../02-class-and-oop/constructors-and-destructors.md)：构造函数与成员初始化列表（mem-initializer 是直接初始化）；§5 `explicit` 对各种初始化写法的影响。
- [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md)：§2.2 静态初始化与动态初始化。
- [new-delete.md](../../../03-memory-resource/new-delete.md)：new-initializer 的 `()` 与 `{}`。
- [copy-and-move-draft.md](../../../03-memory-resource/copy-and-move-draft.md)：特殊成员函数；拷贝省略与移动的后续内容。

## Note

### i. 版本变化

| 版本 | 变化 |
|---|---|
| C++11 | 列表初始化、`std::initializer_list`、窄化转换的限制；`explicit` 可用于转换函数；转换构造函数不再限于单参数；聚合的起点：无 user-provided 构造函数、无默认成员初始化器、无 private 或 protected 非静态数据成员、无基类、无虚函数（N3337） |
| C++11 的缺陷报告（追溯适用） | CWG 1301（CWG 1324 由它解决）：聚合初始化先于空列表的值初始化；CWG 1270：花括号省略不再限于 `=` 形式；CWG 1467：同类型的单元素列表直接初始化（CWG 2137 又限定为聚合类，即现在的 3.2）；CWG 1518（P0398R0）：`explicit` 默认构造函数不能用于拷贝初始化语境，有 `explicit` 构造函数或继承来的构造函数的类不是聚合；P1957R2：指针到 `bool` 是窄化转换；CWG 2610（追溯 C++17）：只有 private 或 protected 的直接基类才使类不是聚合 |
| C++14 | 聚合允许默认成员初始化器（N3653） |
| C++17 | 保证的拷贝省略（P0135R1）；聚合允许 public 非虚基类（P0017R1）；固定底层类型枚举的直接列表初始化（P0138R2） |
| C++20 | 聚合改为“无用户声明的构造函数”（P1008R1）；圆括号聚合初始化（P0960R3）；指派初始化（P0329R4）；`explicit(bool)`（P0892R2） |

### ii. 措辞差异

- 列表初始化的判定顺序（ii.2）逐版本不同：C++11 原文没有 3.1、3.2、3.3、3.8，并且 3.5 排在 3.4 之前；C++14 起聚合在前；3.2、3.3 出自 CWG 1467，CWG 2137 之后 3.2 只限聚合类；3.8 是 C++17 新增，3.1 是 C++20 新增。
- [dcl.init] 里直接初始化场合的清单，措辞随版本变，内容不变：C++11/14 按“`T x(a);`、`T x{a};` 两种形式，以及 new、static_cast、函数式转换、基类和成员的初始化器”列；C++17 加上 condition 的花括号形式；C++20 起第一项按文法写成“圆括号表达式列表或花括号初始化列表”，mem-initializer 改由 [class.base.init]/7 规定。现行草案里拷贝初始化与直接初始化的定义在 [dcl.init.general]/13、/14，语义在 /15。
- cppreference 的 explicit specifier 页，示例注释 `B b4 = {4, 5}; // error: copy-list-initialization does not consider B::B(int, int)` 不准确：[over.match.list]/1 和它自己的 list-initialization 页都说是考虑之后，选中才 ill-formed。
- cppreference 的 aggregate initialization 页把“no user-provided, inherited, or explicit constructors”标为 C++11 起、C++20 前，而 N3337 原文只有 “no user-provided constructors”：多出的 `explicit` 与继承来的构造函数来自追溯适用的 CWG 1518。本篇按实际生效的规则写。

## Reference

- cppreference：[Copy-initialization](https://en.cppreference.com/w/cpp/language/copy_initialization)、[Direct-initialization](https://en.cppreference.com/w/cpp/language/direct_initialization)、[List-initialization](https://en.cppreference.com/w/cpp/language/list_initialization)、[Aggregate initialization](https://en.cppreference.com/w/cpp/language/aggregate_initialization)、[explicit specifier](https://en.cppreference.com/w/cpp/language/explicit)、[Converting constructor](https://en.cppreference.com/w/cpp/language/converting_constructor)、[Copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)、[std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)、[std::allocator::construct](https://en.cppreference.com/w/cpp/memory/allocator/construct)
- C++ standard (draft)，段号按 C++20（N4861），现行草案的编号不同处已在正文注明：
  - [dcl.init](https://eel.is/c++draft/dcl.init)：/15 拷贝初始化，/16 直接初始化，/17.6 类类型的语义；[dcl.init.list](https://eel.is/c++draft/dcl.init.list)：/1 定义，/2 initializer-list 构造函数，/3 判定顺序，/4 求值顺序，/5、/6 底层数组，/7 窄化转换；[dcl.init.aggr](https://eel.is/c++draft/dcl.init.aggr)：/1 聚合，/2 元素，/4、/5 元素的初始化，/11 子句过多，/15、/16 花括号省略
  - [over.match.ctor](https://eel.is/c++draft/over.match.ctor)、[over.match.copy](https://eel.is/c++draft/over.match.copy)、[over.match.list](https://eel.is/c++draft/over.match.list)、[over.ics.list](https://eel.is/c++draft/over.ics.list)、[over.ics.user](https://eel.is/c++draft/over.ics.user)
  - [class.conv](https://eel.is/c++draft/class.conv)：/4、[class.conv.ctor](https://eel.is/c++draft/class.conv.ctor)
  - [class.copy.elision](https://eel.is/c++draft/class.copy.elision)、[class.base.init](https://eel.is/c++draft/class.base.init)：/7、[expr.prim.lambda.capture](https://eel.is/c++draft/expr.prim.lambda.capture)：/15
  - [initializer.list.syn](https://eel.is/c++draft/initializer.list.syn)：/1 的注、[container.requirements.general](https://eel.is/c++draft/container.requirements.general)：/3（现行草案在 [container.requirements.pre]/3）、[allocator.traits.members](https://eel.is/c++draft/allocator.traits.members)
- 缺陷报告：[CWG 1270](https://cplusplus.github.io/CWG/issues/1270.html)、[CWG 1301](https://cplusplus.github.io/CWG/issues/1301.html)、[CWG 1324](https://cplusplus.github.io/CWG/issues/1324.html)、[CWG 1467](https://cplusplus.github.io/CWG/issues/1467.html)、[CWG 1518](https://cplusplus.github.io/CWG/issues/1518.html)、[CWG 2137](https://cplusplus.github.io/CWG/issues/2137.html)、[CWG 2610](https://cplusplus.github.io/CWG/issues/2610.html)
- 提案：[N3653](https://wg21.link/N3653)、[P0017R1](https://wg21.link/P0017R1)、[P1008R1](https://wg21.link/P1008R1)、[P0960R3](https://wg21.link/P0960R3)、[P0329R4](https://wg21.link/P0329R4)、[P0398R0](https://wg21.link/P0398R0)、[P0135R1](https://wg21.link/P0135R1)、[P0138R2](https://wg21.link/P0138R2)、[P0892R2](https://wg21.link/P0892R2)、[P1957R2](https://wg21.link/P1957R2)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)：[C.46](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rc-explicit)、[C.164](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#ro-conversion)、[ES.23](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-list)
