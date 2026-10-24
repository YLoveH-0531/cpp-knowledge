# Smart pointers

> 种子草稿：从 [RAII-draft.md](RAII-draft.md) 拆出来的智能指针细节，已按评审意见校正，还没按模板（Why / What / How / Relation / Example）整理，待你扩写。

## unique_ptr and deleters

`unique_ptr`：独占，可移动不可拷贝。在析构、`reset()`、移动赋值放弃旧指针时，对**非空**指针调用一次删除器；`release()` 不调用。

- 默认删除器是 `std::default_delete<T>`：`delete p`；`unique_ptr<T[]>` 对应 `delete[]`。管理非内存资源必须自定义删除器：`unique_ptr<FILE>` 能编译，运行时却对 `fopen` 的结果执行 `delete`（未定义行为）。
- 删除器必须不抛异常：标准把"`get_deleter()(get())` 不抛异常"列为 `unique_ptr` 析构的前提条件，违反就是未定义行为。我们实测的是 g++ 11.4 下的表现：删除器抛出的异常逃出 `noexcept` 的析构，程序 `std::terminate`（退出码 134）。这是实现观察，不是标准保证。
- `Closer::operator()` 标 `const noexcept`：`const` 是无状态函数对象的惯例（`const` 删除器对象也能调用），`noexcept` 把契约写进接口。两者去掉都能编译。
- `unique_ptr<FILE, Closer>(fopen(...))`：先求值得到裸指针，再调用 `explicit unique_ptr(pointer) noexcept`，拿到和接管之间没有空档；`fopen` 失败时存的是 `nullptr`，删除器不会被调用。
- 大小：g++ 11.4 上 `sizeof(unique_ptr<FILE, Closer>)` 等于 `sizeof(FILE*)`（空的 `Closer` 不占空间）。这是该实现的观察结果，不是 `unique_ptr` 的大小保证。

```cpp
struct Closer { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
std::unique_ptr<std::FILE, Closer> p(std::fopen("a", "r"));   // fopen 失败时 p 为空，不会调用 Closer
```

`unique_ptr<Circle>` 转成 `unique_ptr<Shape>` 靠转换构造（要求 `Circle*` 能转成 `Shape*`）。转换之后删除器按基类指针 `delete`，所以基类必须有虚析构。

## make_unique and make_shared

`make_unique<T>(args...)` 约等于 `unique_ptr<T>(new T(std::forward<Args>(args)...))`：不出现裸 `new`，类型名只写一遍。

C++17 之前，`f(shared_ptr<A>(new A), g())` 的参数求值可以交错：`new A` 之后 `g()` 抛异常，会泄漏（`new` 和接管之间有窗口）。C++17 起各参数的求值不再交错，这个窗口消失；用 `make_shared` 在任何标准下都没有这个窗口。

## shared_ptr and the control block

引用计数放在**控制块**里（use count、weak count、删除器），要单独占存储。

```text
shared_ptr<T>(new T):   [对象]      [控制块]      两次分配
make_shared<T>():       [控制块 | 对象]           一次分配（标准建议）
```

一次分配更快、更紧凑、缓存更友好。代价：对象与控制块在同一块内存里，`use_count` 归零时对象**析构**，但内存要等 `weak_ptr` 也归零才释放。两次分配时，`use_count` 归零对象的内存就还了，控制块留到 weak 归零。`make_shared` 不能指定删除器，不能调用私有构造函数。

`shared_ptr` 的删除器记在控制块里（类型擦除），按**构造时的指针类型**决定怎么删：

```cpp
std::shared_ptr<Base> p1(new Derived);   // 构造时指针是 Derived*，控制块按 Derived 删除，Base 没有虚析构也行
Base* raw = new Derived;
std::shared_ptr<Base> p2(raw);           // 构造时指针是 Base*，按 Base 删除，Base 没有虚析构就是未定义行为
std::unique_ptr<Base> p3(new Derived);   // 默认删除器针对 Base*，Base 没有虚析构就是未定义行为
```

所以不能简化成"`shared_ptr` 不需要虚析构"。

## weak_ptr

`weak_ptr` 不拥有所观察的对象，不计入 `use_count`，不会阻止对象被销毁；但它参与控制块的生命周期管理：只要还有 `weak_ptr`，控制块就保留，`weak_ptr` 析构时会减少 weak count，最后一个归零才释放控制块。

互相持有 `shared_ptr` 会成环，计数永远降不到 0：

```cpp
struct B;
struct A { std::shared_ptr<B> b; };
struct B { std::shared_ptr<A> a; };   // 把其中一边改成 weak_ptr<A> 即可打破环
```

用时 `lock()` 得到一个临时的 `shared_ptr`，对象已死则为空：

```cpp
if (auto sp = wp.lock()) { sp->use(); }   // 临时共享所有权，用完即还
```

Core Guidelines R.24 注里两位作者的分歧：Sutter 认为本质是"临时共享所有权"，Stroustrup 认为"打破环是必须做的事，临时共享只是做法"，而且临时共享用另一个 `shared_ptr` 也行。可以这样合起来看：能打破环，因为它是非拥有的；非拥有的东西要访问对象，只能临时升级成拥有。（推论）

## Reference

- C++ Core Guidelines：R.20、R.21、R.22（`make_shared`）、R.24（`weak_ptr` 打破环）
- cppreference: [`unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr)、[`shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr)、[`weak_ptr`](https://en.cppreference.com/w/cpp/memory/weak_ptr)、[`make_shared`](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared)
- C++ standard (draft), by claim:
  - [unique.ptr.single.dtor](https://eel.is/c++draft/unique.ptr.single.dtor): 删除器调用不得抛异常的前提条件（据评审意见，我没有回查原文）
