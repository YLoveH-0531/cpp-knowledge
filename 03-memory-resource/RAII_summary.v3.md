# RAII(Resource Acquisition Is Initialization)

> AI 合并稿：以 [RAII-draft.md](RAII-draft.md) 的 How 结构为骨架，并入 [RAII_summary.md](RAII_summary.md) 独有的内容；What、Why 取自你的 RAII_summary.md（译成中文）。读起来仍像"知识点"的段落，请按自己的话重写。Example 的输出是 g++ 11.4 `-std=c++17` 实跑结果。

## What

RAII 其实是一种 C++ 编程技术：把一个使用前必须先获取的资源的生命周期，绑定到一个对象的生命周期上。核心思想是：资源始终由一个局部（有作用域的）对象持有，这个对象叫资源句柄（resource handle）。句柄的构造函数获取资源，析构函数释放资源。

```text
构造函数   获取资源        失败 → 抛异常，对象不存在
   │
   ▼       对象持有资源
使用
   │
   │       以任何方式离开作用域：正常结束、提前 return/break，
   │       或者异常（栈展开）
   ▼
析构函数   释放资源
```

## Why

这套构造函数/析构函数的技术，适用于任何有作用域的资源使用。

### 1. 什么是资源？

资源是程序从系统的另一部分获取、用完之后必须（显式或隐式地）归还给它的所有者的任何东西。

例如：内存、文件、套接字、锁、线程、数据库事务。

### 2. 泄漏为什么危险？

资源如果没有被正确释放，系统的性能会下降，长期运行的系统最终会因为没有可用内存而失败。

### 3. 泄漏是怎么造成的？

C++ 自动管理对象的生命周期，但它无法自动判断怎样释放裸指针或句柄所代表的每一种资源：销毁一个裸指针，不会对它指向的资源做任何事。手写清理很脆弱，因为提前 return、异常和其他控制流路径都可能绕过清理代码。

**例子：**

```cpp
void f() {
    FILE* p = fopen("a", "r");
    g();            // g() 抛异常：这里是一条隐形出口
    fclose(p);      // 走不到
}
```

### 4. 解决办法？

核心问题：在不去数路径的前提下，**怎样保证无论从哪条出口离开，资源都会被释放？**

1. `finally`：C++ 没有 `finally` 语句，而且它要求在每个获取点都写一段清理。获取点远多于资源种类，所以代码量比 RAII 还多；RAII 里每种资源只需在句柄类里写一次释放（[Stroustrup FAQ](https://www.stroustrup.com/bs_faq2.html#finally)）。
2. 把释放操作绑定到一个无论正常离开作用域，还是因异常离开（栈展开），都保证会执行的东西上：对象的析构函数。

## How

### i. Key concepts

#### 1. 类不变量

类不变量：对象处于稳定、可供使用的状态时，必须成立的规则。构造函数负责建立它；成员函数进入时可以假设它成立，退出时要恢复它（函数执行中间可以暂时打破）。类不变量和 RAII 是两个概念：不变量是要守住的条件，RAII 是帮助守住它的设计方式之一。

资源句柄类的不变量可以这样规定：对象要么拥有一个有效资源，要么处于明确的空状态；析构函数能安全地处理这两种状态。

| | 规定 | 例子 |
|---|---|---|
| 不可为空 | 对象存在 ⇒ 持有资源 | `lock_guard` |
| 可为空 | 持有有效资源，或处于明确的空状态 | `unique_ptr`、可移动的 `File` |

空状态是设计时规定、接口上可查的（移动之后落在这里），不是"构造失败留下的无效对象"。

#### 2. RAII 承诺什么

有了句柄对象，Why 里的 `f()` 在任何一条退出路径上都不需要写 `fclose`：

```cpp
void f() {
    File p("a", "r");   // 构造函数获取资源
    g();                // g() 抛异常时，~File() 照样运行（栈展开）
}                       // 正常退出时，~File() 在这里运行
```

（`File` 是 Example 里的句柄类，封装一个 `FILE*`。）函数里没有任何清理代码，靠的是两条语言规则：

1. **离开作用域，就销毁局部对象。** 无论是执行到末尾，还是 `return`、`break`、`goto` 提前离开，该作用域里已经构造完成的局部对象（自动存储期）都会被销毁。
2. **异常传播时，同样销毁。** 异常沿调用栈向外传递，直到被某个 handler 捕获；途中每个被离开的作用域里，已经构造完成的局部对象都会被销毁。这个过程叫栈展开（stack unwinding）。

两条规则销毁对象的顺序都与构造顺序相反。这个逆序是有用的：后获取的资源往往依赖先获取的，逆序释放不会破坏这种依赖。

两条规则合起来，就是 Why 里要的保证：只要作用域被离开，不论是正常结束还是因异常离开，析构函数都会执行。**承诺的是清理代码会被执行，不是清理一定成功。** 不在承诺内的情况：

- 对象没有构造完成：构造到一半失败的对象不会被销毁（见 ii）；
- 清理本身可能失败：析构里的释放操作（例如 `close()`）可能失败（见 iii）；
- 它只管资源，不管对象状态：栈展开只调用析构，不撤销已经做过的修改；异常之后对象的类不变量是否还成立，要靠代码的结构（见 [exception-safety-draft.md](../07-language-mechanics/exception-safety-draft.md)）；
- 有些退出方式不经过析构：进程被强制终止（`abort`、`_exit`、崩溃）时不会；`std::exit` 不展开栈，局部对象同样不会被销毁；异常没有被任何 handler 捕获时，会调用 `std::terminate`，在此之前栈是否展开由实现定义。

#### 3. 所有权

谁拥有，谁负责释放；只是引用的，不释放。

| 类别 | 例子 | 析构时 |
|---|---|---|
| 拥有型 | 值成员、`vector`、`unique_ptr` | 释放资源 |
| 非拥有型观察者 | 引用、裸指针、`string_view` | 不释放 |
| 特殊：`weak_ptr` | `weak_ptr` | 不拥有所观察的对象，但参与控制块的生命周期管理（见 [smart-pointers-draft.md](smart-pointers-draft.md)） |

非拥有型多一份责任：要保证被引用的对象比自己活得久（局部变量按声明的逆序析构，所以先声明的被引用者，比后声明的引用者活得久）。

### ii. Constructor failure

只有已完整构造的对象才会被销毁（见 i.2），所以构造函数只有两种可以接受的结果：要么把对象完整建立起来，要么干净地失败，既不留下对象，也不泄漏资源。

**为什么在构造函数里获取资源。** Resource Acquisition Is Initialization 这个名字给出了答案：获取资源是初始化的一部分，所以句柄不会处于"还没获取"这种意外状态：它要么持有一个有效资源，要么处于设计时明确规定的空状态，这正是 i.1 里的类不变量。析构函数只需要处理这两种状态。

这也让 What 里的"绑定"更精确：对象持有资源的那段时间包含在它的生命周期之内，但两者并不相等，因为对象可以是空的，也可以提前释放（`reset()`）。所以获取资源是"通常"而不是"必须"在构造函数里。

**构造失败怎么报告？** 构造函数没有返回值，常见做法有四种：

| 方案 | 失败时的表现 | 代价 |
|---|---|---|
| 构造抛异常 | 对象不存在，异常向外传播 | 需要异常机制可用 |
| `valid()` | 对象存在但无效，调用者要检查 | 忘记检查；成员函数处处防御 |
| 两段式初始化 | 对象存在但未初始化 | 忘记调用 `init` |
| 工厂 + `optional/expected` | 返回空或错误信息，调用者处理 | 错误不能自动向上传播 |

默认选择是抛异常（Core Guidelines C.42）：构造成功后，对象满足类不变量；构造失败并抛出异常时，不会产生一个已完整构造的对象。异常保证的不是"获取一定成功"，而是失败不会被悄悄吞掉，也不会留下无效对象。反过来，RAII 让异常能被安全使用：异常制造大量隐形出口，RAII 在栈展开时替你清理。

其余几种做法的前提：工厂函数同样不留下对象，是异常不可用时的常用选择，但每个调用者都必须检查结果；`valid()` 标志或两段式 `init()` 只有在"还没获取"本身就是该类型的设计状态时才站得住（"可为空"的句柄，比如默认构造的 `ofstream`），此时每个使用者都得处理这个状态。绝对不能存在的，是游离在类不变量之外的半初始化对象。

**语言规则**（构造函数抛异常时，[except.ctor](https://eel.is/c++draft/except.ctor)）：

1. 对象的生命周期从构造完成才开始，所以**本类析构函数不调用**；
2. **已完整构造的成员和基类**按构造完成的逆序析构；
3. 若是 `new T(...)`，由 new 表达式调用匹配的 `operator delete`（见 [new-delete.md](new-delete.md)）。

所以，如果某个资源还没有交给已完整构造的对象持有，构造函数的后续步骤一旦抛出异常，它就泄漏了。成员是对象的**子对象**，不是构造函数里的局部变量（`new Good` 时它们在堆上，没有"栈"可退）。`Bad` 泄漏了 `a_`，`Good` 的成员会自己清理：

```cpp
class Bad {                       // b_ 的 new 抛异常 → a_ 泄漏
    int *a_, *b_;                 // 裸指针没有析构动作，~Bad 也不会被调用
public:
    Bad() { a_ = new int[100]; b_ = new int[100]; }
};

class Good {                      // b_ 抛异常 → 已构造的 a_ 自动析构
    std::unique_ptr<int[]> a_, b_;
public:
    Good() : a_(new int[100]), b_(new int[100]) {}
};
```

由此得到纪律（Core Guidelines R.12）：资源一获取，就立刻交给一个已完整构造的对象。落到类的设计上：**每个资源由一个成员对象持有**，清理放在这个成员自己的析构里，而不是外层对象的析构里，构造函数体里不要出现裸的 `new/open/lock`。最底层的句柄直接持有一个裸资源，所以它只持有这一个，并且获取成功之后不再做可能抛异常的事。

**和正常析构对比**：销毁成员和基类的顺序、规则一致，区别只有：

| | 正常析构 | 构造失败 |
|---|---|---|
| 本类析构函数体 | 执行 | **不执行** |
| 被销毁的子对象 | 全部 | 只有已完整构造的 |
| function-try-block 的 handler | 无 | 在成员销毁之后执行，结束时异常自动重新抛出 |

最关键的是第一行：析构函数体里写的清理逻辑，构造失败时一行都不会跑，所以清理必须放进**成员对象自己的析构**里。

**委托构造的例外**：目标构造函数一旦完成，对象就算构造完成。此后委托构造函数的体抛异常，会调用析构函数，且先于 function-try-block 的 handler；目标构造函数本身抛异常，则和普通构造一样，不调用析构（g++ 与 clang++ 实测一致）。

### iii. Destructors must not throw

栈展开期间，若析构函数又抛出异常并逃出，同时就有两个异常在传播，C++ 无法处理，直接 `std::terminate`。所以析构函数**默认 `noexcept`**（除非某个成员或基类的析构不是）。C++ Core Guidelines C.36、C.37 也建议析构函数不能失败。

析构里调用的释放操作失败了怎么办：

- 尽力而为：记日志或吞掉。在析构内部自己 `try/catch` 没问题，禁止的是让异常**逃出**；
- 调用者需要知道结果：提供显式 `close()`，析构只做兜底（`ofstream` 就是这样）；
- "既是释放、又可能失败"的操作，拆开：`commit()` 是普通操作，可以抛，析构只做回滚。

**`noexcept` 的含义**：对调用者承诺"异常不会传出这个函数"；违反时调用 `std::terminate`，此时栈是否展开由实现定义。它不阻止函数抛异常，只改变"传出去"的后果。

### iv. Ownership forces copy and move decisions

一个类开始拥有资源，拷贝就不能想当然：编译器生成的逐成员拷贝，会让两个对象持有同一个资源，两个析构各释放一次。所以资源所有者必须明确选一种：

- 禁止拷贝，独占所有权（`File`、`unique_ptr`）；
- 深拷贝，各持一份；
- 共享所有权（`shared_ptr`）。

移动则是把所有权转给另一个对象，被移走的对象必须仍然能安全析构。对句柄来说，通常的设计是落到空状态，析构时什么都不做（标准库对象在移动之后，一般只保证处于有效但未指定的状态）。**零法则**：让成员（`unique_ptr`、`vector`）管资源，外层类一个特殊成员都不写，编译器生成的就都正确。生成规则、多态基类的写法见 [copy-and-move-draft.md](copy-and-move-draft.md)。

### v. Smart pointers and non-memory resources

智能指针是 RAII 的标准化应用，用类型表达所有权：`unique_ptr` 独占，`shared_ptr` 共享，`weak_ptr` 观察。资源不是内存时，提供自定义删除器即可，否则默认删除器会对它 `delete`：

```cpp
struct Closer { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
std::unique_ptr<std::FILE, Closer> p(std::fopen("a", "r"));   // 析构时 fclose，不是 delete
```

删除器的规则、`make_unique/make_shared`、控制块见 [smart-pointers-draft.md](smart-pointers-draft.md)。

## Relation

### i. Issues

没有 RAII，资源释放靠调用者在每条路径上手写，提前 `return`、异常、后来新增的分支都会漏掉；裸指针只存地址，不表达谁拥有、对象能活多久。

### ii. Connection

- [new-delete](new-delete.md)：`new/delete` 把"存储"和"对象"分成两半；RAII 把"谁负责走完这两半"绑到析构上。构造失败时由 new 表达式调用匹配的 `operator delete`，是同一条清理思想。
- [Array](../01-basic/array.md)：`new T[n]` 得到不带大小的裸指针，`vector` 把 `new[]`、大小和 RAII 封装起来。
- [exception-safety](../07-language-mechanics/exception-safety-draft.md)：栈展开时 RAII 释放资源，是异常安全的基础。保持对象的不变量（基本保证）或把状态恢复到调用前（强保证，如 copy-and-swap）不归 RAII 管，见该篇。
- 移动语义：移动技术负责在作用域之间转移所有权。

**See also:** [copy-and-move-draft.md](copy-and-move-draft.md)：所有权的转移与特殊成员函数；[smart-pointers-draft.md](smart-pointers-draft.md)：RAII 的标准化应用。

## Example

文件句柄、带两个资源的类、`lock_guard`。`TwoFiles` 没有任何特殊成员函数，清理全靠成员 `File` 的析构。

```cpp
#include <cstdio>
#include <mutex>
#include <stdexcept>
#include <utility>

class File {
    std::FILE* f_ = nullptr;
public:
    File(const char* path, const char* mode) : f_(std::fopen(path, mode)) {
        if (!f_) throw std::runtime_error("open failed");   // failure: no object
        std::puts("  File opened");
    }
    ~File() {
        if (f_) { std::fclose(f_); std::puts("  File closed"); }
    }
    File(const File&) = delete;                             // sole owner
    File(File&& o) noexcept : f_(std::exchange(o.f_, nullptr)) {}
    File& operator=(File&&) = delete;
    std::FILE* get() const { return f_; }
};

struct TwoFiles {                                           // no special members: members are RAII
    File a_;
    File b_;
    TwoFiles(const char* pa, const char* pb) : a_(pa, "r"), b_(pb, "r") {}
};

void work_that_throws(const File&) { throw std::runtime_error("work failed"); }

std::mutex m;
bool is_locked() { if (m.try_lock()) { m.unlock(); return false; } return true; }

int main() {
    std::puts("1) open fails:");
    try { File f("/no/such/file", "r"); }
    catch (const std::exception& e) { std::printf("  caught: %s\n", e.what()); }

    std::puts("2) exception after open:");
    try { File f("/etc/hostname", "r"); work_that_throws(f); }
    catch (const std::exception& e) { std::printf("  caught: %s\n", e.what()); }

    std::puts("3) second member fails:");
    try { TwoFiles t("/etc/hostname", "/no/such/file"); }
    catch (const std::exception& e) { std::printf("  caught: %s\n", e.what()); }

    std::puts("4) move:");
    {
        File a("/etc/hostname", "r");
        File b(std::move(a));
        std::printf("  a.get() == nullptr: %d\n", a.get() == nullptr);
    }

    std::puts("5) lock_guard:");
    try {
        std::lock_guard<std::mutex> g(m);
        std::printf("  inside: locked=%d\n", is_locked());
        throw std::runtime_error("work failed");
    } catch (const std::exception& e) {
        std::printf("  caught: %s, locked=%d\n", e.what(), is_locked());
    }
}
```

Output:

```text
1) open fails:
  caught: open failed
2) exception after open:
  File opened
  File closed
  caught: work failed
3) second member fails:
  File opened
  File closed
  caught: open failed
4) move:
  File opened
  a.get() == nullptr: 1
  File closed
5) lock_guard:
  inside: locked=1
  caught: work failed, locked=0
```

Verifies:

- 1：构造失败时没有 `File closed`，对象没存在过，析构不会被调用；
- 2：`File closed` 先于 `caught`，清理发生在进入 handler 之前（栈展开）；
- 3：第二个成员构造失败，已构造的第一个成员被自动析构；
- 4：移动后旧对象为空，只有一个所有者关闭文件；
- 5：异常离开作用域后锁已释放。

## Reference

- Stroustrup, *Foundations of C++*（ETAPS 2012），据 [RAII.md](RAII.md) 的阅读路线读第 6–8 节；本草稿未对照原文核对
- C++ Core Guidelines：R.1（用资源句柄和 RAII 管理资源）、R.12（显式分配的结果立即交给管理对象）、E.6（用 RAII 防止泄漏）、C.31（类获取的资源由析构释放，含"不拥有"的注）、C.36、C.37（析构不能失败，应 `noexcept`）、C.42（构造函数无法构造有效对象时抛异常）
- cppreference: [Destructors](https://en.cppreference.com/w/cpp/language/destructor)、[RAII](https://en.cppreference.com/w/cpp/language/raii)
- C++ standard (draft), by claim:
  - [except.ctor](https://eel.is/c++draft/except.ctor): 栈展开，构造失败时销毁已构造的子对象
