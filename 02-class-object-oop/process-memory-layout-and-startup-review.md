# 程序的内存分布与启动过程：GPT 评审与核对批注

> 整理于 2026-11-09。环境：Ubuntu 22.04（WSL2）、x86-64、g++ 11.4、glibc 2.35、binutils 2.38、gdb 12.1；C++ 标准以 C++20 为基线，版本差异单独写明。

## 0. 来龙去脉与读法

- **来龙去脉**：读 cppreference 的 "Non-local variables" → 整理进 `lifetime-and-initialization-guide.md` 的"补充：非局部变量的两阶段初始化" → 追问"程序的内存分布，结合加载、运行、结束讲一遍" → 我答了一版（§2）→ 你把 GPT 对这一版（和那一节）的评审贴过来（§3）→ 我逐条核对后写成这份文件。
- **GPT 原文不改。** 只做了三处纯格式处理：① 粘贴时被压成逐行文字的表格和示意图，按原词重排成 Markdown 表格或列表；② 删去粘贴带进来的来源标签（"eel.is +1"、"ELF Object File Format 4.2 documentation"、"Linux manual page"），这些出处我另行核对，写在批注里；③ 没有别的改动。
- **我的批注**都用引用块，开头标：
  - **【核实】**：查证后成立。
  - **【补充】**：原文没错，但可以更精确，或有实测。
  - **【更正】**：说得不对或不准。
- **实验**在附录 A（实验 1/2/3），标准与规范原文摘录在附录 B。

## 1. 结论速览

GPT 的评审整体可靠：我没有发现事实性错误，只有 **1 处顺序描述不准确**（主线程的 `thread_local` 与静态对象谁先析构，见 4.4）。其余是"说得保守"或"可以再具体"，补充如下：

| # | 性质 | 要点 | 证据 |
|---|---|---|---|
| 1 | 更正 | 退出时（`std::exit`）**先**销毁当前线程的 `thread_local`，**再**销毁静态存储期对象；GPT 把线程局部对象排在静态对象之后 | 标准 [support.start.term]、实验 3 |
| 2 | 补充 | `.data`/`.bss` 按**值**分，不按 C++ 的初始化类别分：`int g_zero_const = 0;` 是**常量初始化**，却在 `.bss` | 实验 1 |
| 3 | 补充 | `.init_array` 在 GCC 下**每个翻译单元一项**（`_GLOBAL__sub_I_*`），外加 crt 的 `frame_dummy`；不是每个全局对象一项 | 实验 2 |
| 4 | 补充 | `.init_array` 在**可写**的 `PT_LOAD` 里，装载后被 `PT_GNU_RELRO` 设成只读；它不和 `.text`/`.rodata` 同段 | 实验 1（`readelf -lW`、`/proc/pid/maps`） |
| 5 | 补充 | `.tdata`/`.tbss` 只是线程局部变量的**初始映像**，活的实例在每线程自己的 TLS 块里；GCC 对有动态初始化的 `thread_local` 是**每线程首次使用时**才初始化 | 实验 1、实验 3 |
| 6 | 澄清 | GPT 第五节的两条是措辞建议和语境说明，不是纠错：我原文的语义本来就是"常量初始化，否则零初始化"，而且那句话说的是非局部变量 | 附录 B |
| 7 | **我自己的错** | guide 里写"非局部变量：…（及 `thread_local`）｜程序启动时"不准：`thread_local` 是线程启动时或首次使用时，每线程各一次（GPT 在三里点到了这一处） | 实验 3、[basic.start.dynamic]/7 |

对我 §2 原回答的修正清单见 §4；guide 里要改的两处见 §5（我没有动 guide，等你点头）。

## 2. 我 11-09 的原回答（原样保留）

<details>
<summary>点开查看原回答（不改动；被指出的地方见 §4）</summary>

以下是典型的 Linux / ELF / x86-64 情形。内存分段是实现细节，C++ 标准没有规定。

### 一、内存布局（从低地址到高地址）

```text
低地址
┌──────────────┐
│ .text        │ 代码（机器指令），只读+可执行
│ .rodata      │ 只读常量：字符串字面量、const 全局变量，只读
│ .init_array  │ 函数指针表：每个全局对象的"动态初始化函数"（C++ 全局构造）
├──────────────┤  ← 以上来自可执行文件，装载时直接映射
│ .data        │ 有非零初值的全局/静态变量，可读写
│ .bss         │ 零初始化的全局/静态变量，可读写，文件里不占空间
├──────────────┤
│ 堆 heap      │ new / malloc，向高地址增长
│     ↓        │
│   （空洞）    │
│     ↑        │
│ mmap 区      │ 共享库（libc.so 等）、mmap 的文件和大块内存
│ 栈 stack     │ 局部变量、参数、返回地址，向低地址增长
├──────────────┤
│ 内核空间      │
└──────────────┘
高地址
```

一般把 `.text`、`.rodata` 称为"程序段"，`.data`、`.bss` 称为"数据段"。

### 二、各变量放哪

```cpp
int a = 5;              // .data（常量初始化）
int b;                  // .bss（零初始化）
const int c = 7;        // .rodata（也可能被直接优化成立即数）
int d = f();            // .bss 先是 0，运行时由 .init_array 里的函数赋值（动态初始化）

void g() {
    int x;              // 栈
    static int s = 1;   // .data（函数内 static，生命周期同全局）
    int* p = new int;   // p 在栈，*p 在堆
    const char* t = "hi"; // t 在栈，"hi" 在 .rodata
}
```

### 三、从加载到结束的全过程

**1. 加载（`execve`，内核完成）**

- 读 ELF 头，按"程序头"把文件各段 `mmap` 进新地址空间：代码和 `.rodata` 映射为只读，`.data` 映射为可写（写时复制）。
- `.bss` 文件里没有数据，内核分配全零页。
- 建好栈，把 `argv`、环境变量放上去。
- 如果是动态链接的程序，还会加载动态链接器 `ld.so`，由它把 `libc.so` 等共享库映射到 mmap 区，并做重定位。

**到这里，C++ 所说的"阶段 1：静态初始化"已经自然完成了：**

- 常量初始化的变量：值就在 `.data` / `.rodata` 里，随映射就位，没有任何代码在运行。
- 零初始化的变量：在 `.bss` 里，页是全零。

**2. 启动代码**

控制权先到 `_start`，再到 `__libc_start_main`，它依次：

- 初始化 libc。
- **遍历 `.init_array`，逐个调用动态初始化函数**，这就是"阶段 2：动态初始化"，全局对象的构造函数在这里按顺序执行。同一个翻译单元里按定义顺序，跨翻译单元的顺序不确定，就是前面讲的问题。
- 最后才调用 `main`。

**3. 运行期**

- 每次函数调用在栈上压帧，局部变量随之产生和消失。
- `new` / `malloc` 向堆要内存，堆不够时通过 `brk` / `mmap` 向内核扩展。
- 函数内 static 变量的空间在 `.data` / `.bss`，只是"第一次经过声明"时才初始化（编译器用一个 guard 变量判断）。

**4. 结束**

- `main` 返回（或调用 `exit`）后，运行库按**注册的反序**调用 `atexit` 函数和静态对象的析构函数。
- 然后 `_exit` 进内核，内核回收整个地址空间，包括没 `delete` 的堆内存，所以泄漏型单例不会真的"泄漏"到进程之外。
- 栈上的局部对象，早在各自的作用域结束时就析构了。

### 四、一句话串起来

> 可执行文件的各个段被映射进内存，常量初始化和零初始化"天然完成"（静态初始化）；启动代码再跑 `.init_array` 里的函数完成动态初始化；然后才是 `main`；退出时反序析构，最后内核回收一切。

你可以用 `readelf -S a.out` 看各节，用 `size a.out` 看 text/data/bss 大小，用 `cat /proc/<pid>/maps` 看运行时的地址布局。

</details>

## 3. GPT 的评审（原文）与我的批注

整体思路是对的，而且你把 C++ 的初始化规则和 Linux 进程的内存布局联系起来，这个方向很有价值。不过，这份整理里有几处需要修正，尤其是内存段与 C++ 初始化阶段并不是一一对应的。

我建议保留你的整体结构，只修正容易形成错误印象的地方，不必再扩展成一篇操作系统内存管理笔记。

先说结论：

- .text、.rodata、.data、.bss 的基本用途，你理解得基本正确。
- 堆、栈、共享库映射区的整体关系也基本正确，但地址增长方向和变量实际存放位置不能当作语言保证。
- .init_array 与全局对象的动态初始化有关，但它存放的是初始化函数的函数指针，不是对象本身，也不保证每个全局对象恰好对应一个函数。
- 最需要修正的是：静态初始化不是简单的“编译器把值写进 .data 或 .bss”，动态初始化也不是所有情况下都必须在 main 前执行。

下面逐项看。

> **【核实】** 四点都成立，证据在下面各小节的批注里。
>
> **【补充】** 第 4 点"静态初始化不是简单的把值写进 .data/.bss"，我的理解有三层（我的归纳，GPT 没有逐字这样说）：① 装载时还要做重定位，不只是映射（PIE 下 `.init_array` 的每个槽都有 `R_X86_64_RELATIVE` 重定位，见 1.3 批注）；② 标准允许把本该动态的初始化提前成静态（附录 B）；③ 节是按值分的，不按初始化类别分（见 1.2 批注）。

### 一、先看内存布局：大方向对，有四处需要调整

#### 1. .text、.rodata、.data、.bss

这四个概念可以这样理解：

| 区域 | 主要内容 | 典型属性 |
|---|---|---|
| .text | 程序的机器指令 | 可读、可执行，通常不可写 |
| .rodata | 只读数据，例如字符串字面量、只读常量数据 | 可读，通常不可写 |
| .data | 有初始数据的可写全局或静态变量 | 可读、可写 |
| .bss | 初始值为零的全局或静态变量 | 可读、可写 |

这里有一个重要细节：

不能简单地认为 const 变量一定放在 .rodata。

例如：

```cpp
const int a = 10;
int b = 10;
```

编译器可能把 a 放在 .rodata，把 b 放在 .data。但这属于编译器和链接器的具体实现选择，不是 C++ 标准对变量存储位置的规定。

同样，字符串字面量通常放在只读数据区域，但 C++ 标准并不规定它必须位于 ELF 的 .rodata。

ELF 规范对这些段的描述可以参考：.bss 是占用内存映像、但不占用文件实际数据空间的零初始化数据；.rodata 通常属于不可写的程序映像区域。

> **【核实】** 与规范一致：gABI 写 `.bss` "occupies no file space, as indicated by the section type, SHT_NOBITS"；`elf(5)` 写 `.rodata` "typically contributes to a nonwritable segment in the process image"（附录 B）。实测 `.bss` 节类型 `NOBITS`、`.rodata` 标志只有 `A`（不带 `W`）。
>
> **【补充】** 实验 1 里 `const` 与 `.rodata` 的关系：
>
> - `const int g_const = 10;`（被 `g_pconst` 取了地址）在 `.rodata`；`int g_data = 10;` 在 `.data`。
> - 只当值用的 `const int g_const_folded = 12;`：`-O0` 下在 `.rodata`，`-O2` 下**符号整个消失**。所以"const 一定在 `.rodata`"不成立，"const 一定存在"也不成立。
> - 字面量 `"hello"` 在 `.rodata`（`objdump -s -j .rodata` 里能找到）。

#### 2. .bss 不代表 C++ 中所有没有显式初始化的变量

.bss 是 ELF 的一个实现层面的区域，不是 C++ 的变量分类。

例如：

```cpp
int a;             // 全局变量，静态存储期，零初始化
int b = 10;        // 全局变量，静态存储期，常量初始化
int c = getValue();// 全局变量，静态存储期，通常需要动态初始化
```

在一种常见的 GCC/Linux 实现中，可能是：

- a 放在 .bss。
- b 放在 .data。
- c 的对象存储空间放在 .bss，启动时再通过初始化函数完成动态初始化。

但不要把这个对应关系记死：编译器可能优化动态初始化，链接器也可能采用不同布局。

> **【核实】** 实验 1：`g_bss`、`g_dyn` 在 `.bss`，`g_data` 在 `.data`；`g_dyn` 由启动代码调用的初始化函数赋值（见 1.3）。
>
> **【补充】（容易记错）** 节是按**值**分的，不按 C++ 的初始化类别分。实验 1 里 `int g_zero_const = 0;` 是**常量初始化**（初始化器是常量表达式，不是"零初始化"这个类别），GCC 照样把它放进 `.bss`。所以我原回答里"常量初始化→`.data`、零初始化→`.bss`"只是常见情形的简写，准确的说法是：
>
> - 静态存储期变量，初值为 0（不论是零初始化还是常量初始化成 0）→ `.bss`；
> - 有非零初值的常量初始化 → `.data`（加 `const` 且被需要时 → `.rodata`）；
> - 需要动态初始化的 → 先按 0 放在 `.bss`，再由初始化函数赋值。

#### 3. .init_array 不是存放全局对象的地方

你的理解方向是对的，但建议把描述改得更精确。

.init_array 存放的是一个函数指针数组，指向程序或共享库启动时需要执行的初始化函数。

例如：

```cpp
class A {
public:
    A() {
        // 构造函数
    }
};

A global;
```

编译器通常会生成一个类似这样的初始化函数（仅为概念示意）：

```cpp
void initialize_global() {
    new (&global) A;
}
```

然后让 .init_array 中的某个条目指向这个函数。

注意，实际生成的代码不一定采用这个具体形式。

所以：

- global 的对象存储空间可能在 .bss。
- 初始化函数的机器指令在 .text。
- .init_array 保存指向初始化函数的指针。

三个东西不要混淆。

ELF 规范明确把 .init_array 定义为初始化函数指针数组。

> **【核实】** gABI 对 `SHT_INIT_ARRAY` 的定义是 "an array of pointers to initialization functions"，每项当作"无参、返回 void 的过程"（附录 B）。实测 `.init_array` 的节类型就是 `INIT_ARRAY`，存的是函数地址。
>
> **【补充】（实验 2）** 我的两个翻译单元（`exp1.cpp`、`extra.cpp`）里一共 4 个有动态初始化的全局变量（`g_dyn`、`g_obj1`、`g_obj2`、`g_obj3`），`.init_array` 却只有 **3 项**：
>
> ```text
> [0] frame_dummy              ← crt 自带
> [1] _GLOBAL__sub_I__Z1fv     ← exp1.cpp 这个翻译单元
> [2] _GLOBAL__sub_I_g_obj3    ← extra.cpp 这个翻译单元
> ```
>
> `exp1.cpp` 里的 `g_dyn`、`g_obj1`、`g_obj2` 由**同一个**函数按定义顺序初始化；反汇编里依次是 `f()`、`A::A` + `__cxa_atexit`、`A::A` + `__cxa_atexit`。所以 GPT 说"不保证每个全局对象恰好对应一个函数"，在 GCC 上具体是"**每个翻译单元一个函数**"。（名字是编译器随便取的，`_Z1fv` 恰好是 `f()` 的修饰名。）
>
> 顺带说明了"反序析构"的来源：每个对象构造完成后立刻 `__cxa_atexit` 登记析构函数，退出时按登记的反序调用。
>
> **【补充】谁来调用**：用 gdb 断在构造函数里看调用栈（glibc 2.35）：
>
> ```text
> A::A  ←  __static_initialization_and_destruction_0  ←  _GLOBAL__sub_I__Z1fv
>       ←  call_init (csu/libc-start.c:145)  ←  __libc_start_main_impl  ←  _start
> ```
>
> 此时 `main` 还没被调用。GPT 说"运行库和动态链接器负责安排"，主程序这一项本机实测是 libc 的 `call_init`；共享库自己的 `.init_array` 由动态链接器调用，这部分我没有实测。
>
> **【更正我自己的图】** 我原回答把 `.init_array` 画在 `.text`/`.rodata` 一组、"装载时直接映射"里，不准确：实测它在**可写**的 `PT_LOAD` 里（因为要被重定位），装载完成后由 `PT_GNU_RELRO` 改成只读（运行时映射里是 `r--p`）。见第二节批注。

#### 4. 堆和栈的描述可以保留，但要加上实现限定

你的图是经典的 Linux 进程内存示意图，可以继续使用。不过建议加两点：

第一，图里的“堆向高地址增长、栈向低地址增长”是传统布局的典型示意，不是 C++ 规定，也不应理解成所有动态分配都从一个连续堆区取得内存。Linux 的内存分配器也会使用 mmap。

第二，栈上的局部变量是常见情况，但不是绝对情况。编译器优化后，局部变量也可能只存在寄存器里，甚至被优化掉。

还有一个容易遗漏的地方：每个线程有自己的线程栈，不是整个进程只有一个栈。

> **【核实】** 实验 1（`/proc/self/maps` 里查地址落在哪个映射）：
>
> - `new int(2)` → `[heap]`；
> - `malloc(1 MiB)` → 独立的匿名映射 `[anon]`（走 mmap，不在 `[heap]`）；
> - 主线程局部变量 → `[stack]`；
> - 第二线程有自己的栈：它的 `t_data` 和栈上局部变量 `x` 都在同一块 `[anon]` 映射里，地址只差 2228 字节。
>
> 附录 A.1 贴了截取过的映射：堆（低）→ 各共享库、`ld.so`（中）→ 栈（高），相对位置与示意图一致（gdb 关了 ASLR，地址固定；相对位置不变）。"向高/向低增长"我没有逐项验证，按 GPT 说的当典型示意即可。

### 二、一个非常重要的区别：ELF 的 section 和 segment

这个地方我建议你单独记一句，因为它能解释为什么前面的内存图不能完全按照 .text、.data 等顺序理解。

ELF 有两个层次：

- Section（节）：组织文件里的内容，例如 .text、.rodata、.data、.bss、.init_array。
- Segment（段）：描述程序运行时需要怎样将文件内容映射到进程虚拟内存中。

内核装载 ELF 时，主要根据 ELF 的 Program Header（程序头）中的 PT_LOAD 信息建立内存映射。

例如：

**ELF 文件中的 Sections**

| Section | 内容 |
|---|---|
| .text | 机器指令 |
| .rodata | 只读数据 |
| .init_array | 初始化函数指针 |
| .data | 已初始化数据 |
| .bss | 零初始化数据 |
| .tdata/.tbss | 线程局部数据 |

**Program Header：PT_LOAD** — 将需要装载的节组织为运行时的内存映像，并指定读、写、执行权限。

**进程虚拟地址空间** — 代码映射、只读数据映射、可写数据映射等。

概念关系示意，并非 ELF 的实际节排列或一一对应关系。

尤其是 .bss：

它在 ELF 文件中通常不保存实际的零值数据，只记录需要的内存大小。装载时，内存映像中对应的部分会被置零。

可以理解为：

```text
PT_LOAD:
    p_filesz = 文件中实际需要映射的数据大小
    p_memsz  = 进程中需要的总内存大小

    如果 p_memsz > p_filesz：
        多出来的部分置零
```

这正是 .bss 不需要在可执行文件里实际存储大量零字节的原因。

> **【核实】** 实验 1（`readelf -lW`）：
>
> - **4 个 `PT_LOAD`**：`R`（ELF 头、动态链接表）、`R E`（`.text`）、`R`（`.rodata`、`.eh_frame`）、`RW`（`.tdata .init_array .fini_array .data.rel.ro .dynamic .got .data .bss`）。现代链接器默认把代码和只读数据分到不同的段。
> - 最后一个 `PT_LOAD`：`FileSiz 0x3d4 < MemSiz 0x414`，差的 `0x40` 正好是 `.bss` 的大小。GPT 的 `p_filesz`/`p_memsz` 伪代码成立。
> - 还有 `INTERP`（`/lib64/ld-linux-x86-64.so.2`）、`TLS`（`FileSiz 4 / MemSiz 8`，即 `.tdata` 4 字节 + `.tbss` 4 字节）、`GNU_RELRO`。
>
> **【补充】** GPT 的图里没有的两个段，对理解很有用：
>
> - **`GNU_RELRO`** 覆盖 `.tdata .init_array .fini_array .data.rel.ro .dynamic .got`：这些在重定位完成后被设成只读。所以运行时这个程序自己的映射是 5 段：
>
>   ```text
>   r--p  (ELF 头等)
>   r-xp  (.text)
>   r--p  (.rodata, .eh_frame)
>   r--p  (RELRO 区：.tdata/.init_array/.dynamic/.got ...)   ← .init_array 在这里
>   rw-p  (.data, .bss)
>   ```
>
> - 小的 `.bss` 和 `.data` 落在同一页里（文件映射部分的最后一页，尾部被置零），所以实验 1 里 `g_bss` 显示在文件映射 `rw-p` 里，而不是单独的匿名映射；`.bss` 大到超出这一页时，多出来的才是匿名页。

### 三、把变量的存储位置一次性梳理清楚

这里我建议你建立下面这张表。它比单独记忆堆栈和各个段更有用。

| 变量 | C++ 存储期 | 典型实现位置 |
|---|---|---|
| 普通局部变量 `int x` | 自动存储期 | 栈（或寄存器） |
| 全局变量 `int x` | 静态存储期 | .data / .bss |
| 类的静态数据成员 | 静态存储期 | .data / .bss |
| 函数内 static 变量 | 静态存储期 | .data / .bss |
| thread_local 变量 | 线程存储期 | .tdata / .tbss 等 |
| new 创建的对象 | 动态存储期 | 通常由堆分配器管理，也可能使用 mmap |
| 字符串字面量 | 静态存储期的数组对象 | 通常位于 .rodata |

> **【核实】** 实验 1 里 `.data`/`.bss` 各行、栈/堆行、字符串字面量行都与实测一致（附录 A.1 有"符号 → 节"对照表）。
>
> **【补充】（thread_local 一行）** `.tdata`/`.tbss` 只是线程局部变量的**初始映像**（模板）：可执行文件里只有一份（`TLS` 段）；每个线程启动时由运行库拷贝出自己的 TLS 块，**活的对象在那块里**，不在可执行文件的 `.tdata` 映射里。实测：主线程的 `&t_data` 落在 `[anon]`，第二线程的落在它自己栈所在的 `[anon]` 映射里。
>
> **【补充】（函数内 static 一行）** 有动态初始化的函数内 static（`static int s_dyn = f();`）除了变量本身，还带一个 guard 变量（实测符号 `guard variable for fn()::s_dyn`，8 字节，在 `.bss`），这是"只初始化一次、并发时等待"的实现。

这里有三个重点。

第一，函数内的 static 变量并不是普通局部变量。

```cpp
void f() {
    int x = 10;
    static int s = 10;
}
```

x 是自动存储期，通常在栈上。

s 是静态存储期，整个程序运行期间都存在，通常放在 .data。

但是要注意，static 并不代表一定等到第一次执行函数才初始化。

例如：

```cpp
static int s = 10;
```

它可以在程序启动时完成常量初始化。

而：

```cpp
static std::string s = "hello";
```

其动态初始化通常是在第一次执行到声明时进行。

这条规则也适用于函数内的 thread_local 变量。初始化失败时，下一次经过声明还会重新尝试初始化。

> **【核实】** 标准 [stmt.dcl]/3 原文："Dynamic initialization of a block variable with static storage duration or thread storage duration is performed the first time control passes through its declaration … If the initialization exits by throwing an exception, the initialization is not complete, so it will be tried again the next time control enters the declaration."（附录 B）
>
> **【补充】** `static std::string s = "hello";` 在本机（g++ 11.4 / libstdc++ 11）确实是动态初始化：`std::string` 在 C++20 下仍没有 constexpr 构造（`constexpr std::size_t n = std::string("abc").size();` 编译报 "call to non-'constexpr' function"），放在命名空间作用域时编译器生成了 `_GLOBAL__sub_I_*` 初始化函数。换成支持 constexpr `std::string` 的标准库后是否仍如此取决于实现，这个例子别当规则记。

第二，thread_local 要单独看待。

这也是你前文需要调整的一处。

thread_local 虽然也涉及静态初始化和动态初始化，但它不是简单地在整个进程启动时统一初始化。

每个线程都有自己的 thread_local 对象。

ELF 中有：

- .tdata：带有初始数据的线程局部数据。
- .tbss：零初始化的线程局部数据。

每个线程创建自己的对应数据实例。动态初始化可以在该线程开始执行之前完成，也可以推迟到符合标准要求的使用时机。

> **【核实 + 更正我自己的 guide】** GPT 对。实验 3：g++ 11.4 对带动态初始化的 `thread_local` 选的是**推迟到每个线程第一次使用时**：
>
> ```text
>   ctor static global          ← 静态对象：main 之前
> main start
>   ctor thread_local global    ← 主线程第一次用 g_tls（在 main start 之后）
>   ctor static local
> thread start
>   ctor thread_local global    ← 第二线程第一次用 g_tls，各线程各一份
> thread end
>   dtor thread_local global    ← 第二线程退出时析构
> ```
>
> 标准 [basic.start.dynamic]/7 允许的两种时机："is sequenced before the first statement of the initial function of a thread **or is deferred**"（附录 B）。
>
> 所以我写进 guide 的那一行"非局部变量：命名空间作用域的变量、类的静态数据成员（及 thread_local）｜程序启动时"对 `thread_local` **不准确**：它是线程启动时或首次使用时，每个线程各一次。要改 guide 的话是 §5 的第 1 项。

第三，C++ 的存储期不等于物理存放位置。

例如，new 对象具有动态存储期，但不能因此认为所有 new 出来的对象一定在某个连续的堆区。

这张表讲的是典型 Linux 实现，而不是 C++ 标准规定。

> **【核实】** 见 1.4 批注：`new int` 在 `[heap]`，1 MiB 的 `malloc` 走独立 mmap。

### 四、现在把整个程序运行过程串起来

这是你原文最值得保留的部分，我稍微调整一下顺序。

#### 1. 加载阶段：操作系统建立进程内存映像

假设你执行：

```text
./program
```

Linux 大致执行：

- 通过 execve 加载可执行文件。
- 读取 ELF 信息，根据 PT_LOAD 将程序需要的内容映射到进程虚拟地址空间。
- 对 .bss 对应的内存区域进行零填充。
- 如果程序是动态链接的，根据 ELF 的 PT_INTERP 信息加载动态链接器，例如 ld-linux-x86-64.so.2。
- 动态链接器加载共享库，例如 libc.so，完成必要的符号解析和重定位。

同时，操作系统会准备进程启动所需的栈，包括命令行参数、环境变量等。

注意：这时还不能说 main 已经开始执行。

> **【核实】** `readelf -lW` 里 `INTERP` 是 `/lib64/ld-linux-x86-64.so.2`；运行时映射里能看到 `ld-linux-x86-64.so.2`、`libc.so.6`、`libstdc++.so.6`、`libm.so.6`、`libgcc_s.so.1` 各自的几段，还有内核提供的 `[vvar]`、`[vdso]` 和 `[stack]`（附录 A.1）。

#### 2. 程序启动阶段：C++ 对象初始化

现在回到 C++ 的初始化规则。

你前面研究的两个阶段，就是在这个过程中体现的。

**阶段 1：静态初始化** — 常量初始化和零初始化

- .data 等映射区域提供已经初始化的数据。
- .bss 等区域提供零初始化的数据。
- 必要的重定位也由装载和动态链接过程处理。

**阶段 2：动态初始化** — 调用初始化代码完成对象初始化

- 初始化函数的指针通常保存在 .init_array。
- 初始化函数的机器指令通常在 .text。
- 运行库和动态链接器负责安排相应初始化函数的执行。
- 常见情况是在 main 之前完成。

**阶段 3：进入 main** — 程序正式执行用户代码

- _start 等启动代码最终调用 main。
- 运行期间创建局部变量、动态分配对象、创建线程等。

这里特别强调：

C++ 的静态初始化和 ELF 的 .data、.bss 之间是实现上的联系，不是定义上的等价关系。

而且 C++ 标准还允许一个优化：即使某个变量按规则本来需要动态初始化，只要满足标准规定的条件，编译器也可以提前将其作为静态初始化完成。

还有一点：标准允许推迟部分非局部变量的动态初始化，因此不能把所有动态初始化都说成必须发生在 main 之前。Linux 常见实现通常在启动时执行大多数全局对象的初始化函数，但这是实现行为，不是无条件的标准保证。

> **【核实】**
>
> - "必要的重定位"：PIE 构建里 `.init_array` 的 3 个槽，在文件里存的是未加基址的偏移（`0x1390`、`0x1d01`、`0x2196`），每个槽都有一条 `R_X86_64_RELATIVE` 动态重定位，装载时由动态链接器加上基址；整个可执行文件共 12 条 `RELATIVE`（附录 A.2）。
> - "常见在 main 之前"：gdb 断在构造函数里时调用栈是 `_start → __libc_start_main_impl → call_init → _GLOBAL__sub_I_*`，`main` 尚未被调用（见 1.3 批注）。
> - "允许提前成静态"与"允许推迟"：标准 [basic.start.static]/3 和 [basic.start.dynamic]/5 的原文，见附录 B。我 11-06 已经读过后者，措辞是 "implementation-defined whether … sequenced before the first statement of main or is deferred"。

#### 3. main 运行阶段：栈、堆和静态数据开始发挥作用

看一个例子：

```cpp
int global = 10;

void func() {
    int x = 20;

    static int s = 30;

    int* p = new int(40);
}

int main() {
    func();
}
```

典型情况下：

**程序运行时的对象分布**

| 区域 | 存在期间 | 内容 |
|---|---|---|
| 静态数据区 .data | 程序整个运行期 | global = 10；s = 30 |
| 栈 Stack | 函数调用期间 | x = 20；指针变量 p |
| 动态分配内存 | 直到释放 | *p = 40（即 new int(40) 创建的对象） |

当 func() 执行结束：

- x 和指针变量 p 的自动存储期结束。
- s 不会消失，它具有静态存储期。
- new 出来的对象也不会因为 func() 返回而自动消失，需要通过 delete 或相应的所有权管理机制释放。

这里也能看出为什么 RAII 很重要：对象的生命周期与资源的释放应该有明确的管理关系，而不是依靠进程最后退出时统一回收。

> **【核实】** 与实验 1 里的 `fn()` 同构：`s_data`（有非零初值）在 `.data`，`s_bss`、`s_dyn` 在 `.bss`；`new int` 在 `[heap]`。`-O0` 下 `x`、`p` 在栈上，开优化后可能只在寄存器里（GPT 在 1.4 已提醒）。

#### 4. 程序退出阶段：析构和内存回收

假设：

```cpp
int main() {
    A a;
    return 0;
}
```

实际顺序可以理解为：

- main 结束，先销毁 main 内具有自动存储期的对象，例如 a。
- 从 main 返回，效果相当于调用 std::exit(0)。
- 运行库执行已经构造完成的静态存储期对象的析构，以及注册的退出处理函数。
- 各线程的线程局部对象在相应线程退出时析构。
- 运行库最终通过操作系统终止进程。
- 内核回收该进程的虚拟地址空间，包括堆、栈和内存映射区域。

这里有两个小细节：

第一，反序析构不是简单地按照源代码从后往前。

更准确地说，静态存储期对象通常按照构造完成顺序的逆序进行析构，并遵循标准中的额外排序规则。只有成功构造完成的对象才需要析构。

第二，std::exit 与 return main 不完全等价于普通函数返回。

return main 会先销毁 main 内部的自动存储期对象，然后调用 std::exit。

如果在函数内部直接调用 std::exit，当前函数栈上的自动对象不会因为这个调用而执行析构。

而 _exit 或 std::abort 也不会执行常规的 C++ 静态对象析构流程。

> **【更正】（小）** 上面第 3、4 两条的先后对**主线程**不准确。标准 [support.start.term] 里 `std::exit` 的效果是：
>
> > First, objects with thread storage duration and associated with the current thread are destroyed. Next, objects with static storage duration are destroyed and functions registered by calling atexit are called. … (Objects with automatic storage duration are not destroyed as a result of calling exit().)
>
> 所以对主线程，顺序是：`main` 里的自动对象 → **主线程的 `thread_local`** → 静态存储期对象（构造完成顺序的逆序）和 `atexit` 函数。其他线程的 `thread_local` 在各自线程退出时就已销毁。
>
> 实验 3 的实测（`main` 里先后构造了：静态全局 → 主线程的 `thread_local` → 静态局部）：
>
> ```text
> main end
>   dtor main local            ← main 的自动对象
>   dtor thread_local global   ← 主线程的 thread_local，先于下面两个静态对象
>   dtor static local
>   dtor static global
> ```
>
> 要注意：`thread_local` 的构造早于 `static local`，如果只是"按构造完成的逆序"，它应该排在 `static local` 之后；实测排在前面，正是 `exit` 的"先线程局部、后静态"规则。
>
> **【核实】**（实验 3，其余几处）：
>
> - **只有构造完成的才析构**：每个对象构造完后才 `__cxa_atexit` 登记析构（反汇编里 `A::A` 之后紧跟 `__cxa_atexit`，见 1.3 批注）。
> - **`std::exit` 不析构调用处的自动对象**：在 `bye()` 里调 `exit(0)`，`bye()` 里的 `local in bye()` 和 `main` 里的 `main local` 都**没有**析构；`thread_local`、`static local`、`static global` 照常析构。
> - **`_exit` 和 `abort` 不跑任何析构**：两者之后都没有任何 `dtor` 输出；`abort` 退出码 134（SIGABRT）。

### 五、最后说一下：你原文中还有两处 C++ 规则值得同步修正

这两处不是内存分布问题，但与你前面的 Non-local variables 笔记直接相关。

**1. 静态初始化的表述需要修正。**

你原文写的是：

> 按顺序尝试两种：常量初始化、零初始化。做不了常量初始化的，先全部置零。

建议改成：

- 可以进行常量初始化的，执行常量初始化。
- 否则，执行零初始化。
- 两者合称静态初始化，且先于任何动态初始化完成。

不要让读者产生每个对象都先零初始化、再常量初始化的印象。

> **【澄清】（不是纠错）** 我原文的语义本来就是"能常量初始化就常量初始化，否则零初始化"，也与标准一致。C++17（N4659）起 [basic.start.static] 的原文是："Constant initialization is performed if … If constant initialization is not performed, a variable with static storage duration or thread storage duration is zero-initialized."（C++20 及最新草案同样。）
>
> **【补充】（版本差异）** C++14 草案（N4140）的写法不同，在 [basic.start.init] 里："Variables with static storage duration or thread storage duration shall be zero-initialized before any other initialization takes place."，即**所有**静态/线程存储期变量先零初始化，再看能否常量初始化。GPT 担心的"先零初始化、再常量初始化"的印象，恰恰是旧措辞的字面意思。两种写法的可观察结果相同（我的判断：常量初始化会覆盖掉先前的零）。建议 guide 里保留我的写法，需要时加半句"C++14 及以前的措辞是先零初始化"。

**2. 全局变量初始化异常与函数内 static 初始化异常不同。**

你原文说：

> 抛异常就 std::terminate，没法在外面 catch。

这对于非局部变量（包括静态存储期和线程存储期的非局部变量）的初始化是正确的。

但是函数内的动态初始化 static 变量不同。

```cpp
void f() {
    static A a;
}
```

假如 A 的构造函数抛出异常：

- 异常可以传播到调用 f() 的地方，由外部捕获。
- 这次初始化视为没有完成。
- 下一次执行到声明时，会再次尝试初始化。

这是非常值得记住的区别。

> **【澄清】（不是纠错）** 我那句话写在"阶段 2：动态初始化"一节下，说的就是非局部变量，当时是对的；函数内 static 的情形我在对话里（"Static block variables"那一段）单独讲过，没有写进 guide。GPT 的提醒合理：两句话挨着容易混，建议在 guide 那句后面加括号"（仅非局部变量；函数内 static 抛异常可以传播，下次再试）"。
>
> **【核实】** 标准：[basic.start.dynamic]/8 "If the initialization of a non-block variable with static or thread storage duration exits via an exception, the function std::terminate is called."；[stmt.dcl]/3 管函数内的那一种（附录 B）。

### 六、我建议你最终记住的整条时间线

1. **加载 ELF** — 内核建立程序的虚拟地址空间，映射代码和数据，处理零填充，加载动态链接器。
2. **动态链接** — 加载共享库，处理重定位，准备程序启动环境。
3. **静态初始化** — C++ 的常量初始化和零初始化已经先于动态初始化完成。通常由映射和装载过程中准备好的数据状态体现。
4. **动态初始化** — 运行库执行全局对象等需要运行代码的初始化函数，通常发生在 main 之前，但标准允许推迟。
5. **进入 main** — 开始执行用户代码。局部自动变量、动态分配对象、函数内静态变量、线程局部对象分别按照自己的存储期和初始化规则管理。
6. **程序退出** — 销毁相应的自动存储期对象、线程局部对象和静态存储期对象，执行退出处理函数。
7. **进程结束** — 操作系统回收进程的虚拟内存资源。

我的最终建议： 你原来的内存布局图可以保留，但把它定位为 Linux ELF 的典型实现示意；把 C++ 的两阶段初始化单独作为语言层面的规则。

这两套知识只需要在 .data、.bss 和 .init_array 这几个位置建立联系，不需要继续深入 Linux 内核的内存管理机制。

对于你目前的 C++ 知识体系，掌握到这里已经足以理解 Non-local variables、静态存储期、动态初始化顺序，以及我们之前讨论的初始化与对象生命周期之间的关系。

> **【核实】** 时间线与实测一致：`execve` → 内核映射 `PT_LOAD`、动态链接器映射并重定位共享库 → `_start` → `__libc_start_main` → `call_init` 跑 `.init_array`（`main` 之前）→ `main` → 返回后 `exit`（先主线程 `thread_local`，再静态对象和 `atexit`）→ 内核回收地址空间。
>
> **【补充】** GPT 的最终建议（内存布局只当作 Linux ELF 的典型实现示意，两阶段初始化当作语言规则，两者只在 `.data`/`.bss`/`.init_array` 三处连接）我同意，也是这份文件里批注的取向。加上本次发现，连接点要补两个细节：`.bss`/`.data` 按**值**分（不按初始化类别）；`.init_array` 是**每个翻译单元一项**，在装载后只读的 RELRO 区里。

## 4. 对我原回答（§2）的修正清单

| # | 原回答 | 改成 |
|---|---|---|
| 1 | 图里 `.init_array` 与 `.text`/`.rodata` 同组，注释"每个全局对象的动态初始化函数" | 在可写 `PT_LOAD` 里，装载后 RELRO 只读；GCC 下**每个翻译单元一项**（`_GLOBAL__sub_I_*`），外加 crt 的 `frame_dummy` |
| 2 | "常量初始化 → .data，零初始化 → .bss" | 按值分：值为 0（含常量初始化成 0）→ `.bss`；非零 → `.data`；需要动态初始化的先在 `.bss`，再由初始化函数赋值 |
| 3 | `.rodata` 放 const 全局变量 | 加限定：要被取地址（或 `-O0`）才有符号；开优化后可能整个消失；字面量放 `.rodata` 属实现行为 |
| 4 | 堆向高、栈向低 | 标为"典型示意"；每个线程有自己的栈；大块分配走 mmap，不在 `[heap]` |
| 5 | 退出："按注册的反序调用 `atexit` 和静态对象的析构" | 补：先销毁主线程的 `thread_local`，再是静态对象和 `atexit`；`exit` 不析构调用处的自动对象，`_exit`/`abort` 不跑析构 |
| 6 | 没有区分 section 与 segment | 补：`PT_LOAD`（4 个）、`TLS`、`GNU_RELRO`；`p_memsz > p_filesz` 的差就是 `.bss` |

## 5. 待办：guide 里要改的地方（我没有动，等你点头）

对象：`lifetime-and-initialization-guide.md` 的"补充：非局部变量的两阶段初始化"一节。

1. **`thread_local` 一行（必改）**：表里"非局部变量：…（及 `thread_local`）｜程序启动时"，改成"命名空间作用域的变量、类的静态数据成员：程序启动时；`thread_local`：每个线程启动时或首次使用时（各线程各一次，GCC 实测是首次使用时）"。
2. **异常那一句（建议）**：在"抛异常就 `std::terminate`"后加"（仅非局部变量；函数内 static 抛异常可传播，下次再试）"。
3. **静态初始化的措辞（可选）**：在"按顺序尝试两种"后加半句"（C++14 及以前的措辞是先零初始化，可观察结果相同）"。

## 附录 A：实验（可复现）

环境同文件开头。源码在 A.0，输出为实际运行结果（地址相关的细节作了截取）。

### A.0 源码与编译

<details>
<summary>exp1.cpp：变量落在哪个节、哪个映射</summary>

```cpp
// 变量落在哪个节/映射：objdump -t 看节，/proc/self/maps 看运行时映射
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

struct A {
    const char* n;
    explicit A(const char* n) : n(n) { std::printf("  ctor %s\n", n); }
    ~A() { std::printf("  dtor %s\n", n); }
};

int f() { return 7; }

int g_bss;                          // 零初始化
int g_zero_const = 0;               // 常量初始化，值为 0
int g_data = 10;                    // 常量初始化，值非 0
const int g_const = 10;
const int* g_pconst = &g_const;     // 取地址，迫使 g_const 真正存在
const int g_const_folded = 12;      // 只当值用
int g_dyn = f();                    // 动态初始化
const char* g_str = "hello";
thread_local int t_data = 5;
thread_local int t_bss;
A g_obj1("g_obj1");
A g_obj2("g_obj2");

void fn() {
    static int s_data = 1;
    static int s_bss;
    static int s_dyn = f();         // 动态初始化的函数内 static，带 guard 变量
    s_data += s_bss + s_dyn;
}

static void where(const char* name, const void* p) {
    unsigned long a = reinterpret_cast<unsigned long>(p);
    std::FILE* fp = std::fopen("/proc/self/maps", "r");
    char line[512];
    while (std::fgets(line, sizeof line, fp)) {
        unsigned long lo = 0, hi = 0;
        char perms[8] = "";
        char path[400] = "";
        std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %399[^\n]", &lo, &hi, perms, path);
        if (a >= lo && a < hi) {
            const char* base = std::strrchr(path, '/');
            base = base ? base + 1 : path;
            std::printf("%-26s %s  %s\n", name, perms, base[0] ? base : "[anon]");
            break;
        }
    }
    std::fclose(fp);
}

int main() {
    fn();
    int local = 1;
    int* heap = new int(2);
    void* big = std::malloc(1 << 20);
    where("f (代码)", reinterpret_cast<void*>(&f));
    where("puts (libc 代码)", reinterpret_cast<void*>(&::puts));
    where("\"hello\" 字面量", "hello");
    where("g_const", &g_const);
    where("g_data", &g_data);
    where("g_zero_const", &g_zero_const);
    where("g_bss", &g_bss);
    where("g_dyn", &g_dyn);
    where("g_str(指针本身)", &g_str);
    where("local (栈)", &local);
    where("new int", heap);
    where("malloc(1MiB)", big);
    where("t_data (主线程)", &t_data);
    std::thread th([] {
        int x = 0;
        where("t_data (第二线程)", &t_data);
        where("x (第二线程的栈)", &x);
        std::printf("t_data 与 x 地址相差 %ld 字节\n",
                    static_cast<long>(reinterpret_cast<char*>(&t_data) - reinterpret_cast<char*>(&x)));
    });
    th.join();
    delete heap;
    std::free(big);
}
```

extra.cpp（第二个翻译单元，用来数 `.init_array` 的项数）：

```cpp
#include <cstdio>

struct B {
    B() { std::puts("  ctor g_obj3 (另一个翻译单元)"); }
};
B g_obj3;
```

</details>

<details>
<summary>exp2.cpp：退出顺序、thread_local 的初始化时机、exit / _exit / abort</summary>

```cpp
// 退出顺序、thread_local 的初始化时机、exit/_exit/abort 的区别（全部写 stderr，避免缓冲干扰）
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>

struct T {
    const char* n;
    explicit T(const char* n) : n(n) { std::fprintf(stderr, "  ctor %s\n", n); }
    ~T() { std::fprintf(stderr, "  dtor %s\n", n); }
};

T g_static("static global");
thread_local T g_tls("thread_local global");

static void bye(const char* how) {
    T x("local in bye()");
    if (!std::strcmp(how, "exit")) std::exit(0);
    if (!std::strcmp(how, "_exit")) _exit(0);
    if (!std::strcmp(how, "abort")) std::abort();
}

int main(int argc, char** argv) {
    const char* how = argc > 1 ? argv[1] : "return";
    std::fprintf(stderr, "main start (mode=%s)\n", how);
    T local("main local");
    std::fprintf(stderr, "main 第一次用 g_tls: %s\n", g_tls.n);
    static T s_local("static local");
    std::thread th([] {
        std::fprintf(stderr, "thread start\n");
        std::fprintf(stderr, "thread 第一次用 g_tls: %s\n", g_tls.n);
        std::fprintf(stderr, "thread end\n");
    });
    th.join();
    std::fprintf(stderr, "main end\n");
    if (std::strcmp(how, "return")) bye(how);
}
```

</details>

编译（本文用到的构建）：

```bash
g++ -std=c++20 -O0 -Wall -Wextra -pthread exp1.cpp extra.cpp -o exp1       # PIE（默认）
g++ -std=c++20 -O0 -pthread -no-pie exp1.cpp extra.cpp -o exp1np           # 非 PIE：直接读 .init_array 的值
g++ -std=c++20 -O2 -pthread exp1.cpp extra.cpp -o exp1o2                   # 对照 -O2
g++ -std=c++20 -O0 -g -pthread exp1.cpp extra.cpp -o exp1g                 # gdb 用
g++ -std=c++20 -O0 -Wall -Wextra -pthread exp2.cpp -o exp2
```

### A.1 实验 1：变量落在哪个节、哪个映射

**节表**（`readelf -SW exp1`，只留相关行）：

```text
  [Nr] Name              Type            Address          Off    Size   ES Flg
  [ 1] .interp           PROGBITS        0000000000000350 000350 00001c 00   A
  [16] .text             PROGBITS        00000000000012b0 0012b0 000ef4 00  AX
  [18] .rodata           PROGBITS        0000000000003000 003000 0001fe 00   A
  [20] .eh_frame         PROGBITS        0000000000003400 003400 00081c 00   A
  [22] .tdata            PROGBITS        0000000000005c5c 004c5c 000004 00 WAT
  [23] .tbss             NOBITS          0000000000005c60 004c60 000004 00 WAT
  [24] .init_array       INIT_ARRAY      0000000000005c60 004c60 000018 08  WA
  [25] .fini_array       FINI_ARRAY      0000000000005c78 004c78 000008 08  WA
  [27] .dynamic          DYNAMIC         0000000000005d00 004d00 000210 10  WA
  [28] .got              PROGBITS        0000000000005f10 004f10 0000e0 08  WA
  [29] .data             PROGBITS        0000000000006000 005000 000030 00  WA
  [30] .bss              NOBITS          0000000000006030 005030 000040 00  WA
```

**程序头**（`readelf -lW exp1`，截取；`Flg` 是段权限）：

```text
  Type           Offset   VirtAddr           FileSiz  MemSiz   Flg Align
  INTERP         0x000350 0x0000000000000350 0x00001c 0x00001c R   0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x000000 0x0000000000000000 0x000e78 0x000e78 R   0x1000
  LOAD           0x001000 0x0000000000001000 0x0011b1 0x0011b1 R E 0x1000
  LOAD           0x003000 0x0000000000003000 0x000c63 0x000c63 R   0x1000
  LOAD           0x004c5c 0x0000000000005c5c 0x0003d4 0x000414 RW  0x1000
  TLS            0x004c5c 0x0000000000005c5c 0x000004 0x000008 R   0x4
  GNU_RELRO      0x004c5c 0x0000000000005c5c 0x0003a4 0x0003a4 R   0x1

 Section to Segment mapping:
   03     .init .plt .plt.got .plt.sec .text .fini
   04     .rodata .eh_frame_hdr .eh_frame .gcc_except_table
   05     .tdata .init_array .fini_array .data.rel.ro .dynamic .got .data .bss
   09     .tdata .tbss                                                   (TLS)
   13     .tdata .init_array .fini_array .data.rel.ro .dynamic .got      (GNU_RELRO)
```

**符号 → 所在节**（`objdump -t --demangle exp1`，`-O0`）：

| 符号 | 所在节 | 说明 |
|---|---|---|
| `g_bss` | .bss | 零初始化 |
| `g_zero_const` | **.bss** | 常量初始化成 0，仍在 .bss |
| `g_data` | .data | 常量初始化，非 0 |
| `g_const` | .rodata | 被取了地址 |
| `g_const_folded` | .rodata（`-O2` 下**不存在**） | 只当值用 |
| `g_pconst`、`g_str` | .data | 指针变量本身 |
| `g_dyn` | .bss | 动态初始化，先按 0 放在这里 |
| `g_obj1`、`g_obj2`、`g_obj3` | .bss | 有动态初始化的对象 |
| `fn()::s_data` | .data | 函数内 static，非零常量初值 |
| `fn()::s_bss`、`fn()::s_dyn` | .bss | |
| `guard variable for fn()::s_dyn` | .bss（8 字节） | 动态初始化的函数内 static 的 guard |
| `t_data` | .tdata | 线程局部，初始映像 |
| `t_bss` | .tbss | 线程局部，零初始化 |

**运行输出**（`./exp1`；每行是"变量名 / 所在映射的权限 / 映射名"）：

```text
  ctor g_obj1
  ctor g_obj2
  ctor g_obj3 (另一个翻译单元)
f (代码)                 r-xp  exp1
puts (libc 代码)         r-xp  libc.so.6
"hello" 字面量          r--p  exp1
g_const                    r--p  exp1
g_data                     rw-p  exp1
g_zero_const               rw-p  exp1
g_bss                      rw-p  exp1
g_dyn                      rw-p  exp1
g_str(指针本身)        rw-p  exp1
local (栈)                rw-p  [stack]
new int                    rw-p  [heap]
malloc(1MiB)               rw-p  [anon]
t_data (主线程)         rw-p  [anon]
t_data (第二线程)      rw-p  [anon]
x (第二线程的栈)     rw-p  [anon]
t_data 与 x 地址相差 2228 字节
  dtor g_obj2
  dtor g_obj1
```

**运行中的映射**（gdb 断在 `main` 入口，读 `/proc/<pid>/maps`；关了 ASLR，地址固定；省略 libm、libgcc_s、libstdc++、ld-linux 和几段匿名映射）：

```text
555555554000-555555555000 r--p 00000000   exp1        ← ELF 头等
555555555000-555555557000 r-xp 00001000   exp1        ← .text
555555557000-555555558000 r--p 00003000   exp1        ← .rodata
555555559000-55555555a000 r--p 00004000   exp1        ← RELRO 区：.tdata/.init_array/.dynamic/.got
55555555a000-55555555b000 rw-p 00005000   exp1        ← .data + .bss
55555555b000-55555557c000 rw-p 00000000   [heap]
7ffff7b3c000-7ffff7b64000 r--p 00000000   libc.so.6
7ffff7b64000-7ffff7cf9000 r-xp 00028000   libc.so.6
7ffff7cf9000-7ffff7d51000 r--p 001bd000   libc.so.6
7ffff7d51000-7ffff7d52000 ---p 00215000   libc.so.6
7ffff7d52000-7ffff7d56000 r--p 00215000   libc.so.6
7ffff7d56000-7ffff7d58000 rw-p 00219000   libc.so.6
7ffff7fbd000-7ffff7fc1000 r--p 00000000   [vvar]
7ffff7fc1000-7ffff7fc3000 r-xp 00000000   [vdso]
7ffffffdd000-7ffffffff000 rw-p 00000000   [stack]
```

### A.2 实验 2：`.init_array` 里是什么、谁来调用

**非 PIE 构建里的 `.init_array`**（直接读文件里的值，再用 `nm` 查符号）：

```text
.init_array 共 3 项:
  [0] 0x401380  frame_dummy
  [1] 0x401ced  _GLOBAL__sub_I__Z1fv
  [2] 0x402182  _GLOBAL__sub_I_g_obj3
```

**PIE 构建**：槽值是未加基址的偏移，每槽一条 `RELATIVE` 重定位：

```text
文件里的槽值: ['0x1390', '0x1d01', '0x2196']
  0000000000005c60  0000000000000008 R_X86_64_RELATIVE   1390
  0000000000005c68  0000000000000008 R_X86_64_RELATIVE   1d01
  0000000000005c70  0000000000000008 R_X86_64_RELATIVE   2196
RELATIVE 重定位总数: 12
```

**`exp1.cpp` 这个翻译单元的初始化函数里依次调用了什么**（`objdump -d -C` 反汇编，只留 `call`）：

```text
call   f()                    ← g_dyn = f()
call   A::A(char const*)      ← g_obj1
call   __cxa_atexit@plt       ← 登记 g_obj1 的析构
call   A::A(char const*)      ← g_obj2
call   __cxa_atexit@plt       ← 登记 g_obj2 的析构
```

**谁调用**（`gdb -batch -ex 'break A::A' -ex run -ex bt`）：

```text
#0  A::A (this=0x55555555a048 <g_obj1>, n=... "g_obj1") at exp1.cpp:9
#1  __static_initialization_and_destruction_0 (__initialize_p=1, __priority=65535) at exp1.cpp:25
#2  _GLOBAL__sub_I__Z1fv () at exp1.cpp:82
#3  call_init (env=<optimized out>, argv=..., argc=1) at ../csu/libc-start.c:145
#4  __libc_start_main_impl (main=0x555555555695 <main()>, argc=1, argv=..., ...)
#5  _start ()
```

### A.3 实验 3：退出顺序、`thread_local` 初始化时机、`exit`/`_exit`/`abort`

`./exp2`（正常 `return`）：

```text
  ctor static global
main start (mode=return)
  ctor main local
  ctor thread_local global           ← 主线程第一次用时才构造
main 第一次用 g_tls: thread_local global
  ctor static local
thread start
  ctor thread_local global           ← 第二线程第一次用时才构造（各线程各一份）
thread 第一次用 g_tls: thread_local global
thread end
  dtor thread_local global           ← 第二线程退出时析构
main end
  dtor main local
  dtor thread_local global           ← 主线程的 thread_local，先于静态对象
  dtor static local
  dtor static global
```

在 `bye()`（栈上有 `local in bye()`）里分别调用三种退出方式，`main end` 之后的输出：

| 调用 | `main end` 之后的输出 | 退出码 |
|---|---|---|
| `std::exit(0)` | `ctor local in bye()`；`dtor thread_local global`；`dtor static local`；`dtor static global`（**没有** `dtor local in bye()`，也**没有** `dtor main local`） | 0 |
| `_exit(0)` | `ctor local in bye()`（之后什么都没有） | 0 |
| `std::abort()` | `ctor local in bye()`（之后什么都没有） | 134 |

## 附录 B：标准与规范原文摘录（核对用，均为短摘）

标准条文取自 C++ 工作草案（eel.is/c++draft）和旧版草案（timsong-cpp.github.io/cppwp），ELF 部分取自 gABI 与 `elf(5)`。

- **[support.start.term]（`std::exit` 的 Effects）**："First, objects with thread storage duration and associated with the current thread are destroyed. Next, objects with static storage duration are destroyed and functions registered by calling atexit are called. … (Objects with automatic storage duration are not destroyed as a result of calling exit().)"
- **[stmt.dcl]/3**："Dynamic initialization of a block variable with static storage duration or thread storage duration is performed the first time control passes through its declaration; … If the initialization exits by throwing an exception, the initialization is not complete, so it will be tried again the next time control enters the declaration."
- **[basic.start.dynamic]/5**（静态存储期、非 inline 的非局部变量）："It is implementation-defined whether the dynamic initialization … is sequenced before the first statement of main or is deferred. If it is deferred, it strongly happens before any non-initialization odr-use of any non-inline function or non-inline variable defined in the same translation unit as the variable to be initialized."
- **[basic.start.dynamic]/7**（线程存储期、非 inline 的非局部变量）："It is implementation-defined whether the dynamic initialization … is sequenced before the first statement of the initial function of a thread or is deferred."
- **[basic.start.dynamic]/8**："If the initialization of a non-block variable with static or thread storage duration exits via an exception, the function std::terminate is called."
- **[basic.start.static]/3**（C++17 起）："An implementation is permitted to perform the initialization of a variable with static or thread storage duration as a static initialization even if such initialization is not required to be done statically, provided that …"（条件：动态版本不改变其他静态/线程存储期对象初始化前的值；静态版本得到的值与动态版本相同。）
- **静态初始化的措辞演变**：
  - C++14（N4140 [basic.start.init]/2）："Variables with static storage duration or thread storage duration shall be zero-initialized before any other initialization takes place. … Constant initialization is performed: …"
  - C++17（N4659 [basic.start.static]/2）和 C++20（N4861）："Constant initialization is performed if … If constant initialization is not performed, a variable with static storage duration or thread storage duration is zero-initialized. Together, zero-initialization and constant initialization are called static initialization; … All static initialization strongly happens before any dynamic initialization."
- **gABI（System V ABI，节头一章）**：`.bss` "holds uninitialized data that contribute to the program's memory image. By definition, the system initializes the data with zeros when the program begins to run. The section occupies no file space, as indicated by the section type, SHT_NOBITS."；`SHT_INIT_ARRAY` "contains an array of pointers to initialization functions … Each pointer in the array is taken as a parameterless procedure with a void return."；`.tdata` 的节标志是 `SHF_ALLOC+SHF_WRITE+SHF_TLS`，`.tbss` 是 `SHT_NOBITS` 加同样的标志。
- **`elf(5)`**：`.rodata` "holds read-only data that typically contributes to a nonwritable segment in the process image."
