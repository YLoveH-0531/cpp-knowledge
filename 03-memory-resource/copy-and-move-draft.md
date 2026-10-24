# Copy and move

> 种子草稿：从 [RAII-draft.md](RAII-draft.md) 拆出来的特殊成员函数细节，已按评审意见校正，还没按模板整理，待你扩写。README 把"拷贝与移动"放在 02-class-object-oop，这个文件放哪里由你定。

## Special member functions

隐式生成规则（"用户声明"包括 `= default` 和 `= delete`）：

| 你声明了 | 拷贝操作 | 移动操作 |
|---|---|---|
| 析构函数 | 仍生成（已废弃） | **不生成** |
| 拷贝构造或拷贝赋值 | 另一个仍生成（已废弃） | **都不生成** |
| 移动构造或移动赋值 | **都是 deleted** | 另一个**不生成** |

两种后果的危险程度不同：只声明析构，移动会静默退化成拷贝（能编译、行为对、只是慢）；声明移动，拷贝被删，编译期就报错。

`= delete` 的移动构造**不会回退到拷贝**：被删除的函数仍参与重载决议。想"允许拷贝、禁止移动"，应该只写拷贝、不写移动。

```cpp
struct S {
    S(S&&) = delete;
    S(const S&) = default;
};
S a;
S b = std::move(a);   // 错误：选中了被删除的移动构造，不会退回拷贝
```

首选**零法则**：让成员（`unique_ptr`、`vector`）管资源，类里一个特殊成员都不写。确实要直接管一个资源时，五个都写出来（五法则）。

## Polymorphic base classes

`virtual ~Base() = default;` 只为虚析构：通过基类指针删除派生类对象，没有它是未定义行为。

**切片**：按值把派生类对象赋给基类对象，派生部分被丢掉：

```cpp
Derived d;
Base b = d;   // 只复制了 Base 部分，b 的动态类型是 Base
```

防切片：基类的拷贝和移动设为 `protected` 或 `= delete`，另提供虚函数 `clone()`，在运行期按真实类型复制：

```cpp
std::unique_ptr<Shape> clone() const override {
    return std::make_unique<Circle>(*this);   // Circle 知道自己是 Circle
}
```

`= delete` 会让派生类的隐式拷贝也变成 deleted（它要调用基类拷贝），上面这行就编译不过，派生类要手写拷贝构造；`protected` 则派生类仍可用。这是 Core Guidelines C.67 优先推荐 `protected` 的原因。

## Move and noexcept

`vector` 扩容要搬运旧元素：移动快，拷贝安全。`std::move_if_noexcept` 的选择规则：

- 移动构造不抛异常，或者类型不可拷贝：采用移动；
- 移动构造可能抛异常，且类型可以拷贝：典型地选择拷贝，以帮助维持强异常保证。

所以移动构造、移动赋值和 `swap` 应该标 `noexcept`，否则容器会悄悄退化成拷贝。

## Note

待归位的两条小实验，不属于 RAII 主线：

- **返回值对象与会抛异常的析构**：标准规定 `return` 时若销毁局部变量或临时对象的析构抛异常，已构造的返回值对象也要被销毁。实测（g++ 11.4，本机 clang++，版本未查）并不一致：g++ 在返回 prvalue 时销毁、NRVO 路径不销毁；clang 这几种路径都没销毁。别依赖这条规则。
- **虚基类的析构**：虚基类只由最派生类的析构销毁（否则菱形继承里同一个子对象会被销毁多次）；构造也一样，由最派生类直接初始化。

## Reference

- C++ Core Guidelines：C.20、C.21（零法则、五法则）、C.67（防切片）
- cppreference: [The rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three)、[`std::move_if_noexcept`](https://en.cppreference.com/w/cpp/utility/move_if_noexcept)
