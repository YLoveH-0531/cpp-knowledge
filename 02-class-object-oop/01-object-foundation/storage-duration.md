# Storage Duration

- a. The storage duration is the property of an object that defines the minimum potential lifetime of the storage containing the object. The storage duration is determined by the construct used to create the object.
- b. some rules
  - The storage duration categories apply to references as well.
  - The storage duration of subobjects and reference members is that of their complete object.
- c. type
  - **static** storage duration：A variable satisfying all following conditions has static storage duration；存储持续整个程序期间。每个这样的变量（实体）只有一个对象；内部链接的名字（如头文件里命名空间作用域的 `static` 变量）在每个翻译单元里各是一个实体，所以每个翻译单元各有一个对象（[storage-class-specifiers-and-linkage.md](../../07-language-mechanics/storage-class-specifiers-and-linkage.md) 的 Linkage 一节）。
    - It belongs to a namespace scope or is first declared with `static` or `extern`.
    - It does not have thread storage duration.(since C++11)
    - 字符串字面量对象也是静态存储期，它不是变量，不在上面的条件里。
  - **thread** storage duration (since C++11)：声明为 `thread_local` 的变量，每个线程各有一份，线程开始时分配、结束时释放（暂不展开）。
  - **automatic** storage duration
    - Variables that belong to a block scope and are not explicitly declared `static`, `thread_local`,(since C++11) or `extern`. 存储持续到所在的块退出。
    - Variables that belong to a parameter scope (i.e. function parameters). 存储持续到形参销毁之后。
    - Temporary objects：标准说临时对象的存储期是静态、线程、自动三者之一（C++20 [basic.stc]/2），通常是自动；cppreference 只写了"自动"，并不完整。临时对象因绑定到引用而被延长生命周期时，存储期跟着那个引用走：命名空间作用域的 `const int& r = 5;`，临时对象是静态存储期，程序结束才销毁（[class.temporary]/6、/7）；函数里的同类写法则是自动存储期。实现观察（g++ 实测，不是语言保证）：前者的临时对象放在 `.rodata`（符号 `_ZGR1r_`），后者在栈上。
  - **dynamic** storage duration：`new` 表达式创建的对象，存储由分配函数分配、由释放函数回收，何时结束由程序显式决定，不由作用域决定。在已有存储上隐式创建的对象也是动态存储期（现行草案 [basic.stc.general]/2）。
