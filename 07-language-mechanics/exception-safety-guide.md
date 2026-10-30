# 异常安全：学习指南

> 主线第 4/4 篇。排期：W2 第 1 篇，4h，深档（正文 ≤ 200 行）。现有草稿：[exception-safety-draft.md](exception-safety-draft.md)，AI 整理的、结构完整的草稿（Why / What / How / Relation / Example，带三个已跑通的实验），还没有经过你的重写。这份指南要做三件事：让你用自己的话重写 Why / What，把 How 压短，并把前三篇接起来。

## 在主线上的位置

[RAII 笔记](../03-memory-resource/RAII_summary.md) → [① 构造](../02-class-object-oop/lifetime-and-initialization-guide.md) → [② 移动](../03-memory-resource/copy-and-move-guide.md) → [③ 智能指针](../03-memory-resource/smart-pointers-guide.md) → **④ 异常安全（本篇）**

### 承接：前三篇各自解决了一半

前三篇都围绕同一个问题：操作被异常打断时，对象和资源怎么办。它们各解决了一部分，这张表就是本篇 Relation 的骨架：

| 篇 | 给异常安全提供了什么 | 落在保证的哪里 |
|---|---|---|
| ① 构造 | 构造失败时只销毁已经构造完的子对象，不留半个对象；成员用 RAII 类型而不是裸指针，已经构造的那几个才会被清理 | 基本保证的第一半（不泄漏），构造那一段 |
| ② 移动 | `noexcept` 的移动和 `swap` 是"提交"这一步的材料；`move_if_noexcept` 决定 `vector` 扩容走搬还是走拷 | 强保证的前提 |
| ③ 智能指针 | 持有者：栈展开时自动释放；`make_*` 去掉 `new` 与接管之间的窗口 | 基本保证的第一半（不泄漏），持有那一段 |
| ④ 异常安全（本篇） | 把上面三样放进分级的承诺：基本、强、不抛，再加"先准备、后提交"的做法 | 基本保证的第二半（不变量），以及强、不抛 |

三篇加起来只解决了"不漏"和"不留半个对象"，没有回答：一个操作做到一半被异常打断，对象还能不能用、值是什么？草稿的 Why 用 `Image::resize` 讲了这件事：`vector` 管得很好，什么也没漏，`Image` 的不变量却坏了。**资源被清理，不等于状态是对的。** 这就是本篇。

### 引出：主线到此结束

这四篇合起来，是"一个类怎样持有资源"的一整条线：对象怎样出生（①）→ 持有者怎样被拷贝和移动（②）→ 标准库给了哪些现成的持有者（③）→ 失败时它们承诺什么（④）。W3 的容器笔记会直接用到这里的分级：去读 `vector::push_back`、`insert` 在 cppreference 上的 Exceptions 一节，问每个操作给的是哪一级保证。

## 读完应能回答

1. 抛异常之后，除了"不泄漏"，还要保证什么？基本、强、不抛三个保证各是什么？
2. 一个赋值运算符或插入操作怎样做到强保证？（先准备、后提交；copy-and-swap 是其中一种做法。）
3. 为什么 `swap`、移动、析构必须不抛？这对 `vector` 扩容有什么后果？
4. 哪些事 RAII 管得了（释放），哪些管不了（状态一致性）？
5. `noexcept` 是保证本身吗？声明了 `noexcept` 却抛出，会怎样？

## 本篇不写什么

- 栈展开、构造失败、析构不抛：RAII 笔记 How ii、iii 已经写了，只放链接。
- `operator=` 的完整写法（按值形式与 `const&` 形式）：属于 [② 移动](../03-memory-resource/copy-and-move-guide.md)；这里只讲 copy-and-swap **为什么**能给出强保证。
- 异常类的层次、`catch` 的写法、函数 try 块、`std::exception_ptr`：C++ Primer 18.1 备查，不进笔记。
- 每个标准库容器、每个操作的保证表：W3 的容器篇按需查 cppreference，不在这里列。

## 资料

### 必读（第 2 步，共约 60 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| David Abrahams，*Exception-Safety in Generic Components*（Boost 网站） | 读开头到三个保证的定义为止；后面讲泛型组件如何给出保证的技术部分跳过 | 第 1、2 问 | 25 min |
| cppreference：[throw expression](https://en.cppreference.com/w/cpp/language/throw)、[noexcept specifier](https://en.cppreference.com/w/cpp/language/noexcept_spec) | throw 页的 Stack unwinding 一节；noexcept 页看"声明了却抛出会怎样" | 第 4、5 问 | 15 min |
| Scott Meyers，*Effective C++* 第 3 版 Item 29 | 全文：两个要求（不泄漏资源、不破坏数据结构）、三个保证、copy-and-swap | 第 1、2 问 | 20 min |

### 写完后对照（第 4 步，不占读的时间）

- Herb Sutter，*Exceptional C++* 的 "Exception-Safety Issues and Techniques" 一章：用 `Stack<T>` 一步步改造，最适合当主线案例。
- *Effective C++* 第 3 版 Item 25（不抛的 `swap`）、Item 8（析构不抛）；*Effective Modern C++* Item 14（`noexcept`）。
- PPP 第 19 章 "Resources and exceptions" 一节，含 Guarantees 小节（章节号请对一下你手上的书）。
- C++ Core Guidelines：E.2、E.6、E.12、E.13、E.15、E.16、E.17，以及 C.36、C.37、C.83、C.84、C.85。
- C++ Primer 5e：12.1.4（智能指针与异常）。

### 备查（不读）

- 标准：[except.ctor](https://eel.is/c++draft/except.ctor)、[except.terminate](https://eel.is/c++draft/except.terminate)、[vector.modifiers](https://eel.is/c++draft/vector.modifiers)、[vector.capacity](https://eel.is/c++draft/vector.capacity)、[class.copy.assign](https://eel.is/c++draft/class.copy.assign)。
- cppreference：[std::vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) 的 Exceptions 一节、[std::move_if_noexcept](https://en.cppreference.com/w/cpp/utility/move_if_noexcept)。
- C++ Primer 5e：18.1。

## 学习过程

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 1 先写 | 合上草稿和资料，用自己的话写 Why：一个操作被异常打断后，除了"不漏"还有什么要保证？自己想一个例子（别照抄草稿的 `Image::resize`），写出它会怎样坏掉 | 0.5 | 一小段话，加你自己的例子 |
| 2 读 | 只读上面的"必读"三项；边读边在你的例子旁边标：这里的承诺是哪一级？ | 1 | 带标注的例子 |
| 3 动手 | 实验 A：把草稿里的三个实验按"先预测再跑"过一遍（0.5h）。实验 B：对你自己的 `Vector.h` 做强保证的断言验证（0.5h，选做，可以放到机动时间） | 1 | 预测与输出；断言验证表 |
| 4 写 | 打开草稿对照：Why / What 用你自己的话重写；How 压成三段（基本 → 强 → 不抛），每段一个例子；Relation 用上面"前三篇各自解决了一半"那张表，改成你自己的话；Example 最多留两个实验。栈展开、构造失败、析构不抛只放 RAII 笔记的链接 | 1.25 | 笔记初稿 |
| 5 封板 | 把草稿里指向 `RAII-draft.md` 的链接改成 [RAII_summary.md](../03-memory-resource/RAII_summary.md)（`RAII-draft.md` 计划在 W4 清理）；四条合格线自检；周五审一次，只修 P0/P1 | 0.25 | 封板 |

## 实验

### 实验 A：草稿里的三个实验（必做）

草稿的 Example 里已经有三个确定性实验。要做的不是照抄，而是**先预测再跑**：

1. 不变量：`resize_bad` 与 `resize_good` 的内存管理完全一样，只有语句顺序不同。失败之后 `invariant()` 各是什么？
2. 半新半旧：`update_basic` 与 `update_strong`，`std::stoi` 抛出之后 `host` 和 `port` 各是什么？
3. 复制赋值：编译器生成的版本与 copy-and-swap 版本，在第 2 次元素拷贝抛出之后，目标对象的 `name` 和 `items` 各是什么？

草稿的 Example 用了 `std::is_nothrow_move_assignable_v` 和 `static inline` 成员，需要 `-std=c++17`。

### 实验 B（选做）：验证你自己 Vector.h 的强保证断言

你在 [Vector.h](../../My_PPP/chapter19/include/exer/Vector.h) 的注释里写了几处保证：`AllocGuard`（"强异常安全"）、`operator=`（copy-and-swap，"强异常保证"）、`resize`（"基本异常保证"）、`emplace`（`tmp` 构造失败时"强异常保证"）。这是对你自己代码的断言，值得用实验证实或证伪。

要写的程序：

- 元素类型 `Th`：拷贝构造函数里有一个静态计数，**第 k 次拷贝时抛异常**；再带一个 `live` 计数（构造加一、析构减一）用来查泄漏；移动构造**不标 `noexcept`**，这样 `reserve` 会走拷贝路径，抛出点才会落在搬运途中。
- 对 `KaKaRot::vector<Th>` 的下面这些操作，各让第 1、2、3 次拷贝抛异常。

要填的表：

| 操作 | 抛出点 | 你的注释声称 | 观察：`size` / `capacity` / 各元素的值 / `live` 是否回到预期 |
|---|---|---|---|
| `vector(const vector&)` | 第 k 次拷贝 | | |
| `operator=` | 第 k 次拷贝 | | |
| `reserve(n)` | 搬运途中 | | |
| `resize(n)` | 构造第 k 个新元素 | | |
| `emplace(pos, ...)` 中间插入 | 移位途中 | | |

`emplace` 的移位用的是移动构造和移动赋值，所以要让**移动**操作抛异常，才测得到那一段。

没有参考答案：这是对你自己代码的断言。拷贝构造、`operator=`、`reserve` 这三行的实验装置（`Th` 加 `Vector.h`，`-std=c++11`）已经确认能编译、能运行；`resize` 和 `emplace` 要你自己扩展。

## 封板自检

- 能解释：不看资料，用你自己的例子讲清基本、强、不抛各自的承诺；讲清"RAII 不等于异常安全""`noexcept` 不等于异常安全"。
- 有模型：一张三级保证表（异常传出之后：资源、不变量、值各是什么），加"先准备、后提交"这条做法。
- 有联系：链到 RAII How ii、iii（只引用）、② 移动（`noexcept` 移动、copy-and-swap 的写法）、③ 智能指针（持有者）；用上面那张"前三篇各自解决了一半"的表。
- 能验证：实验 A 里留两个做 Example，Verifies 每条对应一个结论。

## 跑完之后再读

实验 A 的参考输出在 [草稿的 Example 一节](exception-safety-draft.md)，是 g++ 11.4 `-std=c++17` 的实跑结果。先写预测，再去看。实验 B 没有参考答案。
