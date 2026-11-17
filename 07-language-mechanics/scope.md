# Scope

本节的术语（locus、inhabit、target scope、bound、belongs to，以及 parent scope、immediate scope、intervenes）来自 P1787R6，2020-12 并入标准草案，属于 C++23；C++20 用的是 declarative region、potential scope、point of declaration 这套词。

初衷（P1787R6 引言）：旧的 [basic.scope] 把"scope"定义了两遍，对命名空间是一个还是多个 declarative region 前后不一，再加上模块让"只看本翻译单元里靠前的声明"这个默认假设不再成立，所以整体重写。

四层各回答一个问题：

| 层 | 要回答的问题 | 关键术语 |
|---|---|---|
| ① 结构 | 作用域怎么组织？ | global scope、contains、parent scope |
| ② 定位 | 代码某一处，落在哪些作用域里？ | program point、enclosing scope、immediate scope、intervenes |
| ③ 归属 | 一条声明属于哪个作用域？ | locus、inhabit、target scope |
| ④ 绑定 | 名字和实体如何关联到作用域？ | bound、belongs to |

## ① 结构：作用域是一棵嵌套的树

- 整个程序有一个根：global scope。
- 其他作用域由声明、参数、语句、handler、lambda 表达式引入（现行草案还有 contract assertion）。
- 每个作用域都出现在另一个作用域中，后者包含前者。
- Parent scope 是包含该作用域的最小外层作用域（模板参数作用域是例外：它只做其他模板参数作用域的 parent，暂不展开）。

```text
global
 └── S       parent(S) = global
      └── T  parent(T) = S
```

## ② 定位：站在某个程序点上看

- Enclosing scope：包含该程序点的所有作用域。
- Immediate scope：包含该程序点的最小作用域，即最内层作用域。

例如，程序点 Y 位于 T 中：

```text
global
 └── S
      └── T
           Y
```

那么：

- Enclosing scopes：`{T, S, global}`。
- Immediate scope：`T`。

另一个辅助术语 intervenes（理解定义和图示即可）：作用域 K 介于程序点 P 和作用域 S（S 不含 P）之间，指 K 就是 S 或包含 S，但不包含 P（[basic.scope.scope]/1）。

```text
global
 ├── P
 └── S
      └── T
```

P 在 global 里：S 和 T 都介于 P 与 T 之间（都是或包含 T，都不含 P），global 不是（它含 P）。标准里用到它的地方之一是现行草案的 [basic.def.odr]/10：局部实体能不能在 lambda、函数参数这类作用域里被 odr-use，要逐个看介于它被引入的位置和该作用域之间的作用域。

## ③ 归属：声明落到哪个作用域

1. Locus：声明在程序中对应的标准规定位置。例如 `int x = 10;`，其 locus 在 `x` 之后、初始化器之前。
   - 所以 `int x = x;` 右边的 `x` 已经是新声明的那个 `x`（还没初始化）。枚举器不同，它的 locus 在整个定义之后：`enum { x = x + 1 };` 右边的 `x` 还是外层的（g++ 实测）。
2. Inhabit：默认情况下，声明 inhabits 其 locus 所在程序点的 immediate scope。
3. Target scope：默认情况下，声明的 target scope 就是它 inhabits 的作用域。

例如：

```cpp
{               // S
    {           // T
        int y;
    }
}
```

`int y;` 的 locus 位于 T 中，因此默认：

```text
Immediate scope = T
Inhabited scope = T
Target scope = T
```

注意：以上 target scope 与 inhabited scope 相等，是默认规则（标准里写的是 "Unless otherwise specified"），并非无例外。现行草案 [basic.scope.scope]/2 的注和 [dcl.meaning.general] 列了特例，常见的有：

- 带限定名的声明，即在类外（或命名空间外）定义成员：`int C::count = 0;`、`void C::f() {}` 写在命名空间作用域里，inhabit 的是那个命名空间，target scope 却是它所对应的声明所在的作用域（类 `C`），所以实体属于 `C`（[dcl.meaning.general]/3）。
- 块作用域里的 `extern` 声明和函数声明：target scope 是外层的命名空间作用域（实体属于该命名空间），名字却只绑定在所在的块里。例：`namespace X { void p() { extern int q; } }`，`q` 属于 `X`（目标文件里的符号是 `X::q`），但在 `p` 外面用 `q` 会报未声明（g++、clang 实测）。
- 友元声明：不绑定名字，实体属于最内层的外围命名空间。所以 `friend void ff();` 之后，类外直接写 `ff()` 找不到，只有参数相关查找（ADL）能找到（实测）。
- 无作用域枚举的枚举器：名字绑定在枚举和外层两处，`E::e1`（C++11 起）和 `e1` 都能用；`enum class` 的只在枚举里。
- 匿名联合的成员：名字绑定在联合的父作用域。

## ④ 绑定：名字和实体

- Bound：声明引入的名称，默认在 target scope 中绑定到该声明。
- Belongs to：如果实体某条声明的 target scope 是 S，那么该实体属于 S。这里的「属于」是语言层面的作用域归属，与对象的内存位置、存储期和生命周期无关。

| 概念 | 关联的两端 | 回答的问题 |
|---|---|---|
| Bound | 名字 → 声明 | 这个名字在这个作用域里是哪条声明？ |
| Belongs to | 实体 → 作用域 | 这个实体属于哪个作用域？ |

绑定的是名字和声明，不是名字和实体：名字查找把一次使用关联到一组声明（[basic.lookup.general]/1），再由声明确定所指的实体。

绑定与归属是两件事：绑定决定名字能在哪里被找到，归属决定实体在语义上算哪个作用域的成员。默认情况下两者都由声明的 target scope 决定，所以一个声明在什么范围内"生效"，既定了能在哪里用这个名字，也定了被声明的实体归入哪个作用域；上面的特例（块作用域里的 `extern` 和函数声明、友元声明）正是两者分开的地方。

一个实体可以属于多个作用域，因为归属看的是它的每一条声明。例：具有 C 语言链接的同名函数，在不同命名空间里的声明指的是同一个函数（[dcl.link]/7 的例子；C++20 就有这条规则，只是没有 target scope、belongs to 这套词）：

```cpp
namespace A { extern "C" void f(); }
namespace B { extern "C" void f(); }   // 与 A::f 是同一个函数
void A::f() {}                         // 唯一的定义
int main() { B::f(); }                 // 调用的就是上面这个定义
```

`A::f` 和 `B::f` 是两个名字，各自绑定在自己命名空间里的那条声明上；两条声明声明的是同一个函数实体，按 Belongs to 的定义它既属于 A，也属于 B（这一步是按 [basic.scope.scope]/2 推出的，标准没有对这个例子明说）。实测（g++ 11.4、clang 14）：只在 A 里写了定义，调用 `B::f` 却能链接并运行，`nm` 看目标文件，除 `main` 外只有一个符号 `f`；去掉两处 `extern "C"`，`A::f()` 和 `B::f()` 是两个实体，链接失败，报 `B::f()` 未定义。

**一句话：** 作用域是一棵嵌套的树；程序点定出 immediate scope，声明的 locus 定出它 inhabit 哪个作用域，默认 target scope 就是它；名字在 target scope 里绑定到声明，实体属于它各条声明的 target scope。
