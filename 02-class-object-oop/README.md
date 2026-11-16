# Class, Object & OOP

类、对象、生命周期、继承、多态

## 解决什么问题

<!-- TODO: 1-3 句：为什么名字、对象、实现这三层要放在一起看。写完删掉这行 -->

核心问题：一个 C++ 对象从哪里来，什么时候开始存在，能存在多久，又是怎样结束的？

## 在地图上的位置

下面六个问题按“名字 → 对象 → 实现”的顺序，把一个变量从写下到消失的路走一遍；静态存储期的变量走得最全，[static-data-members.md](02-class-and-oop/static-data-members.md) 是案例。

| 层 | 问题 | 概念 | 笔记 |
|---|---|---|---|
| 名字（编译、链接期） | Q1 这个名字在哪里看得见？ | 作用域 | [lifetime-and-initialization.md](01-object-foundation/lifetime-and-initialization.md) §3.6 |
| | Q2 别的 .cpp 里的同名，是同一个实体吗？ | 链接、ODR | [odr.md](../07-language-mechanics/odr.md)；[lifetime-and-initialization.md](01-object-foundation/lifetime-and-initialization.md) §3.4 |
| 对象（运行期） | Q3 存储属于哪一类？对象何时存在？ | 存储期、生命周期 | [lifetime-and-initialization.md](01-object-foundation/lifetime-and-initialization.md) §3.3、§3.5 |
| | Q4 怎么初始化，谁先谁后？ | 初始化 | [lifetime-and-initialization.md](01-object-foundation/lifetime-and-initialization.md) §3.7；[initialization-forms.md](01-object-foundation/initialization-forms.md)；[process-memory-layout-and-startup-review.md](01-object-foundation/process-memory-layout-and-startup-review.md) §2 |
| 实现（Linux） | Q5 落在可执行文件和进程的哪里？ | 节与段、栈和堆 | [process-memory-layout-and-startup-review.md](01-object-foundation/process-memory-layout-and-startup-review.md) §3 |
| | Q6 从加载到退出，各在何时发生？ | 启动与退出 | [process-memory-layout-and-startup-review.md](01-object-foundation/process-memory-layout-and-startup-review.md) §4 |
| 案例 | 类的静态数据成员，把 Q1–Q6 各走一遍 | | [static-data-members.md](02-class-and-oop/static-data-members.md) |

与相邻主题的关系：

- [new-delete](../03-memory-resource/new-delete.md) 把“存储”和“对象”分成两件事；本章补上另一半：存储从哪来、能活多久（存储期），对象何时开始、何时结束（生命周期）。
- [RAII](../03-memory-resource/RAII.md) 用到“对象的生命周期从初始化完成才开始”这个前提，本章把它展开。
- 本章之后的主线：[拷贝与移动](../03-memory-resource/copy-and-move-guide.md) → [智能指针](../03-memory-resource/smart-pointers-guide.md) → [异常安全](../07-language-mechanics/exception-safety-guide.md)。

## 目录

- [01-object-foundation](01-object-foundation/README.md)：对象的生命周期、初始化、内存布局与启动
- [02-class-and-oop](02-class-and-oop/README.md)：类的成员与面向对象机制

## 主题清单

| 主题 | 为什么存在 | 状态 |
|---|---|---|
| [lifetime-and-initialization.md](01-object-foundation/lifetime-and-initialization.md) | TODO | 笔记完成 |
| [initialization-forms.md](01-object-foundation/initialization-forms.md) | TODO | 笔记完成 |
| [process-memory-layout-and-startup-review.md](01-object-foundation/process-memory-layout-and-startup-review.md) | TODO | 笔记完成 |
| [odr.md](../07-language-mechanics/odr.md) | TODO | 笔记完成 |
| [static-data-members.md](02-class-and-oop/static-data-members.md) | TODO | 笔记完成 |
| 构造与析构、运算符重载、继承、虚函数 | TODO | 待写 |

## 读完应能回答

1. 对象的生命周期从哪一刻开始、到哪一刻结束？它和存储期（自动、静态、线程、动态）是一回事吗？存储为什么能先于对象存在、晚于对象结束？
2. 作用域管的是什么？它和生命周期是一回事吗？举出作用域结束而对象还活着的例子，再举出名字不可见而对象仍在的例子。
3. `T x;`、`T x{};`、`T x = T();`、`new T`、`new T()` 分别是哪种初始化？对内置类型、对带默认成员初始化器的类、对有用户提供构造函数的类，各自得到什么？初始化和赋值的区别是什么？构造函数为什么该用初始化列表而不是在函数体里赋值？
4. 构造一个派生类对象时，基类、成员、构造函数体的先后顺序由什么决定？成员的顺序由声明顺序还是初始化列表顺序决定？销毁的顺序呢？
5. （进阶，知道时机即可）全局、静态局部、自动、动态四种存储期的对象，各在什么时候构造、什么时候销毁？

<!-- TODO: Q2（ODR）、Q5（节与段）没有对应的问题，各补一问 -->
