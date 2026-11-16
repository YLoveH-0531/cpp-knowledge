# Object Foundation

对象的生命周期、初始化、内存布局与启动

这三篇覆盖[上一级地图](../README.md)里的 Q1–Q6；其中 Q2 的 ODR 一半在 [odr.md](../../07-language-mechanics/odr.md)。

| 笔记 | 内容 | 状态 |
|---|---|---|
| [lifetime-and-initialization.md](lifetime-and-initialization.md) | 声明与定义、存储类说明符、存储期、链接、生命周期、作用域；类对象的初始化（构造函数、成员初始化列表） | 笔记完成 |
| [initialization-forms.md](initialization-forms.md) | 拷贝初始化与直接初始化、列表初始化（含聚合初始化、initializer-list 构造函数优先）、`explicit` | 笔记完成 |
| [process-memory-layout-and-startup-review.md](process-memory-layout-and-startup-review.md) | 初始化的种类与阶段，非局部变量和静态局部变量的初始化顺序与时机，Linux 可执行文件的节与段，从加载到退出的时间线 | 笔记完成 |

把这些概念在一个变量上走一遍的案例：[static-data-members.md](../02-class-and-oop/static-data-members.md)。
