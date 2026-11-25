# Object Foundation

对象的生命周期、初始化、内存布局与启动

这个文件夹和语言机制里的 [declaration-and-definition.md](../../07-language-mechanics/declaration-and-definition.md)、[storage-class-specifiers-and-linkage.md](../../07-language-mechanics/storage-class-specifiers-and-linkage.md)、[scope.md](../../07-language-mechanics/scope.md)、[odr.md](../../07-language-mechanics/odr.md) 一起，覆盖[上一级地图](../README.md)里的 Q1–Q6。

| 笔记 | 内容 | 状态 |
|---|---|---|
| [01-initialization/](01-initialization/README.md) | 初始化专题：基础概念、初始化形式、静态初始化与程序启动（含 Linux 可执行文件的节与段） | 笔记完成 |
| [storage-duration.md](storage-duration.md) | 存储期：静态、线程、自动、动态 | 笔记完成 |
| [lifetime.md](lifetime.md) | 生命周期：对象和引用何时开始、何时结束 | 笔记完成 |

object 与 reference 的内容已并入 [pointer-reference-const.md](../../01-basic/pointer-reference-const.md)。

把这些概念在一个变量上走一遍的案例：[static-data-members.md](../02-class-and-oop/static-data-members.md)。
