# Memory & Resource Management

内存、生命周期、RAII、智能指针、资源管理

## 解决什么问题

谁拥有资源，何时释放。C++ 没有垃圾回收：先把"存储"和"对象的生命周期"分开（new/delete 的两半），再用 RAII 把"谁负责释放"绑到对象的析构上，智能指针把这件事标准化。检验标准是异常安全：构造抛异常、提前返回时，资源也不能泄漏。

核心问题：没有 GC，C++ 靠什么不泄漏？

## 在地图上的位置

```text
01 Basic (pointer, array)          --+
                                     +--> new/delete --> RAII --> smart pointer
02 Class (constructor, destructor) --+        |
                                              +--> allocator --> 05 STL
```

- 02 的析构函数是 RAII 的前提：对象销毁时自动做清理。
- 03 是 05 的底座：容器靠 allocator 把"拿存储"和"造对象"分开；`vector` 把动态数组的存储、大小和释放封装成 RAII（见 [array.md](../01-basic/array.md) 的 Why）。
- 智能指针转移所有权要用到移动语义（02 的拷贝与移动、07 的值类别，都还没写）。

## 主题清单

| 主题 | 为什么存在 | 状态 |
|---|---|---|
| [new/delete](new-delete.md) | 创建对象 = 分配存储 + 初始化，销毁反过来；`operator new`/`operator delete` 只管存储那一半 | 笔记完成，Example 待写 |
| 内存布局 | 对象的存储从哪来、能活多久：栈、堆、静态存储 | 待写 |
| RAII | 把"谁负责释放"绑到对象的生命周期上，由析构函数保证 | 待写 |
| 智能指针 | RAII 的标准化身，用类型表达所有权：`unique_ptr`、`shared_ptr`、`weak_ptr` | 待写 |
| [异常安全](../07-language-mechanics/exception-safety-draft.md) | 检验标准：异常中止后资源不泄漏（RAII 管这一半），对象状态仍有效或不变（基本、强保证）；笔记放在 07 | 待写 |

## 读完应能回答

- 没有 GC，C++ 靠什么不泄漏？
- `new T` 的构造函数抛异常时，已经分配的那块存储由谁释放？（new/delete 的 Example 要验证）
