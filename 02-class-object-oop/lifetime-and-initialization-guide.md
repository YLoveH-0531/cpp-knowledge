# 对象生命周期、存储期与初始化：学习指南

> 主线第 1/4 篇。排期：W1 第 1 篇，3h，标准档（正文 ≤ 100 行）。笔记正文由你自己写；这份指南只给承接关系、资料、过程和实验设计，实验的参考观察放在最后一节。

## 在主线上的位置

[RAII 笔记](../03-memory-resource/RAII.md) → **① 构造（本篇）** → [② 移动](../03-memory-resource/copy-and-move-guide.md) → [③ 智能指针](../03-memory-resource/smart-pointers-guide.md) → [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)

### 承接：RAII 笔记用了、但没有展开的前提

[RAII 笔记](../03-memory-resource/RAII.md)的 How ii 给出构造失败时的三条规则：非委托构造函数抛出时，对象自己的析构函数不会被调用；已经构造完成的基类和成员子对象按构造的逆序销毁；`new` 表达式里还会释放已分配的存储。这三条背后是同一个前提：**对象从初始化完成的那一刻才算存在。** 构造函数还没返回，对象就还不存在，所以没有析构；已经构造完的子对象是存在的，所以要销毁。RAII 笔记在使用这个前提，没有展开它。

[new-delete 笔记](../03-memory-resource/new-delete.md)把"对象"和"存储"分成两件事：`new` 表达式 = 分配存储 + 初始化对象。本篇补上缺的另一半：存储从哪来、能活多久（**存储期**）；对象何时开始、何时结束（**生命周期**）；**初始化**怎样把一块存储变成一个对象。

你已经在 [lifetime-and-initialization.md](lifetime-and-initialization.md)（原 temp.md）写下了这篇的起点：初始化是比"调用构造函数"更广的过程，基类和成员子对象在构造函数体执行之前完成初始化，整个对象在非委托构造函数返回时初始化完成。本篇要把这段话变成一张能查、能验证的模型。

### 这篇的整体模型：一条时间线加一条轴

- **时间线（对象）**：分配存储 → 初始化完成（生命周期开始）→ 使用（赋值只出现在这一段，作用在已存在的对象上）→ 销毁（生命周期结束）→ 释放存储。初始化是对象从无到有，赋值是对已有对象换值，两者要成对写清。
- **轴（名字）**：作用域只管名字在代码的哪一段可见，是编译期的事，和对象的存续是两件事。只有自动存储期的对象，作用域结束才恰好等于生命周期结束；静态对象、`new` 出来的对象、临时对象都对不上。
- **与 RAII 的接口**：RAII 把资源的获取绑在初始化上、释放绑在销毁上，之所以可靠，是因为销毁的时机和顺序是确定的。这一头在本篇写清，构造失败时和异常展开时怎样销毁，由 RAII How ii 和 ④ 承接。

### 引出：用另一个对象来初始化新对象

初始化里有一类特殊的形式：用同类型的另一个对象初始化新对象——`T a = b;`、按值传参、按值返回，走的都是拷贝构造或移动构造。当对象是一份资源的管理者时，这一步决定资源怎么办：复制、转移，还是禁止。这是下一篇 [② 移动](../03-memory-resource/copy-and-move-guide.md)的问题。

## 读完应能回答

1. 对象的生命周期从哪一刻开始、到哪一刻结束？它和存储期（自动、静态、线程、动态）是一回事吗？存储为什么能先于对象存在、晚于对象结束？
2. 作用域管的是什么？它和生命周期是一回事吗？举出作用域结束而对象还活着的例子，再举出名字不可见而对象仍在的例子。
3. `T x;`、`T x{};`、`T x = T();`、`new T`、`new T()` 分别是哪种初始化？对内置类型、对带默认成员初始化器的类、对有用户提供构造函数的类，各自得到什么？初始化和赋值的区别是什么？构造函数为什么该用初始化列表而不是在函数体里赋值？
4. 构造一个派生类对象时，基类、成员、构造函数体的先后顺序由什么决定？成员的顺序由声明顺序还是初始化列表顺序决定？销毁的顺序呢？
5. 全局、静态局部、自动、动态四种存储期的对象，各在什么时候构造、什么时候销毁？

## 本篇不写什么

- 构造失败时的清理：RAII 笔记 How ii 和 new-delete 笔记已经写了，这里只放链接。
- placement new 里"存储先于对象、晚于对象"的时间差：new-delete 的 Example 在验证，不重复。
- 继承体系里的构造细节（虚基类、构造期间调用虚函数）：留给 W4 的继承与多态。
- 名字查找、命名空间、ADL：作用域这里只讲“名字可见范围与对象生命周期不是一回事”，查找规则留给 W4 的名字查找。
- 拷贝构造、移动构造本身的规则：下一篇。

## 待写概念（只列名词，内容你自己写）

| 对应的问 | 名词 |
|---|---|
| 1 生命周期、存储期 | 对象（object）与存储（storage）；存储期（storage duration）的四种：自动（automatic）、静态（static）、线程（thread）、动态（dynamic）；生命周期的开始与结束（begin / end of lifetime）；构造完成（fully constructed）；临时对象（temporary）与完整表达式（full-expression） |
| 2 作用域 | 名字（name）、声明（declaration）与定义（definition）；作用域（scope）；块作用域（block scope）、命名空间作用域（namespace scope，全局作用域属于它）；名字隐藏（name hiding） |
| 3 初始化与赋值 | 初始化（initialization）与赋值（assignment）；默认初始化（default）、值初始化（value）、零初始化（zero）；直接初始化（direct）、拷贝初始化（copy，只点到，细节在 ②）；列表初始化（list，分直接列表与拷贝列表）；聚合（aggregate）与聚合初始化；窄化转换（narrowing）；`initializer_list` 构造函数优先；默认成员初始化器（default member initializer）；成员初始化列表（member initializer list）；委托构造（delegating constructor）；`explicit`；不确定值（indeterminate value）；最令人烦恼的解析（most vexing parse） |
| 4 构造与销毁的顺序 | 基类子对象（base subobject）、成员子对象（member subobject）；按声明顺序初始化；构造函数体；销毁与构造相反；`-Wreorder` |
| 5 各存储期的时机 | 静态初始化（static initialization，含常量初始化与零初始化）与动态初始化（dynamic initialization）；静态局部变量的一次性初始化；跨翻译单元的静态初始化顺序（static initialization order fiasco）；静态对象在 `main` 之后按构造完成的逆序销毁 |

不在本篇的名词（别提前写）：拷贝省略、RVO / NRVO、值类别、移动语义属于 ②；`new` 失败与析构不抛异常属于 RAII How ii 和 ④；虚基类、构造期间调用虚函数属于 W4。

## 资料

### 必读（第 2 步，共约 50 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| cppreference：[Initialization](https://en.cppreference.com/w/cpp/language/initialization) | 开头的语法总览；Default initialization、Value initialization、Zero initialization 三节；Non-local variables 里静态初始化与动态初始化的区分 | 第 3、5 问 | 20 min |
| cppreference：[Constructors and member initializer lists](https://en.cppreference.com/w/cpp/language/constructor) | Initialization order 一节（基类、成员、构造函数体的顺序）；顺带扫一眼 Delegating constructor | 第 4 问 | 10 min |
| cppreference：[Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)、[Lifetime](https://en.cppreference.com/w/cpp/language/lifetime) | 前者读四种存储期的定义；后者读对象生命周期的开始与结束，跳过 linkage 和 implicit-lifetime 类型 | 第 1、5 问 | 15 min |
| cppreference：[Scope](https://en.cppreference.com/w/cpp/language/scope) | 开头的定义和 Block scope、Namespace scope 两节；名字隐藏读一眼，Point of declaration 的细节跳过 | 第 2 问 | 5 min |

### 写完后对照（第 4 步，不占读的时间）

- C++ Primer 5e：2.2.1（初始化、列表初始化、默认初始化）、7.5.1（构造函数初始值列表，含成员初始化顺序）。
- *Effective Modern C++* Item 7：`()` 与 `{}` 的区别——窄化转换、最令人烦恼的解析、`initializer_list` 构造函数优先。
- C++ Core Guidelines：C.41、C.45、C.47、C.48、C.49、ES.20、ES.23，每条读标题和 Reason。

### 你自己的旧笔记：写完初稿后核对，不先读

| 位置 | 看哪里 | 对应的名词 | 看之前要知道 |
|---|---|---|---|
| [lifetime-and-initialization.md](lifetime-and-initialization.md) | 全文 | 初始化、赋值、生命周期、作用域 | 这是你第 1 步的成果，不是资料；按上一轮标的作用域、生命周期两处核对 |
| [chapter18/README.md](../../My_PPP/chapter18/README.md) | 只看“列表初始化（List-initialization）判定顺序”一节 | 列表初始化、聚合、窄化、`initializer_list` 构造函数优先 | “聚合 ⟺ 没有用户声明的构造函数”只对 C++20 成立，C++11 到 C++17 里 `P() = default;` 的类仍是聚合 |
| 同上 | 第 2 节 `initializer_list` 底层数组的寿命、第 4 节临时对象的销毁 | 临时对象、生命周期 | 可选；“内存段”那几段是实现细节，本篇不用 |
| OneNote：C++ Primer 的 `Chapter 2` → `2.2 Variables` | 只看“Initialization is not assignment”和 Default / Value / List 的分类 | 初始化与赋值、默认 / 值 / 列表初始化 | 其中“Default initialization 归在 Direct 下”不对；“Value initialization MUST use the direct form”只适用于 `vector<int> v(10)` 这种只给大小的写法，不能当定义 |
| OneNote：C++ Primer 的 `Chapter 13` → Copy initialization；Professional 的 `18. Direct and Copy initialization` | 先不看 | 拷贝初始化 | 留到写 ②；里面“继承自 T 的纯右值也被省略”不对，C++17 的保证只针对同一个类的纯右值 |
| [RAII.md](../03-memory-resource/RAII.md) | How ii | 构造完成、构造失败时的销毁 | 本篇只链接，不重写 |
| [new-delete.md](../03-memory-resource/new-delete.md) | 讲“对象与存储”的那一段 | 对象与存储 | 同上 |

### 备查（不读）

- 标准：[basic.life](https://eel.is/c++draft/basic.life)、[basic.stc](https://eel.is/c++draft/basic.stc)、[dcl.init](https://eel.is/c++draft/dcl.init)、[class.base.init](https://eel.is/c++draft/class.base.init)。
- Nicolai Josuttis，*The Nightmare of Initialization in C++*（CppCon 2018）。
- C++ FAQ 里的 "static initialization order fiasco"：跨翻译单元的静态初始化顺序没有保证。

## 学习过程

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 1 先写 | 合上资料，用自己的话写 Why / What 各 3–5 句：对象什么时候开始存在？初始化到底是什么，和赋值有什么不同？作用域和生命周期是不是一回事？起点是 lifetime-and-initialization.md 里你已写的那几段。写不出来的地方，就是第 2 步要读的地方 | 0.25 | 两小段话，加一串"不确定"的问题 |
| 2 读 | 只读上面的"必读"四项；边读边填一张表：存储期（自动 / 静态 / 线程 / 动态）× 何时初始化 × 何时销毁 × 作用域结束时对象怎样 | 0.75 | 填好的表 |
| 3 动手 | 做下面三个实验（实验 3 很短），每个先写预测再跑 | 0.75 | 三份预测，三份实际输出 |
| 4 写 | How：对象的时间线 → 初始化的几种形式（各配一个小例子，并与赋值对照）→ 构造与销毁的顺序规则（用实验 1 的输出）→ 存储期表，加一小段作用域与生命周期的区别（用实验 3）。Relation：链到 RAII How ii、new-delete、下一篇。Example：用实验 1，Verifies 对应三条结论 | 1 | 笔记初稿 |
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

### 实验 3：作用域结束，对象会怎样

要写的程序：

- 同样的会打印的 `Tracer`，一个全局的 `Tracer*`。
- `main` 里一个块：块内依次是一个自动 `Tracer`、一个 `new` 出来并把地址存进全局指针的 `Tracer`、一个静态局部 `Tracer`，块的末尾打印一行。
- 块后面打印一行，再通过全局指针读出那个动态对象的名字，最后 `delete` 它。

先预测：块结束时，三个对象里谁被销毁了？块后面读全局指针，安全吗？静态局部对象的名字已经不可见了，它在什么时候销毁？

它验证：作用域结束只影响名字，自动对象的生命周期跟着作用域结束，动态对象和静态局部对象不跟。

### 复用：对象与存储的时间差

placement new 里"存储先于对象存在、晚于对象结束"，new-delete 的 Example（`test_new_delete.cpp`）已经在验证。写 How 时直接引用那个 Example，不另写实验。

## 封板自检

- 能解释：不看资料，用两分钟讲清"生命周期、存储期和作用域各管什么"，以及"一个派生类对象是怎样按顺序构造出来的"。
- 有模型：对象的时间线；那张"存储期 × 构造时机 × 销毁时机"的表，加一条顺序规则：基类 → 成员（声明顺序）→ 构造函数体，销毁时相反；一句话说明初始化与赋值的区别。
- 有联系：Relation 链到 RAII How ii（构造失败）、new-delete（存储与对象）和下一篇；不重复它们的内容。
- 能验证：实验 1 的输出贴进 Example，Verifies 每条对应一个结论；作用域那一段用实验 3 的输出支撑。

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

### 实验 3 的结果

```text
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
```

怎么读：

- 块一结束，只有 `auto` 被销毁，`dynamic` 和 `static` 都还活着，名字也不需要可见：`dynamic` 是通过全局指针读到的，`static` 的名字出了块就不可见，它一直活到 `main` 结束之后。
- 作用域结束 ≠ 生命周期结束。只有自动存储期的对象，两者才同时发生。
- 这也是 RAII 把资源交给自动对象的原因：离开作用域时销毁是确定的，不需要靠记得 `delete`。

## 附录：OneNote 原文（离线备份）

三页按你贴给我的原文原样放在下面，没有改动。每页前面的“已知错处”是我之前核对时标出的，带 ❌ 的才是错，其余是需要标版本。

### C++ Primer 的 Chapter 2 → 2.2 Variables

已知错处：

- ❌ 3.a 里 “Default initialization” 被归在 “Direct initialization” 的 Type 下。标准里 `T t;` 是单独的默认初始化，既不是直接初始化也不是拷贝初始化；直接初始化指 `T t(x)` 和 `T t{x}`。
- ❌ “Value initialization … MUST use the direct form of initialization” 只适用于只给大小的 `vector<int> ivec(10)`，不能当值初始化的定义。值初始化还包括 `T()`、`T t{}`、`new T()`。
- 版本：Copy initialization 的 “copy constructor is needed too” 只在 C++17 之前对纯右值成立。
- `5. Lifetime:(Added later)` 是空的。

```text
Variable
    1. What
         A variable provides us with named storage that our programs can manipulate. Each variable in C++ has a type. The type determines the size and layout of the variable’s memory, the range of values that can be stored within that memory, and the set of operations that can be applied to the variable
    2. Definitions && Declarations
        a. Why need declaration?
            i. C++ is a statically typed language, which means that types are checked at compile time. The complier need to know the type of variable.
        b. What is difference?
            i. A file that wants to use a name defined elsewhere includes a declaration for that name. A definition creates the associated entity.
        c. How to use or distinguish? 
            i. We use extern key word to indicate this is a declaration not a definition, e.g. extern int j;
            ii. A definition can be used as a declaration, but a declaration may not be a definition, e.g. extern int i = value is a definition but extern int j is not.
            (Note: It is an error to provide an initializer on an extern inside a function)
    3. Initialization && Assignment
        Initialization is not assignment. Initialization happens when a variable is given a value when it is created. Assignment obliterates an object’s current value and replaces that value with a new one
        a. Initialization kinds
          according to initializer value form
            i. Default initialization, when we define a variable without an initializer, the variable is default initialized.
            ii. Value initialization, we can usually omit the value and supply only a size. In this case the library creates a value-initialized element initializer for us. 
                                                MUST use the direct form of initialization.
                e.g.    vector<int> ivec(10); // ten elements, each initialized to 0 
                    vector<strng> svec(10); // ten elements, each an empty string 
            iii. List initialization
          according to initialization form
            i. Copy initialization, using = operator
                1) Mechanism: Copy initialization constructs an implicit conversion sequence: It tries to convert x to an object of type T. It then may copy over that object into the to-initialized object, so a copy constructor is needed too.
            ii. Direct initialization, forms that do not using = operator
                1) Mechanism: Direct initialization behaves like a function call to an overloaded function: The functions, in this case, are the constructors.
                2) Type: 
                    a) Default initialization
                    b) ()  initialization
                    c) {} list initialization
                    
    4. Scope:
        a. What is scope?
            i. The valid area in program that object can work.
        b. Scope kinds
            i. Global scope --- The names defined outside a function—has global scope. Once declared, names at the global scope are accessible throughout the program.
            ii. Block scope --- The names defined inside a block.
    5. Lifetime:(Added later)
Extern
```

### Professional 的 18. Direct and Copy initialization

已知错处（版本问题，不是 C++17 之后的错）：

- “the copy constructor is needed too” 和 “copy constructor is deleted or not public → copy initialization is not allowed”，只在 C++17 之前对纯右值成立。C++17 之后，同一个类的纯右值不再需要可用的拷贝或移动构造。
- “the compiler will help to optimize”：C++17 之前是允许省略，之后对纯右值是保证。

```text
The deep difference between direct and copy initialization:
While they look identical and are supposed to do the same, these two forms are remarkably different in certain cases. The two forms of initialization are direct and copy initialization:
T t(x);
T t = x;
There is behavior we can attribute to each of them:
    • Direct initialization behaves like a function call to an overloaded function: The functions, in this case, are the constructors of T (including explicit ones), and the argument is x. Overload resolution will find the best matching constructor, and when needed will do any implicit conversion required.
    • Copy initialization constructs an implicit conversion sequence: It tries to convert x to an object of type T. It then may copy over that object into the to-initialized object, so a copy constructor is needed too.
    
As you see, copy initialization is in some way a part of direct initialization with regard to possible implicit conversions: While direct initialization has all constructors available to call, and in addition can do any implicit conversion it needs to match up argument types, copy initialization can just set up one implicit conversion sequence.

Note: While copy initialization is used, the complier will help to optimize which leading to not call copy constructor. Once copy constructor is deleted or is declared as not public, the copy initialization is not allowed. And another situation that copy initialization is forbidden is that the corresponding  constructor called by copy initialization is explicit.

Reference: https://stackoverflow.com/questions/1051379/is-there-a-difference-between-copy-initialization-and-direct-initialization
         https://blog.csdn.net/ljianhui/article/details/9245661
```

### C++ Primer 的 Chapter 13 → Copy initialization

已知错处：

- ❌ 第 5 点的问题：`string s = "lit";` 在 C++17 里等价于直接调用单参构造，你的直觉对；但 C++17 之前是“先转换出临时对象，再拷贝或移动构造”，通常被省略，所以拷贝或移动构造函数必须可用。我验证过：带 deleted 拷贝和移动构造的类，`S s = "x";` 在 C++17 通过，在 C++14 报 `use of deleted function ‘S::S(S&&)’`。
- ❌ “只有声明为 explicit 时才会调用单参构造再调用拷贝或移动构造”不对。explicit 时 `E e = "x";` 直接编译失败；`string("...")` 是显式写出的 T 类型纯右值，和 explicit 无关。
- ❌ 答案 A 里 “other 是 T 类型或继承自 T 类型的纯右值，则直接构造在 t 里”，“继承自 T”是错的。`T t = D();`（D 继承自 T）在 C++14 和 C++17 都调用移动构造（切片），不省略；C++17 的保证只针对同一个类的纯右值。
- 第 3 点 d、e 是 Primer 的原话（按 C++11 算对）；e 里 push 和 insert 是拷贝，emplace 系列是直接初始化。

```text
 Copy initialization：
    1. When we use copy initialization, we are asking the compiler to copy the right-hand operand into the object being created, converting that operand if necessary
    2. Copy initialization ordinarily uses the copy constructor. If a class has a move constructor, then copy initialization sometimes uses the move constructor instead of the copy constructor.
    3. When copy initialization happens 
        a. Define variables using an =
        b. Pass an object as an argument to a parameter of nonreference type 
        c. Return an object from a function that has a nonreference return type 
        d. Brace initialize the elements in an array or the members of an aggregate class
        e. Library containers copy initialize their elements when we initialize the container, or when we call an insert or push member
    4. During copy initialization, the compiler is permitted (but not obligated) to skip the copy/move constructor and create the object directly.
        a. e.g. 
        
        
    5. Q：到底是一个怎么的过程？除去复制省略， 在4.a中， string null_book =" 9-999-9999-9", 按照单参数构造函数来说，应该是直接调用
        该单参构造函数，而不用再调用拷贝或者是移动构造函数。只有在单参构造函数声明为explicit 时候，例如 string null_book = string("9-999-9999-9"),
        理论上来说会笑调用该单参构造函数，然后再调用拷贝或者移动构造函数。需要查明资料
A: 在C++17之后，标准有复制省略的规定， 即形如 T t = others的复制初始化语句，如果other是一个T类型或者继承自T类型的纯右值表达式，则other直接被构造在t的内存空间里。在C++ 17之前，如果others是T类型或者继承自T类型的值，则调用对应的构造函数；如果other是和T无关的类型，则调用用户自定义的转换函数把other变成T类型的临时对象，然后使用该临时对象direct initialization t，但是最后一步常常由于编译器优化被省略掉，即临时对象直接构造在t的内存空间，不用再复制一次（不用 direct initialization）.
```
