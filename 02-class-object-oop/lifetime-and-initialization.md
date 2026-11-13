# lifetime-and-initialization

## 1. some explains

- **初始化**（Initialization）描述对象如何获得初始状态，是更广泛的过程。它涉及选择适当的初始化方式、初始化基类和成员子对象，以及执行构造函数体等步骤。构造函数调用是其中的重要组成部分，而不是初始化的全部定义。对于通过构造函数初始化的类类型对象，初始化过程涉及调用构造函数；基类和成员子对象在构造函数体执行前完成初始化，而整个对象的初始化在非委托构造函数返回时完成（CWG 2756，见 §3.7）。初始化的各种写法和种类（默认、值、零、拷贝、直接、列表、聚合）见 [process-memory-layout-and-startup-review.md](process-memory-layout-and-startup-review.md) §2.2。
- **赋值**（Assignment）是用一个新的值去替换旧值，发生于初始化完成之后的使用过程。
- **生命周期**（Lifetime）是对象（和引用）的运行期性质：从获得存储、且初始化（如果有；什么也不做的平凡初始化也算）完成时开始，到析构函数调用开始（非类类型是对象被销毁）、或存储被释放或复用时结束。完整规则见 §3.5。
- **作用域**（Scope）指的是一个名字能够被代码看见的区域，是编译期的事情。
- **存储期**（Storage duration）是对象所占存储的最短潜在存活期，由创建对象的构造决定，分静态、线程、自动、动态四种。

## 2. relations

- 时间顺序：获得存储 → 初始化完成（生命周期开始）→ 析构函数调用开始（生命周期结束）→ 释放存储。对象的生命周期等于或嵌套在其存储的存活期之内。
- 作用域、生命周期、存储期互相独立：作用域是名字的编译期性质，生命周期是对象的运行期性质，存储期是对象存储的存活期，由创建方式决定。
  - 名字在作用域内，对象不一定活着：`int x = x;` 右边的 `x` 已经在作用域内，初始化却还没完成；悬空引用的名字还在，被引用的对象已经结束。
  - 对象活着，名字不一定看得见：`new` 出来的对象没有名字；外层的名字被内层同名声明遮住。
  - 静态存储期的变量，存储在整个程序期间都在，生命周期却从初始化完成才开始：函数内的 `static` 变量要到第一次执行到声明处才构造（实测：先打印 main 开头，第一次调用才构造，main 结束后才析构；`nm` 里它在 `.bss`，旁边多一个 guard variable 记录"是否已初始化"）。
- 初始化让对象得到第一个值，赋值是给生命周期已开始的对象换值。

## 3. some concepts

### 1) declaration

- **name**, 用于指代实体的名称。名字不等于实体：同一个实体可以有多个名字（引用、类型别名、命名空间别名），也可以没有名字（临时对象、`new` 出来的对象）。
- **entity**, C++ 程序中被识别和描述的事物，例如对象、函数、类型、模板、命名空间等。标准的清单还包括值、引用、枚举器、类成员、位域等：引用是实体，但不是对象。
- **object**, Entity 的一种，具有类型、存储期、生命周期等属性，并占据一块存储区域（构造期间、生命周期内、析构期间都占着）。对象可以有名字，也可以没有；函数不是对象。
- **Reference**（引用）

  **Why** — 设计目的

  引用最初主要是为支持运算符重载而引入的。它使函数能够通过自然的对象表达式接收已有对象，而不需要复制对象或显式传递指针；同时允许函数返回已有对象的别名，支持下标访问等操作。

  出处：Stroustrup 的 FAQ（[Why does C++ have both pointers and references?](https://www.stroustrup.com/bs_faq2.html#pointers-and-references)）说，指针是从 C 继承来的、去不掉，而引入引用的直接原因就是支持运算符重载："the direct reason I introduced them in C++ was to support operator overloading"。例：`z = x + y` 比 `z = *x + *y` 自然。

  **What** — 语言概念

  1. 引用是一种实体，引用类型是复合类型；引用不是 Object。
  2. 引用初始化时绑定到对象或函数。
  3. 引用初始化后不能重新绑定。
  4. 引用具有自己的生命周期，但不一定占据独立存储空间（标准：引用是否需要存储是未指定的）。
  5. 引用名作为表达式时，类型被调整为被引用的类型，指代被引用的对象或函数（`sizeof(r)` 是被引用类型的大小，`&r` 是被引用对象的地址）。
  6. 引用的生命周期从初始化完成时开始，按照一个需要存储空间的标量对象的方式结束。所以引用会悬空：被引用对象的生命周期可以先于引用结束。
  7. 因为引用不是对象：没有引用数组、没有指向引用的指针、没有引用的引用（经 typedef 或模板形成时，按引用折叠规则处理）。

  **How** — 基本使用

  ```cpp
  int x = 10;
  int& r = x;

  r = 20;       // 修改 x
  int* p = &r;  // 获取 x 的地址
  ```

  **History** — 发展

  - 早期 C++：引入左值引用，主要支持运算符重载和函数参数、返回值。
  - C++98：`const T&` 可以绑定临时对象，并把临时对象的生命周期延长到引用的生命周期。
  - C++11：引入右值引用，支持移动语义和完美转发；同时有了引用折叠和转发引用。
  - C++23：range-for 的 range-initializer 里的临时对象，生命周期延长到循环体结束（此前 `for (auto& x : f().items())` 这类写法会悬空）。

  **Implementation** — 实现

  引用不等于指针常量。编译器可以使用指针或其他方式实现引用语义，具体由实现决定。

- **declaration**, Declarations are how names and/or entities are introduced (or re-introduced) into the C++ program。对于引入名字的声明，它会规定该名字的解释及相关语义属性，使名字能够指代相应实体。声明可以是实体的首次声明，也可以是对已有实体的重新声明。按标准的规则，**声明默认就是定义，除非属于例外**（C++20 [basic.def]/2）。常碰到的例外有：没有函数体的函数声明；不带初始化器的 `extern` 变量声明；类内的非 inline 静态数据成员声明（`inline` 的，以及 C++17 起隐含 inline 的 `constexpr` 静态数据成员，在类内就是定义）；typedef、using、`static_assert` 这类不引入对象或函数的声明。
- **definition**, A definition is a declaration that fully defines the entity it introduces. The number and consistency of definitions are governed by the ODR.
  - 为什么要分声明和定义：声明可以重复，每个用到这个实体的翻译单元里都要有；定义（对应一份存储或一份代码）整个程序里只能有一个。inline 函数和变量、类、模板是例外，可以在各个翻译单元各写一份相同的定义。编译器一次只看一个翻译单元，靠链接器把各处的引用指向同一个定义。Stroustrup 的说法是 "to avoid complicated linker rules, C++ requires that every object has a unique definition"（[FAQ](https://www.stroustrup.com/bs_faq2.html#in-class)）。

#### declaration

Note that the C++ standard distinguishes between declaration (a syntactic construct) and declaration (a C++ term). A declaration(C++ term) can be not only a declaration but also a class specifier(syntactic construct), an enum specifier, a function parameter, and so on. 对一个名字来说，引入它的声明和引入它所指实体的声明不一定是同一个：`template <class T> struct A {};` 里，引入名字 `A` 的是 class specifier，引入类模板 `A` 这个实体的是外层的 template declaration。

语法上最常见的形式：

```text
declaration → block-declaration → simple-declaration → decl-specifier-seq init-declarator-list ;
init-declarator → declarator initializer
```

- 一条 simple declaration 可以声明多个实体，每个实体的类型 = decl-specifier-seq 的类型 + 各自声明符的修饰：`int a = 1, *p = &a, f(int);` 声明了一个 `int`、一个 `int*` 和一个函数。
- 声明点（locus）在声明符之后、初始化器之前，见 §3.6 ③。

### 2) storage class specifiers

The following keywords are storage class specifiers:

- `auto` (until C++11)
- `register` (until C++17)
- `static`
- `thread_local` (since C++11)
- `extern`
- `mutable`

作用：与名字所在的作用域一起，控制名字的两个独立属性——存储期（§3.3）和链接（§3.4）。

- 边角：
  - 一个声明里最多一个存储类说明符，唯一的例外是 `thread_local` 可以和 `static` 或 `extern` 同用。
  - `mutable` 虽然列在这里，但与存储期、链接都无关，它的作用是让非静态数据成员在 `const` 对象里仍可修改。
  - `static` 的含义随位置变：命名空间作用域里是内部链接；块作用域里是静态存储期（无链接）；类内的数据成员则是静态数据成员，属于类而不属于对象，静态存储期，链接与类名相同。
- 历史：
  - `auto` 作为存储类说明符到 C++11 被移除，改作类型推导；C++03 里的 `auto int x;` 可能因此不合法（Annex C [diff.cpp03.dcl.dcl]）。
  - `register` C++11 起弃用，C++17 起移除，关键字保留，理由是"为将来重新利用这个关键字"（Annex C [diff.cpp14.dcl.dcl]）。

### 3) Storage duration

- a. The storage duration is the property of an object that defines the minimum potential lifetime of the storage containing the object. The storage duration is determined by the construct used to create the object.
- b. some rules
  - The storage duration categories apply to references as well.
  - The storage duration of subobjects and reference members is that of their complete object.
- c. type
  - **static** storage duration：A variable satisfying all following conditions has static storage duration；存储持续整个程序期间，只有一份。
    - It belongs to a namespace scope or is first declared with `static` or `extern`.
    - It does not have thread storage duration.(since C++11)
    - 字符串字面量对象也是静态存储期，它不是变量，不在上面的条件里。
  - **thread** storage duration (since C++11)：声明为 `thread_local` 的变量，每个线程各有一份，线程开始时分配、结束时释放（暂不展开）。
  - **automatic** storage duration
    - Variables that belong to a block scope and are not explicitly declared `static`, `thread_local`,(since C++11) or `extern`. 存储持续到所在的块退出。
    - Variables that belong to a parameter scope (i.e. function parameters). 存储持续到形参销毁之后。
    - Temporary objects：标准说临时对象的存储期是静态、线程、自动三者之一（C++20 [basic.stc]/2），通常是自动；cppreference 只写了"自动"，并不完整。临时对象因绑定到引用而被延长生命周期时，存储期跟着那个引用走：命名空间作用域的 `const int& r = 5;`，临时对象放在 `.rodata`（符号 `_ZGR1r_`），程序结束才销毁（[class.temporary]/6、/7；g++ 实测）；函数里的同类写法则在栈上。
  - **dynamic** storage duration：`new` 表达式创建的对象，存储由分配函数分配、由释放函数回收，何时结束由程序显式决定，不由作用域决定。在已有存储上隐式创建的对象也是动态存储期（现行草案 [basic.stc.general]/2）。

### 4) Linkage

linkage 是名字的性质：名字能否指代另一个作用域里的名字所指代的同一个实体（[basic.link]/2）。

- 初衷：它来自 C 的"分离编译 + 链接器"模型。外部链接的名字在目标文件里是全局符号，链接器把各个 `.o` 里指向同一个名字的引用合并到同一个定义上；内部链接的名字是文件私有的本地符号；无链接的名字，别的作用域根本引用不到。`nm` 里大写（`D`、`B`、`R`、`T`）是外部，小写（`d`、`b`、`r`、`t`）是内部或局部，`u` 是 inline 变量的"唯一全局"符号（实测）。

A name can have external linkage, module linkage(since C++20), internal linkage, or no linkage:

- An entity whose name has external linkage can be redeclared in another translation unit.
  - Any of the following names declared at namespace scope have external linkage, unless they are declared in an unnamed namespace
    - variables and functions not listed below (that is, functions not declared `static`, non-`const` variables not declared `static`, and any variables declared `extern`);
    - enumerations;
    - names of classes, their member functions, `static` data members (`const` or not), nested classes and enumerations, and functions  first introduced with `friend` declarations inside class bodies;
    - names of all templates (that is, not function templates declared `static`).
  - Any of the following names first declared at block scope have external linkage:
    - names of variables declared `extern`;
    - names of functions.
  - 边角：类的成员（成员函数、静态数据成员、嵌套类和枚举）的链接与所在类的名字相同，所以类在匿名命名空间里时，它们也是内部链接（C++20 [basic.link]/5；g++ 实测匿名命名空间里类的静态数据成员是本地符号 `d`）。
- An entity whose name has internal linkage can be redeclared in another scope in the same translation unit.
  - Any of the following names declared at namespace scope have internal linkage:
    - variables, functions, or function templates declared `static`;
    - non-template variables of non-`volatile` `const`-qualified type, unless they are `inline` (since C++17), explicitly declared `extern`, or previously declared and the prior declaration did not have internal linkage;
    - data members of anonymous unions;
    - all names declared in unnamed namespaces or a namespace within an unnamed namespace, even ones explicitly declared `extern`
- **no linkage**, An entity whose name has no linkage can only be redeclared in the same scope.
  - Any of the following names declared at block scope have no linkage:
    - variables that are not explicitly declared `extern` (regardless of the `static` modifier);
    - local classes and their member functions;
    - other names declared at block scope such as typedefs, enumerations, and enumerators.
  - Names not specified with external, module, or internal linkage also have no linkage, regardless of which scope they are declared in.
- **module linkage** (since C++20)，暂不展开。

初衷与历史：

- C 里文件作用域的 `const` 是外部链接，C++ 里是内部链接。标准 Annex C 给的理由（[diff.basic]）：const 对象在 C++ 里可以当编译期的值用，所以要求写初始化器；这样也就能把 const 对象放进被多个翻译单元包含的头文件，每个翻译单元各有一份私有拷贝（实测：`const int c1 = 1;` 取地址后是本地符号 `r c1`）。
- 这条规则的判据后来改过几次：C++11 原文是"显式声明为 `const` 或 `constexpr`"，缺陷报告 CWG 1686（追溯适用）改成"类型是 const 限定的"；变量模板不适用（缺陷报告 CWG 2387）；C++17 起 `inline` 变量不适用，`inline const int c2 = 2;` 是外部链接（实测符号 `u c2`）；C++20 起还有模块链接。
- 匿名命名空间：C++11 起整体是内部链接；C++03 里只是名字唯一，仍是外部链接。

### 5) Lifetime

Lifetime is a runtime property, which applies to every object and reference.

- **Object**
  - The lifetime of an object begins when: storage with the proper alignment and size for its type is obtained, and its initialization (if any) is complete (including vacuous initialization).
  - The lifetime of an object ends when:
    - if it is of a non-class type, the object is destroyed (maybe via a pseudo-destructor call), or
    - if it is of a class type, the destructor call starts, or
    - the storage which the object occupies is released, or is reused by an object that is not nested within it.
- **Reference**
  - The lifetime of a reference begins when its initialization is complete.
  - The lifetime of a reference ends as if it were a scalar object.

作用：对象的各种性质只在它的生命周期内成立，生命周期之前和之后使用对象受到严格限制（[basic.life]）。

边角：

- 什么也不做的初始化也算：`int x;`（默认初始化，不做任何事）的生命周期，在获得存储时就开始了。
- 对象在构造期间、析构期间也占着存储，但此时生命周期还没开始、或已经结束，只允许特定操作（[class.cdtor]，例如虚函数调用只解析到当前正在构造或析构的那个类）。
- 委托构造：初始化在目标（非委托）构造函数返回时就算完成，所以生命周期可以在所有委托构造函数完成前开始。委托者的函数体抛异常时，析构函数会被调用；非委托构造函数的函数体抛异常则不会（g++ 11.4 与 clang 14 实测一致，见 §3.7）。
- 存储可以被复用：在已有对象的存储上用 placement new 建新对象，旧对象的生命周期随之结束。
- 引用可以悬空：被引用对象的生命周期可以先于引用结束。

历史（都是缺陷报告，追溯适用于旧版本）：

- CWG 119：最初只有"带非平凡构造函数的类对象"要等构造函数完成才开始生命周期；现在是"初始化完成（含什么也不做的初始化）"。
- CWG 2012：引用的生命周期原本与存储期一致，extern 引用在初始化器运行前就算"活着"；现在从初始化完成时开始。
- CWG 2256：原来非类类型（和平凡析构的类）的生命周期到存储期结束才结束，类类型对象按构造的逆序结束，两套规则不一致；现在统一成：非类类型在对象被销毁时结束，类类型在析构函数调用开始时结束。

### 6) scope

本节的术语（locus、inhabit、target scope、bound、belongs to，以及 parent scope、immediate scope）来自 P1787R6，2020-12 并入标准草案，属于 C++23；C++20 用的是 declarative region、potential scope、point of declaration 这套词。

初衷（P1787R6 引言）：旧的 [basic.scope] 把"scope"定义了两遍，对命名空间是一个还是多个 declarative region 前后不一，再加上模块让"只看本翻译单元里靠前的声明"这个默认假设不再成立，所以整体重写。

#### ① 结构：作用域是一棵嵌套的树

- 整个程序有一个根：global scope。
- 其他作用域由声明、参数、语句、handler、lambda 表达式引入
- 每个作用域都出现在另一个作用域中，后者包含前者。
- Parent scope 是包含该作用域的最小外层作用域。

```text
global
 └── S       parent(S) = global
      └── T  parent(T) = S
```

#### ② 定位：站在某个程序点上看

- Enclosing scope：包含该程序点的所有作用域。
- Immediate scope：包含该程序点的最小作用域，即最内层作用域。

例如，程序点 Y 位于 T 中：

```text
global
 └── S
      └── T
           Y
```

那么：

- Enclosing scopes：`{T, S, global}`。
- Immediate scope：`T`。

#### ③ 归属：声明落到哪个作用域

1. Locus：声明在程序中对应的标准规定位置。例如 `int x = 10;`，其 locus 在 `x` 之后、初始化器之前。
   - 所以 `int x = x;` 右边的 `x` 已经是新声明的那个 `x`（还没初始化）。枚举器不同，它的 locus 在整个定义之后：`enum { x = x + 1 };` 右边的 `x` 还是外层的（g++ 实测）。
2. Inhabit：默认情况下，声明 inhabits 其 locus 所在程序点的 immediate scope。
3. Target scope：默认情况下，声明的 target scope 就是它 inhabits 的作用域。

例如：

```cpp
{               // S
    {           // T
        int y;
    }
}
```

`int y;` 的 locus 位于 T 中，因此默认：

```text
Immediate scope = T
Inhabited scope = T
Target scope = T
```

注意：以上 target scope 与 inhabited scope 相等，是默认规则（标准里写的是 "Unless otherwise specified"），并非无例外。现行草案 [basic.scope.scope]/2 的注列了特例，常见的有：

- 块作用域里的 `extern` 声明和函数声明：target scope 是外层的命名空间作用域（实体属于该命名空间），名字却只绑定在所在的块里。例：`namespace X { void p() { extern int q; } }`，`q` 属于 `X`（目标文件里的符号是 `X::q`），但在 `p` 外面用 `q` 会报未声明（g++、clang 实测）。
- 友元声明：不绑定名字，实体属于最内层的外围命名空间。所以 `friend void ff();` 之后，类外直接写 `ff()` 找不到，只有参数相关查找（ADL）能找到（实测）。
- 无作用域枚举的枚举器：名字绑定在枚举和外层两处，`E::e1`（C++11 起）和 `e1` 都能用；`enum class` 的只在枚举里。
- 匿名联合的成员：名字绑定在联合的父作用域。

#### ④ 绑定：名字和实体

- Bound：声明引入的名称，默认在 target scope 中绑定到该声明。
- Belongs to：如果实体某条声明的 target scope 是 S，那么该实体属于 S。这里的「属于」是语言层面的作用域归属，与对象的内存位置、存储期和生命周期无关。

绑定与归属是两件事：绑定决定名字能在哪里被找到，归属决定实体在语义上算哪个作用域的成员。默认情况下两者都由声明的 target scope 决定，所以一个声明在什么范围内"生效"，既定了能在哪里用这个名字，也定了被声明的实体归入哪个作用域；上面的特例（块作用域里的 `extern` 和函数声明、友元声明）正是两者分开的地方。

### 7) 类对象的初始化：构造函数与成员初始化列表

- **构造函数**：非聚合类类型对象的初始化通过构造函数完成；聚合类走聚合初始化，逐个成员初始化，不经构造函数（如 `struct P { int a; int b; }; P p{1, 2};`）。调用哪一个构造函数，由初始化的写法（如默认、直接、列表初始化）按重载决议决定；构造函数调用不是另一种初始化类别。
- **默认成员初始化器**（C++11）：在成员声明处给初值，`int z = 30;` 或 `int z{30};`。不能写圆括号：`int z(30);` 会被当成函数声明而报错（实测）。
- **成员初始化列表**：在构造函数体执行之前，指定每个基类和非静态成员怎么初始化。
  - 没写的成员：有默认成员初始化器就用它，否则默认初始化（内置类型什么也不做；联合体和匿名联合的变体成员则完全不初始化，[class.base.init]/9）。
  - 写了的成员：它的默认成员初始化器被忽略。
  - 默认初始化行不通的，必须在列表里写（或改用默认成员初始化器）：引用成员、内置类型的 `const` 成员、没有默认构造函数的成员和基类。类类型的 `const` 成员要看它能不能默认初始化，`const std::string s;` 这样的成员可以不写（实测）。
  - 不在列表里写、改在函数体里赋值，是先默认初始化再赋值，两步；列表里写 `m(1)` 一步完成。
  - 写法决定形式：`x(n)` 是直接初始化，`x{n}` 是直接列表初始化，不允许窄化转换（clang 报错，g++ 只警告）。
- **委托构造函数**（C++11）：mem-initializer 里写本类名，并且只能单独出现（再写别的成员初始化会报错，实测）；先执行目标构造函数，它返回后才执行委托者的函数体。对象的初始化在目标（非委托）构造函数返回时就算完成（§1 提到的 CWG 2756；C++20 原文只写了"目标构造函数返回后才执行委托者的函数体"），所以委托者的函数体抛异常时，析构函数会被调用；非委托构造函数的函数体抛异常则不会（g++ 11.4、clang 14 实测一致）。
- **初始化顺序只看声明，不看列表的书写顺序**：虚基类（只由最派生类初始化，按基类图的深度优先、从左到右）→ 直接基类（按基类列表的顺序）→ 非静态成员（按类里的声明顺序）→ 构造函数体。销毁完全相反。标准这样规定，就是为了让销毁顺序恰好是初始化顺序的逆序。所以一个成员依赖另一个成员时，被依赖的要先声明；写反了编译器会警告（g++：`-Wreorder`）。

```cpp
struct A {
    int x;
    const int y;                       // const int 成员：必须在列表里初始化
    int z = 30;                        // 默认成员初始化器
    A(int n) : z(n), y(20), x(10) {}   // 书写顺序 z、y、x；实际顺序 x、y、z，然后函数体；z 的默认值被忽略
};
```

- 验证：[lifetime-and-initialization-exp.cpp](lifetime-and-initialization-exp.cpp) 实验 1（顺序，故意触发 `-Wreorder`）。标准：[class.base.init]/6（委托）、/9、/10、/13（C++20）。
- 关联：全局对象的构造函数是 `constexpr`、且整个初始化是常量表达式时，做常量初始化，不必在运行时调用构造函数，见 [process-memory-layout-and-startup-review.md](process-memory-layout-and-startup-review.md) §2.2 表里的 `gc` 一行和 §2.3。
