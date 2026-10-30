# 智能指针：学习指南

> 主线第 3/4 篇。排期：W1 第 3 篇，6h，深档（正文 ≤ 200 行）。现有种子草稿：[smart-pointers-draft.md](smart-pointers-draft.md)（`unique_ptr` 与删除器、`make_unique` / `make_shared`、`shared_ptr` 与控制块、`weak_ptr`）。这份指南是它的学习路径：先自己写，再对照草稿。草稿里删除器的细节（删除器抛异常、`sizeof` 的实现观察、`const noexcept` 的写法）只作备查，不进笔记正文。

## 在主线上的位置

[RAII 笔记](RAII_summary.md) → [① 构造](../02-class-object-oop/lifetime-and-initialization-guide.md) → [② 移动](copy-and-move-guide.md) → **③ 智能指针（本篇）** → [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)

### 承接：三个旋钮，三种所有权策略

上一篇 [② 移动](copy-and-move-guide.md)留下的是三个旋钮：拷贝做什么、移动做什么、要不要删掉。旋钮设成不同的值，就是不同的所有权策略。你的 [Unique_ptr.h](../../My_PPP/chapter19/include/exer/Unique_ptr.h) 是其中一种：删拷贝、留移动。标准库给了三个现成的答案，本篇要把它们摆在一张表里比较。

[RAII 笔记](RAII_summary.md)的 How v 已经列出 `unique_ptr`（独占，只能移动）和 `shared_ptr`（共享，引用计数），还给了 `unique_ptr<FILE, Closer>` 这个自定义删除器的例子；How v 的末尾和 Relation 的 See also 都把细节指向了智能指针这一篇。本篇展开那两行，并补上第三种：不拥有的 `weak_ptr`。

### 引出：智能指针管"持有"，管不了"中途失败"

智能指针保证的是**持有者**：对象一旦被它接管，栈展开时就会被释放。但它回答不了两类问题。一类很窄：`new` 和接管之间有没有窗口——草稿里的 `f(shared_ptr<A>(new A), g())`，在 C++17 之前，`g()` 抛异常会泄漏。另一类更宽：做到一半的操作失败了，对象的状态还对不对。这两类都是"中途失败"的问题，交给 [④ 异常安全](../07-language-mechanics/exception-safety-guide.md)。

## 读完应能回答

1. 裸指针表达不了什么？三种智能指针各表达什么所有权，对应的拷贝、移动各怎么设置？
2. `unique_ptr` 为什么只能移动？移动之后源是什么状态？什么时候必须自定义删除器？`unique_ptr<FILE>` 能编译，运行时会发生什么？
3. `shared_ptr` 的引用计数放在哪里？`shared_ptr<T>(new T)` 与 `make_shared<T>()` 在分配次数上差几次？
4. 两个对象互持 `shared_ptr` 会怎样？`weak_ptr` 怎样破环？`lock()` 返回什么，什么时候是空？
5. 函数参数怎么传：接管所有权、共享所有权、只是使用，各用什么类型？

## 本篇不写什么

- `enable_shared_from_this`、别名构造函数、`shared_ptr` 的原子计数细节：查询资料，不进笔记。
- 草稿里删除器的边界规则（删除器抛异常是未定义行为、`const noexcept` 的写法、`sizeof` 的实现观察）：留在草稿和备查里。笔记里只写"什么时候需要自定义删除器"和 `unique_ptr<FILE, Closer>` 一个例子。
- 数组形式（`unique_ptr<T[]>`、C++20 的 `make_shared<T[]>`）：一句话带过。
- 函数参数的完整规则（R.30–R.37）：只写三种意图——接管、共享、只使用。

## 资料

### 必读（第 2 步，共约 90 分钟）

| 资料 | 读哪里 | 为了回答 | 时间 |
|---|---|---|---|
| cppreference：[unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)、[shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)、[weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr)、[make_shared](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared) | 每页的页首说明和 Notes；`shared_ptr` 页的 Implementation notes | 第 1–4 问 | 50 min |
| *Effective Modern C++* Item 18、19、20、21 | 18 独占；19 共享；20 `weak_ptr`；21 `make_*` 优于直接 `new`（Item 22 讲 Pimpl，跳过） | 第 1–4 问 | 40 min |

### 看源码（本机的 g++ 11.4，读类声明和数据成员，不读全文）

- `/usr/include/c++/11/bits/shared_ptr_base.h`：搜这四个类。`_Sp_counted_base` 是引用计数的基类；`_Sp_counted_ptr` 是 `shared_ptr<T>(T*)` 用的控制块，构造函数里 `new` 的就是它；`_Sp_counted_deleter` 在带自定义删除器时用；`_Sp_counted_ptr_inplace` 是 `make_shared` 用的，对象和计数在同一块内存里。
- `/usr/include/c++/11/bits/unique_ptr.h`：搜 `_M_t`，看指针和删除器是怎样放进同一个 `tuple` 里的。

### 写完后对照（第 4 步，不占读的时间）

- C++ Primer 5e：12.1（12.1.1–12.1.6）。其中 12.1.4 "Smart pointers and exceptions" 留给异常安全篇。
- C++ Core Guidelines：R.20、R.21、R.22、R.23、R.24、R.30、R.32、R.33、R.34、R.35、R.37。R.36 在原文里标题末尾还写着 `???`，规则没写完，先不引。
- Herb Sutter，GotW #91 *Smart Pointer Parameters*。

### 备查（不读）

- Herb Sutter，GotW #89 *Smart Pointers*；*Leak-Freedom in C++... By Default*（CppCon 2016）。
- cppreference：`enable_shared_from_this`、`shared_ptr` 的别名构造函数。

## 学习过程

| 步 | 做什么 | 小时 | 产出 |
|---|---|---|---|
| 1 先写 | 合上资料和草稿，画一张表：行是 `unique_ptr`、`shared_ptr`、`weak_ptr`，列是所有权、拷贝、移动、谁负责释放。按你现在的理解填，填不出的格子打问号。再写一句：为什么 `unique_ptr` 只能移动？ | 0.5 | 一张带问号的表 |
| 2 读 | 只读上面的"必读"两项；读 `shared_ptr` 页的 Implementation notes 时，给表加一列"控制块里有什么"；再花 10 分钟扫一眼上面两个头文件 | 1.5 | 问号变少的表 |
| 3 动手 | 做实验 1–3（实验 4 选做），每个先写预测再跑；实验 1 用你自己的 `Unique_ptr.h` | 1.5 | 三份预测，三份实际输出 |
| 4 写 | How：所有权模型表（实验 1 的数据）→ `unique_ptr`：移动语义，加删除器（`unique_ptr<FILE, Closer>` 一例）→ `shared_ptr`：控制块与 `make_shared`（实验 2）→ `weak_ptr`：环与 `lock()`（实验 3）→ 参数怎么传（三种意图）。Relation：链回 RAII How v、上一篇、下一篇。Example：实验 2 或 3 选一个，别贪多，其余结果进 Verifies | 2 | 笔记初稿 |
| 5 对照草稿，封板 | 打开 [smart-pointers-draft.md](smart-pointers-draft.md)，只核对事实：计数放在哪、`make_shared` 的限制、`weak_ptr` 为什么参与控制块的生命周期；删除器的边界细节不搬。四条合格线自检；周五审一次，只修 P0/P1 | 0.5 | 封板 |

## 实验

### 实验 1：所有权的物理证据

要写的程序：

- 对 `std::unique_ptr<W>`、`std::shared_ptr<W>`、`std::weak_ptr<W>` 和你的 `KaKaRot::unique_ptr<W>`，用 `std::is_copy_constructible`、`std::is_move_constructible` 各打印一列（`W` 是空结构体）。
- 你的 `unique_ptr`：a 移动给 b 之后，a、b 谁是空？
- `shared_ptr`：拷贝一份之后 `use_count()` 是几？再移动一份，源是什么状态，`use_count()` 变了吗？

```bash
g++ -std=c++14 -Wall -Wextra -I ~/personal/CPlusPlus/My_PPP/chapter19/include exp1.cpp -o exp1 && ./exp1
```

程序里写 `#include "exer/Unique_ptr.h"`。

先预测：把第 1 步那张表的"拷贝""移动"两列预测一遍，再用实验结果对照。

它验证：所有权模型表的"拷贝""移动"两列，以及"谁负责释放"。

### 实验 2：分配次数与大小

要写的程序：

- 替换全局 `operator new` 来数分配次数（沿用你 new-delete Example 的手法）；`W` 是一个小结构体。
- 四种写法各数一次：`unique_ptr<W>(new W)`、`make_unique<W>()`、`shared_ptr<W>(new W)`、`make_shared<W>()`。
- 另外打印 `sizeof`：裸指针、`unique_ptr<W>`、`shared_ptr<W>`、`unique_ptr<FILE, Closer>`（`Closer` 是 RAII 笔记里那个空的函数对象）、`unique_ptr<FILE, int(*)(FILE*)>`（函数指针做删除器）。

先预测：四种写法各几次分配？`sizeof` 各多少？为什么最后一个比倒数第二个大？编译用 `-std=c++14`。

它验证：控制块是单独分配的，`make_shared` 把对象和控制块放进同一次分配；删除器的类型会影响智能指针的大小。`sizeof` 这一部分只是为了看清"删除器放在哪里"，不进正文。

### 实验 3：环与 weak_ptr

要写的程序：

- A 持有 `shared_ptr<B>`，B 持有 `shared_ptr<A>`，两个析构函数各打印一行；在块作用域里创建、互相赋值，再离开。
- 把 B 对 A 的引用换成 `weak_ptr<A>`，再来一遍。
- 最后：一个 `weak_ptr` 指向的 `shared_ptr` 销毁之后，调用 `expired()` 和 `lock()`；销毁之前也调用一次。

先预测：离开块作用域时，有没有析构打印？每个 `use_count` 是几？`lock()` 返回什么？

它验证：循环引用为什么泄漏，`weak_ptr` 为什么不阻止销毁，`lock()` 得到的是什么。

### 实验 4（选做，用机动时间）：手写

- 给你的 `Unique_ptr` 加一个删除器模板参数，再用实验 2 的 `sizeof` 对照标准库。
- 手写最小的 `shared_ptr`：控制块里放强、弱两个计数，拷贝加一，析构减一。

## 封板自检

- 能解释：不看资料，讲清三种指针各表达什么所有权；讲清 `make_shared` 为什么更好，又为什么有时不能用（*Effective Modern C++* Item 21 里列了限制）。
- 有模型：那张所有权表，加一张图：`shared_ptr` 里的"对象指针 + 控制块指针"，控制块里的强、弱计数。
- 有联系：链到 RAII How v（标准句柄表）、② 移动（独占 = 删拷贝、留移动）、④ 异常安全（`make_shared` 与求值顺序）。
- 能验证：实验 2 或 3 做 Example，其余进 Verifies。

## 跑完之后再读

以下是 g++ 11.4、`-std=c++14` 的实际结果，先自己跑、先写预测，再对照。

### 实验 1 的结果

```text
copy / move constructible:
  std::unique_ptr        copy=0 move=1
  std::shared_ptr        copy=1 move=1
  std::weak_ptr          copy=1 move=1
  KaKaRot::unique_ptr    copy=0 move=1
after move: a is null=1, b is null=0
after copy:  use_count=2
after move:  s1 is null=1, use_count=2
```

怎么读：

- 你自己写的 `unique_ptr` 和标准库一致：不可拷贝、可移动；移动之后源为空，所有权整个转给了目标。
- `shared_ptr` 拷贝使计数加一；移动不改变计数，只是把一份所有权从 `s1` 转给 `s3`，源变空。
- `weak_ptr` 能拷贝能移动，但它不拥有对象，实验 3 里能看到它不增加强计数。

### 实验 2 的结果

```text
unique_ptr<W>(new W):      1 allocation(s)
make_unique<W>():          1 allocation(s)
shared_ptr<W>(new W):      2 allocation(s)
make_shared<W>():          1 allocation(s)
sizeof(W*)=8  unique_ptr<W>=8  shared_ptr<W>=16
unique_ptr<FILE, Closer>=8  unique_ptr<FILE, int(*)(FILE*)>=16
```

怎么读：

- `shared_ptr<W>(new W)` 两次分配：一次对象，一次控制块（`_Sp_counted_ptr`）；`make_shared` 一次，对象和控制块放在一起（`_Sp_counted_ptr_inplace`）。cppreference 对 `make_shared` 的说法是 "typically"，标准是建议、不是保证，笔记里别写成保证。
- `shared_ptr` 是两个指针：对象指针和控制块指针。
- `unique_ptr` 默认的删除器不占空间，空的函数对象 `Closer` 也不占；用函数指针做删除器就要多存一个指针。这就是 `bits/unique_ptr.h` 里把指针和删除器放进同一个 `tuple` 的效果。

### 实验 3 的结果

```text
shared_ptr in both directions:
  use_count a=2 b=2
  (scope left)
weak_ptr in one direction:
  use_count a=1 b=2
  ~A2
  ~B2
  (scope left)
lock() on an expired weak_ptr:
  alive:   expired=0 lock()=non-null
  ~A2
  dead:    expired=1 lock()=null
```

怎么读：

- 互持 `shared_ptr`：离开作用域时一个析构都没有打印。两边的计数都从 2 降到 1，再也降不到 0，两个对象都泄漏了。
- 一边换成 `weak_ptr`：`a` 的 `use_count` 只有 1，因为 B2 对它只是弱引用，所以 `a` 能销毁；`a` 销毁时释放了对 B2 的持有，B2 随后销毁，所以先是 `~A2`，后是 `~B2`。
- 对象还活着时，`lock()` 返回一个新的 `shared_ptr`，也就是临时共享所有权；对象销毁之后，`lock()` 返回空。
