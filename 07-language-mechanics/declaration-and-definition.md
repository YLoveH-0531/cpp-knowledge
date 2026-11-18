# Declaration and Definition

- **name**, 用于指代实体的名称。名字不等于实体：同一个实体可以有多个名字（引用、类型别名、命名空间别名），也可以没有名字（临时对象、`new` 出来的对象）。
- **entity**, C++ 程序中被识别和描述的事物。按 C++20 [basic.pre]/3，实体是 value、object、reference、structured binding、function、enumerator、type、class member、bit-field、template、template specialization、namespace、pack；其中引用是实体，但不是对象。现行草案 [basic.pre]/8 重写了这份清单：object、reference 不再单列，改列 variable（由引用或对象的声明引入）、non-static data member、type alias、namespace alias、template parameter、function parameter、init-capture 等。object 和 reference 见 [object-and-reference.md](../02-class-object-oop/01-object-foundation/object-and-reference.md)，那里按 C++20 的清单。
- **declaration**, Declarations are how names and/or entities are introduced (or re-introduced) into the C++ program。对于引入名字的声明，它会规定该名字的解释及相关语义属性，使名字能够指代相应实体。声明可以是实体的首次声明，也可以是对已有实体的重新声明。按标准的规则，**声明默认就是定义，除非属于例外**（C++20 [basic.def]/2）。常碰到的例外有：没有函数体的函数声明；不带初始化器的 `extern` 变量声明；类内的非 inline 静态数据成员声明（`inline` 的，以及 C++17 起隐含 inline 的 `constexpr` 静态数据成员，在类内就是定义）；typedef、using、`static_assert` 这类不引入对象或函数的声明。
- **definition**, A definition is a declaration that fully defines the entity it introduces. The number and consistency of definitions are governed by the ODR.
  - 为什么要分声明和定义：编译器一次只处理一个翻译单元，用到别处定义的实体，只需要在当前翻译单元里有它的声明，声明可以重复。定义提供实体本身，数量由 ODR 规定：被 ODR-use 的非 inline 函数和变量，整个程序里恰好一个定义；类、枚举、inline 函数和变量、模板可以在每个翻译单元里各定义一次，但各份必须相同。这些数量要求是语言规则，不是链接器的功能：链接器只按符号名拼接，比较不了各份定义是否相同，标准也不要求实现诊断所有违反。完整规则和各版本差异见 [odr.md](odr.md)。Stroustrup 的说法是 "to avoid complicated linker rules, C++ requires that every object has a unique definition"（[FAQ](https://www.stroustrup.com/bs_faq2.html#in-class)）。

## declaration

Note that the C++ standard distinguishes between declaration (a syntactic construct) and declaration (a C++ term). A declaration(C++ term) can be not only a declaration but also a class specifier(syntactic construct), an enum specifier, a function parameter, and so on. 对一个名字来说，引入它的声明和引入它所指实体的声明不一定是同一个：`template <class T> struct A {};` 里，引入名字 `A` 的是 class specifier，引入类模板 `A` 这个实体的是外层的 template declaration。

语法上最常见的形式：

```text
declaration → block-declaration → simple-declaration → decl-specifier-seq init-declarator-list ;
init-declarator → declarator initializer(opt)
```

- 一条 simple declaration 可以声明多个实体，每个实体的类型 = decl-specifier-seq 的类型 + 各自声明符的修饰：`int a = 1, *p = &a, f(int);` 声明了一个 `int`、一个 `int*` 和一个函数。
- 声明点（locus）在声明符之后、初始化器之前，见 [scope.md](scope.md) ③。
