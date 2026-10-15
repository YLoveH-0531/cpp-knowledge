# Array 题库

配合 [array.md](./array.md) 和 [array-checklist.md](./array-checklist.md) 使用。三份文件按**同一个顺序**对齐（一~八章）：读 array.md 的一节 → 做 checklist 里同一章的自测 → 做这里同一章的题。

## 怎么用

三档分工：

| 档 | 测什么 | 在哪里 |
|---|---|---|
| 第一档（背） | 必须脱稿说出来的核心结论 | checklist 的【背】 |
| 第二档（推） | 只记起点、现场推导 | checklist 的【推】 |
| 第三档（练） | 语法熟练度、动手验证 | **本题库** |

- 每题答案在折叠块里，**先自己答，再点开看答案**，别直接翻答案。
- 题号前的标签是题型：`[判断]` `[问答]` `[推导]` `[默写]` `[找错]` `[动手]`；**★ 是重点题，共 10 道**，时间不够就只做 ★。
- 章的顺序就是 array.md 的学习顺序，前一章是后一章的基础，第一遍建议从前往后做。

| 章 | 这一章的主线 | 对应 array.md |
|---|---|---|
| 一、概念 | 数组是"索引 → 元素"的抽象，连续内存只是内置数组的实现 | What / Why / Key Points |
| 二、声明、初始化与赋值 | 声明靠括号和"由内向外"，数组不能整体拷贝/赋值 | How · Operations 1、2 |
| 三、退化 | 退化是表达式规则，五个例外保留数组类型 | How · Operations 3 |
| 四、传参与返回 | 形参是指针，返回值不能是数组，长度要自己带 | How · Operations 4、5 |
| 五、多维（一）：类型 | `a[i]` 是一整行，退化只剥最外层，步长在类型里 | How · 多维 1、Note 第一部分 |
| 六、多维（二）：布局 | row-major 由类型结构决定，stride 把多维映射到线性 | How · 多维 2、3 |
| 七、因果链 | 所有规则都来自"取消指针格子"这一个决定 | Note 第二部分 |
| 八、综合与动手 | 把前七章用在完整例子和真实编译器上 | 全篇 |

## 一、概念：数组是什么、为什么

> array.md：What / Why / Key Points ｜ 自测清单：一

**一-1 [判断]** "数组的本质就是一块连续的内存。"

<details><summary>答案</summary>

错。数组的核心是"索引 → 元素"：数据集合 + 索引空间 + 定位规则。连续存储是内置数组在内存层的实现性质，不是抽象本身的要求——比如 `vector<vector<int>>` 同样用 `(i, j) → element` 定位元素，却不整块连续。

</details>

**一-2 [问答]** `int a[2][3][4];` 有几个轴？维数是多少？每个轴的轴长是多少？`a[1][2][3]` 里的 1、2、3 叫什么？

<details><summary>答案</summary>

3 个轴，维数（rank / ndim）是 3；轴长分别是 2、3、4；1、2、3 是这三个轴上的索引（index）。

</details>

**一-3 [问答]** 为什么数组的随机访问是 O(1)？链表和 map 为什么不是？

<details><summary>答案</summary>

数组的"定位规则"是一个能直接算出位置的公式（索引空间是整数区间的乘积，位置由 stride 公式算出），所以一步到位。链表找第 k 个元素要从头走 k 步；map 的 key 没有这种公式，只能靠查找（哈希或树）来定位。

</details>

**一-4 [判断]** "4 维以上的数组没法想象，所以高维数组没有可靠的理解模型。"

<details><summary>答案</summary>

错。把维度想成"线 / 面 / 体"，在 4 维以上确实会失效；但更可靠的模型是 index-tuple：维度只是"需要几个坐标定位一个元素"，`(i₁,…,iₙ) → element`，100 维也只是 100 个坐标，没有额外的几何含义。

</details>

**一-5 [问答]** 数组的抽象层、语言/类型层、内存层分别指什么？为什么这三层不能混为一谈？

<details><summary>答案</summary>

抽象层：数据集合 + 索引空间 + 定位规则；语言/类型层：C++ 内置数组 = array of arrays；内存/实现层：内置数组元素连续存储。

三层各管各的：比如"元素连续"只属于内存层——它保证 `&a[i+1] == &a[i] + 1` 这类代码成立，但不是数组抽象本身的要求，也不等于"下标可以越界"。

</details>

**一-6 [问答]** 既然有内置数组，为什么还有 `std::array` 和 `vector`？各补了内置数组的什么？

<details><summary>答案</summary>

内置数组 `T[N]` 的 N 是类型的一部分（编译期常量），元素连续、几乎零开销，但不能拷贝/赋值，还会退化成指针、丢掉长度 → `std::array` 把数组包进 struct，补上值语义。

运行期才知道大小时，`new T[n]` 只给一个不带大小的指针、要手动 `delete[]` → `vector` 把 `new[]`、size/capacity、拷贝和 RAII 封装起来。

</details>

## 二、声明、初始化与赋值

> array.md：How · Operations 1（声明）、2（初始化与赋值） ｜ 自测清单：二

**二-1 [默写]** ① 声明一个指针 `p`，指向包含 20 个 `double` 的数组；② 声明一个引用 `r`，绑定到包含 5 个 `char` 的数组。

<details><summary>答案</summary>

```cpp
double (*p)[20];
char (&r)[5];
```

</details>

**二-2 ★ [问答]** 由内向外读出下面三个声明，并说明为什么必须加括号：

```cpp
int (*p)[10];
int (&r1)[10];
int *(&r2)[10];
```

<details><summary>答案</summary>

`p`：指针，指向含 10 个 `int` 的数组；`r1`：引用，绑定到含 10 个 `int` 的数组；`r2`：引用，绑定到含 10 个 `int*`（int 指针）的数组。

`[]` 的优先级比 `*`、`&` 高：不加括号，`int *p[10]` 会被解析成"10 个 `int*` 组成的数组"，而不是"指向数组的指针"。

</details>

**二-3 [找错]** `int &r[10];` 哪里错？想要"绑定到数组的引用"该怎么写？

<details><summary>答案</summary>

不存在"引用的数组"（数组的元素必须是对象，引用不是对象），编译报错。想要绑定到数组的引用，写成 `int (&r)[10];`。

</details>

**二-4 [判断]** "标准 C++ 里，`int n = 5; int a[n];` 是合法的。"

<details><summary>答案</summary>

错。数组的长度必须是编译期常量，标准 C++ 没有变长数组。运行期才知道大小，用 `vector`（见一-6）。

</details>

**二-5 [推导]** ① `int a[] = {1, 2, 3, 4};` 的类型和长度；② `int a[5] = {1, 2};` 中 `a[2]`、`a[3]`、`a[4]` 的值；③ `char s[] = "hello";` 的类型、`sizeof(s)`、最后一个元素。

<details><summary>答案</summary>

① `int[4]`，长度 4——省略长度时由初始化列表的元素个数推导。

② 都是 0——显式写了 `{...}` 时，不够的元素被值初始化（`int` 补 0）；这和"默认初始化 `int a[5];`（元素值未定义，全局/静态数组除外）"是两回事。

③ `char[6]`；6；`'\0'`——字符串字面量自带结尾的 `\0`，它本身也算一个数组元素。

</details>

**二-6 [找错]** 下面哪几行编译失败？为什么？怎么绕过？

```cpp
int b[4] = {};
int d[4] = b;
int c[4];
c = b;
```

<details><summary>答案</summary>

`int d[4] = b;` 和 `c = b;` 都编译失败：数组不支持整体拷贝和赋值（内置数组没有普通类对象那样的值语义）。

要拷贝，可以逐元素复制、用 `std::copy`，或者把数组包进 `std::array`（它有值语义，可以直接拷贝/赋值）。

</details>

## 三、退化：数组与指针

> array.md：How · Operations 3（数组与指针的关系） ｜ 自测清单：三

**三-1 [问答]** 数组发生 array-to-pointer 退化时，哪 5 个场景是明确的例外？

<details><summary>答案</summary>

`sizeof`、`&`（取地址）、`typeid`、`decltype`、绑定到引用。

</details>

**三-2 [判断]** 对普通数组变量 `int arr[10];`（不是函数参数），`sizeof(arr)` 和 `sizeof(&arr[0])` 的结果一定相等。

<details><summary>答案</summary>

错。`sizeof(arr)` 是整个数组的字节数（`sizeof(int)*10`，通常 40）；`&arr[0]` 是 `int*`，`sizeof` 它得到的是一个指针的大小（通常 8）。这就是 `sizeof` 这个例外。

</details>

**三-3 [推导]** `int a[4];` ——`a + 1`、`&a[0] + 1`、`&a + 1` 分别比起点多走几个字节？它们的类型各是什么？

<details><summary>答案</summary>

`a + 1`：`int*`，4 字节；`&a[0] + 1`：`int*`，4 字节；`&a + 1`：`int (*)[4]`，16 字节（跨过整个数组）。

三者起点相同、步长不同，差别只来自类型。

</details>

**三-4 ★ [推导]** `int a[10]; auto x = a; auto& y = a; auto p = &a;` ——`x`、`y`、`p` 分别是什么类型？

<details><summary>答案</summary>

`x` 是 `int*`（`auto` 按值推导，数组退化）；`y` 是 `int (&)[10]`（`auto&` 按引用推导，引用绑定不退化，和"绑定到引用"这条例外是同一件事）；`p` 是 `int (*)[10]`（`&a` 本身就是取地址的例外，结果已经是"指向整个数组"的指针，`auto` 原样推导，不会再退化一次）。

</details>

**三-5 ★ [推导]** `int a[10]; decltype(a) x; decltype((a)) y;` ——`x`、`y` 分别是什么类型？为什么？

<details><summary>答案</summary>

`x` 是 `int[10]`，`y` 是 `int (&)[10]`。

`decltype(a)` 里 `a` 是一个"未加括号的 id-expression"，命中一条特殊规则：直接拿实体 `a` 声明时的类型，原样返回 `int[10]`，不看值类别。

`decltype((a))` 里 `(a)` 加了括号，不再是"未加括号的 id-expression"，退回一般规则：先看 `(a)` 是左值还是右值——具名数组是左值，按"左值 → T&"，结果是 `int (&)[10]`。

一句话记忆：**括号会把"问声明类型"变成"问表达式的值类别"**。这条规则对普通变量同样成立：`int i; decltype(i)` 是 `int`，`decltype((i))` 是 `int&`。

</details>

**三-6 [判断]** `int a[10];` 下面哪些会发生 array-to-pointer 退化？A. `auto p = a;`　B. `auto& r = a;`　C. `sizeof(a);`　D. `&a;`　E. `int* p = a;`

<details><summary>答案</summary>

会退化：A、E。不会退化：B（引用绑定）、C、D（`sizeof`、`&` 是明确的例外）。

</details>

## 四、传参与返回

> array.md：How · Operations 4（数组作为函数参数）、5（函数返回指向数组的指针） ｜ 自测清单：四

**四-1 [判断]** `void f(int arr[10]);` 和 `void f(int arr[]);` 是两个重载的不同函数。

<details><summary>答案</summary>

错。形参里的数组都被调整为指向元素类型的指针，这两个声明（连同 `void f(int*);`）的参数类型都是 `int*`，是同一个函数的重复声明，不是重载；`[10]` 不参与形成参数类型。

</details>

**四-2 ★ [找错]** 下面的函数想打印 40，实际会打印什么？为什么？

```cpp
void printArray(int arr[10]) {
    std::cout << sizeof(arr) << std::endl;
}
```

<details><summary>答案</summary>

打印指针的大小（通常 8）。`arr` 作为形参已经是 `int*`，`sizeof(arr)` 得到的是指针的大小，不是数组的大小——这是"sizeof 数组形参"最经典的陷阱。

</details>

**四-3 [问答]** 函数拿不到数组的原始长度时，有哪几种办法？（说出 3 个以上即可）

<details><summary>答案</summary>

① 哨兵/结束标记（如 C 风格字符串的 `\0`）；② 标准库 begin/end 约定（传两个指针）；③ 显式传长度参数；④ 数组引用形参（不退化，保留长度）；⑤ 数组引用形参 + 模板推导长度，消除"只能匹配固定长度"的限制。

</details>

**四-4 [默写]** 写一个函数 `print`，用"数组引用形参"的方式接收一个包含 8 个 `int` 的数组（不退化，函数内部能用 range-for 拿到真实长度）。

<details><summary>答案</summary>

```cpp
void print(int (&arr)[8]) {
    for (auto elem : arr) std::cout << elem << std::endl;
}
```

</details>

**四-5 [问答]** 数组引用形参的代价是什么？怎么消除？

<details><summary>答案</summary>

代价：只能匹配长度恰好是 8 的数组，别的长度编译失败。消除办法：把长度做成非类型模板参数，让编译器针对每次调用各自推导。

</details>

**四-6 [默写]** 用函数模板写一个 `array_size`，对任意元素类型、任意长度的数组都返回它的长度。

<details><summary>答案</summary>

```cpp
template <class T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) { return N; }
```

引用形参不退化，所以 N 能被推导出来；`std::begin` / `std::end` 内部靠的就是同一个机制。

</details>

**四-7 [问答]** "返回指向数组的指针"这个类型，有哪四种写法？（以 `int (*)[10]` 为例）

<details><summary>答案</summary>

① 原始形式 `int (*func(int i))[10]`；② 先给数组类型起别名，再 `arrT* func(int i)`（`typedef int arrT[10];`，此时 `func` 返回 `int (*)[10]`）；③ 尾置返回类型 `auto func(int i) -> int(*)[10]`；④ 用 `decltype` 借已有数组的类型，如 `decltype(odd) *arrPtr(int i)`。

</details>

**四-8 [默写]** ① 用"尾置返回类型"声明函数 `func`：接收一个 `int`，返回指向包含 6 个 `int` 的数组的指针；② 再用 `using` 起别名的方式写出等价的声明。

<details><summary>答案</summary>

```cpp
auto func(int i) -> int(*)[6];   // ① 尾置返回类型

using arrT = int[6];             // ② 别名版
arrT* func(int i);
```

</details>

**四-9 [找错]** 下面这段代码有几处问题？

```cpp
int[10] getArray() {
    int a[10] = {1,2,3,4,5,6,7,8,9,10};
    return a;
}
```

<details><summary>答案</summary>

两处：① 函数的返回类型不能是数组类型，`int[10]` 这种写法本身不合法（只能返回指向数组的指针/引用，或包住数组的类对象，比如 `std::array`）；② 就算改成返回 `int (*)[10]` 或 `int*`，`a` 是函数内的局部数组，函数返回后已经销毁，返回它的指针会悬空——只有数组是 `static`、全局，或堆上分配时才安全（这一点 array.md 没有展开，但很容易一起踩）。

</details>

## 五、多维（一）：类型

> array.md：How · 多维 1（内置多维数组索引求值）、Note 第一部分（多维数组作为参数） ｜ 自测清单：五

**五-1 [推导]** `int a[3][4];` ——`a`、`a + 1`、`a[0]`、`a[0] + 1`、`a[0][0]` 分别是什么？

<details><summary>答案</summary>

`a` 在值语境里退化为 `int (*)[4]`（指向第 0 行）；`a + 1` 指向下一行（`a[1]`，一次跳过整个 `int[4]`，不是一个 `int`）；`a[0]` 本身是 `int[4]`（一整行），当值用时才退化成 `int*`；`a[0] + 1` 指向 `a[0][1]`；`a[0][0]` 就是一个普通 `int`。

</details>

**五-2 ★ [找错]** 下面这行为什么编译不过？有哪几种改法？

```cpp
int a[3][4];
int* p = a;
```

<details><summary>答案</summary>

`a` 的元素类型是 `int[4]`（一整行），退化后是 `int (*)[4]`，不是 `int*`，类型不匹配。

改法：① `int (*p)[4] = a;`（指向"行"的指针）；② `int* p = a[0];`——`a[0]` 是 `int[4]`，它退化成 `int*`，指向 `a[0][0]`；③ `int* p = &a[0][0];`。

</details>

**五-3 ★ [推导]** `int a[3][4]; int (*p)[4] = a;` ——① `p` 指向什么 ② `p + 1` 指向什么 ③ `(*p)[2]` 是什么 ④ `p[1][2]` 是什么？

<details><summary>答案</summary>

① `a[0]`（第 0 行）　② `a[1]`　③ `a[0][2]`　④ `a[1][2]`。

这题做熟了，"pointer-to-array"就不再是奇怪的声明语法，而是"行指针"——加 1 就是跳到下一行。

</details>

**五-4 ★ [推导]** `int a[2][3][4]; auto p = a;` ——`p` 是什么类型？

<details><summary>答案</summary>

`int (*p)[3][4];`。`a` 的元素类型是去掉最外层维度后剩下的 `int[3][4]`，退化就是"指向元素类型的指针"。

</details>

**五-5 [判断]** `void print(int (*cube)[3][4], int size1);` 和 `void print(int cube[][3][4], int size1);` 是等价写法。

<details><summary>答案</summary>

对。多维数组作形参，只有最外层维度能省略，这两种写法是同一个函数。

</details>

**五-6 [问答]** 为什么 `void print(int matrix[][10], int rowSize)` 里第二个维度 `10` 不能省略，但最外层维度可以省略？

<details><summary>答案</summary>

退化只发生一次，把整个数组打包成一个指针；这个指针指向的"元素类型"是去掉最外层维度后剩下的部分（这里是 `int[10]`），编译器要靠这个类型算 `sizeof`、做指针算术（`matrix + 1` 要跳过 `10 * sizeof(int)` 字节），所以它必须是编译期已知的合法类型，不能省略。

最外层维度 `N1` 之所以能省略，是因为退化后它根本不出现在结果类型里——退化把它整个替换成了"指针"本身。

</details>

**五-7 ★ [推导]** `int a[3][4];`（设 `sizeof(int)==4`）——`sizeof(a)`、`sizeof(a[0])`、`sizeof(a[0][0])` 分别是多少？再对 `T a[N1][N2][N3];` 写出这三个以及 `sizeof(a[0][0][0])` 的通用公式。

<details><summary>答案</summary>

48、16、4。通用公式：`sizeof(a)` = N1×N2×N3×sizeof(T)；`sizeof(a[0])` = N2×N3×sizeof(T)；`sizeof(a[0][0])` = N3×sizeof(T)；`sizeof(a[0][0][0])` = sizeof(T)。

正因为 `sizeof` 不退化，才能拿到整个数组/整行的真实大小，而不是一个指针的大小。

</details>

**五-8 [推导]** `int a[3][4];` ——`a + 1`、`a[0] + 1`、`&a + 1` 各比起点多走几个字节？为什么起点相同、步长不同？

<details><summary>答案</summary>

16、4、48。三者的类型分别是 `int (*)[4]`、`int*`、`int (*)[3][4]`，步长 = 所指对象的大小（`int[4]` 是 16，`int` 是 4，`int[3][4]` 是 48）。指针值里只有地址，步长在类型里。

</details>

## 六、多维（二）：布局

> array.md：How · 多维 2（三种"二维数据"的实现对比）、3（stride / layout） ｜ 自测清单：六

**六-1 [判断]** 不管一维还是多维，C++ 内置数组的元素在内存里都是连续存放的。

<details><summary>答案</summary>

对。这是 ISO C++ 标准规定的，多维数组作为 array of arrays，元素连续存储的性质会一路传递下去。

</details>

**六-2 [问答]** 比较 `int a[3][4]`、`std::array<std::array<int,4>,3>`、`std::vector<std::vector<int>>`：大小何时确定？存在哪里？是否整块连续？

<details><summary>答案</summary>

- `int a[3][4]`：编译期确定；栈 / 静态存储；整块连续（标准保证）。
- `std::array<std::array<int,4>,3>`：编译期确定；栈 / 静态存储；实践上整块连续，但标准只保证每一层内部连续。
- `std::vector<std::vector<int>>`：运行期确定；free store；每个内层 vector 独立连续，行与行之间不保证相邻，访问时多一层间接（好处是各行可以不等长）。

需要紧凑二维布局时，常见做法是用一维 `vector<int>`，自己算下标 `a[i*cols + j]`。

</details>

**六-3 [推导]** `shape=(2,3,4)` 的 row-major 数组（stride 按元素计）：每个轴的 stride 是多少？`a[1][2][3]` 的 offset 是多少？

<details><summary>答案</summary>

stride = (12, 4, 1)：某一维的 stride 等于其右侧所有维度长度的乘积。`offset(i,j,k) = i*12 + j*4 + k*1`，所以 `a[1][2][3]` 的 offset 是 1×12 + 2×4 + 3×1 = 23。

</details>

**六-4 [问答]** C++ 内置多维数组是 row-major 还是 column-major？能改吗？哪些库默认 column-major，混用时会踩什么坑？

<details><summary>答案</summary>

row-major（先走完一整行再换行）。不能改：它是"元素连续 + 多维数组是 array of arrays"这两条规则直接推出的唯一结果，不是可配置的默认值（NumPy 才能用 `order='C'` / `'F'` 选）。

Fortran、MATLAB、Eigen 默认 column-major；用 Eigen、MATLAB，或用 BLAS/LAPACK 传递裸内存时，如果对方是 column-major 而你按 row-major 理解，维度顺序会理解反。

</details>

## 七、因果链：为什么是这样

> array.md：Note 第二部分 ｜ 自测清单：七

先自己说，再对照 array.md 的 Note 第二部分。

**七-1 ★ [问答]** 用"于是"把数组退化的因果链说出来（4~5 句）。

<details><summary>答案</summary>

① 结构体里的数组要"就是那几个字节"，B 里那个存首地址的"指针格子"没处放、嵌套对象里的格子也没法初始化——**于是取消格子**。

② **于是**数组对象就是元素本身，指针只在表达式里现算：数组名指代对象，只有被当成值用时才退化，`a[i]` 定义为 `*(a+i)`，形参只收到指针。

③ **于是**指针类型必须带步长：留内层维度（它是元素类型的一部分），丢最外层数量。

④ **于是**类型能保证寻址和形状一致；数量没了，边界保证不了。

</details>

**七-2 [判断]** "数组退化是 C 为了写起来方便，专门设计的一个特性。"

<details><summary>答案</summary>

错。它是为了解决"结构体里的数组没处放指针"：取消指针格子之后，指针改成表达式里现算出来的值——"values of array type are converted, when they appear in expressions, into pointers to the first of the objects making up the array"。

</details>

**七-3 [问答]** "取消格子"是拿时间换空间吗？

<details><summary>答案</summary>

不是，时间和空间都省了：不再为每个数组多存一个格子，不用在进入作用域时初始化它，也不用每次访问前先读一次它。更准确的说法是：**把运行期的一个存储，换成了编译期的一条类型规则**。代价转到了语言规则上：数组 ≠ 指针，数组表达式却能隐式变成指针。

</details>

**七-4 [问答]** `int a[10];` 和 `int* p = new int[10];` 里各有几个"指针对象"？哪个像 B 的"格子 + 存储"？

<details><summary>答案</summary>

`a` 里没有指针对象，只有 10 个 `int`（`sizeof(a)` 是 40）；`new int[10]` 那个 `p` 是你显式写出的指针对象，加上 `new[]` 创建的存储，才像 B 的"格子 + 存储"。

</details>

**七-5 [问答]** 数组名为什么不能被赋值？

<details><summary>答案</summary>

B 里数组名背后有一个可以改的格子，改格子就是"挪起点"；C 取消格子之后，数组名不再是可赋值的变量（原文："possible in B and BCPL, meaningless in C"）。至于"整体拷贝/赋值为什么也不支持"，原文没直接说，是推论。

</details>

**七-6 [问答]** 形参里的 `int a[10]` 为什么只是个指针？分两步说。

<details><summary>答案</summary>

① 实参 `arr` 出现在表达式里，先退化成指针，传进去的就是一个指针值；② `int a[]` 本来是 NB 里"声明指针"的写法，C 只在形参位置保留并按指针解释。

所以形参里 `sizeof(a)` 是指针的大小，`[10]` 不参与参数类型。

</details>

**七-7 [问答]** 退化保留了什么、丢掉了什么？各带来什么后果？想把"数量"留住有哪几种写法？

<details><summary>答案</summary>

保留内层维度（它是元素类型的一部分）：寻址要用，所以形参必须写全，形状不同的指针类型不能互转。丢掉最外层数量：第一维可以省略，但函数拿不到行数，也没有边界信息。

想留住数量，就别让它退化：`int (&r)[3][4] = a;`、`int (*w)[3][4] = &a;`（`sizeof` 都是 48，3 还在类型里），或者用模板从数组引用形参里推导 N（`compare` / `array_size`）。

</details>

**七-8 [判断]** "类型系统保证数组访问不会越界。"

<details><summary>答案</summary>

错。类型保证的是寻址正确和形状一致，不保证访问合法：对 `int a[3][4]`，`a[0][4]`、`a[3][0]` 都是 UB；"连续 ≠ 可越界"——用 `&a[0][0]` 当一维指针遍历整个二维数组，形式上也是 UB。需要长度时，用不退化的手段把长度带在类型或对象里（引用、`std::array`）。

</details>

## 八、综合与动手

> array.md：全篇 ｜ 自测清单：八

**八-1 ★ [推导]** 给定 `double a[5][6][7];`，不看资料，完整回答：

```text
① a 的完整类型
② a 的元素类型
③ a 在值语境下的退化类型
④ a[0] 的类型
⑤ a[0][0] 的类型
⑥ a[0][0][0] 的类型
⑦ auto p = a; 中 p 的类型
⑧ double (*p)[6][7] = a; 合法吗？int (*p)[6][7] = a; 呢？
⑨ a + 1 向后移动多大一个对象？
⑩ double* q = a[0][0]; 合法吗？double* q = a[0]; 呢？
```

<details><summary>答案</summary>

① `double[5][6][7]`　② `double[6][7]`　③ `double(*)[6][7]`　④ `double[6][7]`　⑤ `double[7]`　⑥ `double`　⑦ `double(*)[6][7]`　⑨ 一个 `double[6][7]`（6×7×8 = 336 字节）

⑧ `double (*p)[6][7] = a;` 合法——这正是 `a` 退化后的类型。`int (*p)[6][7] = a;` **不合法**：形状 `[6][7]` 对得上，但元素类型是 `int` 不是 `double`，指针类型必须连元素类型一起匹配，光维度对上不够。

⑩ `double* q = a[0][0];` 合法——`a[0][0]` 是 `double[7]`，退化成 `double*`。`double* q = a[0];` **不合法**——`a[0]` 是 `double[6][7]`，退化成 `double(*)[7]`，不是 `double*`。

</details>

**八-2 [动手]** 编译运行，对照结论。代码里的注释就是结论：可以先遮住注释，自己预测每条 `static_assert` 是否成立、屏幕输出什么，再核对。用 `-std=c++17 -Wall -Wextra -pedantic` 编译（g++ 13.3 下验证，另在 g++ 11.4、clang 14 下复核一致）。

```cpp
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
using std::is_same_v;

template <class T, std::size_t N>
constexpr std::size_t array_size(const T (&)[N]) { return N; }   // 引用不退化 -> N 可推导

int main() {
    int b[4] = {};
    int a[3][4] = {};

    // ---- 类型：编译通过就是验证通过 ----
    static_assert(is_same_v<decltype(b),     int[4]>);         // decltype：不退化
    static_assert(is_same_v<decltype(b + 1), int*>);           // 表达式里：退化
    static_assert(is_same_v<decltype(&b),    int (*)[4]>);     // &b：T(*)[N]
    static_assert(is_same_v<decltype(a + 0), int (*)[4]>);     // 二维：退化成"指向一行"
    static_assert(is_same_v<decltype(a[1]),  int (&)[4]>);     // a[i] 仍是数组
    auto c = b;                                                // auto 不拷贝数组
    static_assert(is_same_v<decltype(c), int*>);
    static_assert(sizeof(std::array<int, 4>) == sizeof(int[4]));   // 实践上 std::array 无额外开销

    // ---- 步长与大小 ----
    auto addr = [](const void* p) { return reinterpret_cast<std::uintptr_t>(p); };
    std::cout << addr(b + 1) - addr(b) << ' '                  // 4  跨一个元素
              << addr(&b + 1) - addr(&b) << ' '                // 16 跨整个数组
              << array_size(a) << ' ' << array_size(a[0]) << '\n';   // 3 4   ⚠️ 易错：是 3 不是 12，见答案

    // ---- 值语义：内置数组不能拷贝，包进 struct（std::array）就能 ----
    // int d[4] = b;                                           // ✗ 编译失败
    std::array<int, 4> s1{1, 2, 3, 4};
    auto s2 = s1;                                              // ✓ 拷贝
    std::cout << s2[3] << '\n';                                // 4

    // ---- range-for：二维数组必须用引用 ----
    for (auto& row : a)        // 写成 auto row 会编译失败：row 是 int*
        for (auto x : row) (void)x;
}
```

<details><summary>答案</summary>

无警告通过，输出两行：`4 16 3 4` 和 `4`。（`b + 1` 跨 4 字节，`&b + 1` 跨 16 字节；`array_size(a)` 是 3，`array_size(a[0])` 是 4。）

⚠️ 易错：`array_size(a)` 是 3，不是 3×4 = 12。它问的是"外层有几个元素"（像 Python 的 `len`），不是"一共有几个 `int`"。

- 从外往里读类型：`int a[3][4]` 是"3 个 `int[4]` 组成的数组"，它的元素是一整行（`sizeof(a[0])` 是 16）。
- `const T (&)[N]` 匹配 `int[3][4]` 只有一种对法：`N = 3`，`T = int[4]`，只剥最外层。再剥一层才是 `array_size(a[0])`：`N = 4`，`T = int`。
- 可以结合int *p = a[0],或者 int (*p)[4] = decltype(a[0])一起理解

</details>

**八-3 [动手]** "连续"和"越界"的区别。用 `g++ -std=c++17 -O0 -fsanitize=bounds` 编译。注释同样是结论，先遮住，预测 stdout 和 stderr 各输出什么。

```cpp
#include <cstdio>
int main() {
    static int a[3][4];
    for (int i = 0; i < 12; ++i) (&a[0][0])[i] = i;      // 内存层：12 个 int 连续，一维指针写得进去
    int* p = &a[0][0];
    std::printf("%d %d\n", p[5], a[1][1]);                // 5 5
    volatile int k = 5;
    std::printf("%d\n", a[0][k]);                         // 语言层：下标越出内层 int[4]
                                                          // → 报 runtime error: index 5 out of bounds for type 'int [4]'
}
```

<details><summary>答案</summary>

stdout 输出 `5 5` 和 `5`（越界那一行实际读到的是 `a[1][1]`），同时 stderr 报 `runtime error: index 5 out of bounds for type 'int [4]'`。报错走 stderr，在终端里和 stdout 的先后顺序可能不同。

</details>

## 复习建议

- **第一遍**：按章从前往后做，每章先做自测清单里同一章的【背】【推】，卡住了回 array.md 对应小节，再来做这里的题。
- **之后**：只做 ★（10 道）和上次做错的；错题所在章，回到 array.md 对应小节精读一次，不要靠背答案。
- **动手验证**（八-2、八-3）：不是刷题，是把结论交给编译器验证一遍；想巩固哪一章，就回来跑对应的几行。
