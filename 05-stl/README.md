# STL

STL 六大组件,按侯捷经典划分:

1. [Container 容器](01-container) —— 各种数据结构(vector/list/map 等),用来存放数据
2. [Algorithm 算法](02-algorithm) —— sort/find/copy 等常用算法
3. [Iterator 迭代器](03-iterator) —— 容器与算法之间的胶合剂
4. [Function Object 仿函数](04-function-object) —— 重载了 operator() 的对象,给算法当策略/谓词
5. [Adapter 配接器](05-adapter) —— 修饰容器/迭代器/仿函数接口,细分 container adapter / iterator adapter / function adapter
6. [Allocator 空间配置器](06-allocator) —— 负责空间的配置与管理

## 组件间关系

container 通过 allocator 取得数据储存空间,algorithm 通过 iterator 存取 container 内容,function object 协助 algorithm 完成不同的策略变化,adapter 可以修饰或套接 function object。

Sequence Container = 把元素组织成一个 sequence 的容器。

这里的“sequence”不是专门说“排序后的序列”，而是说：

有确定线性先后关系的元素集合

Associative Container = 用 key 把元素组织、关联起来的容器。
