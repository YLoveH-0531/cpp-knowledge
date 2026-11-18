# 静态初始化、动态初始化与程序启动

> 范围：静态存储期和线程存储期变量的初始化阶段、顺序、时机和销毁，以及 Linux/ELF 下的典型实现。初始化的形式与种类见 [01-initialization-basics.md](01-initialization-basics.md)、[02-initialization-forms.md](02-initialization-forms.md)。条文号以 C++20（N4861）为基线，缺陷报告和现行草案的差异另注。环境：Ubuntu 22.04（WSL2）、x86-64、g++ 11.4、glibc 2.35、binutils 2.38、gdb 12.1。

## 1. 全貌

| 问题 | 结论 | 详见 |
|---|---|---|
| 分几个阶段？ | 静态初始化（常量初始化或零初始化）先于动态初始化 | §2.2 |
| 多个非局部变量谁先谁后？ | 静态存储期变量的动态初始化分有序、部分有序、无序三档 | §2.3.1 |
| 什么时候初始化？ | 静态存储期：`main` 之前，或推迟。线程存储期：每个线程各一次，线程初始函数之前，或推迟。均由实现定义 | §2.3.2 |
| 块作用域的 `static`、`thread_local` 呢？ | 静态初始化同上；动态初始化在控制流第一次经过声明时 | §2.4 |
| 初始化抛异常呢？ | 非局部变量：`std::terminate`；块作用域变量：异常传播，下次重试 | §2.5 |
| 落到可执行文件里是什么样？ | 对象在 `.data`、`.bss` 或线程局部存储，初始化函数的指针在 `.init_array` | §3 |
| 怎么启动、怎么销毁？ | 内核装载 → `ld.so` → `__libc_start_main`；销毁时线程存储期先于静态存储期，后者与 `atexit` 函数交错，有先后关系的按倒序 | §4 |

主线是 §2：静态初始化 → 动态初始化 → 顺序与时机 → 销毁（§4.2）。§3、§4 说明这些规则怎样落到实际程序里，不必掌握到同等深度。

## 2. 语言规则

### 2.1 讨论对象

本篇讨论**静态存储期**或**线程存储期**的变量，按声明位置分两类：

- **非局部变量**（non-local variable）：命名空间作用域的变量和类的静态数据成员，具有静态存储期，或带 `thread_local` 而具有线程存储期。
- **块作用域变量**：块作用域里声明为 `static`（静态存储期）或 `thread_local`（线程存储期）的变量。其余的不在讨论范围内：没有 `static`、`thread_local`、`extern` 的变量是自动存储期（[basic.stc.auto]/1），`extern` 声明引用的是别处定义的变量。

声明位置（非局部、块作用域）和存储期（静态、线程、自动）是两个维度。现行草案把 non-local variable 改称 non-block variable（对静态、线程存储期的变量，范围相同），本篇沿用 C++20 和 cppreference 的 non-local。

### 2.2 静态初始化与动态初始化

对具有静态或线程存储期的变量（[basic.start.static]/2）：

1. 满足 constant-initialized 的条件，做**常量初始化**；
2. 否则，做**零初始化**。

两者合称**静态初始化**，其余的初始化是**动态初始化**。静态存储期的变量随程序启动初始化，线程存储期的随线程执行初始化（/1）；在这两个阶段内，所有静态初始化都 strongly happens before 任何动态初始化。

**constant-initialized**（[expr.const]/2）：变量有初始化器（或其默认初始化会执行某些初始化），并且初始化的完整表达式是常量表达式（可以调用 `constexpr` 构造函数）。例如 `int a = 10;` 满足；`int b;` 没有初始化器，对 `int` 的默认初始化什么也不做，不满足；`C gc;` 的 `constexpr` 默认构造函数会执行初始化，且整个是常量表达式，满足（`struct C { int x; constexpr C() : x(10) {} };`）。

没做常量初始化的变量先零初始化；其中还要执行初始化的，再做动态初始化（[dcl.init]/10 的注）。零初始化的结果见 [01-initialization-basics.md](01-initialization-basics.md) §2.2。静态初始化通常在编译期算好、随程序映像装载（§3.3）；标准只保证它先于动态初始化。

验证：C++20 的 `constinit` 要求变量没有动态初始化，否则编译不过（[dcl.constinit]/2）。g++ 11.4：`constinit int a = 10;`、`constinit int b;`、`constinit C gc;` 通过；`constinit int c = f();`（`f` 非 `constexpr`）、`constinit U gu;`（`U()` 非 `constexpr`，即 [01-initialization-basics.md](01-initialization-basics.md) §2.2 的 `U`）报错。

### 2.3 非局部变量的动态初始化

#### 2.3.1 顺序

下面的分类只针对**静态存储期**的非局部变量（[basic.start.dynamic]/1–/3）。线程存储期的变量没有这套顺序规则；cppreference 把 `thread_local` 也写进了这三档（称不同翻译单元间 unsequenced），C++20 和现行草案的原文都没有，以标准为准。

| 档位 | 对象 | 顺序保证 |
|---|---|---|
| **有序**（ordered） | 其他非局部变量 | 同一翻译单元内，定义靠前的先初始化 |
| **部分有序**（partially-ordered，C++17） | inline 变量（隐式或显式实例化的特化除外） | 若 V 部分有序、W 非无序，且 W 的每个定义之前都有 V 的某个定义，则 V 先于 W |
| **无序**（unordered） | 类模板的静态数据成员、变量模板（C++14）的隐式或显式实例化；非 inline 的显式特化除外（它有序） | 无保证 |

“定义 D 先于定义 E”（appearance-ordered，/2）：D 与 E 在同一翻译单元且 D 在前；C++20 起，E 所在的翻译单元接口依赖（模块）D 所在的，也算。

两个变量 V、W 的初始化次序（/3）：

- 满足上表的保证：V sequenced before W。若已启动非主线程，且 V、W 不是同一翻译单元内的有序变量，则为 strongly happens before。
- 否则，若在 V、W 中任一个初始化之前已启动非主线程：它们在哪个线程执行未指定，在同一线程内也是 unsequenced。
- 否则：indeterminately sequenced。

跨翻译单元的例子：

```cpp
// a.cpp
extern B b;
A a;            // A 的构造函数使用 b

// b.cpp
B b;
```

`a`、`b` 在不同翻译单元，动态初始化的先后没有保证。`a` 先初始化、`b` 的动态初始化还没执行时，`A::A()` 读到的是只被零初始化的 `b`；但实现可以把 `b` 提前成静态初始化（§2.3.2.1），那时读到的就是完整的 `b`。实测（`B()` 只写入常量）：g++ 11.4 的结果随链接顺序变，clang 14 `-O2` 把 `b` 放进 `.data`，两种顺序都读到完整的 `b`。解法见 §2.4：把 `b` 换成函数里的 `static`。

#### 2.3.2 时机

静态存储期变量随程序启动初始化，线程存储期变量随线程执行初始化（[basic.start.static]/1）。动态初始化的时机由实现定义：

| 存储期 | 时机（[basic.start.dynamic]/5–/7） |
|---|---|
| 静态 | `main` 第一条语句之前，或推迟 |
| 线程 | 该线程初始函数第一条语句之前，或推迟；每个线程各初始化一次 |

g++ 11.4 实测：静态存储期变量（含 inline 变量）都在 `main` 之前构造；`thread_local` 变量推迟到本线程第一次 ODR-use 同一翻译单元里有动态初始化的 `thread_local` 变量时，该翻译单元里这类变量一起构造（只用到常量初始化的不触发），没用到的线程不构造。

##### 2.3.2.1 提前初始化（early dynamic initialization）

满足下面两个条件时，实现可以把本该动态的初始化当作静态初始化来做（[basic.start.static]/3）：

- 动态版本的初始化，不会改变其他静态或线程存储期对象在它们自己初始化之前的值；
- 静态版本得到的值，与“所有不要求静态初始化的变量都按动态初始化”时得到的值相同。

```cpp
inline double fd() { return 1.0; }
extern double d1;
double d2 = d1;
double d1 = fd();
```

`d1` 被提前成静态初始化、`d2` 仍动态初始化时，两者都是 1.0；其余组合 `d2` 都是 0.0。标准把 `d2` 的值规定为 unspecified，不应依赖。

##### 2.3.2.2 推迟初始化与 ODR-use

推迟的动态初始化必须在**最晚期限**之前完成，期限由 non-initialization ODR-use 定（[basic.start.dynamic]/4–/7）；在哪个线程、哪个程序点执行，由实现定义。

- **ODR-use**：要求实体有定义的使用（[basic.def.odr]/4、/7），通常包括调用函数、取地址、读写变量、绑定引用；判定细则和例外见 [odr.md](../../../07-language-mechanics/odr.md)。
- **non-initialization ODR-use**：不是由非局部变量的初始化直接或间接引起的 ODR-use，如 `main` 或线程函数里的使用；初始化器调用的函数、这些函数再调用的函数，不算。

| 变量 | 最晚期限（须在此之前完成初始化） |
|---|---|
| 非 inline、静态存储期 | 同一翻译单元里**任何非 inline 函数或非 inline 变量**的第一次 non-initialization ODR-use |
| inline、静态存储期（C++17） | **该变量自己**的第一次 non-initialization ODR-use |
| 非 inline、线程存储期 | 本线程对同一翻译单元里**任何非 inline、有动态初始化的线程存储期变量**的第一次 non-initialization ODR-use† |

† C++20 原文（/7）没有“有动态初始化”这个限定；CWG 2684（2026-06 接受为 DR）加上了，本篇按修正后的规则。

推迟的是执行时机，不是允许晚于使用。例子：

```cpp
// service.cpp
int get_value();
int global = get_value();
void run() {}
```

若 `global` 的动态初始化被推迟，调用 `run()` 之前它就必须完成：`run()` 是同一翻译单元里的非 inline 函数，对它的 ODR-use 触发期限，尽管 `run()` 没碰 `global`。此时即使 `global` 自己没被 ODR-use，只要初始化有副作用，也不能省略（/5 的脚注，[basic.stc.static]/2）。inline 变量只看自己：ODR-use 同一头文件里的 inline 函数，不会触发另一个 inline 变量的初始化。

cppreference 把期限写成“任何静态/线程存储期的**变量**”，没提函数，以标准条文（非 inline 函数或非 inline 变量）为准；它还说，翻译单元里的东西始终没被 ODR-use 时，被推迟的变量可能永不初始化（模拟按需加载的动态库）。

### 2.4 块作用域的 static 与 thread_local 变量

块作用域里的 `static` 变量是静态存储期；`thread_local` 变量是线程存储期，未写其他存储类说明符时隐含 `static`（[dcl.stc]/3）。

| 情形 | 规则 |
|---|---|
| 零初始化、常量初始化 | 属于静态初始化，同非局部变量，先于任何动态初始化 |
| 动态初始化 | 控制流**第一次经过声明**时执行，完成后才算已初始化；之后再经过则跳过（[stmt.dcl]/4）。实现可按 §2.3.2.1 提前 |
| 多个线程同时首次经过（C++11） | 只初始化一次，其他线程等待完成 |
| 初始化期间递归经过同一声明 | 未定义行为 |
| 线程存储期 | 每个线程各有一份，各自在本线程首次经过声明时初始化；没经过的线程不构造也不销毁 |
| 销毁 | 只销毁构造完成的对象（[stmt.dcl]/5）：静态存储期的在程序退出时，线程存储期的在线程退出时 |

```cpp
void f() {
    static int k = 10;    // 常量初始化：f 第一次被调用之前已完成
    static int d = g();   // 动态初始化：第一次执行到这里才调用 g()
}
```

有外部链接的 `inline` 函数（含类内定义的成员函数，它隐式 inline）里的 `static` 变量，全程序只有一个对象（[dcl.inline]/6、/7 的注）。

用途：构造时间由调用决定，不依赖翻译单元之间的次序，可解决 §2.3.1 的跨翻译单元问题：

```cpp
// b.cpp
B& get_b() { static B b; return b; }

// a.cpp
A a;            // A 的构造函数调用 get_b()，此时 b 才构造
```

销毁顺序仍由构造完成的先后决定（§4.2）：`b` 在 `a` 构造期间完成构造，所以晚于 `a` 销毁。若 `A` 只在析构函数里用 `b`、构造函数没调用 `get_b()`，则没有这个保证。

### 2.5 初始化异常

| 变量 | 初始化抛异常 |
|---|---|
| 非局部变量（静态或线程存储期） | 调用 `std::terminate`（[basic.start.dynamic]/8） |
| 块作用域变量 | 异常正常传播；这次初始化不算完成，下次经过声明时重试（[stmt.dcl]/4） |

## 3. 实现层面：Linux/ELF

以下是典型的 Linux / ELF / x86-64 情形；C++ 标准不规定节名、段布局和对象放在哪里。

### 3.1 节与段

- **节**（section）：文件内容的组织单位，如 `.text`、`.data`。
- **段**（segment）：装载时映射进进程地址空间的区域，由程序头（如 `PT_LOAD`）描述。

| 节 | 内容 |
|---|---|
| `.text` | 机器指令，包括动态初始化函数 |
| `.rodata` | 只读数据，如字符串字面量 |
| `.data` | 有非零初值的可写数据 |
| `.bss` | 零值的可写数据；`SHT_NOBITS`，文件里不占空间，装载时补零 |
| `.init_array` | 函数指针数组，指向动态初始化函数 |
| `.tdata` / `.tbss` | 线程局部数据的初始映像（有初值 / 零值） |

### 3.2 对象放哪

| 存储期 | 典型位置 |
|---|---|
| 自动 | 栈（也可能在寄存器里，或被优化掉） |
| 静态（全局变量、静态数据成员、函数内 `static`） | `.data` / `.bss`；字符串字面量通常在 `.rodata` |
| 线程 | 每个线程自己的 TLS 块，初始映像来自 `.tdata` / `.tbss` |
| 动态（`new`） | 分配器管理的内存，通常称堆 |

### 3.3 与初始化的关系

- `.data` / `.bss` 按值放，不按 C++ 的初始化类别放：可写的零值（零初始化，或常量初始化成 0）通常进 `.bss`，可写的非零值进 `.data`。`const` 不等于一定进 `.rodata`：只读且无需重定位的才进（如 `extern const int k = 5;`）；PIE 下含地址的只读数据（如 `int* const p = &g;`）要重定位，进 `.data.rel.ro.local` 等节；需要动态初始化的 `const` 对象要先被写入，仍在 `.bss`。
- 需要动态初始化的对象，先完成零初始化（映像全零，通常在 `.bss`），再由初始化函数写入最终值；被提前成静态初始化的，直接按最终值放，没有初始化函数。
- `.init_array` 存函数指针，不存对象：对象在数据节，函数的机器码在 `.text`，`.init_array` 指向这些函数。

实测（g++ 11.4，`-O0`）：`int a = 10;` 在 `.data`，`int b;` 和 `int c = f();` 在 `.bss`；`c` 由本翻译单元的初始化函数（登记在 `.init_array`）在 `main` 之前写入。`extern const int k = 5;` 在 `.rodata`，`const S cs;`（`S` 的默认构造函数非 `constexpr`）在 `.bss`，`int* const p = &g;` 在 PIE 下进 `.data.rel.ro.local`，`-fno-pie` 下进 `.rodata`。

## 4. 从装载到退出

### 4.1 启动路径

以下是 glibc 2.35 / x86-64 下的典型路径，不是标准规定的步骤（标准允许推迟动态初始化，§2.3.2）：

| 步骤 | 执行者 | 做什么 |
|---|---|---|
| 1 装载 | 内核（`execve`） | 按 `PT_LOAD` 映射各段，`.bss` 补零，建立栈、参数和环境变量；动态链接的程序先把控制权交给 `ld.so` |
| 2 动态链接 | `ld.so` | 装载共享库，解析符号、重定位，然后运行各共享库的初始化函数（`_dl_init`，`elf/dl-init.c`） |
| 3 运行库启动 | `_start` → `__libc_start_main` | 初始化运行库；`call_init` 遍历主程序的 `.init_array`，完成动态初始化（`csu/libc-start.c`） |
| 4 `main` | 用户代码 | 块作用域变量在首次经过声明时初始化；线程存储期变量按线程初始化 |
| 5 退出 | 运行库 | 见 §4.2 |
| 6 终止 | 内核 | 回收地址空间和剩余资源 |

静态初始化需要的数据在步骤 2 结束时已就位：`.data` 里的值、全零的 `.bss`，PIE 里含地址的值由 `ld.so` 重定位。这是实现手段，不是语言规定的阶段。

### 4.2 退出与销毁顺序

从 `main` 返回：先销毁 `main` 的自动存储期对象，再调用 `std::exit`（[basic.start.main]/5）。`std::exit` 依次做：

1. 销毁调用线程的 `thread_local` 对象。其他线程的，只在各自从初始函数返回或各自调用 `std::exit` 时销毁，这次 `std::exit` 不会替仍在运行的线程销毁。每个线程的这些销毁都 strongly happens before 任何静态存储期对象的销毁（[basic.start.term]/2）。
2. 销毁静态存储期对象，并执行 `std::atexit` 登记的函数（/1）。两者交错进行；标准只对下面几种先后关系规定了倒序（/3、/5）：

   - 对象 A 的初始化完成 strongly happens before 对象 B 的，则 B 先销毁；
   - 对象的初始化完成 strongly happens before 某次 `atexit` 调用，则该函数先于这个对象的析构执行；
   - 某次 `atexit` 调用 strongly happens before 对象的初始化完成，则这个对象先析构，该函数后执行；
   - 某次 `atexit` 调用 strongly happens before 另一次，则后一次登记的函数先执行；
   - 静态初始化的对象，销毁顺序与按动态初始化时相同。

不能理解成“先析构完所有静态对象，再统一调用 `atexit` 函数”。g++ 11.4 实测：

| 事件（按发生先后） | 退出时的执行次序 |
|---|---|
| 对象 A 初始化完成 | 5. 析构 A |
| 登记 `h1` | 4. 调用 `h1` |
| 对象 B 初始化完成 | 3. 析构 B |
| 登记 `h2` | 2. 调用 `h2` |
| 块作用域的静态对象 C 初始化完成 | 1. 析构 C |

实现：g++ 在每个需要析构的静态存储期对象构造完成后，调用 `__cxa_atexit` 登记它的析构函数；glibc 的 `atexit` 只是 `__cxa_atexit` 的包装，二者共用一张表，所以顺序天然是倒序。`thread_local` 对象另用 `__cxa_thread_atexit`。

直接调用 `std::exit` 不会销毁调用者栈上的自动存储期对象（[support.start.term]）；`std::abort` 不销毁任何对象，也不调用 `atexit` 登记的函数（[basic.start.term]/7）。

## Reference

- cppreference：[Initialization](https://en.cppreference.com/w/cpp/language/initialization)（Non-local variables）、[Storage duration](https://en.cppreference.com/w/cpp/language/storage_duration)（Static block variables）、[std::exit](https://en.cppreference.com/w/cpp/utility/program/exit)
- C++20 草案 N4861：[basic.start.static](https://timsong-cpp.github.io/cppwp/n4861/basic.start.static)、[basic.start.dynamic](https://timsong-cpp.github.io/cppwp/n4861/basic.start.dynamic)、[basic.start.term](https://timsong-cpp.github.io/cppwp/n4861/basic.start.term)、[basic.start.main](https://timsong-cpp.github.io/cppwp/n4861/basic.start.main)、[stmt.dcl](https://timsong-cpp.github.io/cppwp/n4861/stmt.dcl)、[basic.def.odr](https://timsong-cpp.github.io/cppwp/n4861/basic.def.odr)、[basic.stc.static](https://timsong-cpp.github.io/cppwp/n4861/basic.stc.static)、[expr.const](https://timsong-cpp.github.io/cppwp/n4861/expr.const)、[dcl.init](https://timsong-cpp.github.io/cppwp/n4861/dcl.init)、[dcl.stc](https://timsong-cpp.github.io/cppwp/n4861/dcl.stc)、[dcl.constinit](https://timsong-cpp.github.io/cppwp/n4861/dcl.constinit)、[dcl.inline](https://timsong-cpp.github.io/cppwp/n4861/dcl.inline)、[support.start.term](https://timsong-cpp.github.io/cppwp/n4861/support.start.term)
- 缺陷报告：[CWG 2684](https://cplusplus.github.io/CWG/issues/2684.html)（thread_local dynamic initialization）
- ELF：[Sections](https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.sheader.html)、[Program Header](https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.pheader.html)
- glibc 2.35：`csu/libc-start.c`（`call_init`，主程序的初始化函数）、`elf/dl-init.c`（`_dl_init`，共享库的初始化函数）
