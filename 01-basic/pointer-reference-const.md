# Pointers, References and const

<!-- DRAFT v3 2026-11-25 for T01. v1 (11-24) is in _sources/drafts/T01-draft-raw-v1.md and v2 in T01-draft-raw-v2.md; the raw copy of this version is _sources/drafts/T01-draft-raw.md. Source tags are HTML comments; remove them when sealing. -->

> A pointer is an object whose value is one of four kinds: a pointer to an object or function, a pointer past the end of an object, the null pointer value, or an invalid pointer value. A reference is another name for an object or function, and is not an object itself. `const` limits what can be done through such an access path; it makes an object unmodifiable only if the object itself is declared `const`.
>
> Reading order: (1) a pointer or a reference gives indirect access to an object (What, i to iv); (2) `const` limits what can be done through that path (What v, How i); (3) what a reference can bind to depends on its initializer's type and value category (How ii); (4) value categories in full, conversions, moving and forwarding continue in other notes (Relation).

## Why

<!-- 【你的材料】P02-04 KEY POINT; C-08. -->

Two needs lead to the features in this note:

- **Access an object without copying it.** A pointer or a reference gives indirect access to an existing object. C++ has both because pointers came from C, while references were added mainly so that overloaded operators read naturally (Note).
- **Limit what can be done through that access.** This is what `const` is for: it restricts the access path (a pointer or a reference), not the object.

## What

### i. Expression and value category

<!-- 【你的材料】C-18 (1.2 value category; R-value); P16-02 3.c. 【外部补充】[expr]/1, [expr]/5, [expr.unary.op]/1 and /3, N4659 [basic.lval]/1, N3337 [basic.lval]/1, [expr.prim.general]/8, [dcl.type.simple]/4; cppreference Value categories -->

An *expression* is a sequence of operators and operands that specifies a computation ([expr]/1). `x`, `42`, `a + b`, `*p` and `f()` are expressions.

A pointer accesses the object it points to through the indirection expression `*p`, which designates that object ([expr.unary.op]/1). A reference is used through its name: the expression `r` designates the object to which `r` is bound ([expr]/5). The rules for `&`, `*` and reference binding are stated for expressions, and two properties of an expression matter for them: its type and its value category. cppreference calls them "two independent properties".

The type of an expression is never a reference type: if an expression initially has the type "reference to `T`", its type is adjusted to `T` before any further analysis ([expr]/5).

Every expression belongs to exactly one of three value categories: lvalue, xvalue or prvalue ([basic.lval]/1). The definitions below are the C++17 wording; N3337 words them differently (see Note). The standard defines **glvalue** first and derives the others from it:

- **glvalue** ("generalized lvalue"): an expression whose evaluation determines the identity of an object, bit-field or function.
  - **lvalue**: a glvalue that is not an xvalue. Examples: a variable name, `*p`, a call of a function that returns an lvalue reference.
  - **xvalue** ("expiring value"): a glvalue that denotes an object or bit-field whose resources can be reused (usually because it is near the end of its lifetime). The object does not have to be near the end of its lifetime: `static_cast<int&&>(x)` is an xvalue, and `x` is still alive afterwards. Another example is a call of a function that returns an rvalue reference, such as `std::move(x)` (T04).
- **prvalue** ("pure rvalue"): an expression whose evaluation initializes an object or computes the value of an operand of an operator, as specified by the context in which it appears. Examples: `42`, `a + b`, `&x`, a call of a function that returns by value.

An **rvalue** is a prvalue or an xvalue. So an xvalue is both a glvalue and an rvalue: glvalue = lvalue or xvalue, and rvalue = prvalue or xvalue. These terms classify expressions, not values. The built-in `&`, for example, requires an lvalue operand ([expr.unary.op]/3), so `&x` is valid for a variable `x`, while `&42` and `&static_cast<int&&>(x)` are ill-formed; the result of `&x` is itself a prvalue.

Type and value category are independent. For an `int x`, the expressions `x`, `static_cast<int&&>(x)` and `42` all have type `int`, and are an lvalue, an xvalue and a prvalue respectively. The declared type of a variable does not decide the category of the expression that names it either:

```cpp
int fun() { return 42; }
int&& x = fun();                                                      // the variable x is declared with type int&&
static_assert(std::is_same<decltype(x), int&&>::value, "x has type int&&");   // decltype(x) reports that declared type
int& y = x;                                                           // OK: the expression x is an lvalue
// int&& z = x;                                                       // error: an rvalue reference cannot bind to an lvalue
```

The declaration `int&&` decided what `x` could bind to when it was initialized. Once the reference has a name, the expression `x` is an lvalue ([expr.prim.general]/8), which is why a named rvalue reference is an lvalue. T02 covers value categories in more detail.

### ii. Pointer

<!-- 【你的材料】P02-03 2.b (Pointers, Values, void); C-08. 【外部补充】N4659 [basic.compound]/3 and [basic.stc]/4; N3337 [basic.compound]/3, [expr.add]/5, [expr.unary.op]/1 and /3, [conv.ptr]/2 -->

A pointer is the first way to refer to an object indirectly. It is itself an object, and every value of a pointer type is one of four kinds ([basic.compound]/3 in C++17; see Note):

- a **pointer to an object or function** (the pointer is said to point to it);
- a **pointer past the end of an object**;
- the **null pointer value** for that type;
- an **invalid pointer value**.

A pointer to an object, or past the end of one, represents an address: that of the first byte the object occupies, or that of the first byte after its storage.

The pointer past the end needs care. For `int a[3]`, `a + 3` is a pointer past the end of `a`, and `for (int* p = a; p != a + 3; ++p)` stops there. It can be compared and used as an end marker, but it does not point to an element, so it must not be dereferenced, and it is not considered to point to an unrelated object that happens to be located at that address. This holds for every array, a character array included: `char s[] = "abc"` has four elements counting the terminating `'\0'`, so its end pointer is `s + 4`. See [array.md](array.md).

An invalid pointer value is what a dangling pointer holds. When the end of the duration of a region of storage is reached, the values of all pointers representing the address of any part of that region become invalid ([basic.stc]/4 in C++17); after `delete p;`, `p` is such a pointer. Indirection through an invalid pointer value is undefined behavior.

Two built-in operators connect pointers to value categories: the built-in `&e` needs `e` to be an lvalue and yields a pointer, which is a prvalue; the built-in `*p` is an lvalue that designates the object `p` points to ([expr.unary.op]/1, /3). A class can overload both operators; they are then ordinary function calls. So `&x` compiles for a variable `x`, while `&42` does not.

The value representation of a pointer is implementation-defined ([basic.compound]/3), so a pointer must not be treated as "just an integer". What its type decides is what `*p` denotes and how far `p + 1` moves.

`void*` is an object pointer type. The standard guarantees that it can hold the value of any object pointer ([conv.ptr]/2); a standard conversion cannot put a function pointer into it. It is used as the return type or parameter type of generic interfaces (for example, `malloc` returns `void*`). A `void*` cannot be dereferenced ([expr.unary.op]/1), and pointer arithmetic needs a pointer to a completely-defined object type, which `void` is not ([expr.add]/1). To get an object pointer back from a `void*`, use `static_cast` ([expr.static.cast]/13); the other casts between pointer types belong to N1 (planned).

### iii. Reference

<!-- 【你的材料】object-and-reference.md (What 2-5); C-18 (1.2, 4.1); P02-03 2.a; P16-02 3.b (one sentence). 【外部补充】[dcl.ref]/1, /2, /4, /5 and /6, [basic]/3, [dcl.spec.auto]/6 -->

Using a pointer takes `&` to get an address and `*` to reach the object. A reference needs neither: it is initialized to refer to an object or function ([dcl.ref]/5), and can be thought of as another name for it ([dcl.ref]/1, note). A reference is an entity, and a reference type is a compound type, but a reference is not an object. An *object* is a region of storage ([intro.object]/1); it has a type and a storage duration, it may or may not have a name, and a function is not an object. Later standards word this differently; see Note. The standard lists *object* and *reference* separately among the entities ([basic]/3). It is unspecified whether a reference requires storage ([dcl.ref]/4).

There are two kinds, and they are distinct types ([dcl.ref]/2): an **lvalue reference** `T&` and an **rvalue reference** `T&&`. They differ in what they can bind to (How, ii.1). An rvalue reference lets a function tell an rvalue argument from an lvalue argument, and implement operations such as moving when appropriate (T04, planned).

A reference to a reference cannot be written directly ([dcl.ref]/5). When one is formed indirectly, through a typedef-name, a template type parameter or `decltype` (and `auto`, which is deduced as if by a template type parameter, [dcl.spec.auto]/6), reference collapsing applies ([dcl.ref]/6). The rules, and their use in forwarding, are in T12 (planned).

### iv. Pointer vs reference

<!-- 【为连贯补充】the comparison table. 【你的材料】object-and-reference.md (What 2-5); P02-03 2.a; P02-04. 【外部补充】citations -->

A pointer is an object and a reference is not. The other differences are separate rules of the language, not consequences of that one.

| | Pointer | Reference |
|---|---|---|
| Is it an object? | Yes | No: another name for an object or function, storage unspecified ([dcl.ref]/4) |
| Initializer | Optional | Required, except for `extern`, class member declarations, parameters and return types ([dcl.ref]/5) |
| Can it be null? | Yes | No null reference in a well-defined program ([dcl.ref]/5, note) |
| Can it change its target? | Yes: assignment re-points it | No: assignment changes the referred object ([dcl.init.ref]/2) |
| Can it be const itself? | Yes: `int* const p` | No: `int& const r` is ill-formed; `const int& r` is a reference *to* const `int` ([dcl.ref]/1) |
| Levels | Pointer to pointer is allowed | No reference to reference, array of references or pointer to reference ([dcl.ref]/5) |
| `&` and `sizeof` on the name | The pointer's own address and size | Built-in `&` and `sizeof` act on the referred object ([expr]/5, [expr.sizeof]/2); a class's overloaded `operator&` is a separate case |

An uninitialized pointer of automatic storage duration has an indeterminate value, and reading it is undefined ([dcl.init]/11, [conv.lval]/1). A reference member has no initializer in the class definition; the constructor initializes it in its mem-initializer list. A `const` that reaches a reference through a typedef-name or a template type parameter is ignored instead of making the program ill-formed ([dcl.ref]/1).

### v. const

<!-- 【你的材料】P02-04 (Const; KEY POINT); C-14. 【外部补充】[dcl.type.cv]/3-4, [dcl.stc]/11 -->

Both handles can be limited with `const`. Two different things can be `const`:

- **A const object**, such as `const int ci = 3;`. It must not be modified during its lifetime, except for its `mutable` members. Modifying it is undefined behavior, even if a `const_cast` has removed the `const` first ([dcl.type.cv]/4).
- **A const access path**, such as `const int*` or `const int&`. It cannot be used to modify the object, even if the object itself is not const and can be modified through another path ([dcl.type.cv]/3).

> **Key point (C++ Primer 2.4):** It is important to realize that a reference to const restricts only what we can do through that reference. Binding a reference to const to an object says nothing about whether the underlying object itself is const. Because the underlying object might be non-const, it might be changed by other means.

A const object must be initialized, because it cannot be assigned to afterwards: `const int ci;` is an error. The rules for class types (`const std::string s;` compiles, `const int x;` does not) are in [01-initialization-basics.md](../02-class-object-oop/01-object-foundation/01-initialization/01-initialization-basics.md), section 2.2.

A data member declared `mutable` is exempt from the constness of its object ([dcl.stc]/11). This does not contradict `const`: the class author decides which members are exempt. Details: N3 (planned).

## How

The model is in place: a pointer or a reference is an access path, `const` limits the path, and a reference's initializer decides what it can bind to. How shows the declarations that spell this and the rules the compiler applies.

### i. Key concepts

<!-- 【你的材料】P02-04 (Pointers and const; Reference to Const; Top-Level && Low-Level); C-14; P02-05 (reading declarations). -->

Two points that are easy to confuse, side by side.

**1. Three const declarations.** Read a declaration from the name outwards, right to left: the symbol closest to the name has the most immediate effect on its type.

- `const int* p` (also written `int const* p`) is a **pointer to const**: the `int` is read-only through `p`, but `p` itself can be re-pointed to another `int`.
- `int* const p` is a **const pointer**: `p` cannot be re-pointed, but the `int` it points to is writable through `p`.
- `const int& r` is a **reference to const**: the `int` is read-only through `r`. A reference cannot be const itself, so a "const reference" always means this.
- `const int* const p` combines the first two.

**2. Top-level and low-level const.** *Top-level const* means that the object itself is const; it can appear on any object type (arithmetic, class or pointer). *Low-level const* appears in the base type of a pointer or reference: the object pointed to or referred to is read-only through this path. In `const int* const p`, the right `const` is top-level (on `p`) and the left one is low-level (on `*p`). The standard speaks of "top-level cv-qualifiers" ([dcl.fct]/5) but has no "low-level const"; that name comes from C++ Primer.

### ii. Mechanism

#### 1. Reference binding

<!-- 【你的材料】P02-03 2.a; P02-04 (Reference to Const; Q&A); C-18 (4.2 lifetime, 4. Note); C-08. 【外部补充】[dcl.init.ref]/5, [class.temporary]/5; TC++PL 4th ed. 7.7.1; MEC++ Item 19 -->

The value category of the initializer and `const` decide what a reference can bind to ([dcl.init.ref]/5). In the table, `T` is the referenced type; the main cases are shown. An initializer of an unrelated type is converted: for `const T&` and `T&&`, a temporary of type `T` is created from it, whatever its category.

| Initializer | `T&` | `const T&` | `T&&` |
|---|---|---|---|
| Lvalue of type `T` | Direct binding | Direct binding | Error |
| Lvalue of an unrelated type (an `int` for `double`) | Error | Temporary created | Temporary created |
| Prvalue of a non-class type (`5`, `a + b`) | Error | Temporary created | Temporary created |
| Class prvalue (`Noisy()`) or xvalue, of type `T` | Error | Direct binding | Direct binding |

One more case is in the standard: if the initializer is a class object with a conversion function that returns an lvalue reference, even `T&` can bind, to the result of that function (`int g; struct B { operator int&() { return g; } }; int& r = B();`).

1. In *direct binding* the reference becomes another name for the initializer's object. Otherwise a temporary of type `T` is created and initialized from the initializer, and the reference is bound to it: `const int& r = 5;` and `const double& d = i;` (with `int i`) both create one. Direct binding is a category in the rules; it does not mean that no temporary is involved: a class prvalue is bound to the temporary object that holds its result.
2. A temporary bound to a reference lives as long as the reference ([class.temporary]/5). There are exceptions; for example, a temporary bound to a reference parameter lives only until the end of the full-expression that contains the call. See [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md).

> **Q (C++ Primer 2.4):** Why can a non-const lvalue reference not bind to a temporary object?

The standard states the rule but gives no reason. The reason is a design choice that avoids a silent mistake. Stroustrup: "References to variables and references to constants are distinguished because introducing a temporary for a variable would have been highly error-prone; an assignment to the variable would become an assignment to the — soon-to-disappear — temporary." (*The C++ Programming Language*, 4th ed., §7.7.1; Meyers gives the same reason in *More Effective C++*, Item 19.)

The temporary itself is not the problem. For a reference to const the compiler creates the same temporary, and it lives until the end of the full-expression that contains the call, which is as long as the call needs it (item 2). What differs is what the function can do with it:

```cpp
double show(const double& d) { return d; }   // can only read d
void   incr(double& d)       { d += 1; }     // meant to change the caller's variable

void f() {
    int i = 1;
    show(i);        // a temporary double is created from i and bound to d
    // incr(i);     // error: a double& cannot bind to that temporary
}
```

- `const double&`: the function can only read, and reading the copy is the same as reading `i`.
- `double&`: the function is meant to change the caller's variable. If the call were accepted, the change would land on the temporary copy, `i` would stay `1`, and nothing would say so.
- `double&&`: the function may change an rvalue, and the type says so openly. Called with `i`, it would change the temporary `double` made from `i`, and `i` would stay `1`. It can also bind an xvalue of another object, such as `static_cast<double&&>(d)` for a `double d`; then the change does reach `d`.

#### 2. Copy and qualification conversion

<!-- 【你的材料】P02-04 (Top-Level && Low-Level); P06-02 3.a; P19-08 2.b. 【外部补充】[conv.lval]/1, [conv.qual]/1 and /4 -->

Binding is one kind of initialization; copying a pointer or a reference is another.

1. Copying an object does not carry over the source's top-level const: the destination's constness comes from its own declaration. After `int* const fixed = &a;`, `int* p = fixed;` is fine.
2. When a pointer or a reference is initialized, the cv-qualification of the pointed-to or referred-to type can be increased but not removed (qualification conversion, [conv.qual]/1; `volatile` follows the same rule). `int*` converts to `const int*` and not the other way round; an `int` lvalue can bind to a `const int&` and not the other way round.
3. With more than one pointer level, `const` can be added at an inner level only if `const` is also present at every level between it and the outermost pointer ([conv.qual]/4). So `char**` does not convert to `const char**`, but it does convert to `const char* const*`. This is the example from the standard, showing what would go wrong if the first conversion were allowed:

```cpp
const char c = 'c';
char* pc;
const char** pcc = &pc;   // rejected by the compiler; suppose it were accepted
*pcc = &c;                // allowed through const char**: pc now points to the const object c
*pc = 'C';                // allowed through char*: modifies a const object
```

With `const char* const* pcc = &pc;` instead, the second assignment is an error, because `*pcc` is const.

#### 3. Null pointer conversion and nullptr

<!-- 【外部补充】C-27 has only a title. [conv.ptr]/1, [lex.nullptr]/1, [support.types]/3 -->

`0` (an integral constant expression with the value zero) and `nullptr` are null pointer constants, and a null pointer constant converts to any pointer type; the result is the null pointer value of that type ([conv.ptr]/1). `nullptr` is a keyword introduced in C++11: a prvalue of type `std::nullptr_t` ([lex.nullptr]/1). `NULL` is a macro with an implementation-defined definition; the standard says it may be defined as `0` or `0L`, but not as `(void*)0` ([support.types]/3). Prefer `nullptr`.

### iii. Syntax and lookup tables

#### 1. Reading declarations

<!-- 【你的材料】P02-03 3; P02-05 (Type Aliases); C-14. 【外部补充】[dcl.constexpr]/9, [expr.const]/3; compiler check -->

The harder cases, read as in Key concepts:

| Declaration | Reads as |
|---|---|
| `int *&r = p;` | `r` is a reference to a pointer to `int` |
| `typedef char* pstring;` then `const pstring cstr` | `char* const`: the `const` applies to the pointer, not to `char`; a typedef is not text substitution |
| `typedef char* pstring;` then `const pstring* ps` | pointer to `char* const` |
| `static int n;` then `constexpr int* p = &n;` | `int* const`: `constexpr` applies to the pointer itself, not to what it points to |

The initializer of a `constexpr` object must be a constant expression ([dcl.constexpr]/9), and an address constant expression is the address of an object with static storage duration ([expr.const]/3). So `n` must be static or global: with a local `int n`, g++ 11.4 and clang 14 both reject `constexpr int* p = &n;`. The full rules of `constexpr` are out of scope here.

#### 2. Pointer conversions

<!-- 【你的材料】P04-11 1.f-1.i. 【外部补充】[conv.ptr], [conv.qual], [conv.bool] -->

| Conversion | Standard | Details |
|---|---|---|
| Null pointer constant (`0`, `nullptr`) to any pointer type | [conv.ptr]/1 | Mechanism 3 |
| Object pointer to `void*` or `const void*` | [conv.ptr]/2 | Object types only; adding `const` is a further qualification conversion |
| Qualification: `int*` to `const int*`, not the reverse | [conv.qual]/1, /4 | Mechanism 2 |
| Pointer to `bool` | [conv.bool]/1 | Null becomes `false`, anything else `true`; this is why `if (p)` works |

Not covered here: array-to-pointer conversion ([array.md](array.md)), derived-class to base-class pointer (inheritance note, T05, planned) and the casts between pointer types (N1, planned).

### iv. Usage

<!-- 【你的材料】object-and-reference.md (How; Why: parameters and return values) -->

```cpp
int x = 10;
int& r = x;          // r is an alias for x
r = 20;              // modifies x
int* p = &r;         // &r is the address of x
```

```cpp
void bump(int& n) { ++n; }                              // receives an existing object: no copy, no pointer syntax
int& first(std::vector<int>& v) { return v.front(); }   // returns an alias to an element: first(v) = 7;
```

## Relation

### i. Issues

<!-- 【你的材料】C-13; object-and-reference.md (Why). 【外部补充】[lex.string]/8 and /12 -->

A reference or a pointer to a local object dangles once the function returns (the pointer then has an invalid pointer value, What ii):

```cpp
int& bad() { int local = 1; return local; }   // dangling: local is destroyed when bad returns
```

A reference has its own lifetime, so it can dangle too; the details are in [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md). Do not keep a reference to a temporary that was bound to a function argument either: the temporary ends with the full-expression of the call.

A string literal has type `const char[N]` ([lex.string]/8) and modifying it is undefined ([lex.string]/12), so write `const char* p = "Hello";`.

### ii. Connection

**See also:**

- [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md): the lifetime of a reference, and of the temporaries bound to it.
- [array.md](array.md): array-to-pointer conversion and pointer arithmetic.
- [01-initialization-basics.md](../02-class-object-oop/01-object-foundation/01-initialization/01-initialization-basics.md): which `const` objects may be default-initialized.
- [storage-class-specifiers-and-linkage.md](../07-language-mechanics/storage-class-specifiers-and-linkage.md): `mutable` members, and the internal linkage of a `const` variable at namespace scope.
- Value categories (planned, T02): the full definitions, the category of any expression, why a named rvalue reference is an lvalue, `decltype` and `auto`.
- Copy and move (planned, T04): move operations and `std::move`, which build on rvalue references and xvalues.
- Forwarding (planned, T12): reference collapsing in full, forwarding references and `std::forward`.
- Expressions and conversions (planned, N1): the full catalog of implicit conversions, and the casts between pointer types.
- Functions (planned, N2): reference parameters and return values; top-level `const` on parameters.
- Class basics (planned, N3): `const` member functions, `this` and `mutable`.
- Later: inheritance (T05) for derived-to-base pointer conversion, callable objects (T10) for function pointers.

## Example

Every program was compiled with `g++ -std=c++11 -Wall -Wextra -pedantic` (g++ 11.4) and `clang++ -std=c++11 -Wall -Wextra -pedantic` (clang 14). Both give the same output and no warnings.

### i. References and const

```cpp
#include <iostream>

struct Noisy {
    Noisy()  { std::cout << "  created\n"; }
    ~Noisy() { std::cout << "  destroyed\n"; }
};

int main() {
    int x = 10;
    int& r = x;                 // r is another name for x
    const int& cr = x;          // reference to const: restricts only this path
    const double& d = x;        // type mismatch: d binds to a temporary double copy
    // double& bad = x;         // error: a double& cannot bind to that temporary
    x = 20;                     // x itself is not const, so it may change
    std::cout << (&r == &x) << ' ' << (sizeof(r) == sizeof(int)) << ' ' << cr << ' ' << d << '\n';
    // cr = 30;                 // error: cr is a reference to const
    {
        const Noisy& n = Noisy();   // the temporary lives as long as n
        std::cout << "  in block\n";
        (void)n;
    }
    std::cout << "after\n";
}
```

Output:

```text
1 1 20 10
  created
  in block
  destroyed
after
```

Verifies:

- `1 1`: `&r == &x` is true and `sizeof(r) == sizeof(int)` is true, so the name `r` stands for `x` ([expr]/5, [expr.sizeof]/2).
- `20`: `cr` sees the new value, so a reference to const restricts the path and not the object ([dcl.type.cv]/3).
- `10`: `d` still shows the old value, because it bound to a temporary copy made at initialization ([dcl.init.ref]/5).
- `created`, `in block`, `destroyed`: the temporary lives as long as `n` ([class.temporary]/5).
- The two commented-out lines were each confirmed to fail to compile in both compilers: `cr = 30` (`cr` is a reference to const) and `double& bad = x` (a non-const lvalue reference does not bind to the temporary, [dcl.init.ref]/5).

### ii. Pointers, const and conversions

```cpp
#include <iostream>
#include <type_traits>

static int g = 1;
constexpr int* cp = &g;                      // constexpr applies to the pointer itself
static_assert(std::is_same<decltype(cp), int* const>::value, "cp is int* const");

int main() {
    int a = 1, b = 2;
    const int* pc = &a;                      // pointer to const: *pc is read-only
    pc = &b;                                 // but pc itself can be re-pointed
    int* const fixed = &a;                   // const pointer: fixed target, writable object
    *fixed = 5;
    int* p = fixed;                          // the source's top-level const is not copied
    const int* q = p;                        // adding low-level const is allowed
    char** pp = nullptr;                     // nullptr converts to any pointer type
    const char* const* ok = pp;              // const added at every inner level: allowed
    // int* bad1 = q;                        // error: cannot remove low-level const
    // const char** bad2 = pp;               // error: char** to const char** is rejected
    void* v = p;                             // object pointer to void*
    std::cout << *pc << ' ' << *q << ' ' << *static_cast<int*>(v) << ' '
              << (ok == nullptr) << ' ' << *cp << '\n';
}
```

Output:

```text
2 5 5 1 1
```

Verifies:

- `2`: `*pc` reads `b` after `pc` was re-pointed: only the `int` is read-only through `pc`.
- `5 5`: `*fixed = 5` changed `a`, so the object behind a const pointer is writable; `q` was copied from `p`, which was copied from `fixed`, so the top-level const was not carried over. The `void*` round trip gives back the same address ([conv.ptr]/2, [expr.static.cast]/13).
- `1` (first): `ok == nullptr` is true: `pp` is null, and `char**` converted to `const char* const*` ([conv.qual]/4).
- `1` (last): `*cp` reads `g`, and the `static_assert` shows that `decltype(cp)` is `int* const` ([dcl.constexpr]/9).
- The two commented-out error lines were each confirmed to fail to compile in both compilers.

## Note

<!-- 【你的材料】object-and-reference.md (Implementation; History: C++23); C-08; C-18 (3. Why we introduce reference; 4.2). 【外部补充】FAQ, TC++PL 4th ed. 7.7.1, MS Learn, [basic.stc]/3, CWG 2012, N4861 [intro.object], N4659 and current draft [basic.lval]/1 and [basic.compound]/3, CWG 1438, current draft [iterator.requirements.general], P2718R0, [diff.lex] -->

**Why references exist.** Stroustrup: "C++ inherited pointers from C, so I couldn't remove them without causing serious compatibility problems. References are useful for several things, but the direct reason I introduced them in C++ was to support operator overloading" (see his [FAQ](https://www.stroustrup.com/bs_faq2.html#pointers-and-references)). His example, shortened:

```cpp
void f1(const complex* x, const complex* y) { complex z = *x + *y; }   // without references: ugly
void f2(const complex& x, const complex& y) { complex z = x + y; }     // with references: better
```

He adds that to have both the functionality of pointers and that of references, one needs either two different types, as in C++, or two sets of operations on one type. References are also used for parameters and return values; [dcl.ref]/3 shows both. The rule that a non-const lvalue reference does not bind a temporary is not a leftover of that history: it is a deliberate choice (How, ii.1). Older Visual C++ releases did let it bind, and Microsoft's documentation lists the change as a breaking change ([Microsoft Learn](https://learn.microsoft.com/en-us/previous-versions/cfbk5ddc(v=vs.85))).

**Implementation (common implementation).** In the language, a reference is not a const pointer: it is unspecified whether a reference requires storage ([dcl.ref]/4). Stroustrup: "The obvious implementation of a reference is as a (constant) pointer that is dereferenced each time it is used. It doesn't do much harm to think about references that way, as long as one remembers that a reference isn't an object that can be manipulated the way a pointer is" (*The C++ Programming Language*, 4th ed., §7.7.1). So "a constant pointer that is dereferenced automatically" is a fair mental model of the implementation, but it is not a statement about the language.

**Lifetime of a reference.** The N3337 text ([basic.stc]/3) says that the lifetime of a reference is its storage duration. [CWG 2012](https://cplusplus.github.io/CWG/issues/2012.html) changed this retroactively: the lifetime of a reference begins when its initialization is complete and ends as if it were a scalar object. This note uses the corrected rule; see [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md).

**Definition of an object.** C++11 says "An object is a region of storage" ([intro.object]/1). N4861 (C++20) says that an object occupies a region of storage in its period of construction, throughout its lifetime, and in its period of destruction.

**Pointer values.** What ii uses the C++17 classification of pointer values ([basic.compound]/3 in N4659 and the [current draft](https://eel.is/c++draft/basic.compound)), which says more about the pointer past the end and the invalid pointer value than N3337 does. N3337 says only that a valid value of an object pointer type represents the address of a byte in memory or a null pointer, and that any use of an invalid pointer value is undefined ([basic.stc.dynamic.deallocation]/4); [CWG 1438](https://cplusplus.github.io/CWG/issues/1438.html) (C++14) narrowed the latter to indirection and deallocation, and any other use is implementation-defined. The rest of this note follows C++11.

**Value categories.** What i uses the C++17 definitions ([basic.lval]/1 in N4659 and the [current draft](https://eel.is/c++draft/basic.lval)): a glvalue is defined by identity, a prvalue by what its evaluation does, an xvalue as a glvalue whose resources can be reused, an lvalue as a glvalue that is not an xvalue, and an rvalue as a prvalue or an xvalue. N3337 defines them in the other direction: an lvalue designates a function or an object; an xvalue refers to an object, usually near the end of its lifetime (so that its resources may be moved, for example); an rvalue is an xvalue, a temporary object or a subobject thereof, or a value that is not associated with an object; a prvalue is an rvalue that is not an xvalue; a glvalue is an lvalue or an xvalue. The categories and their grouping are the same. Since C++17 a prvalue becomes a temporary only where one is needed ([conv.rval]); T02 covers that. The C++20 text adds that an expression of type `void` is a prvalue.

**"Value type".** The standard has no such term for the type of a variable as opposed to the value category of the expression that names it: say *type* and *value category*. In the library the words mean something else, for example the type of `*i` for an input iterator `i` ([iterator.requirements.general](https://eel.is/c++draft/iterator.requirements.general), current draft); other languages use them as the opposite of "reference type".

**String literals.** In C++11, `char* p = "Hello";` is ill-formed: string literals were made const to avoid calling an overloaded function that expects to modify its argument (Annex C, [diff.lex]).

**Range-based for.** Before C++23, in `for (auto& x : f().items())` the temporary returned by `f()` is destroyed before the body runs, because the loop is defined through `auto&& __range = f().items();` ([stmt.ranged]/1). C++23 extends the lifetime of such temporaries to the end of the loop ([P2718R0](https://wg21.link/p2718r0)).

## Reference

- cppreference: [Reference declaration](https://en.cppreference.com/w/cpp/language/reference), [Reference initialization](https://en.cppreference.com/w/cpp/language/reference_initialization), [Value categories](https://en.cppreference.com/w/cpp/language/value_category), [Pointer declaration](https://en.cppreference.com/w/cpp/language/pointer), [cv type qualifiers](https://en.cppreference.com/w/cpp/language/cv), [Implicit conversions](https://en.cppreference.com/w/cpp/language/implicit_conversion)
- Stroustrup: [Why does C++ have both pointers and references?](https://www.stroustrup.com/bs_faq2.html#pointers-and-references). *The C++ Programming Language*, 4th ed., §7.7.1: the sentence quoted in How, ii.1, and the one on how references are implemented in Note.
- Meyers, *More Effective C++*, Item 19 (temporaries and non-const reference parameters). Lippman, Lajoie, Moo, *C++ Primer*, 5th ed., §2.4 (const; the Key point quoted in What v).
- C++ standard, by claim. Section and paragraph numbers in the text are N3337's (the C++11 draft), and the links lead to it, except where the text says C++17 (the value categories in What i and the pointer values in What ii):
  - [intro.object](https://timsong-cpp.github.io/cppwp/n3337/intro.object), [basic](https://timsong-cpp.github.io/cppwp/n3337/basic), [basic.compound](https://timsong-cpp.github.io/cppwp/n3337/basic.compound): an object is a region of storage; object and reference are separate entities; the C++11 wording of valid pointer values; implementation-defined representation of pointers
  - [basic.lval](https://timsong-cpp.github.io/cppwp/n3337/basic.lval), [expr.prim.general](https://timsong-cpp.github.io/cppwp/n3337/expr.prim.general): the C++11 definitions of lvalue, xvalue and prvalue; a variable name is an lvalue
  - [dcl.ref](https://timsong-cpp.github.io/cppwp/n3337/dcl.ref), [dcl.init.ref](https://timsong-cpp.github.io/cppwp/n3337/dcl.init.ref), [dcl.spec.auto](https://timsong-cpp.github.io/cppwp/n3337/dcl.spec.auto), [dcl.type.simple](https://timsong-cpp.github.io/cppwp/n3337/dcl.type.simple): lvalue and rvalue references; cv-qualified references; no reference to reference; reference collapsing; storage unspecified; no re-binding; direct binding and temporaries; `decltype` of a variable name is its declared type
  - [expr](https://timsong-cpp.github.io/cppwp/n3337/expr), [expr.sizeof](https://timsong-cpp.github.io/cppwp/n3337/expr.sizeof), [expr.unary.op](https://timsong-cpp.github.io/cppwp/n3337/expr.unary.op), [expr.add](https://timsong-cpp.github.io/cppwp/n3337/expr.add): a reference type is adjusted to the referred type in an expression; `sizeof` on a reference; `&` and `*`; pointer arithmetic
  - [dcl.type.cv](https://timsong-cpp.github.io/cppwp/n3337/dcl.type.cv), [dcl.stc](https://timsong-cpp.github.io/cppwp/n3337/dcl.stc), [dcl.fct](https://timsong-cpp.github.io/cppwp/n3337/dcl.fct), [dcl.constexpr](https://timsong-cpp.github.io/cppwp/n3337/dcl.constexpr), [expr.const](https://timsong-cpp.github.io/cppwp/n3337/expr.const): const object and const access path; `mutable`; top-level cv-qualifiers; a `constexpr` object is const; address constant expressions
  - [dcl.init](https://timsong-cpp.github.io/cppwp/n3337/dcl.init), [conv.lval](https://timsong-cpp.github.io/cppwp/n3337/conv.lval): an uninitialized automatic object has an indeterminate value, and reading it is undefined
  - [conv.ptr](https://timsong-cpp.github.io/cppwp/n3337/conv.ptr), [conv.qual](https://timsong-cpp.github.io/cppwp/n3337/conv.qual), [conv.bool](https://timsong-cpp.github.io/cppwp/n3337/conv.bool), [expr.static.cast](https://timsong-cpp.github.io/cppwp/n3337/expr.static.cast), [lex.nullptr](https://timsong-cpp.github.io/cppwp/n3337/lex.nullptr), [support.types](https://timsong-cpp.github.io/cppwp/n3337/support.types): pointer conversions; qualification conversion; `static_cast` from `void*`; `nullptr` and `NULL`
  - [class.temporary](https://timsong-cpp.github.io/cppwp/n3337/class.temporary), [basic.stc](https://timsong-cpp.github.io/cppwp/n3337/basic.stc): a temporary bound to a reference, and its exceptions; N3337 says the lifetime of a reference is its storage duration (see CWG 2012)
  - [basic.lval](https://eel.is/c++draft/basic.lval) (current draft; N4659 has the same definitions in [basic.lval]/1): glvalue, lvalue, xvalue, prvalue and rvalue as used in What i
  - [basic.compound](https://eel.is/c++draft/basic.compound), [basic.stc](https://eel.is/c++draft/basic.stc) (current draft; N4659 has the same text in [basic.compound]/3 and [basic.stc]/4): the four kinds of pointer value; pointer values become invalid when storage duration ends; indirection through an invalid pointer value is undefined
  - [lex.string](https://timsong-cpp.github.io/cppwp/n3337/lex.string), [diff.lex](https://timsong-cpp.github.io/cppwp/n3337/diff.lex), [stmt.ranged](https://timsong-cpp.github.io/cppwp/n3337/stmt.ranged): string literals; their change in C++11; the range-based `for` expansion
- Defect reports and proposals: [CWG 2012](https://cplusplus.github.io/CWG/issues/2012.html), [CWG 1438](https://cplusplus.github.io/CWG/issues/1438.html), [P2718R0](https://wg21.link/p2718r0)
