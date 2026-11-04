# 对象生命周期、存储期与初始化：学习指南

> 主线第 1/4 篇。排期：W1 第 1 篇，3h，标准档（正文 ≤ 100 行）。笔记正文由你自己写；这份指南只给承接关系、资料、过程和实验设计，按第一步到第五步的顺序往下做；实验的参考观察放在文末。

## 怎么用这份指南

- **按顺序做。** 下面从“第一步”到“第五步”依次排列，每一步一节，做完一步再往下看。“跑完之后再读”放在文末，第三步写完预测、跑完之后才看。
- **名词不是任务清单。** 一个名词出现，不等于它成了你要研究透的任务。下面的名词分成 A、B、C 三级，只有 A 级是这篇要真正建立的模型。
- **从问题出发，不从名词出发。** 先回答“读完应能回答”的 5 个问题，需要哪个名词再去查哪个。
- **不买新书，不扩成新项目。** 现有资料够用；这 3 小时是第一轮整理的预算，不是把所有语言规则的边界研究完。

## 在主线上的位置

[RAII 笔记](../03-memory-resource/RAII.md) → **① 构造（本篇）** → [② 移动](../03-memory-resource/copy-and-move-guide.md) → [③ 智能指针](../03-memory-resource/smart-pointers-guide.md) → [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)

### 承接：RAII 笔记用了、但没有展开的前提

[RAII 笔记](../03-memory-resource/RAII.md)的 How ii 给出构造失败时的三条规则：非委托构造函数抛出时，对象自己的析构函数不会被调用；已经构造完成的基类和成员子对象按构造的逆序销毁；`new` 表达式里还会释放已分配的存储。这三条背后是同一个前提：**对象的生命周期，从初始化完成的那一刻才开始。** 构造函数还没返回，生命周期就还没开始，所以没有析构；已经构造完的子对象，生命周期已经开始，所以要销毁。RAII 笔记在使用这个前提，没有展开它。

[new-delete 笔记](../03-memory-resource/new-delete.md)把"对象"和"存储"分成两件事：`new` 表达式 = 分配存储 + 初始化对象。本篇补上缺的另一半：存储从哪来、能活多久（**存储期**）；对象何时开始、何时结束（**生命周期**）；**初始化**怎样把一块存储变成一个对象。

你已经在 [lifetime-and-initialization.md](lifetime-and-initialization.md)（原 temp.md）写下了这篇的起点：初始化是比"调用构造函数"更广的过程，基类和成员子对象在构造函数体执行之前完成初始化，整个对象在非委托构造函数返回时初始化完成。本篇要把这段话变成一张能查、能验证的模型。

### 这篇的整体模型：三个问题加一条时间线

整篇围绕一个问题：一个 C++ 对象从哪里来，什么时候开始存在，能存在多久，又是怎样结束的。三个容易混的概念，分别回答三个不同的问题：

| 概念 | 回答什么 | 关注的是 |
|---|---|---|
| 作用域（scope） | 名字在代码的哪一段可见 | 程序文本里的名字，编译期的事 |
| 存储期（storage duration） | 对象用的存储属于哪一类：自动、静态、线程、动态 | 存储 |
| 生命周期（lifetime） | 对象什么时候真正存在 | 对象，运行期的事 |

三个例子，看它们怎样对不上：

| 例子 | 名字可见 | 存储期 | 对象存活 |
|---|---|---|---|
| 块里的 `int x = 10;` | 到块结束 | 自动 | 到块结束 |
| 函数里的 `static int x = 10;` | 只在函数内 | 静态 | 第一次执行到这里时初始化，一直到程序结束 |
| 块里 `int* q = new int(10);` 并把地址交给外面的指针 | `q` 到块结束 | 动态 | 从 `new` 到 `delete` |

只有自动存储期的对象，作用域结束才恰好等于生命周期结束。

**时间线（对象）**：分配存储 → 初始化完成（生命周期开始）→ 使用（赋值只出现在这一段，作用在生命周期已经开始的对象上）→ 销毁（生命周期结束）→ 释放存储。初始化让对象获得初始状态，完成时生命周期开始；赋值是给生命周期已经开始的对象换值。两者要成对写清。

**与 RAII 的接口**：RAII 把资源的获取绑在初始化上、释放绑在销毁上，之所以可靠，是因为销毁的时机和顺序是确定的。这一头在本篇写清，构造失败时和异常展开时怎样销毁，由 RAII How ii 和 ④ 承接。

### 引出：用另一个对象来初始化新对象

初始化里有一类特殊的形式：用同类型的另一个对象初始化新对象——`T a = b;`、按值传参、按值返回，走的都是拷贝构造或移动构造。当对象是一份资源的管理者时，这一步决定资源怎么办：复制、转移，还是禁止。这是下一篇 [② 移动](../03-memory-resource/copy-and-move-guide.md)的问题。

## 读完应能回答

1. 对象的生命周期从哪一刻开始、到哪一刻结束？它和存储期（自动、静态、线程、动态）是一回事吗？存储为什么能先于对象存在、晚于对象结束？
2. 作用域管的是什么？它和生命周期是一回事吗？举出作用域结束而对象还活着的例子，再举出名字不可见而对象仍在的例子。
3. `T x;`、`T x{};`、`T x = T();`、`new T`、`new T()` 分别是哪种初始化？对内置类型、对带默认成员初始化器的类、对有用户提供构造函数的类，各自得到什么？初始化和赋值的区别是什么？构造函数为什么该用初始化列表而不是在函数体里赋值？
4. 构造一个派生类对象时，基类、成员、构造函数体的先后顺序由什么决定？成员的顺序由声明顺序还是初始化列表顺序决定？销毁的顺序呢？
5. （进阶，知道时机即可）全局、静态局部、自动、动态四种存储期的对象，各在什么时候构造、什么时候销毁？

## 本篇不写什么

- 构造失败时的清理：RAII 笔记 How ii 和 new-delete 笔记已经写了，这里只放链接。
- placement new 里"存储先于对象、晚于对象"的时间差：new-delete 的 Example 在验证，不重复。
- 继承体系里的构造细节（虚基类、构造期间调用虚函数）：留给 W4 的继承与多态。
- 名字查找、命名空间、ADL：作用域这里只讲“名字可见范围与对象生命周期不是一回事”，查找规则留给 W4 的名字查找。
- 拷贝构造、移动构造本身的规则：下一篇。
- 为这篇买新书、逐个名词做笔记：都不做。

## 步骤总览

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 第一步 先写 | 合上资料，凭现在的理解写 Why / What | 0.25 | 两小段话，加一串"不确定"的问题 |
| 第二步 读 | 读我整理的表和要点（11-04 起，原文只在存疑时查） | 0.75 | 读懂这张表 |
| 第三步 动手 | 做三个实验，每个先预测再跑 | 0.75 | 三份预测，三份实际输出 |
| 第四步 写 | 写笔记初稿，写完后对照书和你自己的旧笔记 | 1 | 笔记初稿 |
| 第五步 封板 | 四条合格线自检，周五审一次，commit | 0.25 | 封板 |

合计 3h。

## 第一步：先写（0.25h）

合上资料，用自己的话写 Why / What 各 3–5 句，只回答三件事：

- 对象和存储是什么关系？
- 生命周期、存储期、作用域分别管什么？
- 初始化和赋值有什么不同？

起点是 [lifetime-and-initialization.md](lifetime-and-initialization.md) 里你已写的那几段。写不出来的地方，就是第二步要读的地方。

产出：两小段话，加一串"不确定"的问题。

## 第二步：读（0.75h）

> 11-04：你读完了 Storage duration 和 Lifetime 两页，用了约 4 小时，决定不再读原文。这五页（含还没读的三页）我已经读完，整理在本节最后：一张存储期的表，加第 1 到第 4 问的要点。你只读整理；存疑的地方，按每条标的出处去查原文。

### 读什么（原计划：读原文，共约 50 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| cppreference：[Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)、[Lifetime](https://en.cppreference.com/w/cpp/language/lifetime) | 已读（11-03）。前者读四种存储期的定义，跳过 Linkage 一节；后者读对象生命周期的开始与结束，跳过 implicit-lifetime 类型 | 第 1、5 问 | 15 min |
| cppreference：[Scope](https://en.cppreference.com/w/cpp/language/scope) | 见下面“还没读的三页”的清单 | 第 2 问 | 5 min |
| cppreference：[Initialization](https://en.cppreference.com/w/cpp/language/initialization)，以及页面里链出去的 Default、Value、Zero initialization 三个单独页面 | 见清单 | 第 3、5 问 | 20 min |
| cppreference：[Constructors and member initializer lists](https://en.cppreference.com/w/cpp/language/constructor) | 见清单 | 第 4 问 | 10 min |

### 还没读的三页：具体读哪里（查证时用）

你不再通读这三页。整理里存疑的某一条，按下面的清单找到对应小节核对；清单外的小节一律跳过。每项后面的词数是正文的大致长度，三页合计约 3400 词。

#### Scope（约 900 词）

页面：[Scope](https://en.cppreference.com/w/cpp/language/scope)

- [ ] 页面开头两句（目录之前）
- [ ] General（约 360 词）：作用域的定义，以及那张标了 program point 的示意图
- [ ] Block scope（约 290 词）
- [ ] Namespace scope（约 160 词）
- [ ] Point of declaration：只读开头两句和第一个例子 `int x = x;`（约 90 词）

跳过：Function parameter scope、Lambda scope、Class scope、Enumeration scope、Template parameter scope、Contract-assertion scope，以及 Point of declaration 的其余部分。这一页没有“名字隐藏”的专门小节，它是 C 级，不用找。

#### Initialization（约 1600 词：本页 800 词，三个单独页面 800 词）

页面：[Initialization](https://en.cppreference.com/w/cpp/language/initialization)

- [ ] 页面开头一段（目录之前）
- [ ] Initializers（约 140 词）：初始化器的四种写法
- [ ] Initializer semantics（约 230 词）：哪种写法对应哪种初始化；例子里 `std::string s2();` 那一行是最令人烦恼的解析
- [ ] Non-local variables 开头一段，加 Static initialization、Dynamic initialization 两个小节（约 370 词）：静态初始化与动态初始化的区分

跳过：Early dynamic initialization、Deferred dynamic initialization（C 级）、Static local variables（Storage duration 页里读过了）、Class members。

Default、Value、Zero initialization 在这一页里只是链接，各是一个单独的页面，各读下面标出的部分：

- [ ] [Default initialization](https://en.cppreference.com/w/cpp/language/default_initialization)：Explanation（约 200 词）；Indeterminate and erroneous values 只读标着 `(until C++26)` 的那两句，C++26 的 erroneous value 跳过
- [ ] [Value initialization](https://en.cppreference.com/w/cpp/language/value_initialization)：Explanation（约 210 词）和 Notes（约 110 词）
- [ ] [Zero initialization](https://en.cppreference.com/w/cpp/language/zero_initialization)：Explanation（约 220 词）

这三页的 Syntax、Example、Defect reports，以及 Default initialization 页的 Default-initialization of a const object 都跳过。表里静态存储期的“何时初始化”来自 Non-local variables 这一节。

#### Constructors and member initializer lists（约 900 词）

页面：[Constructors and member initializer lists](https://en.cppreference.com/w/cpp/language/constructor)

- [ ] Member initializer list（约 240 词）：只读语法、(1)(2) 两条说明和最后的小例子，(3) 参数包跳过
- [ ] Explanation 的第 1、2 段（约 200 词）：第 1 段讲构造函数在初始化时被调用，以及 `explicit`；第 2 段说构造函数体开始执行之前，基类和成员已经初始化完，这是成员初始化列表存在的理由
- [ ] Explanation 里 “If a non-static data member has a default member initializer…” 那一段和例子（约 55 词）
- [ ] Delegating constructor（约 105 词）：扫一眼
- [ ] Initialization order（约 135 词）：基类、成员、构造函数体的顺序
- [ ] Example（约 200 词）：对照里面的注释，和实验 1 的结果互相印证

跳过：Syntax、Explanation 里其余的段落（虚基类、函数 try 块、引用成员不能绑定临时对象）、Operations during construction and destruction（构造期间调用虚函数，属于 W4）、Inheriting constructors。

### 读到什么程度：名词分级（只列名词，内容你自己写）

分级规则：A 级要能不看资料讲清，并用实验验证；B 级知道常见规则，笔记里写一句话加一个例子，不抠边界；C 级只知道有这回事，笔记里留一个 `[C]` 标记，需要时再查。

#### A 级：必须真正理解

| 对应的问 | 名词 | 要做到 |
|---|---|---|
| 1 | 对象（object）与存储（storage） | 是两件事：存储可以先于对象存在、晚于对象结束 |
| 1 | 生命周期（lifetime）的开始与结束；构造完成（fully constructed） | 从初始化完成开始；非委托构造函数没返回时，生命周期还没开始。结束有两条路径：销毁（类类型是析构函数调用开始），或存储被释放、被不嵌套在它里面的对象复用，后一条不会调用析构 |
| 1、5 | 存储期（storage duration）：自动、静态、线程、动态 | 四种各自的对象何时开始、何时结束；线程存储期知道含义即可，实验不覆盖 |
| 2 | 名字（name）、作用域（scope）：块作用域、命名空间作用域（全局作用域属于它） | 名字的可见范围；与生命周期不是一回事（实验 3） |
| 3 | 初始化（initialization）与赋值（assignment） | 初始化：对象获得初始状态，完成时生命周期开始；赋值：给生命周期已开始的对象换值 |
| 3 | 默认（default）、值（value）、直接（direct）、列表（list）初始化 | 基本区别（实验 2） |
| 3 | 成员初始化列表（member initializer list） | 为什么用它，而不是在函数体里赋值 |
| 4 | 基类子对象（base subobject）、成员子对象（member subobject）；构造与销毁的顺序 | 基类 → 成员（声明顺序）→ 函数体，销毁相反（实验 1） |

#### B 级：知道常见规则

| 名词 | 知道到什么程度 |
|---|---|
| 零初始化（zero initialization） | 静态存储期的对象先被零初始化 |
| 静态初始化（static）与动态初始化（dynamic） | 区别 |
| 块里的静态变量（static local） | 第一次经过声明时才初始化，只初始化一次；初始化抛异常则下次再试；只有初始化成功的才会在程序退出时销毁 |
| 临时对象（temporary）与完整表达式（full-expression） | 临时对象通常在完整表达式结束时销毁 |
| 子对象（subobject）的存储期 | 跟包含它的完整对象一致；引用成员自己的存储跟完整对象走，它指向的对象另算 |
| 聚合（aggregate）与聚合初始化 | 什么是聚合、怎么初始化；聚合的定义在 C++14、C++17、C++20 各改过一次，记一句“随版本变”，并写清你用的版本 |
| 窄化转换（narrowing） | 花括号初始化禁止某些窄化 |
| 默认成员初始化器（default member initializer） | 成员可以在声明处给默认值 |
| 委托构造（delegating constructor） | 构造函数可以交给同类的另一个构造函数 |
| `explicit` | 限制隐式转换 |
| `initializer_list` 构造函数优先 | `vector<int> v{10, 5}` 与 `v(10, 5)` 不同 |
| 最令人烦恼的解析（most vexing parse） | `Widget a();` 是函数声明，不是对象 |

#### C 级：只知道有这回事

| 名词 | 当前要求 |
|---|---|
| 名字隐藏（name hiding） | 知道有这个现象，查找规则留给 W4 |
| 声明（declaration）与定义（definition） | 能区分即可，不展开特殊情况 |
| 不确定值（indeterminate value） | 知道未初始化的内置类型变量不能读 |
| 声明点（point of declaration） | 知道声明之后名字才可见，细节以后再查 |
| 跨翻译单元的静态初始化顺序（static initialization order fiasco） | 理解现象：初始化顺序没有保证 |
| 静态初始化的完整规则 | 不深入排序规则 |
| 隐式创建对象（implicitly created objects） | 知道 `malloc`、`operator new` 返回的存储里会隐式创建简单类型的对象；细节不展开 |
| 引用的存储期 | 知道引用变量也分自动、静态等；引用所在的存储期结束，它就不能再用 |
| 函数形参的销毁时机 | 形参是自动存储期；销毁时机由实现定义：被调函数退出时，或调用所在的完整表达式结束时（g++ 是后者） |
| 异常对象的存储 | 异常对象是动态存储期，但存储不是 `operator new` 分配的，由实现决定 |
| 链接性（linkage）：内部链接 | 管“名字在别的翻译单元里是不是同一个实体”；命名空间作用域里，`static` 的变量和函数、`const` 变量（`extern` 或 `inline` 除外）、匿名命名空间里的名字是内部链接 |
| inline 函数里的静态局部变量 | 函数是外部链接时，各翻译单元里的定义共用同一个静态局部变量；`static inline` 或匿名命名空间里的 inline 函数则每个翻译单元各一个 |

不在本篇的名词（别提前写）：拷贝初始化的细节、拷贝省略、RVO / NRVO、值类别、移动语义属于 ②；`new` 失败与析构不抛异常属于 RAII How ii 和 ④；虚基类、构造期间调用虚函数属于 W4。

### 边读边填的表（我读完填好了）

这张表原本由你边读边填，现在由我读完这五页填好。你读表就行；存疑的格子，按表后的“出处”核对原文。

| 存储期 | 存储何时获得 | 何时初始化 | 何时销毁 | 作用域结束时对象怎样 |
|---|---|---|---|---|
| 自动（块里的局部变量） | 标准没规定起点，只规定持续到所在的块退出（见注 1） | 控制流每次执行到声明时 | 离开所在的块时，按构造的逆序 | 销毁，存储释放；作用域结束就是生命周期结束 |
| 静态 · 命名空间作用域 | 程序开始时 | 程序启动阶段：先静态初始化（能常量初始化就常量初始化，否则零初始化），再动态初始化；通常在 `main` 之前 | 程序退出时，按初始化完成的逆序 | 不受影响，活到程序退出 |
| 静态 · 块作用域（静态局部变量） | 程序开始时 | 零初始化、常量初始化在启动阶段就完成；还需要动态初始化的，在第一次经过声明时做，只做一次 | 程序退出时，和上一行的对象一起按初始化完成的逆序；只销毁初始化成功的 | 不受影响；再次进入这个块，不重新初始化 |
| 线程（`thread_local`，每个线程一份） | 线程开始时 | 每个线程各初始化一份；时机的细节暂不展开 | 线程退出时 | 不受影响，活到线程退出 |
| 动态（`new`） | `new` 表达式调用分配函数时 | 在 `new` 表达式里，存储到手之后立刻 | `delete` 表达式里：先调用析构函数，再释放存储；不 `delete`、也不显式调用析构函数，就不销毁 | 不受影响；指向它的指针变量（自动存储期）销毁了，对象还在，没有所有者就泄漏 |

注与补充（表里放不下的）：

1. 自动存储期的起点：标准（[basic.stc.auto]）和 cppreference 的 Storage duration 页都只写存储“持续到块退出”，没写从哪一刻开始，所以表里不写时刻。能确定的只有先后：对象要等存储到手、初始化完成才开始。想象成“进块时存储就有了”可以，但那是理解用的模型，不是标准的规定。
2. 自动存储期的内置类型变量，没写初始化器就不初始化，值不确定，读它是未定义行为（C++26 起改称“错误值”，本篇按 C++26 之前写）。块里调用 `std::exit` 或 `abort` 结束程序时，自动存储期的对象不会被销毁。
3. 静态存储期的变量，不论写没写初始化器，都在其他初始化之前先零初始化（能常量初始化的除外），所以没写初始化器的全局 `int` 是 0。实现上通常把零初始化的变量放在 `.bss`（不占可执行文件的磁盘空间），这是实现的做法，语言没有规定。
4. 命名空间作用域的静态对象：同一翻译单元里按定义的先后顺序初始化；不同翻译单元之间的顺序没有保证（C 级，记现象）；类模板的静态数据成员、变量模板、inline 变量的顺序规则另有不同，不展开。初始化时抛异常会调用 `std::terminate`。
5. 静态局部变量：初始化抛异常，视为没初始化，下次经过声明再试；初始化过程中递归进入同一个声明，是未定义行为。
6. 动态存储期：构造函数抛异常时，`new` 表达式在找得到配对的释放函数时，调用它还回存储，对象的生命周期没开始，所以不调用它自己的析构函数，但已经构造好的基类和成员子对象会按相反顺序析构（g++ 11.4、clang 14 实测；RAII 笔记 How ii 已写）；找不到就不释放 [expr.new]：自己写了 `operator new(size_t, Tag)`，却没写 `operator delete(void*, Tag)`，构造失败存储就泄漏（g++ 11.4、clang 14 实测）。也可以先拿到存储（`malloc`、缓冲区），再用 placement new 创建对象。
7. 子对象和引用成员的存储期，随它所在的完整对象。

出处：Storage class specifiers 页的 Storage duration、Static block variables；Initialization 页的 Non-local variables；Zero initialization 页；标准草案的 [basic.stc]、[stmt.dcl]、[stmt.jump]、[basic.start.static]、[basic.start.dynamic]、[basic.start.term]、[class.dtor]、[expr.new]。

### 第 1 到第 4 问的要点（我读完后整理）

第 5 问就是上面的表。下面每条后面方括号里是出处。

#### 第 1 问：生命周期、存储期与存储

- 开始：存储拿到，并且初始化完成 [Lifetime 页]。初始化在非委托构造函数返回时就算完成，委托构造函数的函数体即使还没执行完，对象的生命周期也已经开始；非委托构造函数没返回，生命周期就还没开始 [[class.base.init]/11 及其 Note。这句明确的话是 CWG 2756 在 2026 年 6 月作为缺陷报告加上的，C++20 的原文没有，只写了“目标构造函数返回后，才执行委托构造函数的函数体”]。基类子对象和成员子对象各自在自己初始化完成时开始 [Lifetime 页]。
- 结束有三种情况，前两种是销毁，第三种不是：非类类型的对象被销毁；类类型的对象，析构函数调用开始；对象占的存储被释放，或被一个不嵌套在它里面的对象复用，后一种不会调用析构函数 [Lifetime 页]。
- 存储期是对象的属性，规定装着它的那块存储最短要保留多久（minimum potential lifetime of the storage）；对象的生命周期总是等于或嵌套在存储的生命周期之内 [Storage duration 页第一段；Lifetime 页]。所以存储能先于对象存在：自动变量先有存储、声明处才构造；`new` 先由 `operator new` 给存储、构造完成对象才开始；placement new 是缓冲区先在、对象后建；`std::vector` 容量里还没构造的槽位也是。存储也能晚于对象结束：`delete` 里先析构、后释放；显式调用 `p->~T()` 之后存储还在；`std::vector` 里已经销毁的元素，槽位也还在。

#### 第 2 问：作用域

- 作用域管的是名字（声明）：一个声明在程序文本的哪些位置可见；在一个作用域里，非限定名字查找把名字和它的声明联系起来。它是编译期的事，不管对象活多久 [Scope 页开头两句]。
- 块作用域：`if`、`switch`、`for`、`while`、`do-while` 语句，异常处理器（handler），复合语句（`{}`）各引入一个，里面声明的变量叫块变量 [Block scope]。内层块可以用同名声明遮蔽外层的（`for (int i = 0; …)` 里的 `i` 和外面的 `i` 是两个对象）；但函数体最外层的块里不能再声明和形参同名的变量，`if` 条件里声明的变量也不能在它的子语句最外层重新声明，这两种是错误（g++、clang 实测都报错）。
- 命名空间作用域：命名空间定义引入一个；全局作用域就是全局命名空间的命名空间作用域 [Namespace scope]。
- 声明点：名字从它的声明符之后、初始化器之前开始可见，所以 `int x = x;` 里右边的 `x` 是正在初始化的这个，读到的是它自己不确定的值，不是外层的 `x` [Point of declaration]。
- 名字和对象是两件事，各举两类例子。作用域结束而对象还活着：块里的 `static` 变量；`int* p = new int(10);` 出了块，名字 `p` 没了，对象还在；返回指向静态局部变量的指针。名字不可见而对象仍在：内层的同名变量遮蔽了外层，外层的对象还活着；被调函数看不到调用者的局部变量，它们照样活着。

#### 第 3 问：几种初始化

写法和种类 [Initialization 页的 Initializers、Initializer semantics]：没有初始化器 → 默认初始化；`= 表达式` → 拷贝初始化；`(表达式列表)` → 直接初始化；`{…}` 或 `= {…}` → 列表初始化；空括号 `()` → 值初始化（只能出现在 `new T()`、`T()`、成员初始化器里，不能写成 `T x();`）。

各写法对各类对象得到什么：

| 写法 | 种类 | 内置类型 | 默认构造函数不是用户提供的类（隐式生成的，或类内 `= default` 的） | 默认构造函数是用户提供的类 |
|---|---|---|---|---|
| `T x;` | 默认初始化 | 不确定 | 有默认成员初始化器的成员用它，其余内置类型的成员不确定 | 调用该构造函数；它没初始化的内置类型成员不确定 |
| `T x{};` | 列表初始化，花括号为空时：聚合 → 聚合初始化；有默认构造函数的非聚合类、内置类型 → 值初始化 | 0 | 有默认成员初始化器的成员用它，其余内置类型的成员为 0 | 值初始化：只调用该构造函数，不清零；它没初始化的内置类型成员不确定 |
| `T x = T();` | 拷贝初始化；右边的 `T()` 是值初始化（C++17 起它的结果直接构造在 `x` 里，不再有临时对象） | 0 | 同 `T x{};` | 同 `T x{};` |
| `new T` | 默认初始化 | 不确定 | 同 `T x;` | 同 `T x;` |
| `new T()` | 值初始化 | 0 | 同 `T x{};` | 同 `T x{};` |

- 表里的“成员”只指内置类型的成员。类类型的成员要按它自己的类型再判断一遍：默认初始化时调用它自己的默认构造函数（`std::string` 成员是空串，不是“不确定”）。值初始化（`T x = T();`、`new T()`）在 T 的默认构造函数不是用户提供的时，先把整个对象清零，再默认初始化；而 `T x{}` 当 T 是聚合时是聚合初始化，没写的成员各自单独初始化（有默认成员初始化器的用它，否则用 `{}` 值初始化）[[dcl.init.aggr]]，两条路径对类类型的成员可能不同。例：聚合 `struct A { M m; int i; };`，`M` 的默认构造函数是用户提供的、不碰 `m.n`；标准保证 `A b = A();` 的 `b.m.n` 是 0，`A a{};` 的 `a.m.n` 不确定。clang 14 实测 `a.m.n` 读到原来的垃圾值；g++ 11.4 对 `A{}` 生成的代码和 `A()` 的一样（先整体清零），读到 0，这是允许的（读不确定的值本来就是未定义行为），所以在 g++ 上看不出两者的差别。
- 表里的“不确定”，在静态存储期的变量上是 0，因为它们先零初始化。
- 验证：`new T`、`new T()`、`new T{}` 三种写法和静态存储期的情形，我用 g++ 11.4、clang 14 在 `-std=c++11` 到 `c++20` 下都跑过（成员都是内置类型），结果一致，和上表一致；`T x;`、`T x{};`、`T x = T();` 三行按标准推出，没有单独跑，它们和对应的 `new` 行走同一条初始化规则。
- 值初始化的规则：类类型，有未被删除的默认构造函数时：若它不是用户提供的，先零初始化再默认初始化；若是用户提供的，只做默认初始化，所以用户提供的构造函数没碰的成员不会被清零；没有默认构造函数、或它被删除时，值初始化不能成功（编译报错，g++、clang 在 `-std=c++11`、`c++17`、`c++20` 下实测）；数组逐元素；其余类型零初始化 [Value initialization 页]。“用户提供”的意思是：这个构造函数是自己声明的，并且不是在第一次声明时就写成 `= default` 或 `= delete`；所以隐式生成的、类内写 `= default` 的都不算（即使类里还有别的用户提供的构造函数），类内只声明、类外再写 `T::T() = default;` 的算 [[dcl.fct.def.default]]。我在 g++ 11.4、clang 14 的 `-std=c++11`、`c++17`、`c++20` 下实测：前一种的 `new T()`、`new T{}` 把成员清零，后一种不清零。默认初始化：类类型调用默认构造函数，数组逐元素，其余什么也不做 [Default initialization 页]。零初始化：标量变 0；类的每个成员、基类子对象、填充位都清零 [Zero initialization 页]。
- `T x();` 不是初始化，是函数声明（最令人烦恼的解析）；C++11 以前给命名变量做值初始化要写 `T x = T();`；标准容器用单个大小参数构造或 `resize` 增长时，新元素是值初始化的（所以 `vector<int>(5)` 全是 0）[Value initialization 页的 Notes]。
- 初始化与赋值：`T x = e;` 里的 `=` 是拷贝初始化，不是赋值；赋值（`x = e;`）作用在生命周期已经开始的对象上。
- 为什么用成员初始化列表：构造函数体开始执行之前，所有直接基类、虚基类和非静态成员都已经初始化完了；初始化列表是指定它们怎样初始化的地方，没写的走默认初始化（或默认成员初始化器）[Constructors 页的 Explanation]。在函数体里写 `m = 1`，是先默认初始化 `m` 再赋值：我实测一个成员类 `M`，函数体里赋值会打印 `M()` 再打印 `operator=(int)`，列表里写 `m(1)` 只调用一次 `M(int)`。不能默认初始化的成员和基类，如果没有默认成员初始化器，就必须写在列表里：引用成员、没有初始化器的 `const int` 这类 `const` 标量成员、没有默认构造函数的成员和基类。`const` 的类类型成员不一定要写，只要它的类型是 const-default-constructible 的，就能默认初始化：有用户提供的默认构造函数；或者每个成员都有默认成员初始化器、或本身又是 const-default-constructible 的类类型，每个基类也是，数组看元素类型 [[dcl.init.general]/7]（g++、clang 实测：成员是“有用户提供的默认构造函数的类”的类可以，成员是只有一个 `int y;` 的类不行）。成员既有默认成员初始化器、又出现在列表里时，用列表的，忽略默认成员初始化器。
- 委托构造：类名自己出现在初始化列表里时，列表里只能有这一项；目标构造函数先执行，然后才执行委托构造函数的函数体；不能递归 [Constructors 页的 Delegating constructor]。

#### 第 4 问：构造与销毁的顺序

- 构造的顺序，和初始化列表里的书写顺序无关：1. 虚基类（只在最派生类的构造函数里初始化，按深度优先、从左到右）；2. 直接基类，按基类列表里从左到右；3. 非静态成员，按在类定义里的声明顺序；4. 构造函数体 [Constructors 页的 Initialization order]。
- 销毁相反：先执行析构函数体，再按声明顺序的逆序销毁成员，再按逆序销毁基类。标准写明，规定按声明顺序初始化，就是为了让销毁顺序恰好是初始化顺序的逆序 [[class.base.init]、[class.dtor]]；基类子对象和成员的生命周期也按这个顺序开始和结束 [Lifetime 页]。
- 例子：cppreference 的 `Class(double a) : y(a + 1), x(y)`，`x` 先于 `y` 初始化，`x(y)` 读到的是 `y` 的不确定值 [Constructors 页的 Example]。

产出：上面的表和要点。读完这一节，第二步结束，进入第三步。

## 第三步：动手（0.75h）

做下面三个实验（实验 3 很短），每个先写预测再跑。跑完之后，再去读文末的“跑完之后再读”。

产出：三份预测，三份实际输出。

### 实验 1：构造与销毁的顺序

要写的程序：

- 一个会打印的 `Tracer`：构造、析构各打印一行，带名字；一个同样会打印的基类 `Base`。
- 派生类 `D`：两个 `Tracer` 成员，**声明顺序是 `a`、`b`，构造函数的初始化列表却写成 `b`、`a`**；构造函数体和析构函数体各打印一行。
- 一个全局的 `Tracer`；一个函数，里面有静态局部 `Tracer`，`main` 里调用它两次；一个块作用域里的 `D`；一个 `new` 出来再 `delete` 的 `Tracer`。

先预测（写在代码旁边的注释里）：

1. `D` 构造时，`Base`、`a`、`b`、构造函数体谁先谁后？销毁的顺序呢？
2. 编译器会不会提醒你初始化列表的顺序有问题？
3. 全局对象在 `main` 之前还是之后构造？静态局部对象呢？函数调用两次，它构造几次？
4. `main` 结束后，全局对象和静态局部对象谁先销毁？

```bash
g++ -std=c++11 -Wall -Wextra exp1.cpp -o exp1 && ./exp1
```

它验证：成员按声明顺序初始化，和初始化列表的书写顺序无关；销毁与构造相反；静态存储期对象的构造与销毁时机。

### 实验 2：几种初始化各得到什么

要写的程序：

- `struct P { int x; };`、`struct R { int x = 7; int y; };`，一个静态 `int`，一个静态 `P`。
- 依次做 `new P()`、`new P{}`、`R r{}`、`new R`、`new R()`，打印**能读的**成员。

**注意**：默认初始化的内置类型成员，值是不确定的，读它是未定义行为。`new R` 的 `y`、`new P` 的 `x` 都不要打印。

只做这五种，不要为了覆盖更多情形再加测试。

先预测：每一行打印什么？为什么 `new R` 只能打印 `x`？分别用 `-std=c++11` 和 `-std=c++17` 编译，输出一样吗？

它验证：静态存储期的对象先被零初始化；`()` 和 `{}` 都把没有初始化器的成员清零（`()` 是值初始化，`{}` 对聚合是聚合初始化）；有默认成员初始化器的成员总是用它。

### 实验 3：作用域结束，对象会怎样

要写的程序：

- 同样的会打印的 `Tracer`，一个全局的 `Tracer*`。
- `main` 里一个块：块内依次是一个自动 `Tracer`、一个 `new` 出来并把地址存进全局指针的 `Tracer`、一个静态局部 `Tracer`，块的末尾打印一行。
- 块后面打印一行，再通过全局指针读出那个动态对象的名字，最后 `delete` 它。

先预测：块结束时，三个对象里谁被销毁了？块后面读全局指针，安全吗？静态局部对象的名字已经不可见了，它在什么时候销毁？

它验证：作用域结束只影响名字，自动对象的生命周期跟着作用域结束，动态对象和静态局部对象不跟。

### 复用：对象与存储的时间差

placement new 里"存储先于对象存在、晚于对象结束"，new-delete 的 Example（`test_new_delete.cpp`）已经在验证。写 How 时直接引用那个 Example，不另写实验。

## 第四步：写（1h）

### 先写笔记初稿

1. How：对象的时间线 → 初始化的几种形式（各配一个小例子，并与赋值对照）→ 构造与销毁的顺序规则（用实验 1 的输出）→ 存储期表，加一小段作用域与生命周期的区别（用实验 3）。
2. Relation：链到 RAII How ii、new-delete、下一篇。
3. Example：用实验 1，Verifies 对应三条结论。

按第二步的名词分级取舍：A 级要写到，B 级一句话加一个例子，C 级留 `[C]`。

产出：笔记初稿。

### 写完初稿后：对照书（不占读的时间）

- C++ Primer 5e：2.2.1（初始化、列表初始化、默认初始化）、7.5.1（构造函数初始值列表，含成员初始化顺序）。
- *Effective Modern C++* Item 7：`()` 与 `{}` 的区别——窄化转换、最令人烦恼的解析、`initializer_list` 构造函数优先。
- C++ Core Guidelines：C.41、C.45、C.47、C.48、C.49、ES.20、ES.23，每条读标题和 Reason。

### 写完初稿后：对照你自己的旧笔记（不先读）

| 位置 | 看哪里 | 对应的名词 | 看之前要知道 |
|---|---|---|---|
| [lifetime-and-initialization.md](lifetime-and-initialization.md) | 全文 | 初始化、赋值、生命周期、作用域 | 这是你第一步的成果，不是资料；写完初稿后，用第二步填好的表核对其中存储期、赋值两句 |
| [chapter18/README.md](../../My_PPP/chapter18/README.md) | 只看“列表初始化（List-initialization）判定顺序”一节 | 列表初始化、聚合、窄化、`initializer_list` 构造函数优先 | “聚合 ⟺ 没有用户声明的构造函数”只对 C++20 成立，C++11 到 C++17 里 `P() = default;` 的类仍是聚合 |
| 同上 | 第 2 节 `initializer_list` 底层数组的寿命、第 4 节临时对象的销毁 | 临时对象、生命周期 | 可选；“内存段”那几段是实现细节，本篇不用 |
| OneNote：C++ Primer 的 `Chapter 2` → `2.2 Variables` | 只看“Initialization is not assignment”和 Default / Value / List 的分类 | 初始化与赋值、默认 / 值 / 列表初始化 | 其中“Default initialization 归在 Direct 下”不对；“Value initialization MUST use the direct form”只适用于 `vector<int> v(10)` 这种只给大小的写法，不能当定义 |
| OneNote：C++ Primer 的 `Chapter 13` → Copy initialization；Professional 的 `18. Direct and Copy initialization` | 先不看 | 拷贝初始化 | 留到写 ②；里面“继承自 T 的纯右值也被省略”不对，C++17 的保证只针对同一个类的纯右值 |
| [RAII.md](../03-memory-resource/RAII.md) | How ii | 构造完成、构造失败时的销毁 | 本篇只链接，不重写 |
| [new-delete.md](../03-memory-resource/new-delete.md) | 讲“对象与存储”的那一段 | 对象与存储 | 同上 |

OneNote 三页的原文在文末的附录里。

## 第五步：封板（0.25h）

对照下面的四条合格线自检；周五审一次，只修 P0/P1；commit。

- 能解释：不看资料，用两分钟讲清"生命周期、存储期和作用域各管什么"，以及"一个派生类对象是怎样按顺序构造出来的"。
- 有模型：对象的时间线；那张"存储期 × 构造时机 × 销毁时机"的表，加一条顺序规则：基类 → 成员（声明顺序）→ 构造函数体，销毁时相反；一句话说明初始化与赋值的区别。
- 有联系：Relation 链到 RAII How ii（构造失败）、new-delete（存储与对象）和下一篇；不重复它们的内容。
- 能验证：实验 1 的输出贴进 Example，Verifies 每条对应一个结论；作用域那一段用实验 3 的输出支撑。

本篇另加一条，分了级：A 级每个名词都能不看资料讲清；B 级每个能写一句话加一个例子；C 级只在笔记里留 `[C]` 标记，不展开。

## 备查（不读）

- 标准：[basic.life](https://eel.is/c++draft/basic.life)、[basic.stc](https://eel.is/c++draft/basic.stc)、[dcl.init](https://eel.is/c++draft/dcl.init)、[class.base.init](https://eel.is/c++draft/class.base.init)。
- Nicolai Josuttis，*The Nightmare of Initialization in C++*（CppCon 2018）。
- Stroustrup，*A Tour of C++*（第 3 版）的 Scope and Lifetime 一节（章节号以手上的书为准）：以后想补全景时读，本篇不用。
- C++ FAQ 里的 "static initialization order fiasco"：跨翻译单元的静态初始化顺序没有保证。

## 跑完之后再读

以下是 g++ 11.4、`-std=c++11 -Wall -Wextra` 的实际结果，先自己跑、先写预测，再对照。

### 实验 1 的结果

编译器的提醒：

```text
warning: ‘D::b’ will be initialized after [-Wreorder]
warning:   ‘Tracer D::a’ [-Wreorder]
warning:   when initialized here [-Wreorder]
```

运行输出：

```text
  ctor global
main start
scope: D
  Base ctor
  ctor a
  ctor b
  D body
  D dtor body
  dtor b
  dtor a
  Base dtor
  ctor static local
  f: after static local
  f: after static local
  ctor dynamic
  dtor dynamic
main end
  dtor static local
  dtor global
```

怎么读：

- `ctor global` 出现在 `main start` 之前：全局对象在 `main` 之前构造。
- `D` 的构造顺序是基类 → `a` → `b` → 构造函数体，初始化列表写的是 `b`、`a`，编译器用 `-Wreorder` 提醒这两者不一致；销毁完全相反。
- 静态局部对象在第一次调用 `f` 时构造，只构造一次。
- `main` 返回之后，销毁顺序是构造完成顺序的逆序：静态局部对象先于全局对象。

### 实验 2 的结果

`-std=c++11` 与 `-std=c++17` 输出相同：

```text
static int / static P.x: 0 0
new P(): x=0   new P{}: x=0
R r1{}:  x=7 y=0
new R:   x=7 (y indeterminate, not read)
new R(): x=7 y=0
```

怎么读：

- 静态存储期的对象在任何其他初始化之前先被零初始化，所以是 0。
- `new P()` 是值初始化，`P` 没有用户提供的构造函数，所以成员被清零；`new P{}` 因为 `P` 是聚合，走聚合初始化，`x` 从空列表初始化，也是 0。
- `R` 的 `x` 有默认成员初始化器，三种写法下都是 7；`y` 没有：`new R()`（值初始化）和 `R r1{}` 都把它清零，`R r1{}` 在 C++14 起是聚合初始化，在 C++11 里 `R` 不是聚合，走值初始化，所以两个标准下输出相同；`new R`（默认初始化）不管它，所以只有这一行不能读 `y`。

### 实验 3 的结果

```text
enter block
  ctor auto
  ctor dynamic
  ctor static
leave block
  dtor auto
after block
  dynamic object still alive: dynamic
  dtor dynamic
main end
  dtor static
```

怎么读：

- 块一结束，只有 `auto` 被销毁，`dynamic` 和 `static` 都还活着，名字也不需要可见：`dynamic` 是通过全局指针读到的，`static` 的名字出了块就不可见，它一直活到 `main` 结束之后。
- 作用域结束 ≠ 生命周期结束。只有自动存储期的对象，两者才同时发生。
- 这也是 RAII 把资源交给自动对象的原因：离开作用域时销毁是确定的，不需要靠记得 `delete`。

## 附录：OneNote 原文（离线备份）

三页按你贴给我的原文原样放在下面，没有改动。每页前面的“已知错处”是我之前核对时标出的，带 ❌ 的才是错，其余是需要标版本。

### C++ Primer 的 Chapter 2 → 2.2 Variables

已知错处：

- ❌ 3.a 里 “Default initialization” 被归在 “Direct initialization” 的 Type 下。标准里 `T t;` 是单独的默认初始化，既不是直接初始化也不是拷贝初始化；直接初始化指 `T t(x)` 和 `T t{x}`。
- ❌ “Value initialization … MUST use the direct form of initialization” 只适用于只给大小的 `vector<int> ivec(10)`，不能当值初始化的定义。值初始化还包括 `T()`、`T t{}`、`new T()`。
- 版本：Copy initialization 的 “copy constructor is needed too” 只在 C++17 之前对纯右值成立。
- `5. Lifetime:(Added later)` 是空的。

```text
Variable
    1. What
         A variable provides us with named storage that our programs can manipulate. Each variable in C++ has a type. The type determines the size and layout of the variable’s memory, the range of values that can be stored within that memory, and the set of operations that can be applied to the variable
    2. Definitions && Declarations
        a. Why need declaration?
            i. C++ is a statically typed language, which means that types are checked at compile time. The complier need to know the type of variable.
        b. What is difference?
            i. A file that wants to use a name defined elsewhere includes a declaration for that name. A definition creates the associated entity.
        c. How to use or distinguish? 
            i. We use extern key word to indicate this is a declaration not a definition, e.g. extern int j;
            ii. A definition can be used as a declaration, but a declaration may not be a definition, e.g. extern int i = value is a definition but extern int j is not.
            (Note: It is an error to provide an initializer on an extern inside a function)
    3. Initialization && Assignment
        Initialization is not assignment. Initialization happens when a variable is given a value when it is created. Assignment obliterates an object’s current value and replaces that value with a new one
        a. Initialization kinds
          according to initializer value form
            i. Default initialization, when we define a variable without an initializer, the variable is default initialized.
            ii. Value initialization, we can usually omit the value and supply only a size. In this case the library creates a value-initialized element initializer for us. 
                                                MUST use the direct form of initialization.
                e.g.    vector<int> ivec(10); // ten elements, each initialized to 0 
                    vector<strng> svec(10); // ten elements, each an empty string 
            iii. List initialization
          according to initialization form
            i. Copy initialization, using = operator
                1) Mechanism: Copy initialization constructs an implicit conversion sequence: It tries to convert x to an object of type T. It then may copy over that object into the to-initialized object, so a copy constructor is needed too.
            ii. Direct initialization, forms that do not using = operator
                1) Mechanism: Direct initialization behaves like a function call to an overloaded function: The functions, in this case, are the constructors.
                2) Type: 
                    a) Default initialization
                    b) ()  initialization
                    c) {} list initialization
                    
    4. Scope:
        a. What is scope?
            i. The valid area in program that object can work.
        b. Scope kinds
            i. Global scope --- The names defined outside a function—has global scope. Once declared, names at the global scope are accessible throughout the program.
            ii. Block scope --- The names defined inside a block.
    5. Lifetime:(Added later)
Extern
```

### Professional 的 18. Direct and Copy initialization

已知错处（版本问题，不是 C++17 之后的错）：

- “the copy constructor is needed too” 和 “copy constructor is deleted or not public → copy initialization is not allowed”，只在 C++17 之前对纯右值成立。C++17 之后，同一个类的纯右值不再需要可用的拷贝或移动构造。
- “the compiler will help to optimize”：C++17 之前是允许省略，之后对纯右值是保证。

```text
The deep difference between direct and copy initialization:
While they look identical and are supposed to do the same, these two forms are remarkably different in certain cases. The two forms of initialization are direct and copy initialization:
T t(x);
T t = x;
There is behavior we can attribute to each of them:
    • Direct initialization behaves like a function call to an overloaded function: The functions, in this case, are the constructors of T (including explicit ones), and the argument is x. Overload resolution will find the best matching constructor, and when needed will do any implicit conversion required.
    • Copy initialization constructs an implicit conversion sequence: It tries to convert x to an object of type T. It then may copy over that object into the to-initialized object, so a copy constructor is needed too.
    
As you see, copy initialization is in some way a part of direct initialization with regard to possible implicit conversions: While direct initialization has all constructors available to call, and in addition can do any implicit conversion it needs to match up argument types, copy initialization can just set up one implicit conversion sequence.

Note: While copy initialization is used, the complier will help to optimize which leading to not call copy constructor. Once copy constructor is deleted or is declared as not public, the copy initialization is not allowed. And another situation that copy initialization is forbidden is that the corresponding  constructor called by copy initialization is explicit.

Reference: https://stackoverflow.com/questions/1051379/is-there-a-difference-between-copy-initialization-and-direct-initialization
         https://blog.csdn.net/ljianhui/article/details/9245661
```

### C++ Primer 的 Chapter 13 → Copy initialization

已知错处：

- ❌ 第 5 点的问题：`string s = "lit";` 在 C++17 里等价于直接调用单参构造，你的直觉对；但 C++17 之前是“先转换出临时对象，再拷贝或移动构造”，通常被省略，所以拷贝或移动构造函数必须可用。我验证过：带 deleted 拷贝和移动构造的类，`S s = "x";` 在 C++17 通过，在 C++14 报 `use of deleted function ‘S::S(S&&)’`。
- ❌ “只有声明为 explicit 时才会调用单参构造再调用拷贝或移动构造”不对。explicit 时 `E e = "x";` 直接编译失败；`string("...")` 是显式写出的 T 类型纯右值，和 explicit 无关。
- ❌ 答案 A 里 “other 是 T 类型或继承自 T 类型的纯右值，则直接构造在 t 里”，“继承自 T”是错的。`T t = D();`（D 继承自 T）在 C++14 和 C++17 都调用移动构造（切片），不省略；C++17 的保证只针对同一个类的纯右值。
- 第 3 点 d、e 是 Primer 的原话（按 C++11 算对）；e 里 push 和 insert 是拷贝，emplace 系列是直接初始化。

```text
 Copy initialization：
    1. When we use copy initialization, we are asking the compiler to copy the right-hand operand into the object being created, converting that operand if necessary
    2. Copy initialization ordinarily uses the copy constructor. If a class has a move constructor, then copy initialization sometimes uses the move constructor instead of the copy constructor.
    3. When copy initialization happens 
        a. Define variables using an =
        b. Pass an object as an argument to a parameter of nonreference type 
        c. Return an object from a function that has a nonreference return type 
        d. Brace initialize the elements in an array or the members of an aggregate class
        e. Library containers copy initialize their elements when we initialize the container, or when we call an insert or push member
    4. During copy initialization, the compiler is permitted (but not obligated) to skip the copy/move constructor and create the object directly.
        a. e.g. 
        
        
    5. Q：到底是一个怎么的过程？除去复制省略， 在4.a中， string null_book =" 9-999-9999-9", 按照单参数构造函数来说，应该是直接调用
        该单参构造函数，而不用再调用拷贝或者是移动构造函数。只有在单参构造函数声明为explicit 时候，例如 string null_book = string("9-999-9999-9"),
        理论上来说会笑调用该单参构造函数，然后再调用拷贝或者移动构造函数。需要查明资料
A: 在C++17之后，标准有复制省略的规定， 即形如 T t = others的复制初始化语句，如果other是一个T类型或者继承自T类型的纯右值表达式，则other直接被构造在t的内存空间里。在C++ 17之前，如果others是T类型或者继承自T类型的值，则调用对应的构造函数；如果other是和T无关的类型，则调用用户自定义的转换函数把other变成T类型的临时对象，然后使用该临时对象direct initialization t，但是最后一步常常由于编译器优化被省略掉，即临时对象直接构造在t的内存空间，不用再复制一次（不用 direct initialization）.
```
