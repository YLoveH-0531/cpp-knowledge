# lifetime-and-initialization

1.some explains

初始化（Initialization）描述对象如何获得初始状态,是更广泛的过程。它涉及选择适当的初始化方式、初始化基类和成员子对象，以及执行构造函数体等步骤。构造函数调用是其中的重要组成部分，而不是初始化的全部定义。对于通过构造函数初始化的类类型对象，初始化过程涉及调用构造函数；基类和成员子对象在构造函数体执行前完成初始化，而整个对象的初始化在非委托构造函数返回时完成.

赋值(Assignment)是用一个新的值去替换旧值，发生于初始化完成之后的使用过程。

生命周期(Lifetime)指的是一个对象从初始化完成到析构函数执行之前这段存活的时间。

作用域(Scope)指的是一个名字能够被代码看见的区域。是编译时期的事情

存储期(Storage)指的是存储的分类，有静态，自动，动态，线程

2. relations
先分配存储-->对象初始化 --> 对象析构 ---> 释放存储
生命周期、存储期、作用域如定义
初始化让对象得到第一个值，赋值是给生命周期已开始的对象换值


3.some concepts
1).declatation:
name, 用于指代实体的名称
entity, C++ 程序中被识别和描述的事物，例如对象、函数、类型、模板、命名空间等
object, Entity 的一种，具有类型、存储期、生命周期等属性，并占据一块存储区域
declaration, Declarations are how names and/or entities are introduced (or re-introduced) into the C++ program。对于引入名字的声明，它会规定该名字的解释及相关语义属性，使名字能够指代相应实体。声明可以是实体的首次声明，也可以是对已有实体的重新声明；其中一部分声明同时也是定义
defination, A definition is a declaration that provides a complete definition for the introduced entity。其数量和一致性受到 ODR 约束

declaration
Note that the C++ standard distinguishes between declaration (a syntactic construct) and declaration (a C++ term). A declaration(C++ term) can be not only a declaration but also a class specifier(syntactic construct), an enum specifier, a function parameter, and so on. 

scope declaration --> Simple declaration -->  decl-specifier-seq init-declarator-list(declarator initializer)

2). strorage class specifiers
The following keywords are storage class specifiers:
auto (until C++11)
register (until C++17)
static
thread_local (since C++11)
extern
mutable

3)

3)Storage duration
a.The storage duration is the property of an object that defines the minimum potential lifetime of the storage containing the object
b.some rules
The storage duration categories apply to references as well.
The storage duration of subobjects and reference members is that of their complete object.
c.type
static storage duration,
   It belongs to a namespace scope or are first declared with static or extern.
   It does not have thread storage duration.(since C++11)
thread storage duration (since C++11)
automatic storage duration
   Variables that belong to a block scope and are not explicitly declared static, thread_local,(since C++11) or extern. 
   Variables that belong to a parameter scope (i.e. function parameters). 
   Temporary objects also have an automatic storage duration.
dynamic storage duration, new expressions. 


4) Linkage
A name can have external linkage, module linkage(since C++20), internal linkage, or no linkage:

An entity whose nam
3)

3)
e has external linkage can be redeclared in another translation unit.
    Any of the following names declared at namespace scope have external linkage, unless they are declared in an unnamed namespace
       variables and functions not listed below (that is, functions not declared static, non-const variables not declared static, and any variables declared extern);
       enumerations;
       names of classes, their member functions, static data members (const or not), nested classes and enumerations, and functions  first introduced with friend declarations inside class bodies;
       names of all templates (that is, not function templates declared static).
    Any of the following names first declared at block scope have external linkage:
       names of variables declared extern;
       names of functions.
An entity whose name has internal linkage can be redeclared in another scope in the same translation unit.
    Any of the following names declared at namespace scope have internal linkage:
       variables, functions, or function templates declared static;
       variables of non-volatile const-qualified type, unless they are explicitly declared extern, or they were previously declared and the prior declaration did not have internal linkage;
       data members of anonymous unions;
       all names declared in unnamed namespaces or a namespace within an unnamed namespace, even ones explicitly declared extern
no linkage, An entity whose name has no linkage can only be redeclared in the same scope.
    Any of the following names declared at block scope have no linkage:
       variables that are not explicitly declared extern (regardless of the static modifier);
       local classes and their member functions;
       other names declared at block scope such as typedefs, enumerations, and enumerators.