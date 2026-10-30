# 拷贝与移动：学习指南

> 主线第 2/4 篇。排期：W1 第 2 篇，6h，深档（正文 ≤ 200 行）。现有种子草稿：[copy-and-move-draft.md](copy-and-move-draft.md)（特殊成员函数的生成规则表、多态基类、移动与 `noexcept`、两条待归位的小实验）。这份指南是它的学习路径：先自己写，再对照草稿。草稿里"多态基类"（切片、虚析构、`clone`）一节留给 W4 的继承与多态，本篇不写。

## 在主线上的位置

[RAII 笔记](RAII_summary.md) → [① 构造](../02-class-object-oop/lifetime-and-initialization-guide.md) → **② 移动（本篇）** → [③ 智能指针](smart-pointers-guide.md) → [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)

### 承接：上一篇留下的问题，和 RAII 笔记欠的三件事

上一篇 [① 构造](../02-class-object-oop/lifetime-and-initialization-guide.md)的最后一个问题：用同类型的另一个对象初始化新对象，走的是拷贝构造或移动构造。

[RAII 笔记](RAII_summary.md)的 How iv 已经交代了**为什么**要管这件事：编译器生成的拷贝只复制句柄的值、不复制资源，两个对象都把同一份资源当独占的，就会双重释放（iv.1）；所以类必须自己定义所有权策略，Rule of Three / Five / Zero 就是这套策略的写法（iv.2）。它留下了三件没讲的事：编译器**什么时候**生成这些操作；移动**到底做什么**；容器在**什么条件下**采用移动。这三件事就是本篇。

你手上已经有两份现成的材料：[Unique_ptr.h](../../My_PPP/chapter19/include/exer/Unique_ptr.h)（删拷贝、留移动）和 [Vector.h](../../My_PPP/chapter19/include/exer/Vector.h)（拷贝构造、copy-and-swap 的拷贝赋值、`noexcept` 的移动构造与移动赋值、`noexcept` 的 `swap`，`reserve` 里用了 `std::move_if_noexcept`）。本篇要把这两份代码里的做法，变成能讲清"为什么这样写"的笔记。

### 引出：同一套旋钮，不同的所有权策略

Rule of Zero 说：优先用现成的 RAII 成员，类里一个特殊成员都不用写。现成的所有权策略里最典型的是两种：只允许移动的 `unique_ptr`，和拷贝即共享的 `shared_ptr`。它们就是把本篇的"拷贝做什么、移动做什么"设成了不同的答案。这是下一篇 [③ 智能指针](smart-pointers-guide.md)的问题。另外，`noexcept` 的移动和 copy-and-swap 会在 [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)里回收。

## 读完应能回答

1. 编译器什么时候替你生成拷贝、移动操作，什么时候不生成？声明了析构函数、拷贝操作或移动操作之后，另外几个各自怎么变？
2. `std::move` 做了什么？移动之后的源对象处于什么状态，标准库对它的最低要求是什么？
3. 为什么移动构造要标 `noexcept`？`vector` 扩容在什么条件下搬、什么条件下拷？
4. 返回局部对象、传 prvalue 时，什么时候根本没有拷贝或移动？C++17 前后有什么不同？
5. 你在 `Vector.h` 的拷贝赋值旁边写了一条注释：用 `const&` 而不是按值传参，是为了和 `noexcept` 的移动赋值共存而不产生二义。二义出在哪里？
6. 给你一个管理资源的类，Rule of Five 的五个函数各写什么？什么时候 Rule of Zero 就够了？

## 本篇不写什么

- 多态基类（切片、虚析构、`clone`）：草稿里这一节留给 W4 的继承与多态。
- 值类别的完整定义（prvalue、xvalue、glvalue）和转发引用：W2 的"值类别与转发"。这里只要最小集：左值、右值，以及 `std::move` 把左值转成右值引用。
- 草稿 Note 里的两条小实验（返回值对象与会抛异常的析构、虚基类的析构）：不属于本篇主线。
- copy-and-swap **为什么**能给出强保证：异常安全篇。但 `operator=` **怎么写**（按值形式与 `const&` 形式）归本篇。

## 资料

### 必读（第 2 步，共约 90 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| cppreference：[Copy constructor](https://en.cppreference.com/w/cpp/language/copy_constructor)、[Move constructor](https://en.cppreference.com/w/cpp/language/move_constructor) | 各自的 Implicitly-declared 与 Deleted implicitly-declared 两节 | 第 1 问 | 15 min |
| cppreference：[The rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three) | 全页 | 第 1、6 问 | 10 min |
| cppreference：[std::move](https://en.cppreference.com/w/cpp/utility/move)、[std::move_if_noexcept](https://en.cppreference.com/w/cpp/utility/move_if_noexcept) | 页首说明和 Notes | 第 2、3 问 | 10 min |
| cppreference：[Copy elision](https://en.cppreference.com/w/cpp/language/copy_elision) | Mandatory elision（C++17 起）与 Non-mandatory elision 两节 | 第 4 问 | 15 min |
| *Effective Modern C++* Item 17、23 | 17：特殊成员函数的生成；23：`std::move` 与 `std::forward` 本质上是转换 | 第 1、2 问 | 25 min |
| C++ Core Guidelines：C.20、C.21、C.22、C.64、C.66 | 标题和 Reason | 第 3、6 问 | 15 min |

### 写完后对照（第 4 步，不占读的时间）

- C++ Primer 5e：13.1–13.6（拷贝控制、资源管理类、swap、移动对象）。
- PPP 第 18 章讲拷贝与移动的 18.2（章节号请对一下你手上的书）。
- *Effective Modern C++* Item 14、29：`noexcept`；别假设移动存在、便宜、被用到。
- C++ Core Guidelines：C.65、F.18、F.20、F.48。

### 备查（不读）

- Howard Hinnant，*Everything You Ever Wanted To Know About Move Semantics*（ACCU 2014，约 1 小时，想看就看前半讲设计动机的部分）。
- Nicolai Josuttis，*The Nightmare of Move Semantics for Trivial Classes*（CppCon 2017）。
- 提案 N2027，*A Brief Introduction to Rvalue References*。
- 标准：[special](https://eel.is/c++draft/special)、[class.copy.ctor](https://eel.is/c++draft/class.copy.ctor)、[class.copy.assign](https://eel.is/c++draft/class.copy.assign)、[class.copy.elision](https://eel.is/c++draft/class.copy.elision)。

## 学习过程

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 1 先写 | 合上资料和草稿，用自己的话写 Why / What：拷贝和移动各自对资源做什么？你认为 `std::move` 做了什么？读完第 2 步回头对照，看哪句错了 | 0.5 | 一小段话，加一个"我认为"清单 |
| 2 读 | 只读上面的"必读"六项；边读边把"你声明了什么 × 拷贝/移动各自还生成吗"那张表的空格填出来 | 1.5 | 填了一半的规则表 |
| 3 动手 | 做实验 1–3（实验 4 选做），每个先写预测再跑；实验 1 直接用你自己的 `Vector.h`，只需写会计数的元素类型 | 1.5 | 三份预测，三份实际输出 |
| 4 写 | How：规则表（实验 3）→ 拷贝与移动各做什么，用你的 `Unique_ptr` 和 `Vector` 当例子 → `std::move` 是转换，加 moved-from 状态 → `noexcept` 与 `move_if_noexcept`（实验 1）→ 拷贝省略（实验 2）。Relation：链到 RAII How iv、上一篇、下一篇。Example：实验 1，实验 2、3 的结果写进 Verifies | 2 | 笔记初稿 |
| 5 对照草稿，封板 | 打开 [copy-and-move-draft.md](copy-and-move-draft.md)，只核对：规则表是否一致、"`= delete` 的移动构造不会回退到拷贝"这条写到了没有；"多态基类"和 Note 两条不写。四条合格线自检；周五审一次，只修 P0/P1 | 0.5 | 封板 |

## 实验

### 实验 1：reserve 时搬还是拷（用你自己的 vector）

要写的程序：

- 三个元素类型，拷贝构造和移动构造都带计数：A（移动构造 `noexcept`，可拷贝）、B（移动构造不标 `noexcept`，可拷贝）、C（不可拷贝，移动构造不标 `noexcept`）；两个全局计数器 `copies`、`moves`。
- 对 `KaKaRot::vector<A/B/C>` 和 `std::vector<A/B/C>` 各做一遍：先 `push_back` 8 个元素，把计数器清零，再调用 `reserve(16)`，打印计数。

**用 `reserve`，不要继续 `push_back`**：你的 `push_back(T&&)` 在容量用完时，会先把参数移动到一个临时对象，扩容后再移动一次（为了避免参数引用的正是容器里的元素），这两次会混进计数。

```bash
g++ -std=c++11 -Wall -Wextra -I ~/personal/CPlusPlus/My_PPP/chapter19/include exp1.cpp -o exp1 && ./exp1
```

程序里写 `#include "exer/Vector.h"`。`Vector.h` 用了 `alloc.construct` / `alloc.destroy`，只能在 C++11、14、17 下编译，它自己的注释里写了。

先预测：六行各是几次 copy、几次 move？你的 vector 和 `std::vector` 一致吗？把 B 的移动构造改成 `noexcept`，结果怎么变？

它验证：`std::move_if_noexcept` 的选择规则，和 `noexcept` 在容器扩容时的实际意义。

### 实验 2：返回时发生了什么

要写的程序：

- 一个类 `T`，默认构造、拷贝构造、移动构造各打印一行。
- 三个函数：`f1` 返回具名局部对象（`T t; return t;`），`f2` 返回 `std::move(t)`，`f3` 返回 `T()`；`main` 里分别用它们初始化一个 `T`。

编译四次：`-std=c++11`、`-std=c++17`，以及各自再加 `-fno-elide-constructors`；其中不带 `-fno-elide-constructors` 的两次加上 `-Wall`，看编译器对 `f2` 说了什么。

先预测：四种配置下，每个函数各打印几次 copy、几次 move？哪些结果在 `-fno-elide-constructors` 下也不变？

它验证：具名局部对象的省略（NRVO）是允许但不保证的；C++17 起 prvalue 的省略是保证的；`return std::move(t)` 反而阻止了省略（Core Guidelines F.48）。

### 实验 3：声明了什么，另外几个怎么变

要写的程序：

- 一个类 `M`，拷贝构造和移动构造各打印一行。
- 四个类，各含一个 `M` 成员：S0 什么都没声明；S1 只声明析构函数；S2 只声明拷贝构造函数；S3 只声明移动构造函数。
- 对每个类写 `S b = std::move(a);`，看 `M` 的哪个构造被调用；对 S3 再试 `S3 c = a;`，看编译器怎么说。

先预测：S0–S3 各打印 `M copy` 还是 `M move`？S3 的拷贝能不能编译？

它验证：草稿里那张规则表——声明析构函数之后移动不再生成，静默退化为拷贝；声明了移动操作，拷贝操作被删除。

### 实验 4（选做，用机动时间）：按值的赋值和移动赋值同时存在

要写的程序：一个类，同时定义按值传参的 `operator=(T rhs)` 和 `operator=(T&& rhs) noexcept`，写 `a = std::move(b);`，看编译器怎么说。再把单独的移动赋值删掉，只留按值版本，分别对 `a = std::move(b);` 和 `a = b;` 打印它走了哪个构造。

它验证：你在 `Vector.h` 里那条注释，也就是第 5 问。

## 封板自检

- 能解释：不看资料，说出"声明了析构函数之后移动会怎样"和"`return std::move(local)` 为什么反而更差"。
- 有模型：一张规则表（你声明了什么 × 拷贝/移动各自还生成吗），加一句话：拷贝复制资源，移动转移所有权，`std::move` 只是转换。
- 有联系：链到 RAII How iv、上一篇的"用另一个对象初始化"、下一篇的智能指针；不重复 Rule of Three / Five / Zero 的概述，从"生成规则"讲起。
- 能验证：实验 1 做 Example，实验 2、3 的结果作为 Verifies。

## 跑完之后再读

以下是 g++ 11.4 的实际结果，先自己跑、先写预测，再对照。

### 实验 1 的结果

`-std=c++11 -Wall -Wextra`：

```text
A: move noexcept, copyable | B: move may throw, copyable | C: move may throw, move-only
reserve(16) after 8 elements:
  KaKaRot::vector<A>   copies=0 moves=8
  KaKaRot::vector<B>   copies=8 moves=0
  KaKaRot::vector<C>   copies=0 moves=8
  std::vector<A>       copies=0 moves=8
  std::vector<B>       copies=8 moves=0
  std::vector<C>       copies=0 moves=8
```

怎么读：

- 只有 B 走了拷贝：它可拷贝，而移动构造可能抛，所以 `move_if_noexcept` 选了拷贝。
- C 的移动构造也可能抛，但它不可拷贝，没有别的路可走，只能搬。这时强保证被放弃，是异常安全篇的内容。
- 你的 vector 和 `std::vector` 的选择完全一致。

### 实验 2 的结果

| 配置 | `f1`：`return t` | `f2`：`return std::move(t)` | `f3`：`return T()` |
|---|---|---|---|
| `-std=c++11` | 无 copy/move | 1 次 move | 无 copy/move |
| `-std=c++17` | 无 copy/move | 1 次 move | 无 copy/move |
| `-std=c++11 -fno-elide-constructors` | 2 次 move | 2 次 move | 2 次 move |
| `-std=c++17 -fno-elide-constructors` | 1 次 move | 1 次 move | 无 copy/move |

加 `-Wall` 时，`f2` 会收到 `moving a local object in a return statement prevents copy elision [-Wpessimizing-move]`。

怎么读：

- 默认配置下 `f1` 被省略掉了，但这是允许、不是保证：关掉省略，`f1` 立刻多出 move。
- `f2` 手写的 `std::move` 把返回值变成右值，反而挡住了省略，比 `f1` 多一次 move。
- `f3` 在 C++17 下即使关掉省略也没有 copy/move：prvalue 直接初始化目标对象，这是语言规则，不是优化。

### 实验 3 的结果

```text
  S0 (nothing declared), S0 b = std::move(a):
    M move
  S1 (destructor declared):
    M copy
  S2 (copy constructor declared):
    M copy
  S3 (move constructor declared), S3 b = std::move(a):
    M move
```

`S3 c = a;` 编译失败：`error: use of deleted function ‘S3::S3(const S3&)’`。

怎么读：S1 和 S2 都没有隐式的移动构造，`std::move(a)` 退回到拷贝构造，能编译、行为对，只是慢；S3 声明了移动构造，拷贝构造被隐式删除。

### 实验 4 的结果

按值版和 `T&&` 版同时存在：

```text
error: ambiguous overload for ‘operator=’
```

只留按值版本：

```text
  a = std::move(b):
    move ctor
    operator=(T)
  a = b:
    copy ctor
    operator=(T)
```

怎么读：按值形式让一个函数同时服务拷贝和移动（参数由拷贝构造或移动构造得到），不需要单独的移动赋值；一旦再写一个 `T&&` 版本，`a = std::move(b)` 对两个重载一样好，就二义了。你的 `Vector.h` 为了保留 `noexcept` 的移动赋值，选了 `const&` 形式。
