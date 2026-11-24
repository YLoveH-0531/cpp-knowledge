# Pointers, References and const

<!-- DRAFT 2026-11-24 for T01. Raw copy: _sources/drafts/T01-draft-raw.md. Source tags are HTML comments; remove them when sealing. -->

> A pointer is an object that holds an address. A reference is not an object: it is another name for an existing object or function. `const` restricts what can be done *through an access path* (a pointer or a reference); it makes the object itself unmodifiable only if the object is declared `const`.

## Why

<!-- 【你的材料】P02-04 KEY POINT; object-and-reference.md (Why; History: early C++, C++98); C-08; C-18. 【外部补充】[dcl.type.cv]/3 -->

Two separate needs lead to pointers, references and `const`:

- **Access an object without copying it.** A pointer or a reference gives indirect access to an existing object.
- **Limit what can be done through that access.** This is what `const` is for. The key point, in the words of C++ Primer: *a reference to const restricts only what we can do through that reference.* It restricts the access path, not the object: the same non-const object can still be changed through another path ([dcl.type.cv]/3).

A word on history. References were introduced mainly to support operator overloading. In Stroustrup's words, "the direct reason I introduced them in C++ was to support operator overloading" (see his [FAQ](https://www.stroustrup.com/bs_faq2.html#pointers-and-references)), while pointers came from C and could not be removed: `z = x + y` reads better than `z = *x + *y`. Early C++ used lvalue references for operator overloading, function parameters and return values. C++98 let a `const T&` bind a temporary and extended the temporary's lifetime to the lifetime of the reference.

## What

### i. Object and reference

<!-- 【你的材料】object-and-reference.md (object; What 1-7). 【外部补充】[intro.object]/1, [basic]/3, [dcl.ref]/4-6 -->

An *object* is a region of storage ([intro.object]/1). It has a type and a storage duration, it may or may not have a name, and a function is not an object. Later standards word this differently; see the Note section.

A *reference* is an entity, and a reference type is a compound type, but a reference is not an object: the standard lists *object* and *reference* separately among the entities ([basic]/3). A reference is another name, an alias, for an existing object or function. It is unspecified whether a reference requires storage ([dcl.ref]/4).

The language also states explicit restrictions on reference types ([dcl.ref]/5): there are no references to references, no arrays of references and no pointers to references. A reference type that is formed through a typedef, a template parameter or `decltype` follows the *reference collapsing* rules instead ([dcl.ref]/6).

A reference has its own lifetime, so it can dangle: the object it refers to may end before the reference does. The details are in [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md).

### ii. Pointer

<!-- 【你的材料】P02-03 2.b (Pointers, Values, void); C-08. 【外部补充】[basic.compound]/3, [conv.ptr]/2, [expr.unary.op]/1, [expr.add]/1 -->

A pointer is itself an object. The valid value of an object pointer type is either the address of a byte in memory or the null pointer value ([basic.compound]/3); the address one past the end of an array is also a valid value (see [array.md](array.md)). A function pointer points to a function (see Relation). The value representation of a pointer is implementation-defined, so a pointer must not be treated as "just an integer". As a mental model, a pointer is an address whose size depends only on the machine's address width (common implementation, not guaranteed by the standard). What the type does decide is what `*p` denotes and how far `p + 1` moves.

`void*` is an object pointer type. The standard guarantees that it can hold the value of any object pointer ([conv.ptr]/2); a standard conversion cannot put a function pointer into it. It is used as the return type or parameter type of generic interfaces (for example, `malloc` returns `void*`). A `void*` cannot be dereferenced ([expr.unary.op]/1), and pointer arithmetic needs a pointer to a completely-defined object type, which `void` is not ([expr.add]/1). Compilers differ in how strictly they enforce this; see the Note section.

### iii. Pointer vs reference

<!-- 【为连贯补充】the comparison table. 【你的材料】object-and-reference.md (What 2-5); P02-03 2.a; P02-04. 【外部补充】citations -->

A pointer is an object and a reference is not. The other differences are separate rules of the language, not consequences of that one.

| | Pointer | Reference |
|---|---|---|
| Is it an object? | Yes: it has its own storage and address | No: an alias for an existing object or function |
| Initialization | Can be declared without an initializer | The declaration must have an initializer (except `extern`, class member declarations, parameters and return types), and it must refer to a valid object or function ([dcl.ref]/5) |
| Can it be null? | Yes, it can hold the null pointer value | No: a null reference cannot exist in a well-defined program ([dcl.ref]/5, note) |
| Can it change its target? | Yes: assignment re-points it | No: it cannot be changed to refer to another object after initialization ([dcl.init.ref]/2); assignment changes the referred object |
| Can it be const itself? | Yes, a top-level const: `int* const p` | No: a cv-qualified reference is ill-formed, and ignored if introduced through a typedef or template argument ([dcl.ref]/1) |
| Storage | The pointer object occupies storage | Unspecified ([dcl.ref]/4); commonly a pointer under the hood |
| In expressions | `p` is the pointer value, `*p` is the object it points to, `&p` is the address of the pointer itself | The name stands for the referred object ([expr]/5): `&r` is the address of that object and `sizeof(r)` is its size ([expr.sizeof]/2) |

### iv. const

<!-- 【你的材料】P02-04 (Const; Pointers and const; Top-Level && Low-Level); C-14. 【外部补充】[dcl.type.cv]/3-4, [basic.link]/3, [dcl.init]/6, [dcl.fct]/5 -->

A `const` variable must be initialized (a class type with a user-provided default constructor is the exception, [dcl.init]/6) and cannot be modified. At namespace scope it has internal linkage unless it is declared `extern` ([basic.link]/3).

Two different things can be `const`:

- **A const object**, such as `const int ci = 3;`. It must not be modified during its lifetime (members declared `mutable` excepted). Modifying it is undefined behavior, even if a `const_cast` has removed the `const` first ([dcl.type.cv]/4).
- **A const access path**, such as `const int*` or `const int&`. It cannot be used to modify the object, even if the object itself is not const and can be modified through another path ([dcl.type.cv]/3).

> **Key point (C++ Primer 2.4):** It is important to realize that a reference to const restricts only what we can do through that reference. Binding a reference to const to an object says nothing about whether the underlying object itself is const. Because the underlying object might be non-const, it might be changed by other means.

*Top-level const* means that the object itself is const; it can appear on any object type (arithmetic, class or pointer). *Low-level const* appears in the base type of a pointer or reference: the object pointed to or referred to is read-only through this path. The standard speaks of "top-level cv-qualifiers" ([dcl.fct]/5) but has no "low-level const"; that name comes from C++ Primer. A reference cannot be const itself, so a "const reference" always means a reference to const.

## How

### i. Key concepts

<!-- 【你的材料】P02-04 (Pointers and const; Reference to Const); C-14 -->

Three declarations are easy to confuse. Side by side:

- `const int* p` (also written `int const* p`) is a **pointer to const**: the `int` is read-only through `p`, but `p` itself can be re-pointed to another `int`.
- `int* const p` is a **const pointer**: `p` cannot be re-pointed, but the `int` it points to is writable through `p`.
- `const int& r` is a **reference to const**: the `int` is read-only through `r`. A reference never re-binds, so there is no reference counterpart of the const pointer.

`const int* const p` combines the first two. In it, the right `const` is top-level (on `p`) and the left one is low-level (on `*p`).

### ii. Mechanism

#### 1. Reference binding

<!-- 【你的材料】P02-03 2.a; P02-04 (Reference to Const; Q&A); C-18; object-and-reference.md (History: C++98). 【外部补充】[dcl.init.ref]/5, [class.temporary]/5 -->

1. A non-const lvalue reference can normally bind only to an lvalue of a compatible type; `int& r = 5;` is an error ([dcl.init.ref]/5). A few cases that go through a class's conversion function are not covered here.
2. A reference to const (and an rvalue reference) can also bind to an rvalue.
3. The standard distinguishes two ways of initializing a reference. In *direct binding*, the reference is bound to a compatible lvalue, an xvalue, a class prvalue, or the result of a suitable conversion function. Otherwise a temporary is created, initialized from the initializer, and the reference is bound to the temporary: `const int& r = 5;` and `const double& d = i;` (with `int i`) both create one. Direct binding is only a category in the rules. It does not mean that no temporary is involved, because a class prvalue is itself a temporary.
4. A temporary bound to a reference lives as long as the reference ([class.temporary]/5). There are exceptions; for example, a temporary bound to a reference parameter lives only until the end of the full-expression that contains the call. See [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md).

> **Q (C++ Primer 2.4):** Why can a non-const reference not bind to a temporary object?
>
> **My earlier answer (kept as written):** "Temporary objects have expression lifetime, they will die at the end of the expression they are in. When bind to non-const reference, the memory created for temporary objects will be freed as program jump out the scope of the expression, which leaving the non-const reference to a dangling pointer. It do harm to program especially when pass a temporary object as non-const reference argument. So C++ do not allow non-const reference bind to temporary object."
>
> **Correction:** "dangling" is not the reason. A reference to const may bind a temporary, and the temporary's lifetime is then extended (step 4), so the rule is not about lifetime. The standard states the condition ([dcl.init.ref]/5) but gives no reason.
>
> **Intuition (not given by the standard):** with `void f(double&); int i = 0; f(i);`, the compiler would convert `i` to a temporary `double`, and `f` would modify that temporary instead of `i`. The change would be silently lost. Rejecting the binding turns this mistake into a compile error.

#### 2. Copy and qualification conversion

<!-- 【你的材料】P02-04 (Top-Level && Low-Level); P06-02 3.a; P19-08 2.b. 【外部补充】[conv.lval]/1, [conv.qual]/1 and /4 -->

1. Copying an object does not carry over the source's top-level const: the destination's constness comes from its own declaration. This holds for a const pointer too: after `int* const fixed = &a;`, `int* p = fixed;` is fine.
2. When a pointer or a reference is initialized, the cv-qualification of the pointed-to or referred-to type can be increased but not removed (qualification conversion, [conv.qual]/1; `volatile` follows the same rule). `int*` converts to `const int*` and not the other way round; an `int` lvalue can bind to a `const int&` and not the other way round.
3. For multi-level pointers, `const` must be added at every inner level ([conv.qual]/4). `char**` does not convert to `const char**`, but it does convert to `const char* const*`:

```cpp
char** pp = nullptr;
const char* const* ok = pp;      // allowed: const is added at every inner level
// const char** bad = pp;        // error: see the explanation below
```

If `bad` were allowed, `*bad = &c;` (with `const char c`) would store the address of a const object into the `char*` that `pp` points to, and a later `**pp = 'x';` would modify a const object (the example in [conv.qual]/4).

#### 3. Null pointer conversion and nullptr

<!-- 【外部补充】C-27 has only a title. [conv.ptr]/1, [lex.nullptr]/1, [support.types]/3 -->

`0` (an integral constant expression with the value zero) and `nullptr` are null pointer constants, and a null pointer constant converts to any pointer type; the result is the null pointer value of that type ([conv.ptr]/1). `nullptr` is a keyword introduced in C++11: a prvalue of type `std::nullptr_t` ([lex.nullptr]/1). `NULL` is a macro with an implementation-defined definition; the standard says it may be defined as `0` or `0L`, but not as `(void*)0` ([support.types]/3). Prefer `nullptr`.

### iii. Syntax and lookup tables

#### 1. Reading declarations

<!-- 【你的材料】P02-03 3; P02-05 (Type Aliases); C-14. 【外部补充】[dcl.constexpr]/9, [expr.const]/3; compiler check -->

Read a declaration from the name outwards, right to left: the symbol closest to the name has the most immediate effect on its type.

| Declaration | Reads as |
|---|---|
| `int *&r = p;` | `r` is a reference to a pointer to `int` |
| `const int* p`, `int const* p` | pointer to `const int` |
| `int* const p` | `const` pointer to `int` |
| `const int* const p` | `const` pointer to `const int` |
| `const int& r` | reference to `const int` |
| `typedef char* pstring;` then `const pstring cstr` | `char* const`: the `const` applies to the pointer, not to `char`; a typedef is not text substitution |
| `typedef char* pstring;` then `const pstring* ps` | pointer to `char* const` |
| `static int n;` then `constexpr int* p = &n;` | `int* const`: `constexpr` applies to the pointer itself, not to what it points to |

The initializer of a `constexpr` object must be a constant expression ([dcl.constexpr]/9), and an address constant expression is the address of an object with static storage duration ([expr.const]/3). So `n` must be static or global: with a local `int n`, g++ 11.4 and clang 14 both reject `constexpr int* p = &n;`. The full rules of `constexpr` are out of scope here.

#### 2. Pointer conversions

<!-- 【你的材料】P04-11 1.f-1.i. 【外部补充】[conv.ptr], [conv.qual], [conv.bool], [conv.array] -->

| Conversion | Standard | Note | Covered in |
|---|---|---|---|
| Null pointer conversion: `0` or `nullptr` to any pointer type | [conv.ptr]/1 | The result is the null pointer value of that type | here, Mechanism 3 |
| Pointer to `void*` or `const void*` | [conv.ptr]/2 | Object pointer types only; adding `const` is a further qualification conversion | here |
| Qualification conversion: `int*` to `const int*`, not the reverse | [conv.qual]/1, /4 | Multi-level pointers need `const` at every inner level | here, Mechanism 2 |
| Pointer to `bool` | [conv.bool]/1 | A zero or null value becomes `false`, anything else `true`; this is why `if (p)` works | here |
| Array to pointer | [conv.array]/1 | Decay, with its exceptions | [array.md](array.md) |
| Derived-class pointer to base-class pointer | [conv.ptr]/3 | | inheritance note (planned) |
| Explicit conversions (`static_cast`, `const_cast`, `reinterpret_cast`) | | | expressions and conversions note (planned) |

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

<!-- 【你的材料】C-13; object-and-reference.md (Why). 【外部补充】[lex.string]/8 and /12, Annex C [diff.lex] -->

A reference or a pointer to a local object dangles once the function returns:

```cpp
int& bad() { int local = 1; return local; }   // dangling: local is destroyed when bad returns
```

Both g++ and clang warn about this. Do not keep a reference to a temporary that was bound to a function argument either: the temporary ends with the full-expression of the call (see [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md)).

A string literal has type `const char[N]` and static storage duration ([lex.string]/8), and modifying it is undefined ([lex.string]/12). Write `const char* p = "Hello";`. In C++11, `char* p = "Hello";` is ill-formed: string literals were made const to avoid calling an overloaded function that expects to modify its argument (Annex C, [diff.lex]). g++ 11.4 and clang 14 only warn.

### ii. Connection

**See also:**

- [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md): the lifetime of a reference, and of the temporaries bound to it.
- [array.md](array.md): arrays decay to pointers; pointer arithmetic; array parameters.
- Value categories (planned, T02): why an rvalue can bind to a `const T&`; how `auto` and `decltype` treat top-level const and references.
- Expressions and conversions (planned, N1): the full catalog of implicit conversions and the explicit casts. The pointer conversions are covered here, in How.
- Functions (planned, N2): reference parameters and return values; top-level const on parameters.
- Class basics (planned, N3): `const` member functions and `this`.
- Later: copy and move (T04) for rvalue references and move semantics, inheritance (T05) for derived-to-base pointer conversion, forwarding (T12) for reference collapsing and forwarding references, callable objects (T10) for function pointers.

## Example

Both programs were compiled with `g++ -std=c++11 -Wall -Wextra -pedantic` (g++ 11.4) and `clang++ -std=c++11 -Wall -Wextra -pedantic` (clang 14). Both give the same output and no warnings.

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
    x = 20;                     // x itself is not const, so it may change
    std::cout << (&r == &x) << ' ' << sizeof(r) << ' ' << cr << ' ' << d << '\n';
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
1 4 20 10
  created
  in block
  destroyed
after
```

Verifies:

- `1 4`: `&r == &x` is true and `sizeof(r)` is `sizeof(int)`, so the name `r` stands for `x` ([expr]/5, [expr.sizeof]/2).
- `20`: `cr` sees the new value, so a reference to const restricts the path and not the object ([dcl.type.cv]/3).
- `10`: `d` still shows the old value, because `const double& d = x` bound to a temporary copy made at initialization ([dcl.init.ref]/5).
- `created`, `in block`, `destroyed`: the temporary lives as long as `n` and is destroyed when `n` goes out of scope ([class.temporary]/5).

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

- `2`: `*pc` reads `b` after `pc` was re-pointed: only the `int` is read-only through `pc`, the pointer itself is not const.
- `5 5`: `*fixed = 5` changed `a`, so the object behind a const pointer is writable. `q` was copied from `p`, which was copied from `fixed`, so the top-level const was not carried over. The `void*` round trip gives back the same address ([conv.ptr]/2, [expr.static.cast]/13).
- `1` (first): `ok == nullptr` is true: `pp` is null, and `char**` converted to `const char* const*` ([conv.qual]/4).
- `1` (last): `*cp` reads `g`, and the `static_assert` shows at compile time that `decltype(cp)` is `int* const` ([dcl.constexpr]/9).
- The two commented-out error lines, and `cr = 30` in the first program, were each confirmed to fail to compile in both compilers.

## Note

<!-- 【你的材料】object-and-reference.md (Implementation; History: C++23); C-08; C-18; P04-11 NOTE. 【外部补充】[basic.stc]/3, CWG 2012, N4861 [intro.object]/1, P2718R0, compiler checks -->

**Implementation (common implementation).** In the language, a reference is not a const pointer: it is unspecified whether a reference requires storage ([dcl.ref]/4), and a compiler may implement a reference with a pointer or by other means. Earlier notes (Cherno) say a reference is "essentially a const pointer", "syntactic sugar" introduced to support operator overloading and readable code. That describes how compilers commonly implement it; it is not a guarantee.

**Lifetime of a reference.** The N3337 text ([basic.stc]/3) says that the lifetime of a reference is its storage duration. [CWG 2012](https://cplusplus.github.io/CWG/issues/2012.html) changed this retroactively: the lifetime of a reference begins when its initialization is complete and ends as if it were a scalar object. This note uses the corrected rule; see [lifetime.md](../02-class-object-oop/01-object-foundation/lifetime.md).

**Definition of an object.** C++11 says "An object is a region of storage" ([intro.object]/1). N4861 (C++20) words it differently: an object occupies a region of storage in its period of construction, throughout its lifetime, and in its period of destruction.

**C++23 and range-based for.** In C++23 the temporaries in the range-initializer of a range-based `for` live until the end of the loop ([P2718R0](https://wg21.link/p2718r0)). Before that, `for (auto& x : f().items())` could dangle: C++11 expands the loop to `auto && __range = (range-init);` ([stmt.ranged]/1), so if `items()` returns a reference into the temporary that `f()` returns, that temporary is destroyed before the loop body runs.

**Compiler behavior with `void*`.** Dereferencing a `void*` and doing arithmetic on it are both ill-formed in standard C++, but the compilers differ. g++ 11.4 rejects `*v` and only warns about `v + 1` (`-Wpointer-arith`); clang 14 only warns about `*v` (`-Wvoid-ptr-dereference`) and rejects `v + 1`.

**Corrections to earlier notes.**

> **My earlier note (C++ Primer 4.11):** "NOTE: we can use static_cast to convert void \* to (not suit for const void\*) any type only, other pointer type conversions other than situations above are not allowed. If we want to convert them, we only could use old-style conversion type."
>
> The first half is nearly right. `static_cast` can convert "pointer to cv1 void" to "pointer to cv2 T" when `T` is an object type and `cv2` is at least as qualified as `cv1` ([expr.static.cast]/13). So a `const void*` cannot be `static_cast` to a non-const `T*`, but it can be `static_cast` to `const T*`. "Any type" should read "any object type".
>
> The second half is wrong. Other pointer conversions have named casts: `reinterpret_cast` between object pointer types (it cannot cast away const, [expr.reinterpret.cast]/7), `const_cast` to change cv-qualification ([expr.const.cast]/3), and `static_cast` between base-class and derived-class pointers ([expr.static.cast]/7). An old-style cast is only another notation for these casts ([expr.cast]/4). The full treatment belongs to the note on expressions and conversions.

The same note says that a pointer to any type converts to `const void*`. The standard limits this to object types ([conv.ptr]/2); a function pointer does not.

## Reference

- cppreference: [Reference declaration](https://en.cppreference.com/w/cpp/language/reference), [Reference initialization](https://en.cppreference.com/w/cpp/language/reference_initialization), [Pointer declaration](https://en.cppreference.com/w/cpp/language/pointer), [cv type qualifiers](https://en.cppreference.com/w/cpp/language/cv), [Implicit conversions](https://en.cppreference.com/w/cpp/language/implicit_conversion), [nullptr](https://en.cppreference.com/w/cpp/language/nullptr), [NULL](https://en.cppreference.com/w/cpp/types/NULL), [constexpr specifier](https://en.cppreference.com/w/cpp/language/constexpr)
- Stroustrup: [Why does C++ have both pointers and references?](https://www.stroustrup.com/bs_faq2.html#pointers-and-references)
- C++ standard (draft), by claim. Paragraph numbers in the text are N3337's (C++11); the links lead to the current draft, whose numbering and some section names differ:
  - [intro.object](https://eel.is/c++draft/intro.object): an object is a region of storage; a function is not an object
  - [basic.pre](https://eel.is/c++draft/basic.pre) ([basic]/3 in C++11): object and reference are listed separately among the entities
  - [basic.compound](https://eel.is/c++draft/basic.compound): compound types; valid pointer values; object pointer type; implementation-defined value representation
  - [dcl.ref](https://eel.is/c++draft/dcl.ref): cv-qualified references; storage unspecified; restrictions on reference types; reference collapsing
  - [dcl.init.ref](https://eel.is/c++draft/dcl.init.ref): a reference cannot be re-bound; binding rules and temporaries
  - [expr.type](https://eel.is/c++draft/expr.type) ([expr]/5 in C++11): a reference type is adjusted to the referred type in expressions
  - [expr.sizeof](https://eel.is/c++draft/expr.sizeof): `sizeof` on a reference is the size of the referred type
  - [expr.unary.op](https://eel.is/c++draft/expr.unary.op): indirection needs a pointer to an object or function type
  - [expr.add](https://eel.is/c++draft/expr.add): pointer arithmetic needs a completely-defined object type
  - [dcl.type.cv](https://eel.is/c++draft/dcl.type.cv): const access path and const object
  - [dcl.fct](https://eel.is/c++draft/dcl.fct): top-level cv-qualifiers of parameter types are dropped
  - [dcl.constexpr](https://eel.is/c++draft/dcl.constexpr), [expr.const](https://eel.is/c++draft/expr.const): a `constexpr` object is const; address constant expressions
  - [dcl.init.general](https://eel.is/c++draft/dcl.init.general) ([dcl.init]/6 in C++11): default-initializing a const object
  - [basic.link](https://eel.is/c++draft/basic.link): a const variable at namespace scope has internal linkage
  - [conv.ptr](https://eel.is/c++draft/conv.ptr), [conv.qual](https://eel.is/c++draft/conv.qual), [conv.bool](https://eel.is/c++draft/conv.bool), [conv.array](https://eel.is/c++draft/conv.array), [conv.lval](https://eel.is/c++draft/conv.lval): pointer, qualification, boolean and array conversions; non-class prvalues are cv-unqualified
  - [expr.static.cast](https://eel.is/c++draft/expr.static.cast), [expr.const.cast](https://eel.is/c++draft/expr.const.cast), [expr.reinterpret.cast](https://eel.is/c++draft/expr.reinterpret.cast), [expr.cast](https://eel.is/c++draft/expr.cast): casts between pointer types
  - [class.temporary](https://eel.is/c++draft/class.temporary): a temporary bound to a reference, and its exceptions
  - [basic.stc](https://eel.is/c++draft/basic.stc): N3337 says the lifetime of a reference is its storage duration (see CWG 2012)
  - [lex.string](https://eel.is/c++draft/lex.string), [lex.nullptr](https://eel.is/c++draft/lex.nullptr): string literals; the `nullptr` literal
  - [stmt.ranged](https://eel.is/c++draft/stmt.ranged): the range-based `for` expansion
  - `NULL` is described in [support.types] in C++11 and is an implementation-defined null pointer constant
- Defect report and proposal: [CWG 2012](https://cplusplus.github.io/CWG/issues/2012.html), [P2718R0](https://wg21.link/p2718r0)
