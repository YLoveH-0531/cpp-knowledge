# Lifetime

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

- vacuous initialization 也算初始化完成：`int x;`（默认初始化，什么也不做）的生命周期，在获得存储时就开始了。vacuous initialization 是标准术语（C++20 [basic.life]/1）：变量被默认初始化，且若是类类型，该类的默认构造函数是平凡的；现行草案（[basic.life]/2）改成：被默认初始化、没有别的初始化动作，且若是类类型，默认初始化选中的是平凡构造函数。
- 对象在构造期间、析构期间也占着存储，但此时生命周期还没开始、或已经结束，只允许特定操作（[class.cdtor]，例如虚函数调用只解析到当前正在构造或析构的那个类）。
- 委托构造：初始化在目标（非委托）构造函数返回时就算完成，所以生命周期可以在所有委托构造函数完成前开始。委托者的函数体抛异常时，析构函数会被调用；非委托构造函数的函数体抛异常则不会（g++ 11.4 与 clang 14 实测一致，见 [constructors-and-destructors.md](../02-class-and-oop/constructors-and-destructors.md) 的委托构造函数一节）。
- 存储可以被复用：在已有对象的存储上用 placement new 建新对象，旧对象的生命周期随之结束。
- 引用可以悬空：被引用对象的生命周期可以先于引用结束。
- 联合体：联合体成员（及其子对象）的生命周期，只有当它是联合体里被初始化的成员时才开始（或按 [class.union]、[class.copy.ctor] 的规定），C++20 [basic.life]/1。

历史（都是缺陷报告，追溯适用于旧版本）：

- CWG 119：最初只有"带非平凡构造函数的类对象"要等构造函数完成才开始生命周期；现在是"初始化完成（含 vacuous initialization）"。
- CWG 2012：引用的生命周期原本与存储期一致，extern 引用在初始化器运行前就算"活着"；现在从初始化完成时开始。
- CWG 2256：原来非类类型（和平凡析构的类）的生命周期到存储期结束才结束，类类型对象按构造的逆序结束，两套规则不一致；现在统一成：非类类型在对象被销毁时结束，类类型在析构函数调用开始时结束。
