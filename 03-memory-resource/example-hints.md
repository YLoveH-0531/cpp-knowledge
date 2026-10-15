# new/delete Example：设计结论与提示

> 回家看。前面各节是设计和提示，**最后一节是答案和解释，写完、预测完、跑完再看。** 这是临时工作文档，Example 并入 `new-delete.md` 后可以删。

## 结论

- Example 要回答的问题：`new` 表达式里构造函数抛异常时，哪个 `operator delete` 被调用？没有配对的 delete 时会怎样？
- 一个程序、五个情形。观察靠"替换全局 `operator new/delete`，每个函数先打印再干活，再用一个计数器数还活着的块"，不依赖外部工具。
- 产出：`new-delete.md` 的 `## Example` = 一句话说明 + 程序 + `Output:` + `Verifies:` 清单。
- 时间盒：写程序 15 分钟，预测加运行 10 分钟，整理进笔记 10 分钟，80% 就收。

## 五个情形

| 情形 | 要观察什么 |
|---|---|
| `new Widget` | 构造失败后调的是哪个 `operator delete` |
| `new (std::nothrow) Widget` | 调 nothrow 配对版；异常照样抛出（nothrow 只管分配失败） |
| `new (pool) Widget` | 配对的 `operator delete(void*, Pool&)` 被调用 |
| `new (buffer) Widget` | 标准 placement：没有任何打印，live 不变 |
| `new (Tag{}) Widget`（放最后） | 没有配对的 delete，没有任何释放函数被调用，live 不归零，即泄漏 |

## 提示（从轻到重，卡住了再往下看）

### 提示 1：怎么预测

把 new-expression 想成下面这段。"配对"的意思是：delete 收到的额外参数和 new 一样。

```cpp
// 伪代码：new T(args) 带额外参数 extra...
p = operator new(sizeof(T), extra...);
try {
    // 在 p 上构造 T
} catch (...) {
    operator delete(p, extra...);   // 找不到配对的函数，就不调
    throw;
}
```

五个情形的 `extra...` 分别是：没有、`std::nothrow`、`pool`、`buffer`、`Tag{}`。用它先填好后面的"预测表"，再写程序。

### 提示 2：程序骨架

```text
全局:   operator new / operator delete(void*) / operator delete(void*, size_t) / nothrow 一对 (打印 + live 计数)
自定义: operator new(size_t, Pool&) + 配对 delete;  operator new(size_t, Tag), 故意不配 delete
struct Fail {};   struct Widget { Widget() { throw Fail{}; } };
run(label, lambda): 打印 label, try/catch Fail, 打印 live blocks
main: 五个 run
```

### 提示 3：要定义哪些函数（只给声明）

```cpp
void* operator new(std::size_t n);
void  operator delete(void* p) noexcept;
void  operator delete(void* p, std::size_t n) noexcept;
void* operator new(std::size_t n, const std::nothrow_t&) noexcept;
void  operator delete(void* p, const std::nothrow_t&) noexcept;

struct Pool { alignas(std::max_align_t) unsigned char buf[64]; };
void* operator new(std::size_t n, Pool& pool);
void  operator delete(void* p, Pool& pool) noexcept;

struct Tag {};
void* operator new(std::size_t n, Tag);   // 故意没有配对的 operator delete(void*, Tag)
```

`Tag` 那个"漏写"是故意的：它在验证"找不到配对的释放函数就不释放"。

### 提示 4：一个函数体的写法（其余照做）

```cpp
static int g_live = 0;

void* operator new(std::size_t n) {
    void* p = std::malloc(n);
    if (!p) throw std::bad_alloc{};
    ++g_live;
    std::printf("  operator new(%zu)\n", n);
    return p;
}
```

- 删除版：先 `--g_live`，再打印，再 `std::free(p)`。
- nothrow 版：`malloc` 失败时返回 `nullptr`，不抛。
- `Pool&` 版：返回 `pool.buf`，既不 malloc 也不 free，计数不动。
- `Tag` 版：和普通版一样 malloc、计数。

### 提示 5：`run` 和 `main`

- `template <class F> void run(const char* label, F f)`：打印 label；`try { f(); } catch (const Fail&) { 打印 caught }`；最后打印 live。
- lambda 里只写 new 表达式，丢掉结果即可：`[] { new Widget; }`、`[&] { new (pool) Widget; }`。
- 标准 placement 需要一块对齐好的缓冲区：`alignas(Widget) unsigned char buffer[sizeof(Widget)];`，然后 `new (buffer) Widget;`，要 `#include <new>`。

### 提示 6：想直接看到标准 placement delete 被调用？

标准那一对不可替换，打印不出来。另写一个类 `Placed`，构造同样抛 `Fail`，类里写同签名的一对静态函数：`operator new(std::size_t, void*)` 和 `operator delete(void*, void*)`，各打印一行。`new (buffer) Placed` 时就能看到类专属版被调用（"类专属同签名允许"，见笔记 Properties 的 Special case）。类里只要声明了 `operator new`，全局的就被隐藏，所以 `Placed` 只能用 placement 形式创建。

## 坑

1. 抛空结构体，别抛 `std::runtime_error`：它的消息字符串会调 `operator new`，污染日志。
2. 日志用 `printf` 或 `puts`：它们走 malloc，不会再触发你替换的 `operator new`。
3. C++14 起把 `operator delete(void*)` 和 `operator delete(void*, std::size_t)` 一起替换，并让它们打印不同的名字。只替换一个，`-Wextra` 会警告 `-Wsized-deallocation`，开 ASan 还会报 `alloc-dealloc-mismatch`。
4. 泄漏先用 live 计数证明，再用 ASan 交叉验证（前提是用到的 new/delete 都被你替换了）。泄漏的那个情形放最后，前面每个情形结束时 live 都应为 0。
5. 替换函数写在全局命名空间，不要加 `static`，不要放进 namespace。

## 编译与运行

```bash
g++ -std=c++17 -Wall -Wextra -g example.cpp -o example && ./example
g++ -std=c++11 -Wall -Wextra -g example.cpp -o example11 && ./example11              # 对比第一个情形
g++ -std=c++17 -g -fsanitize=address example.cpp -o example_asan && ./example_asan   # 交叉验证泄漏
```

## 预测表（先填，再跑）

| 情形 | 我预测的打印 | 实际打印 |
|---|---|---|
| `new Widget` | | |
| `new (std::nothrow) Widget` | | |
| `new (pool) Widget` | | |
| `new (buffer) Widget` | | |
| `new (Tag{}) Widget` | | |

## 放进笔记的格式

`new-delete.md` 的 `## Example`（替换掉里面的 TODO 注释）：一句话说明，然后程序（不超过 70 行），然后 `Output:` 块（原样粘贴，注明编译器和 `-std`），最后 `Verifies:` 清单。`Verifies` 每条对应笔记里的一句话：

- 构造抛异常时，new-expression 自己调用配对的释放函数（三种形式各调哪个）
- 找不到配对的释放函数就不会释放，存储泄漏
- 标准非分配 placement 版什么也不释放
- nothrow 只影响分配失败的报告方式，不拦构造函数的异常
- 第 5 条留给你跑完后发现的意外

写完后把 `README.md` 里 new/delete 的状态从"Example 待写"改掉。

## 跑完之后再读：答案和解释

先自己预测、运行、对照，再往下看。

### 参考输出

g++ 11.4，`-std=c++17`，最后多了一个 `Placed` 情形（提示 6）。打印格式以你自己的为准。

```text
new Widget
  operator new(1)
  operator delete(void*, 1)
  caught Fail
  live blocks: 0
new (std::nothrow) Widget
  operator new(1, nothrow)
  operator delete(void*, nothrow)
  caught Fail
  live blocks: 0
new (pool) Widget
  operator new(1, Pool&)
  operator delete(void*, Pool&)
  caught Fail
  live blocks: 0
new (buffer) Widget
  caught Fail
  live blocks: 0
new (buffer) Placed
  Placed::operator new(size_t, void*)
  Placed::operator delete(void*, void*)
  caught Fail
  live blocks: 0
new (Tag{}) Widget
  operator new(1, Tag)
  caught Fail
  live blocks: 1
```

### 逐个解释

- 前三个情形：new-expression 自己调用了配对的释放函数，配对就是额外参数和 new 一致。
- `Tag`：没有配对的 `operator delete(void*, Tag)`，所以没有任何释放函数被调用，live 停在 1。标准原话（[expr.new]）：找不到唯一匹配的释放函数时，"propagating the exception does not cause the object's memory to be freed"。
- `new (buffer) Widget`：调用的是库里那对标准 placement 函数，它们不可替换，既不打印也不释放。证据是间接的：你的 `operator delete` 一个都没被调用，live 不变。`Placed` 情形则直接看到了同签名的类专属版被调用。
- nothrow 只管分配失败：这里分配成功了，构造函数照样抛出 `Fail`，异常照样传播。

### 那个意外：sized deallocation

第一个情形打印的是 `operator delete(void*, 1)`，不是 `operator delete(void*)`：编译器清理时调的是带 size 的版本。

- `-std=c++14`、`c++17`、`c++20` 输出完全一样；`-std=c++11` 或 `-fno-sized-deallocation` 时，只有这一行变成 `operator delete(void*)`。
- 标准 [expr.delete] 第 9 段（当前草案的编号）：全局作用域、单个对象时，"it is unspecified whether a deallocation function with a parameter of type std::size_t is selected"，即标准没有规定。GCC 从 C++14 起选带 size 的。
- 库里默认的带 size 版只是转发给不带 size 的版本（cppreference：calls `operator delete(ptr)`）。所以只替换了不带 size 的版本时，你看到的仍是 `operator delete(void*)`，选择被默认转发盖住了。这就是要两个都替换、并让它们打印不同名字的原因。
- 回到笔记：3.3 的声明只列了不带 size 的 `operator delete(void*)`。`Verifies` 第 5 条可以写成："`new T` 构造失败后调的是通常的全局 `operator delete`，C++14 起编译器可能选带 size 的那个（标准未指明）"。要不要在 Properties 的 Implicit 那条里补一句，你定。

### ASan 交叉验证

用到的 new/delete 都被替换时，`-fsanitize=address` 恰好报 1 处泄漏，来自 `operator new(unsigned long, Tag)`，退出码 1。只替换了不带 size 的 delete 时，会先报 `alloc-dealloc-mismatch`：ASan 自己带 size 的 delete 被调到了，而指针来自你的 malloc。
