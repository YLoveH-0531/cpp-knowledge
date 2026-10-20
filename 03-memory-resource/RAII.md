# RAII 专题阅读路线：从设计动机到现代 C++ 的资源管理

综合前面两份回答和核实过的资料，我建议采用下面这版最终顺序。它比我最初给你的清单更精简，也更重视逻辑推导。

核心路线是：

设计动机

为什么手动管理资源容易出错？

RAII

把资源管理绑定到对象生命周期

拷贝与移动语义

对象复制或转移时，如何处理资源所有权？

智能指针与自定义删除器

如何表达和实施资源的管理责任？

异常安全与设计原则回顾

检查整套设计是否可靠

## 第一阶段：理解设计动机——最值得优先阅读

### 1. Bjarne Stroustrup：《Foundations of C++》

[Bjarne Stroustrup - The Essence of C++: With Examples in C++84, C++98, C++11, and C++14 | PPTX](https://images.openai.com/static-rsc-4/vpvTpWCAcijb9K4Arbbc-Gmklx1SnEHb-conbK3Vk1OSV3nWIky5ZQxV_tZ1nqfI83Om8yvVHQN4afGjljnafcb4tVfNCpf8iGLHJOLHDqbMZEHJjFp6HVEQFyTal6IkOYd6JoLTrck3VYYr1QKnA4-j__JEY-GkB1UpNRnKlhE?purpose=inline)

必读主线

[阅读原文 PDF](https://www.stroustrup.com/ETAPS12-corrected.pdf?utm_source=chatgpt.com)

只读第 6–8 节：Containers、Copy and Move、RAII。原文约 27 页，不需要通读。

[image](https://www.google.com/s2/favicons?domain=https://www.stroustrup.com\&sz=32)

Bjarne Stroustrup's Homepage

+2



这是整个清单里我最推荐你先读的资料。原因不是它涵盖的内容最多，而是它把几个看似分散的主题串在了一起。

它从一个简化的 `Vector` 入手，让你看到：

- 一个类为什么需要获取和释放资源。
- 当类拥有动态资源时，默认拷贝为什么可能出问题。
- 移动语义如何处理资源所有权的转移。
- 为什么同样的构造函数和析构函数设计，可以推广到文件、锁等其他资源。

第 8 节明确指出，资源不限于内存，还包括文件、套接字、锁、线程和数据库事务；RAII 是将这些资源管理绑定到局部对象生命周期的一种通用设计。

[image](https://www.google.com/s2/favicons?domain=https://www.stroustrup.com\&sz=32)

Bjarne Stroustrup's Homepage

+1



阅读时重点想三个问题：

1. 为什么有了 `new/delete`，仍然需要 RAII？
2. 为什么一个类开始拥有资源后，拷贝语义就不能再想当然？
3. 为什么资源管理逻辑能够通过类封装起来，而不必散落在每个调用者的代码中？

注意：这篇文章主要讲 RAII 和拷贝/移动之间的设计关系，它不是智能指针教程。其中的 `Vector` 是用于说明设计思路的简化实现，重点看推导，不要把它当成今天推荐的容器实现。

## 第二阶段：核对 RAII 定义，建立准确的概念模型

### 2. cppreference：RAII

[RAII — cppreference](https://en.cppreference.com/w/cpp/language/raii?utm_source=chatgpt.com)

这篇放在 Stroustrup 之后快速阅读就可以。它用于核对术语、核心定义以及资源类型的范围。cppreference 将 RAII 描述为把资源的生命周期与对象生命周期绑定起来的 C++ 编程技术。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com



阅读时关注这三个区别：

- Resource：需要获取并在使用后释放的东西，不只指动态内存。
- Object：负责管理资源的对象。
- Lifetime：通过对象的构造、销毁以及作用域规则，让资源的管理有明确的时间边界。

这里不需要再次研究 `new/delete` 的所有细节，你已经有对应笔记了。只需要弄清楚：RAII 是什么，以及它为什么适用于不同类型的资源。

## 第三阶段：拷贝与移动、Rule of Zero

### 3. cppreference：Rule of Three/Five/Zero

[The rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three?utm_source=chatgpt.com)

配合 [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines?utm_source=chatgpt.com) 中的 C.20、C.21 阅读。

- C.20：如果能避免自定义特殊成员函数，就尽量避免。
- C.21：如果定义或删除拷贝、移动或析构操作，要考虑相关操作的一致性。

  [image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

  cppreference.com

  +2



为什么把它们放在 RAII 之后？

因为你需要先理解类为什么拥有资源，再研究资源所有者应该怎样复制、移动和销毁。Rule of Zero 则进一步说明：如果资源已经由 `vector`、智能指针等 RAII 成员负责管理，外层类往往不需要自己实现这些特殊成员函数。

接下来再回到 Core Guidelines 的 C.31、C.33：

- C.31：类获取的资源应当由类的析构函数释放。
- C.33：类中存在拥有型指针成员时，需要考虑析构管理。

  [image](https://www.google.com/s2/favicons?domain=https://isocpp.github.io\&sz=32)

  isocpp.github.io

  +1



这两条规则最好在理解 Rule of Zero 后再读。此时你会更容易看出：它们强调的是资源所有权，而现代 C++ 的目标通常是尽可能把管理责任交给已有的 RAII 类型。

## 第四阶段：核实语言机制——析构函数与异常展开

### 4. cppreference + C++ 标准草案

先阅读：

- [cppreference：Destructors](https://en.cppreference.com/w/cpp/language/destructor?utm_source=chatgpt.com)
- [C++ 标准草案：](https://eel.is/c++draft/except.ctor?utm_source=chatgpt.com)[`[except.ctor]`](https://eel.is/c++draft/except.ctor?utm_source=chatgpt.com)

这一阶段只查清 RAII 依赖的语言规则，不必扩展成完整的异常安全专题。

重点想两个问题：

1. 为什么局部对象离开作用域时，析构函数能自动执行？
2. 异常沿调用栈传播时，哪些已经构造完成的对象会被销毁？

有一个值得知道的细节：普通构造函数在构造过程中抛出异常时，完整对象的析构函数不会运行，但已经成功构造的基类和成员子对象会被销毁。对于委托构造函数，如果被委托的构造函数已经成功完成，而委托构造函数的函数体随后抛出异常，则有不同的规则。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+2



第一次阅读只需要掌握主规则。委托构造函数的特殊情况先作为补充了解，不用深入到所有异常边界情形。

## 第五阶段：智能指针与非内存资源

### 5. 先读 Core Guidelines，再查具体类型

[ C++ Core Guidelines — 官方项目文档](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines?utm_source=chatgpt.com)

建议先看：

- R.20：使用 `unique_ptr` 或 `shared_ptr` 表达所有权。
- R.21：除非确实需要共享所有权，否则优先使用 `unique_ptr`。
- R.22、R.23：了解 `make_shared` 和 `make_unique` 的推荐用法。
- R.24：使用 `weak_ptr` 打破 `shared_ptr` 循环引用。

  [image](https://www.google.com/s2/favicons?domain=https://isocpp.github.io\&sz=32)

  isocpp.github.io

  +2



之后按以下顺序查看 cppreference：

| 阅读顺序 | 资料                                                                                         | 重点                  |
| ---- | ------------------------------------------------------------------------------------------ | ------------------- |
| 1    | [`unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr?utm_source=chatgpt.com) | 独占所有权、移动、不允许复制      |
| 2    | [`shared_ptr`](https://en.cppreference.com/w/cpp/memory/shared_ptr?utm_source=chatgpt.com) | 共享所有权、最后一个所有者销毁后的行为 |
| 3    | [`weak_ptr`](https://en.cppreference.com/w/cpp/memory/weak_ptr?utm_source=chatgpt.com)     | 不参与共享所有权，观察对象是否仍然有效 |

这些类型都是资源管理方案，但采用不同的所有权语义。`unique_ptr` 也允许自定义删除器，因此能够管理的不一定只是通过普通 `new` 创建的对象。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+2



### 6. 补充主题：自定义删除器与非内存资源

建议在读完智能指针之后，再用一个小例子理解 RAII 并不限于内存。

例如，文件句柄可以交给一个带自定义删除器的智能指针管理：

```
auto file = std::unique_ptr<FILE, FileCloser>(
    std::fopen("data.txt", "r"),
    FileCloser{}
);
```

这只是示意代码，实际使用时需要检查 `fopen` 是否成功，并确保 `FileCloser` 正确关闭非空句柄。

对于互斥锁，可以看 `std::lock_guard` 和 `std::unique_lock`；对于文件、套接字等资源，可以考虑 RAII 包装类型。

这一部分只需要弄清楚：RAII 是一般性的资源管理技术，智能指针只是其中一种实现方式。 如果你已经理解了 Stroustrup 第 8 节的文件句柄和锁示例，就不必再寻找更多类似教程。

## 第六阶段：回头总结设计原则

### 7. 最后再读 Core Guidelines 的 R.1、E.6、R.11、R.12

这是整条路线的收尾，而不是起点。

- R.1：使用资源句柄与 RAII 自动管理资源。
- E.6：使用 RAII 防止资源泄漏。
- R.11：避免在普通业务代码中显式调用 `new/delete`。
- R.12：显式分配后，应立即将结果交给管理对象。

  [image](https://www.google.com/s2/favicons?domain=https://isocpp.github.io\&sz=32)

  isocpp.github.io

  +3



在你已经理解对象生命周期、拷贝移动和智能指针后，再读这些规则，会更容易看出它们背后的统一思想，而不只是记住几条建议。

原计划里的 Microsoft Learn《How to design for exception safety》可以暂时跳过。你现阶段的目标是建立 RAII 的概念模型，不是立刻研究异常安全的所有保证等级。如果以后需要系统研究，再回来读它。

## 最后，把整个专题归结为四个思考问题

读完后，不看资料，尝试自己回答以下问题。这四个问题也可以成为你未来 `raii.md` 的组织框架。

### 1. RAII 为什么存在？

它解决了手动资源管理的什么根本困难？为什么作用域和析构函数可以帮助解决这个问题？

### 2. RAII 与异常安全有什么关系？

为什么提前返回或异常传播时，资源仍然能够得到清理？这依赖哪些语言机制？

### 3. RAII 为什么影响拷贝、移动与智能指针的设计？

如果对象拥有资源，复制和转移所有权时分别要保证什么？Rule of Zero 如何减少手动管理？

### 4. RAII 与智能指针是什么关系？

为什么智能指针只是 RAII 的一种应用？文件、锁以及其他非内存资源如何使用同样的思路管理？

最终阅读顺序就是：Stroustrup 第 6–8 节 → cppreference RAII → Rule of Three/Five/Zero 与 C.20/C.21 → 析构与异常展开 → 智能指针及自定义删除器 → Core Guidelines 总结。

这份路线已经足够。先按顺序阅读、自己推导和总结，不必再额外找一套同主题的笔记来对照；遇到确实不理解的规则，再针对那个问题查阅权威资料即可。

---

# 附：GPT 第一版清单（原文，未改动）

我建议你把这次阅读当作一个小型的专题研究：先理解 RAII 为什么存在，再弄清楚 C++ 的语言机制为什么能支撑它，最后研究它与拷贝/移动语义、智能指针之间的关系。

我筛选资料时，优先考虑了三类来源：cppreference 的技术参考、Bjarne Stroustrup 的设计思路文章，以及 C++ Core Guidelines 的设计原则。另外，再配一份 C++ 标准草案，核实具体语言行为。

有一个区别先说明：cppreference 不是 C++ 标准委员会的官方标准文本；C++ Core Guidelines 是设计指南，也不是语言标准。RAII 本身是一种编程惯用法，标准草案主要用来核实它依赖的对象生命周期、析构和异常栈展开规则。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+2



下面按推荐顺序阅读。不需要额外搜集更多资料，这些已经足够搭建一个比较完整的认知框架。

## 第一阶段：建立 RAII 的基本模型

### 1. cppreference：RAII

[Medium](https://images.openai.com/static-rsc-4/uuwpzlQT0BKZgzEvC3fTyf96nzDjfxDdjDIzsrvZowp7jK0xRTfglFFTNuQn8P3AsPE7rAOb2Gp_7MHVJhnzWw8tZnMMTqBdK3bdMPJ76ZddaIuqguw5wodqD7Aqubwxh3v157aQanFDxwTBEIY13tdlgVkr1PPdprw1w4D56uQ?purpose=inline)

首读，理解全貌。

[RAII — cppreference 英文版](https://en.cppreference.com/w/cpp/language/raii?utm_source=chatgpt.com)

[RAII — cppreference 中文版](https://zh.cppreference.com/w/cpp/language/raii?utm_source=chatgpt.com)

重点看定义、基本规则、错误示例与正确示例，以及标准库中的 RAII 类型。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+1



阅读时重点思考四个问题：

- 为什么 RAII 绑定的是资源的生命周期与对象的生命周期，而不仅仅是内存的分配和释放？
- 为什么资源通常在构造函数中获取，在析构函数中释放？
- 为什么提前 `return` 或抛出异常时，析构函数仍然能帮助清理资源？
- 为什么 `std::vector`、`std::string` 和 `std::lock_guard` 都可以体现 RAII？

这一篇先建立模型，不需要马上记住全部细节。

### 2. C++ Core Guidelines：资源管理规则 R.1 与 E.6

[PPT - Typical Error Patterns in C++ PowerPoint Presentation, free download - ID:6206880](https://images.openai.com/static-rsc-4/Qwk4PMemjJTgThXZAO5TEIWxElnGNwrY1g7LaGLcmf88yLiNsphU7FzbEtt1RNh0fwTxyrnA6wIOYBLH2GFvsZro1r79XlLE7g5HcgP2PRMrtRD6w_N-8wTmy3Hc6tunYm4LaDVqoOjCMiIdCjVQEQtFGxQ5I_D9Odfdv9G2vI0?purpose=inline)

理解为什么现代 C++ 推荐这种设计。

[C++ Core Guidelines — Resource management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines?utm_source=chatgpt.com)

直接搜索以下规则即可，不要通读整份指南：

- R.1：使用资源句柄和 RAII 自动管理资源。
- E.6：使用 RAII 防止资源泄漏。
- R.11–R.12：为什么应该避免在业务代码里直接管理 `new/delete`，以及为什么分配后要尽快交给资源管理对象。

  [image](https://www.google.com/s2/favicons?domain=https://isocpp.github.io\&sz=32)

  isocpp.github.io

  +2



这份指南最值得看的不是规则标题，而是每条规则下面的 `Reason`、`Bad example` 和 `Good example`。

思考：手动调用 `open/close`、`lock/unlock`、`new/delete` 有什么共同问题？RAII 是如何将容易遗漏的清理操作，变成对象生命周期的一部分的？

### 3. Bjarne Stroustrup：Foundations of C++

这是我最推荐你认真读的一篇逻辑性文章，因为它把你接下来要研究的三个主题放在了同一个上下文里。

[Foundations of C++ — PDF 原文](https://www.stroustrup.com/ETAPS12-corrected.pdf?utm_source=chatgpt.com)

只需要阅读第 6–8 节：

- Section 6 — Containers
- Section 7 — Copy and Move
- Section 8 — RAII

这几节通过一个简化的 `Vector`，逐步连接资源管理、对象复制、移动和 RAII。作者还用文件句柄和锁的例子解释，为什么手动配对资源操作很容易出错，以及为什么移动语义与资源所有权转移有关。

[image](https://www.google.com/s2/favicons?domain=https://www.stroustrup.com\&sz=32)

Bjarne Stroustrup's Homepage

+1



建议先读完第 6–8 节，不必把整篇论文读完。 重点理解设计思路即可，不要把其中所有旧式示例代码直接当成今天的最佳实践。

## 第二阶段：弄清楚背后的语言机制与异常安全

### 4. C++ 异常处理与析构：为什么 RAII 可以自动清理？

这一阶段的重点不是再背一遍 RAII 的定义，而是确认 C++ 语言到底保证了什么。

阅读顺序：

1. [cppreference — Destructors](https://en.cppreference.com/w/cpp/language/destructor?utm_source=chatgpt.com)
2. [C++ 标准草案 — Stack unwinding（](https://eel.is/c++draft/except.ctor?utm_source=chatgpt.com)[`[except.ctor]`](https://eel.is/c++draft/except.ctor?utm_source=chatgpt.com)[）](https://eel.is/c++draft/except.ctor?utm_source=chatgpt.com)
3. [Microsoft Learn — How to design for exception safety](https://learn.microsoft.com/en-us/cpp/cpp/how-to-design-for-exception-safety?view=msvc-170\&utm_source=chatgpt.com)

第一篇帮你理解析构函数何时调用；第二篇核实异常传播过程中，已经构造完成的局部对象和子对象如何销毁；第三篇用较完整的示例连接资源管理与异常安全。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+2



重点理解一个细节：如果一个构造函数抛出异常，完整对象的析构函数不会因此被调用，但已经成功构造的成员和基类子对象会被销毁。 这也是为什么需要把资源交给已经正确构造的 RAII 成员管理，而不是把所有清理责任都留在外层对象的析构函数里。

暂时不用深入异常安全的所有保证等级。先掌握 RAII 如何为异常安全提供基础即可。

## 第三阶段：理解 RAII 与拷贝、移动语义的联系

### 5. cppreference：Rule of Three/Five/Zero

[The rule of three/five/zero](https://en.cppreference.com/w/cpp/language/rule_of_three?utm_source=chatgpt.com)

配合 C++ Core Guidelines 中的以下规则阅读：

- C.20：能不自定义特殊成员函数就不自定义，即 Rule of Zero。
- C.21：自定义或删除某个拷贝、移动、析构操作时，要考虑其余相关操作。
- C.31 / C.33：类需要负责释放自己获取的资源；拥有型指针成员需要明确的管理策略。

  [image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

  cppreference.com

  +1



这一组资料应该能把你之前学习过的特殊成员函数、拷贝构造、移动构造串起来。

建议沿着这个问题推导：

一个类拥有动态资源，如果直接采用编译器生成的逐成员拷贝，会发生什么？如果使用智能指针作为成员，情况又有什么不同？

由此继续思考：

- 为什么资源所有者必须定义清楚复制行为？
- 为什么移动语义可以转移资源管理责任？
- 为什么 Rule of Zero 是现代 C++ 很重要的设计方向？

读完后，你应该能把 RAII 和拷贝/移动联系起来，而不是将它们当作互不相关的章节。

## 第四阶段：理解智能指针是 RAII 的具体应用

### 6. 智能指针：从独占所有权到共享所有权

先读一篇结构化教程：

- [Microsoft Learn — Object lifetime and resource management (RAII)](https://learn.microsoft.com/en-us/cpp/cpp/object-lifetime-and-resource-management-modern-cpp?view=msvc-170\&utm_source=chatgpt.com)
- [Microsoft Learn — Smart pointers (Modern C++)](https://learn.microsoft.com/en-us/cpp/cpp/smart-pointers-modern-cpp?view=msvc-170\&utm_source=chatgpt.com)

前一篇展示了从手动 `new/delete`、自己定义析构函数，到使用 `std::unique_ptr` 的演进；后一篇介绍现代 C++ 的智能指针。

[image](https://www.google.com/s2/favicons?domain=https://learn.microsoft.com\&sz=32)

Microsoft Learn

+1



接着按顺序参考 cppreference：

`std::unique_ptr`

[参考文档](https://en.cppreference.com/w/cpp/memory/unique_ptr?utm_source=chatgpt.com)

先理解独占所有权、析构时清理资源，以及为什么它可以移动却不能复制。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com

+1



`std::shared_ptr`

[参考文档](https://en.cppreference.com/w/cpp/memory/shared_ptr?utm_source=chatgpt.com)

理解共享所有权、引用计数，以及为什么它的设计比独占所有权更复杂。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com



`std::weak_ptr`

[参考文档](https://en.cppreference.com/w/cpp/memory/weak_ptr?utm_source=chatgpt.com)

理解为什么观察对象不一定意味着拥有对象，以及它如何帮助打破 `shared_ptr` 形成的循环引用。

[image](https://www.google.com/s2/favicons?domain=https://en.cppreference.com\&sz=32)

cppreference.com



这一组文档不必逐项研究成员函数。重点是通过所有权语义理解三种指针的设计差异，再回到 C++ Core Guidelines 的 R.20–R.24，了解它们在实际代码中的选择原则。

[image](https://www.google.com/s2/favicons?domain=https://isocpp.github.io\&sz=32)

isocpp.github.io

+1



## 最后：建议你带着什么问题去阅读？

你不需要把以上所有资料逐条摘抄。每读完一组资料，尝试用自己的话回答相应的问题即可。

RAII 与资源管理

它解决的根本问题是什么？为什么对象生命周期适合承担资源管理责任？

异常安全

为什么 RAII 能处理提前返回和异常退出？它依赖哪些 C++ 语言规则？

拷贝与移动语义

当对象拥有资源时，为什么需要明确拷贝和移动的语义？Rule of Zero 如何减少手动管理？

智能指针

`unique_ptr`、`shared_ptr` 和 `weak_ptr` 分别表达什么所有权关系？RAII 又在其中发挥什么作用？

我建议你优先读第 1、2、3 项，再进入异常安全和拷贝/移动语义。 其中 Stroustrup 的 Foundations of C++ 是这组资料里最值得认真思考的一篇，因为它不仅讲“是什么”，还展示了资源管理、所有权转移与语言机制如何组合成现代 C++ 的设计方式。

读完这些资料后，你就有足够的基础自己整理一篇 RAII 笔记；不需要等智能指针、异常安全等所有细节都研究完再动笔。
