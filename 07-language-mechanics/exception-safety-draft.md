# Exception Safety

> AI 整理的草稿，未经你重写。构造失败的处理、析构不能抛异常，已经在 [RAII-draft.md](../03-memory-resource/RAII-draft.md) 里写过，这里只引用。Example 的输出是 g++ 11.4 `-std=c++17` 实跑结果。

## Why

异常让每一个可能失败的调用都成了出口。RAII 负责出口上的**资源释放**（见 [RAII-draft.md](../03-memory-resource/RAII-draft.md)）。但出口还有另一个后果：**函数在中途离开，已经做过的修改不会被撤销。** 异常被捕获后程序继续运行，对象如果还活着，就带着那些做到一半的修改继续被使用：

```cpp
class Image {
    std::size_t w_ = 0, h_ = 0;
    std::vector<unsigned char> px_;       // invariant: px_.size() == w_ * h_
public:
    void resize(std::size_t w, std::size_t h) {
        w_ = w; h_ = h;                   // already modified
        px_.resize(w * h);                // throws: no leak, but w_ * h_ != px_.size()
    }
};
```

`vector` 把自己的内存管得很好，什么也没漏，`Image` 的不变量却坏了：**资源被清理，不等于状态是对的。**

核心问题：**一个操作因异常中止后，对象处于什么状态？** 异常安全保证就是对这个问题的分级回答。

## What

**异常安全（exception safety）**：一个操作因异常中止后，资源和对象状态还能保持什么性质。承诺从弱到强分为基本、强、不抛三级，没有这些承诺就是无保证。保证是**操作**的属性，不是整个类的属性：同一个类里，有的操作是强保证，有的只有基本保证。

| 保证 | 异常传出后 |
|---|---|
| 无保证 | 可能泄漏，不变量可能被破坏 |
| 基本（basic） | 不泄漏，不变量成立；值可能已经改变 |
| 强（strong） | 同基本，并且这个操作没有留下可观察的状态变化，像没发生过 |
| 不抛（no-throw） | 不会有异常传出 |

它们怎样连起来，顺着一次异常往下问：

```text
异常抛出
  │  栈展开：只调用析构，不撤销已做的修改
  ▼
资源会漏吗？          RAII                  → 不泄漏（基本保证的前一半）
不变量还成立吗？      调整修改顺序（ii）    → 对象有效（基本保证的后一半）
状态和调用前一样吗？  先准备，再提交（iii） → 强保证
提交这一步会失败吗？  只用不抛的操作（iv）  → 不抛保证（提交步骤依赖它）
```

**RAII 解决的是第一个问题**：它的析构里写的是释放资源，不会自动撤销修改，也不会自动维护成员之间的关系，后三个靠代码的结构。最后一行不是强保证的定义，而是"先准备再提交"这种做法所依赖的性质：提交这一步必须不抛（回滚式的做法则要求撤销动作不抛）。**准备**指在临时变量里做完所有可能失败的工作，**提交**指随后把新状态换进对象的那一步（见 How iii）。

## How

**主线**：先分清"资源清理"和"状态正确"（i）→ 基本保证靠调整修改顺序（ii）→ 强保证常用先准备再提交的做法，copy-and-swap 是其中一种经典惯用法（iii）→ 这种做法依赖不抛的提交，`noexcept` 用来声明它（iv）。

### i. Key concepts

- **资源清理 vs 状态正确**：资源已经交给 RAII 对象持有时，栈展开会调用它的析构，资源被释放；但栈展开只"销毁"，不"撤销"已经做过的修改，成员之间是否互相一致，要靠代码保证。
- **基本保证 vs 强保证**：基本，对象还能用，但值可能已经变了；强，值也没变，像操作没发生过。
- **不抛保证 vs `noexcept`**：保证是对行为的承诺，`noexcept` 是语言里声明它的方式；声明了却抛出，不会回滚，而是调用 `std::terminate`。

**不变量**的定义见 [RAII-draft.md](../03-memory-resource/RAII-draft.md) 的"类不变量"一节：对象处于稳定状态时必须成立的规则，函数执行中间可以暂时打破。**有效状态**就是不变量成立的状态，它不等于"值是你想要的"。

**栈展开**（stack unwinding）：异常从 `throw` 传向 handler 的途中，沿调用链往外退，每退出一个作用域，就按构造完成的相反顺序，析构该作用域里已经构造完成的自动对象。它只调用析构，不撤销已经做过的修改。没有任何 handler 接住异常时，是否展开由实现定义。

### ii. Basic guarantee: keep the invariant

基本保证有两半：

1. **不泄漏**：每个资源都由一个对象持有（RAII）；
2. **不变量成立**：不变量允许在函数中间被暂时打破，危险在于**打破之后、恢复之前**这段区间里出现异常。办法是缩小这段区间：会抛的操作放前面，修改成员放后面，而且修改成员的那几步本身不抛。

```cpp
void resize(std::size_t w, std::size_t h) {
    px_.resize(w * h);                // the only step that can throw, done first
    w_ = w; h_ = h;                   // integer assignments cannot throw
}
```

只有一个步骤会抛时，把它放在最前面，失败后对象就完全没动，这已经是强保证（`vector::resize` 失败时自己也不变，所以这里成立）。有两个以上的步骤会抛时，光靠顺序不够，见 iii。Example 1 验证不变量。

### iii. Strong guarantee: prepare, then commit

有两个以上的步骤会抛时，先做完的那一步已经改了对象，后一步再失败就回不去了。例如先 `host = h`，再 `port = std::stoi(p)`（Example 里的 `update_basic`）：`host` 已经是新值，`port` 解析失败，对象有效，但半新半旧，只有基本保证。强保证的做法是把"会抛的工作"和"改动自己"分成两段：

1. **准备**：在临时变量里做所有可能失败的工作，`*this` 不动；
2. **提交**：全部成功之后，用不抛的操作把新状态换进来。

```cpp
void update_strong(const std::string& h, const std::string& p) {
    int np = std::stoi(p);        // prepare: may throw, *this untouched
    std::string nh = h;           // prepare: may throw, *this untouched
    host = std::move(nh);         // commit: noexcept
    port = np;                    // commit: cannot throw
}
```

**copy-and-swap** 是用这个做法实现复制赋值强保证的一种经典惯用法：临时副本是"准备"，`swap` 是"提交"，换下来的旧状态随临时副本一起析构（仍是 RAII 在清理）。成员用标准库类型时，成员自己的 `swap` 就是现成的不抛提交：

```cpp
SwapProfile& operator=(const SwapProfile& rhs) {
    SwapProfile tmp(rhs);         // prepare: copying may throw, *this untouched
    swap(tmp);                    // commit
    return *this;
}                                 // tmp holds the old state and is destroyed here
void swap(SwapProfile& o) noexcept { name.swap(o.name); items.swap(o.items); }
```

对照编译器生成的复制赋值：它按声明顺序逐个成员赋值，不会自动提供跨成员的回滚，整体的保证取决于成员的类型和类的不变量。Example 3 里 `name` 先被赋值，之后 `items` 的元素拷贝抛出，目标半新半旧。这两个成员之间没有共同的不变量，所以只是基本保证；如果有，连基本保证都可能丢。copy-and-swap 的失败发生在副本构造里，原对象一点没动。它的代价是总要多做一次完整拷贝（目标容量够用时也一样），所以用在需要强保证、又是整体替换状态的地方；`operator=` 的完整写法（含按值形式）留给拷贝与移动，见 [copy-and-move-draft.md](../03-memory-resource/copy-and-move-draft.md)。

强保证也不止这一种做法。事务式回滚是另一条路：先改，同时记下撤销动作，失败时由守卫对象的析构去执行（见 [RAII-draft.md](../03-memory-resource/RAII-draft.md) 的"Destructors must not throw"一节："析构只做回滚"）。它同样要求撤销动作本身不能失败。

### iv. Nothrow operations and `noexcept`

先准备再提交的做法有一个依赖：提交这一步不能失败。提交如果会抛，抛出时状态就是半新半旧，又退回基本保证；回滚式的做法同理，撤销动作本身不能失败。所以实现强保证时，经常要用到不抛的操作。这是实现方法的要求，不是强保证的定义：强保证只规定失败之后的结果，不抛保证只规定异常不会传出，两者回答的问题不同。

适合充当提交步骤的操作，通常应当做到不抛：

- 析构函数（见 [RAII-draft.md](../03-memory-resource/RAII-draft.md) 的"Destructors must not throw"一节）和 `operator delete` 这类释放函数；
- `swap`：使用默认分配器时，`std::string::swap` 和 `std::vector::swap` 都是 `noexcept`（Example 3 用 `static_assert` 验证）。自己的类要做 copy-and-swap，`swap` 就由这些不抛的成员 `swap` 组成；
- 移动操作：能做到不抛才声明 `noexcept`（Core Guidelines C.66），不要为了让容器选择移动而虚报。做不到就如实不声明，容器会据此选择策略，见下。

**`noexcept` 的作用**是声明这个承诺，并让标准库据此选择策略。`vector::push_back` 在容量用完时要把旧元素搬到新存储，搬的过程中任何一个元素抛出异常，都不能破坏原来的 `vector`。标准对它的保证取决于元素的移动构造：

- 移动构造是 `noexcept`：可以安全地移动，这一步不会失败；
- 移动构造可能抛，而且元素可拷贝：通常退回到拷贝（实现常用 `std::move_if_noexcept`，见 [copy-and-move-draft.md](../03-memory-resource/copy-and-move-draft.md) 的 Move and noexcept）。拷贝不碰原件，失败了丢弃新存储，原 `vector` 不变；
- 移动构造可能抛，而且元素不可拷贝：强保证被放弃，效果未指定。

具体用移动还是拷贝是实现的选择，标准规定的是上面的保证。**强保证不要求整个操作 `noexcept`：** `push_back` 本身会抛（分配失败、拷贝失败），强保证说的是抛出以后原状态不变。

## Relation

### i. Issues

没有这套分级，"异常安全"只是一个模糊的愿望：调用者不知道捕获异常之后，还能不能继续使用这个对象、要不要重试；评审代码时也说不清"这里抛异常会怎样"。有了分级，每个操作都可以写明承诺：失败后对象还能用（基本），还是和调用前一样（强），还是根本不会失败（不抛）。

三个常见的混淆：

- **RAII ≠ 异常安全**：RAII 保证已托管的资源在栈展开时被释放，但不会自动撤销已做的修改，也不会自动维护成员之间的关系；
- **`noexcept` ≠ 异常安全**：它只是声明，声明了却抛出，程序终止，不会回滚；
- **copy-and-swap ≠ 异常安全的定义**：它只是做出强保证的一种手段，适用于"整体替换状态"的操作。

### ii. Connection

- [RAII](../03-memory-resource/RAII-draft.md)：栈展开时 RAII 保证资源被释放，是基本保证的前一半。构造失败只销毁已构造的子对象、析构不能让异常逃出，是它给异常安全打的底。
- [copy-and-move-draft.md](../03-memory-resource/copy-and-move-draft.md)：`noexcept` 的移动让 `vector` 扩容既能用移动，又不丢强保证；copy-and-swap 的完整写法放在这里。
- [new-delete](../03-memory-resource/new-delete.md)：构造抛出时 new 表达式调用配对的 `operator delete`，是"不泄漏"在 new 表达式上的体现。

## Example

三个小实验：不变量被破坏（1）、半新半旧（2）、复制赋值中途失败（3）。失败都用确定的办法触发，不依赖内存真的耗尽：`resize` 的参数超过 `vector::max_size()`（抛 `length_error`）、`std::stoi` 解析非法输入（抛 `invalid_argument`）、元素的拷贝构造函数在第 2 次调用时抛出。

```cpp
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// 1) Invariant: px_.size() == w_ * h_
class Image {
    std::size_t w_ = 0, h_ = 0;
    std::vector<unsigned char> px_;
public:
    void resize_bad(std::size_t w, std::size_t h) {
        w_ = w; h_ = h;                            // modified first
        px_.resize(w * h);                         // throws: w_ * h_ != px_.size()
    }
    void resize_good(std::size_t w, std::size_t h) {
        px_.resize(w * h);                         // the only step that can throw, done first
        w_ = w; h_ = h;                            // cannot throw
    }
    bool invariant() const { return px_.size() == w_ * h_; }
};

// 2) Two steps can throw: ordering alone is not enough
static_assert(std::is_nothrow_move_assignable_v<std::string>);

struct Config {
    std::string host = "old-host";
    int port = 80;
    void update_basic(const std::string& h, const std::string& p) {
        host = h;                                  // changed
        port = std::stoi(p);                       // throws: host is already new
    }
    void update_strong(const std::string& h, const std::string& p) {
        int np = std::stoi(p);                     // prepare: may throw, *this untouched
        std::string nh = h;                        // prepare: may throw, *this untouched
        host = std::move(nh);                      // commit: noexcept
        port = np;                                 // commit: cannot throw
    }
};

// 3) Compiler-generated copy assignment vs copy-and-swap
struct Item {                                      // a vector element whose copy can be made to fail
    static inline int copies_left = -1;            // -1: never fail; n: the (n+1)-th copy throws
    int id;
    explicit Item(int i) : id(i) {}
    Item(const Item& o) : id(o.id) {
        if (copies_left == 0) throw std::runtime_error("copy failed");
        if (copies_left > 0) --copies_left;
    }
    Item& operator=(const Item&) = default;
};

struct PlainProfile {
    std::string name;
    std::vector<Item> items;
};

struct SwapProfile {
    std::string name;
    std::vector<Item> items;
    SwapProfile(std::string n, std::vector<Item> i) : name(std::move(n)), items(std::move(i)) {}
    SwapProfile(const SwapProfile&) = default;
    SwapProfile& operator=(const SwapProfile& rhs) {
        SwapProfile tmp(rhs);                      // prepare: copying may throw, *this untouched
        swap(tmp);                                 // commit
        return *this;
    }
    void swap(SwapProfile& o) noexcept { name.swap(o.name); items.swap(o.items); }
};
static_assert(noexcept(std::declval<std::string&>().swap(std::declval<std::string&>())));
static_assert(noexcept(std::declval<std::vector<Item>&>().swap(std::declval<std::vector<Item>&>())));

template <class P>
static void assign_with_failing_copy(const char* label) {
    const P src{"new-name", {Item(1), Item(2)}};   // built before the failure is armed
    P dst{"old-name", {}};
    Item::copies_left = 1;                         // the 2nd element copy throws
    try { dst = src; } catch (const std::runtime_error&) {}
    Item::copies_left = -1;
    std::printf("   %-14s name=%s items=%zu\n", label, dst.name.c_str(), dst.items.size());
}

int main() {
    std::puts("1) resize to 2^62 x 2 pixels, more than vector::max_size():");
    Image bad, good;
    try { bad.resize_bad(std::size_t(1) << 62, 2); } catch (const std::exception&) {}
    try { good.resize_good(std::size_t(1) << 62, 2); } catch (const std::exception&) {}
    std::printf("   resize_bad:  invariant=%d\n", bad.invariant());
    std::printf("   resize_good: invariant=%d\n", good.invariant());

    std::puts("2) update(\"new-host\", \"x\"), std::stoi throws:");
    Config a, b;
    try { a.update_basic("new-host", "x"); } catch (const std::invalid_argument&) {}
    try { b.update_strong("new-host", "x"); } catch (const std::invalid_argument&) {}
    std::printf("   update_basic:  host=%s port=%d\n", a.host.c_str(), a.port);
    std::printf("   update_strong: host=%s port=%d\n", b.host.c_str(), b.port);

    std::puts("3) copy assignment, the 2nd element copy throws:");
    assign_with_failing_copy<PlainProfile>("compiler:");
    assign_with_failing_copy<SwapProfile>("copy-and-swap:");
}
```

Output:

```text
1) resize to 2^62 x 2 pixels, more than vector::max_size():
   resize_bad:  invariant=0
   resize_good: invariant=1
2) update("new-host", "x"), std::stoi throws:
   update_basic:  host=new-host port=80
   update_strong: host=old-host port=80
3) copy assignment, the 2nd element copy throws:
   compiler:      name=new-name items=0
   copy-and-swap: name=old-name items=0
```

Verifies:

- 1：两个版本的内存管理完全一样（成员都是 `vector`），只有语句顺序不同：`resize_bad` 失败后不变量被破坏（`invariant=0`），`resize_good` 没有（`invariant=1`）；
- 2：`std::stoi("x")` 抛出后，`update_basic` 留下半新半旧（`host` 已是新值，`port` 仍是旧值），`update_strong` 完全没动；
- 3：第 2 次元素拷贝抛出时，编译器生成的复制赋值已经把 `name` 改了（隐式复制赋值按声明顺序赋值，`name` 在 `items` 之前），`items` 仍是空的，目标半新半旧；copy-and-swap 的失败发生在副本构造里，原对象一点没动。这个例子里两个成员之间没有共同的不变量，所以前者只是基本保证。编译通过的 `static_assert` 说明 `string`、`vector` 的 `swap` 和 `string` 的移动赋值在这个环境下是 `noexcept`。

## Reference

- David Abrahams, *Exception-Safety in Generic Components*：基本、强、不抛三级保证的出处；本草稿未对照原文核对
- C++ Core Guidelines：E.6（用 RAII 防止泄漏）、E.12（函数不可能或不允许因抛出而退出时，用 `noexcept`）、C.66（移动操作应 `noexcept`）、C.83、C.84、C.85（`swap` 不能失败，应 `noexcept`）
- cppreference: [std::vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back)（Exceptions 一节）、[std::move_if_noexcept](https://en.cppreference.com/w/cpp/utility/move_if_noexcept)、[noexcept specifier](https://en.cppreference.com/w/cpp/language/noexcept_spec)、[throw expression](https://en.cppreference.com/w/cpp/language/throw)（Stack unwinding 一节）
- C++ standard (draft), by claim:
  - [except.ctor](https://eel.is/c++draft/except.ctor): 控制权从 throw 传向 handler 的途中，按构造的逆序析构已构造的自动对象（栈展开）
  - [except.terminate](https://eel.is/c++draft/except.terminate): 违反 `noexcept` 时调用 `std::terminate`
  - [vector.modifiers](https://eel.is/c++draft/vector.modifiers): `push_back` 在元素可拷贝、或移动构造不抛时，失败没有效果；元素不可拷贝且移动构造抛出时，效果未指定
  - [vector.capacity](https://eel.is/c++draft/vector.capacity): `resize(n)` 失败时没有效果（元素不可拷贝且移动构造抛出的情况除外）
  - [class.copy.assign](https://eel.is/c++draft/class.copy.assign): 隐式定义的复制赋值先赋值基类，再按声明顺序逐个赋值非静态数据成员
