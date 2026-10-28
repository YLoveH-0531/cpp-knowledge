# RAII(Resource Acquisition Is Initialization)

## What

RAII is actually a C++ programming technique that binds the lifetime of a resource, which must be acquired before use, to the lifetime of an object. The key idea is that a resource is always owned by a local (scoped) object (called a resource handle). The handle’s constructor acquires the resource and the handle’s destructor releases it.

```text
 constructor   acquire the resource          if it fails: throws, no object exists
     |
     v
 in use        the object owns the resource
     |
     |         leave the scope by any exit: normal end, early return/break,
     |         or an exception (stack unwinding)
     v
 destructor    release the resource (promise not to throw)
```

## Why

The constructor/destructor technique generalizes to any scoped use of a resource.

### 1. What is a resource?

A resource is anything that a program acquires from another part of the system and must (explicitly or implicitly) release back to its owner after use.

e.g., memory, files, sockets, locks, threads, and database transactions.

### 2. Why is a leak so dangerous?

If resources are not properly released, the system’s performance will suffer and eventually a long-running system will fail for lack of usable memory.

### 3. What causes a leak?

C++ automatically manages object lifetimes, but it cannot automatically determine how to release every resource represented by a raw pointer or handle: destroying a raw pointer does nothing to the resource it points to. Manual cleanup is fragile because early returns, exceptions, and other control-flow paths may bypass the cleanup code.

**Example:**

```cpp
void f() {
    FILE* p = fopen("a", "r");
    g();            // may throw: a hidden exit path
    fclose(p);      // skipped if g() throws
}
```

### 4. Solutions?

The core question: Without counting paths, **how can we guarantee that resources are released regardless of which exit path is taken?**

1. `finally`: C++ has no `finally` statement, and it would need a cleanup clause at every acquisition site. There are far more acquisitions than kinds of resources, so it means more code than RAII, where each resource kind writes its release once in a handle class ([Stroustrup FAQ](https://www.stroustrup.com/bs_faq2.html#finally)).
2. Bind the release operation to something that is guaranteed to execute when the scope is exited normally or by an exception (stack unwinding): an object's destructor.

## How

### i. Key points

#### 1. class invariant

类不变量（class invariant）：关于对象内部状态的逻辑约束。构造函数成功完成时建立它；公共成员函数在满足前置条件时，可以假设它成立，并应在函数结束时保持它成立（包括抛出异常但对象仍然存活的情况）。函数内部可以暂时破坏不变量，但不能将不一致状态暴露给外部。

资源句柄类的不变量通常涉及资源的所有权与句柄状态：对象要么拥有一个由它负责释放的资源，要么处于类明确允许的空状态；析构函数能安全处理每一种允许的状态。

|          | 规定                             | 例子                          |
|----------|----------------------------------|-------------------------------|
| 不可为空 | 对象存在 ⇒ 持有资源              | `lock_guard`                  |
| 可为空   | 持有有效资源，或处于明确的空状态 | `unique_ptr`、可移动的 `File` |

不变量约束的是对象状态和所有权关系，不意味着底层资源在对象存活期间绝不会失效。资源是否存在、是否可用，还取决于资源本身及其底层 API。

#### 2. ownership

资源从创建或获取开始，到最终释放结束，构成资源的生命周期。Ownership 是资源存续期间的管理责任：初始 owner 通常在获取资源时建立所有权，随后负责管理资源，并确保它得到正确释放。如果发生所有权转移，管理责任就转交给新的 owner，最终由当前负责的 owner 按照约定释放资源。RAII 通过将这份责任绑定到对象生命周期，使资源清理能够自动完成。
eg，
| 类别 | 例子 | 析构时 |
|---|---|---|
| 拥有型 | 值成员、`vector`、`unique_ptr` | 释放资源 |
| 非拥有型观察者 | 引用、裸指针、`string_view` | 不释放 |
| 特殊：`weak_ptr` | `weak_ptr` | 不拥有所观察的对象，但参与控制块的生命周期管理（见 [smart-pointers-draft.md](smart-pointers-draft.md)） |

Note: 资源生命周期是一条时间线；ownership 是这条时间线上持续存在、可能转移或共享的责任关系；创建、移动和释放，则是这条时间线上可能发生的具体动作

#### 3. What RAII guarantees 
RAII 将资源清理责任绑定到拥有者对象的生命周期。当对象正常销毁或在异常栈展开过程中销毁时，会执行析构清理。RAII 不保证底层清理操作一定成功，也不保证进程被强制终止时仍会执行清理。

### ii. Construction: acquire, or leave no object behind

对于设计成“构造成功后即可使用”的资源句柄类，优先让构造函数成功返回时对象已经满足不变量；如果无法建立所要求的状态，就抛出异常，或通过工厂函数返回明确的错误结果，而不是悄悄留下一个未初始化的对象。

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

由此得到纪律（Core Guidelines R.12）：资源一获取，就应立即交给资源管理对象接管，避免资源暂时处于无人管理的状态，之后又执行可能抛异常的操作。落到类的设计上：**每个资源由一个成员对象持有**，清理放在这个成员自己的析构里，而不是外层对象的析构里，构造函数体里不要出现裸的 `new/open/lock`。最底层的句柄直接持有一个裸资源，所以它只持有这一个，并且获取成功之后不再做可能抛异常的事。

**和正常析构对比**：销毁成员和基类的顺序、规则一致，区别只有：

| | 正常析构 | 构造失败 |
|---|---|---|
| 本类析构函数体 | 执行 | **不执行** |
| 被销毁的子对象 | 全部 | 只有已完整构造的 |
| function-try-block 的 handler | 无 | 在成员销毁之后执行，结束时异常自动重新抛出 |

最关键的是第一行：析构函数体里写的清理逻辑，构造失败时一行都不会跑，所以清理必须放进**成员对象自己的析构**里。

**委托构造的例外**：目标构造函数一旦完成，对象就算构造完成。此后委托构造函数的体抛异常，会调用析构函数，且先于 function-try-block 的 handler；目标构造函数本身抛异常，则和普通构造一样，不调用析构（g++ 与 clang++ 实测一致）。


### iii. Destruction: release, and never throw

异常传播时也一样。 异常沿调用栈向外传递，直到被某个 handler 捕获；它离开的每个作用域，都会销毁其中已完整构造的局部对象。这个过程叫栈展开（stack unwinding）。
析构函数会在栈展开期间运行（见 i）。如果其中一个析构函数抛出的异常逃出了它，就有两个异常同时在传播，C++ 会调用 `std::terminate`。所以析构函数**不能让异常逃出**：它默认是 `noexcept` 的（除非某个成员或基类的析构函数不是），Core Guidelines 的 C.36 和 C.37 也是这么说的。析构函数内部自己 `try/catch` 是没问题的。

如果释放操作本身可能失败：

- **尽力而为**：记日志或吞掉，适用于失败了也无法补救的情况。
- **调用者必须知道结果**：提供显式的 `close()`，析构函数只做兜底（`ofstream` 就是这样）。
- **既是释放、又可能失败的操作**：把它拆开。`commit()` 可以抛异常，析构函数只做回滚。

### iv. Ownership: who releases, and how many times

现在一个句柄已经能正确地获取和释放了，但资源只能被释放一次，所以每个资源都需要一套明确的所有权策略。**谁拥有资源，谁负责释放；只是引用它的，不负责。** 拥有者：值成员、`vector`、`unique_ptr`、`shared_ptr`。非拥有者：引用、裸指针、`string_view`，以及 `weak_ptr`（见 [smart-pointers-draft.md](smart-pointers-draft.md)）。

所有权决定了拷贝怎么做。编译器生成的逐成员拷贝会让两个对象持有同一个资源，并各释放一次，所以策略必须明确：

- 独占所有权：禁止拷贝（`File`、`unique_ptr`）；
- 值语义：深拷贝，各自拥有一份（`vector`）；
- 共享所有权：由最后一个拥有者释放（`shared_ptr`）。

**移动**转移所有权，并且被移走的对象必须仍然能安全析构。对句柄来说，通常的设计是落到 ii 里说的空状态（标准库对象在移动之后，一般只保证处于有效但未指定的状态）。**零法则（Rule of Zero）**：让成员（`unique_ptr`、`vector`）去拥有资源，自己不写任何特殊成员函数，编译器生成的就都是正确的。详见 [copy-and-move-draft.md](copy-and-move-draft.md)。

### v. Standard handles: smart pointers and non-memory resources

智能指针是 RAII 的标准化应用，用类型表达所有权。

| 类型 | 管理的资源 | 所有权 |
|---|---|---|
| `vector`、`string` | 动态存储及其中的元素 | 独占，深拷贝 |
| `unique_ptr` | 一个动态分配的对象或数组 | 独占，只能移动 |
| `shared_ptr` | 同上 | 共享，引用计数 |
| `lock_guard` | 一把被持有的互斥锁 | 不可拷贝，不可移动 |
| `ofstream` | 一个打开的文件 | 独占，可移动 |

资源不是内存时，提供自定义删除器即可，否则默认删除器会对它 `delete`：

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
