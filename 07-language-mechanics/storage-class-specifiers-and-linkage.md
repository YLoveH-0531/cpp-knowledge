# Storage Class Specifiers and Linkage

## storage class specifiers

The following keywords are storage class specifiers:

- `auto` (until C++11)
- `register` (until C++17)
- `static`
- `thread_local` (since C++11)
- `extern`
- `mutable`

作用：与名字所在的作用域一起，控制名字的存储期（[storage-duration.md](../02-class-object-oop/01-object-foundation/storage-duration.md)）和链接（本篇的 Linkage 一节）这两个相互独立的属性；`mutable` 除外，它与这两者都无关（见下）。

- 边角：
  - 一个声明里最多一个存储类说明符，唯一的例外是 `thread_local` 可以和 `static` 或 `extern` 同用。
  - `mutable` 虽然列在这里，但与存储期、链接都无关，它的作用是让非静态数据成员在 `const` 对象里仍可修改。
  - `static` 的含义随位置变：命名空间作用域里是内部链接；块作用域里是静态存储期（无链接）；类内的数据成员则是静态数据成员，属于类而不属于对象，静态存储期，链接与类名相同。
- 历史：
  - `auto` 作为存储类说明符到 C++11 被移除，改作类型推导；C++03 里的 `auto int x;` 可能因此不合法（Annex C [diff.cpp03.dcl.dcl]）。
  - `register` C++11 起弃用，C++17 起移除，关键字保留，理由是"为将来重新利用这个关键字"（Annex C [diff.cpp14.dcl.dcl]）。

## Linkage

linkage 是名字的性质：名字能否指代另一个作用域里的名字所指代的同一个实体（[basic.link]/2）。

下文的 `nm` 符号（`D`、`B`、`R`、`T`、`u` 等）是 ELF 目标文件和 GNU `nm`（g++ 11.4）下的实现观察，用来帮助理解，不属于 C++ 语言规则。

- 初衷：它来自 C 的"分离编译 + 链接器"模型。外部链接的名字在目标文件里是全局符号，链接器把各个 `.o` 里指向同一个名字的引用合并到同一个定义上；内部链接的名字是文件私有的本地符号；无链接的名字，别的作用域里的名字指代不到它所指的实体（标准的说法是 "cannot be referred to by names from other scopes"，说的是在别的作用域里另行声明的名字；内层块里直接用外层局部变量的名字，是普通的名字查找）。`nm` 里大写（`D`、`B`、`R`、`T`）是外部，小写（`d`、`b`、`r`、`t`）是内部或局部，`u` 是 inline 变量的"唯一全局"符号（实测）。

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
