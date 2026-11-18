# Initialization

对象的初始化：基础概念、初始化形式、静态初始化与程序启动

| 研究角度 | 主要概念 | 解决的问题 | 在哪篇 |
|---|---|---|---|
| 初始化形式 | 直接、拷贝、列表初始化 | 初始化语法和上下文是什么？ | [02-initialization-forms.md](02-initialization-forms.md) |
| 初始化规则 | 默认、值、零、聚合初始化 | 对象具体如何初始化？ | [01-initialization-basics.md](01-initialization-basics.md) §2.2；聚合初始化见 [02-initialization-forms.md](02-initialization-forms.md) How ii.4 |
| 类初始化机制 | 构造函数、成员初始化列表 | 类对象及其子对象如何初始化？ | [constructors-and-destructors.md](../../02-class-and-oop/constructors-and-destructors.md) |
| 初始化阶段 | 静态、动态、常量初始化 | 静态/线程存储期对象在程序运行的哪个阶段初始化？ | [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md) §2.2 |
| 初始化顺序 | 有序、部分有序、无序 | 多个对象谁先初始化？ | [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md) §2.3.1 |
| 初始化时机 | 提前、推迟、首次经过声明 | 初始化什么时候真正执行？ | [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md) §2.3.2、§2.4 |

这些分类之间存在交叉，不是相互排斥的六类。例如全局的 `int x = 10;` 既是拷贝初始化，又是常量初始化，属于静态初始化阶段。

| 笔记 | 内容 | 状态 |
|---|---|---|
| [01-initialization-basics.md](01-initialization-basics.md) | 初始化与赋值的区别，作用域、链接、存储期、生命周期与初始化的关系；初始化的分类（形式与种类）；默认、值、零初始化；拷贝、直接、列表、聚合初始化的定位 | 笔记完成 |
| [02-initialization-forms.md](02-initialization-forms.md) | 拷贝初始化与直接初始化、列表初始化（含聚合初始化、initializer-list 构造函数优先）、`initializer_list` 的底层数组 | 笔记完成 |
| [03-static-initialization-and-program-startup.md](03-static-initialization-and-program-startup.md) | 静态初始化与动态初始化，非局部变量和块作用域的静态、线程存储期变量的初始化顺序与时机，Linux 可执行文件的节与段，从加载到退出的时间线 | 笔记完成 |
