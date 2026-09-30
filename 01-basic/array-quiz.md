# Array 语法练习题

配合 [array.md](./array.md) 使用，针对"没有道理可讲、只能靠刷熟练度记住"的语法约定（笔记里的"第三档"）。每题答案放在下面的折叠块里，**先自己答，再点开看答案**，别直接翻答案。

## 一、判断题（对/错，并说出理由）

1. 对于普通数组变量 `int arr[10];`（不是函数参数），`sizeof(arr)` 和 `sizeof(&arr[0])` 的结果一定相等。

   <details><summary>答案</summary>错。<code>sizeof(arr)</code> 是整个数组的字节数（<code>sizeof(int)*10</code>，通常 40）；<code>&arr[0]</code> 退化后是 <code>int*</code>，<code>sizeof</code> 它得到的是一个指针的大小（通常 8），两者不相等。</details>

2. `void f(int arr[10]);` 和 `void f(int arr[]);` 是两个重载的不同函数。

   <details><summary>答案</summary>错。数组做形参时都会退化成指向元素类型的指针，这两个声明的参数类型都是 <code>int*</code>，是同一个函数的重复声明，不是重载。</details>

3. `int a[10]; int (&r)[10] = a;` 中，`r` 绑定之后的类型是 `int*`。

   <details><summary>答案</summary>错。数组绑定到引用时保留原始类型，`r` 的类型是"对 <code>int[10]</code> 的引用"，不会退化成 <code>int*</code>。</details>

4. 不管一维还是多维，C++ 内置数组的元素在内存里都是连续存放的。

   <details><summary>答案</summary>对。这是 ISO C++ 标准规定的，多维数组作为 array of arrays，元素连续存储的性质会一路传递下去。</details>

5. 把 `arr` 直接放进一个普通表达式参与运算（比如 `arr + 1`），跟 `decltype(arr)` 拿到的类型是一样的。

   <details><summary>答案</summary>错。<code>arr + 1</code> 里 <code>arr</code> 会发生 array-to-pointer 退化；<code>decltype(arr)</code> 是明确的退化例外之一，拿到的是数组本身的类型，两者不一样。</details>

6. C++ 允许你写一个返回类型是 `int[10]` 的函数。

   <details><summary>答案</summary>错。C++ 标准规定函数的返回类型不能是数组类型，只能返回指向数组的指针/引用，或者包住数组的类对象（比如 <code>std::array</code>）。</details>

7. `void print(int (*cube)[3][4], int size1);` 和 `void print(int cube[][3][4], int size1);` 是等价写法。

   <details><summary>答案</summary>对。多维数组作参数只有最外层维度能省略，这两种写法是同一个函数。</details>

8. `std::vector<std::vector<int>>` 的底层内存布局和 `int a[3][4]` 一样，都是一整块连续内存。

   <details><summary>答案</summary>错。<code>vector&lt;vector&gt;</code> 每个内层 vector 独立管理自己的连续存储，行与行之间不保证相邻；只有 <code>int a[3][4]</code> 和 <code>std::array&lt;std::array&lt;int,4&gt;,3&gt;</code> 是真正整块连续的。</details>

## 二、问答题

1. 数组发生 array-to-pointer 退化时，有哪 5 个明确的例外场景？

   <details><summary>答案</summary><code>sizeof</code>、<code>&</code>（取地址）、<code>typeid</code>、<code>decltype</code>、绑定到引用。</details>

2. 函数拿不到数组原始长度的问题，笔记里提到了哪几种解决办法？（说出 3 个以上即可）

   <details><summary>答案</summary>①哨兵/结束标记（如 C 风格字符串的 <code>\0</code>）②标准库 begin/end 约定 ③显式传长度参数 ④数组引用形参（不退化，保留长度）⑤用模板对数组引用形参做长度推导，消除固定长度限制。</details>

3. "返回指向数组的指针"这个类型，笔记里给出了哪几种写法？

   <details><summary>答案</summary>①原始形式 <code>Type (*func(...))[N]</code> ②给数组类型起别名后 <code>arrT* func(...)</code> ③尾置返回类型 <code>auto func(...) -> Type(*)[N]</code> ④用 <code>decltype</code> 匹配已有数组的类型。</details>

4. 为什么 `void print(int matrix[][10], int rowSize)` 里第二个维度 `10` 不能省略，但最外层维度可以省略？

   <details><summary>答案</summary>退化只发生一次，把整个数组打包成一个指针；这个指针指向的"元素类型"是去掉最外层维度后剩下的部分（这里是 <code>int[10]</code>），编译器要靠这个类型算 <code>sizeof</code>、做指针算术（<code>matrix+1</code> 要跳过 <code>10*sizeof(int)</code> 字节），所以它必须是编译期已知的合法类型，不能省略。最外层维度 <code>N1</code> 之所以能省略，是因为退化后它根本不出现在结果类型里——退化把它整个替换成了"指针"本身。</details>

5. `typedef int arrT[10]; arrT* func(int i);` 中，`func` 返回的是什么类型？

   <details><summary>答案</summary><code>arrT</code> 就是 <code>int[10]</code>，所以 <code>func</code> 返回一个指向"含 10 个 int 的数组"的指针，等价于 <code>int (*)[10]</code>。</details>

## 三、默写题（凭记忆写代码，不许看笔记，写完再对答案）

1. 声明一个指针 `p`，指向一个包含 20 个 `double` 的数组。

   <details><summary>答案</summary>

   ```cpp
   double (*p)[20];
   ```

   </details>

2. 声明一个引用 `r`，绑定到一个包含 5 个 `char` 的数组。

   <details><summary>答案</summary>

   ```cpp
   char (&r)[5];
   ```

   </details>

3. 写一个函数 `print`，用"数组引用形参"的方式接收一个包含 8 个 `int` 的数组（不退化，函数内部能用 range-for 拿到真实长度）。

   <details><summary>答案</summary>

   ```cpp
   void print(int (&arr)[8]) {
       for (auto elem : arr) std::cout << elem << std::endl;
   }
   ```

   </details>

4. 用函数模板写一个 `length` 函数，对任意元素类型 `T`、任意长度的数组都能返回其长度（提示：非类型模板参数）。

   <details><summary>答案</summary>

   ```cpp
   template<typename T, unsigned N>
   unsigned length(T (&arr)[N]) {
       return N;
   }
   ```

   </details>

5. 用"尾置返回类型"写法，声明一个函数 `func`，接收一个 `int`，返回一个指向"含 6 个 int 的数组"的指针。

   <details><summary>答案</summary>

   ```cpp
   auto func(int i) -> int(*)[6];
   ```

   </details>

6. 用 `typedef`/`using` 起别名的方式，实现和第 5 题相同效果的函数声明。

   <details><summary>答案</summary>

   ```cpp
   using arrT = int[6];
   arrT* func(int i);
   ```

   </details>

7. 声明一个变量 `matrix`，类型要能作为 `void f(int (*matrix)[4], int rowSize)` 里 `matrix` 的实参——也就是"指向含 4 个 int 的一维数组"的指针。

   <details><summary>答案</summary>

   ```cpp
   int (*matrix)[4];
   ```

   </details>

## 四、找错题（下面的代码有问题，指出来并说明原因）

1.
   ```cpp
   int[10] getArray() {
       int a[10] = {1,2,3,4,5,6,7,8,9,10};
       return a;
   }
   ```

   <details><summary>答案</summary>两处问题：① 函数返回类型不能是数组类型，<code>int[10]</code> 这种写法本身不合法；② 就算改成返回 <code>int (*)[10]</code> 或 <code>int*</code>，<code>a</code> 是函数内的局部数组，函数返回后 <code>a</code> 就销毁了，返回它的指针/引用会是悬空指针——这种写法只有在数组是 <code>static</code>、全局变量、或堆上分配时才安全（这一点笔记里没直接展开，是数组作为返回值时很容易一起踩的坑）。</details>

2.
   ```cpp
   void printArray(int arr[10]) {
       std::cout << sizeof(arr) << std::endl;  // 期望打印 40
   }
   ```

   <details><summary>答案</summary><code>arr</code> 作为函数参数已经退化成 <code>int*</code>，<code>sizeof(arr)</code> 得到的是指针的大小（通常 8），不是数组的大小（40）——这是"sizeof 数组参数"最经典的陷阱。</details>

3.
   ```cpp
   int a[3][4];
   int *p = a;   // 想让 p 指向 a 的首元素
   ```

   <details><summary>答案</summary>多维数组 <code>a</code> 的"首元素"是 <code>int[4]</code>（一整行），退化后应该是 <code>int(*)[4]</code> 类型，不能直接赋值给 <code>int*</code>，类型不匹配，编译不过。</details>

## 五、初始化与长度推导

1. `int a[] = {1, 2, 3, 4};` —— `a` 的类型和长度分别是什么？

   <details><summary>答案</summary>类型 <code>int[4]</code>，长度 4。省略长度时，编译器按初始化列表的元素个数推导。</details>

2. `int a[5] = {1, 2};` —— `a[2]`、`a[3]`、`a[4]` 的值分别是什么？

   <details><summary>答案</summary>都是 0。用列表初始化时，初始化器不够的剩余元素会被值初始化（对 <code>int</code> 就是补 0）——这跟"默认初始化不给初值、元素值未定义"是两回事，区别在于有没有显式写 <code>{...}</code>。</details>

3. 默写：声明并初始化一个长度为 5 的 `double` 数组，前两个元素是 `1.5`、`2.5`，其余自动为 0。

   <details><summary>答案</summary>

   ```cpp
   double a[5] = {1.5, 2.5};
   ```

   </details>

4. `char a[] = "hello";` —— ① `a` 的类型 ② `sizeof(a)` ③ 最后一个元素是什么？

   <details><summary>答案</summary>① <code>char[6]</code> ② 6 ③ <code>'\0'</code>。<code>"hello"</code> 是 5 个字符，字符串字面量自带结尾的 <code>\0</code>，所以数组长度是 6，<code>\0</code> 本身也算一个数组元素。</details>

## 六、`auto` 与数组退化

这组把"数组本身 vs 数组退化"落实到 `auto` 推导规则上：`auto` 按值推导会退化，`auto&` 按引用推导不会。

1. `int a[10]; auto x = a; auto& y = a;` —— `x`、`y` 分别是什么类型？

   <details><summary>答案</summary><code>x</code> 是 <code>int*</code>（<code>auto</code> 按值推导，相当于把 <code>a</code> 传给一个"按值"的模板参数，数组退化）；<code>y</code> 是 <code>int (&)[10]</code>（<code>auto&</code> 按引用推导，引用绑定不退化，跟笔记里"绑定到引用"这条例外是同一件事）。</details>

2. `int a[10]; auto p = &a;` —— `p` 是什么类型？

   <details><summary>答案</summary><code>int (*p)[10];</code>。<code>&a</code> 本身就是笔记里讲的取地址例外，结果已经是"指向整个数组"的指针类型 <code>int(*)[10]</code>，<code>auto</code> 照原样推导，不会再发生第二次转换。</details>

3. 判断：`int a[10];` 下面哪些会发生 array-to-pointer 退化？`A. auto p = a;`　`B. auto& r = a;`　`C. sizeof(a);`　`D. &a;`　`E. int* p = a;`

   <details><summary>答案</summary>会退化：A、E。不会退化：B、C、D（B 是引用绑定，C、D 是笔记里明确列出的例外）。</details>

## 七、`decltype` 的一个经典坑：加不加括号

笔记里讲了 `decltype(arr)` 保留数组类型，这里补一个更细的区分——**同一个变量，加不加括号，结果可能完全不同**，写模板/泛型代码时很容易被坑。

1. `int a[10]; decltype(a) x; decltype((a)) y;` —— `x`、`y` 分别是什么类型？为什么？

   <details><summary>答案</summary>

   `x` 是 `int[10]`，`y` 是 `int (&)[10]`。

   `decltype(a)` 里 `a` 是一个"未加括号的 id-expression"，命中一条特殊规则：直接拿实体 `a` 声明时的类型，原样返回 `int[10]`，不看值类别。

   `decltype((a))` 里 `(a)` 加了括号，不再是"未加括号的 id-expression"，退回一般规则：先看 `(a)` 是左值还是右值——具名数组是左值，按"左值 → T&"，结果是 `int (&)[10]`。

   一句话记忆：**括号会把"问声明类型"变成"问表达式的值类别"**。这条规则对普通变量同样成立：`int i; decltype(i)` 是 `int`，`decltype((i))` 是 `int&`。

   </details>

## 八、多维数组的索引与指针算术

把"array of arrays + 数组退化 + pointer-to-array + 指针算术"串成一条线，值得反复做。

1. `int a[3][4];` —— `a`、`a + 1`、`a[0]`、`a[0] + 1`、`a[0][0]` 分别是什么？

   <details><summary>答案</summary><code>a</code> 在需要值的语境里退化为 <code>int(*)[4]</code>；<code>a + 1</code> 指向下一行（<code>a[1]</code> 的地址，一次跳过整个 <code>int[4]</code>，不是一个 <code>int</code>）；<code>a[0]</code> 本身是 <code>int[4]</code>，同样会退化成 <code>int*</code>；<code>a[0] + 1</code> 指向 <code>a[0][1]</code>；<code>a[0][0]</code> 就是一个普通 <code>int</code>。</details>

2. `int a[3][4]; int (*p)[4] = a;` —— ① `p` 指向什么 ② `p + 1` 指向什么 ③ `(*p)[2]` 是什么 ④ `p[1][2]` 是什么？

   <details><summary>答案</summary>① <code>a[0]</code>（第 0 行） ② <code>a[1]</code> ③ <code>a[0][2]</code> ④ <code>a[1][2]</code>。这题做熟了，"pointer-to-array"就不再是奇怪的声明语法，而是"行指针"——加 1 就是跳到下一行。</details>

3. 找错：`int a[3][4]; int* p = a;` 有什么问题？

   <details><summary>答案</summary><code>a</code> 的元素类型是 <code>int[4]</code>（一整行），退化后是 <code>int(*)[4]</code>，不是 <code>int*</code>，类型不匹配，编译不过。</details>

4. `int a[2][3][4]; auto p = a;` —— `p` 是什么类型？

   <details><summary>答案</summary><code>int (*p)[3][4];</code>。<code>a</code> 的元素类型是去掉最外层维度后剩下的 <code>int[3][4]</code>，退化就是"指向元素类型的指针"。</details>

## 九、`sizeof` 与多维数组

`sizeof` 是数组退化的例外之一，这组直接检验"逐层剥离"模型。

1. `int a[3][4];`（设 `sizeof(int)==4`）—— `sizeof(a)`、`sizeof(a[0])`、`sizeof(a[0][0])` 分别是多少？

   <details><summary>答案</summary><code>sizeof(a)</code> = 3×4×4 = 48；<code>sizeof(a[0])</code> = 4×4 = 16；<code>sizeof(a[0][0])</code> = 4。正因为 <code>sizeof</code> 不退化，才能拿到整个数组/整行的真实大小，而不是一个指针的大小。</details>

2. 对 `T a[N1][N2][N3];`，写出 `sizeof(a)`、`sizeof(a[0])`、`sizeof(a[0][0])`、`sizeof(a[0][0][0])` 的通用公式。

   <details><summary>答案</summary><code>sizeof(a)</code> = N1×N2×N3×sizeof(T)；<code>sizeof(a[0])</code> = N2×N3×sizeof(T)；<code>sizeof(a[0][0])</code> = N3×sizeof(T)；<code>sizeof(a[0][0][0])</code> = sizeof(T)。</details>

## 十、综合推导题（不看笔记，从声明现场推导）★

给定 `double a[5][6][7];`，不看资料，完整回答：

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
```

<details><summary>答案</summary>

① `double[5][6][7]`　② `double[6][7]`　③ `double(*)[6][7]`　④ `double[6][7]`　⑤ `double[7]`　⑥ `double`　⑦ `double(*)[6][7]`　⑨ 一个 `double[6][7]`

⑧ `double (*p)[6][7] = a;` 合法——这正是 `a` 退化后的类型。`int (*p)[6][7] = a;` **不合法**：形状 `[6][7]` 对得上，但元素类型是 `int` 不是 `double`，指针类型必须连元素类型一起匹配，光维度对上不够。

</details>

---

## 复习建议：两套题库怎么配合用

不用每次从头刷到尾，一~四和五~十是两种组织方式，按需求挑着用：

- **一~四（按题型：判断/问答/默写/找错）**：适合"扫一遍薄弱点"，随机跳着抽 5~8 道。
- **五~十（按主题：初始化/auto 退化/decltype/多维指针/sizeof/综合）**：适合"专攻一个知识点"，比如今天只想练多维指针算术，就只做第八节的 4 道，10 分钟内能做完。

信息密度最高、建议优先保留/常刷的几道：**六-1、七-1、八-2、八-4、九-1、十-1**——分别把 auto 退化、decltype 双坑、pointer-to-array、多维退化、sizeof 多维、完整类型推导串了起来。
