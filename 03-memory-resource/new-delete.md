# New and Delete

## Why

Dynamic allocation is needed when the size or number of objects is only known at runtime, or when an object's lifetime must extend beyond the scope in which it is created.

## What

```text
                     storage                        object

 new-expression      1. allocation function   --->  2. initialization
                        operator new                   constructor call, ...

 delete-expression   4. deallocation function <---  3. destruction (if any)
                        operator delete                destructor call
```

## How

### i. Key concepts

- `new`/`delete`: keywords in C++.
- `new`/`delete` expression: `new` keyword in an expression. It is a high-level language mechanism defined by C++ that encompasses semantics such as storage acquisition, object initialization, and the return of results.
- `operator new`, `operator new[]`, `operator delete`, `operator delete[]`: allocation/deallocation functions. The global versions are free functions; a class may also define static member versions. They can be invoked implicitly by new/delete expressions or called directly by the user, but a direct call only allocates or frees raw storage: no initialization or destruction happens.
- Common mistake: mixing up `new`/`delete` expressions and `operator new`/`operator delete`. New/delete expressions are language constructs, while operator new/delete are functions used by those constructs. A new-expression may call an allocation function and, if initialization fails, the matching deallocation function.

### ii. Expression semantics

#### 1. new expression

**Ordinary new expression:**

1. Calls the selected allocation function (`operator new` or `operator new[]`) to allocate raw, unconstructed memory.
2. Initializes the object(s) from the specified initializers (for class types, this may involve calling a constructor).
3. Evaluates to the appropriate type and returns the pointer.

**New expression with placement arguments** (`new (args) T`):

The steps are the same as above; only step 1 differs. `args` are passed to the allocation function as additional arguments, so overload resolution selects a different allocation function, and that function decides where the storage comes from.

| Form | Selected allocation function | Effect |
|---|---|---|
| `new (std::nothrow) T` | `operator new(size_t, const std::nothrow_t&)` | Allocates; reports failure by returning null instead of throwing `std::bad_alloc`. This applies to allocation failure only; initialization may still throw an exception. |
| `new (buffer) T` | `operator new(size_t, void*)`, the standard non-allocating placement form | Allocates nothing; returns `buffer` unchanged, so the object is initialized in existing storage. |
| `new (pool) T` | user-defined, e.g. `operator new(size_t, Pool&)` | Defined by the user, e.g. takes storage from a pool. |

> **Terminology:** strictly, the standard calls any new-expression with extra arguments in parentheses a *placement new-expression*, including `new (std::nothrow) T`. In everyday use, "placement new" usually means only the `new (buffer) T` form.

#### 2. delete expression

1. The destructor (if any) is run on the object to which `ptr` points, or on the elements in the array to which `arr` points.
2. Calls the selected deallocation function (`operator delete` or `operator delete[]`) to release the memory.

> **Note:** For an object created using placement new, do not use an ordinary delete-expression to release the supplied storage. Destroy the object separately and manage the storage according to its original ownership.

### iii. Operator functions

#### 1. Allocating version

**Declarations:**

```cpp
void* operator new  ( std::size_t count );
void* operator new[]( std::size_t count );
void  operator delete  ( void* ptr ) noexcept;
void  operator delete[]( void* ptr ) noexcept;

void* operator new  ( std::size_t count, const std::nothrow_t& tag ) noexcept;
void* operator new[]( std::size_t count, const std::nothrow_t& tag ) noexcept;
void  operator delete  ( void* ptr, const std::nothrow_t& tag ) noexcept;
void  operator delete[]( void* ptr, const std::nothrow_t& tag ) noexcept;
```

#### 2. Non-allocating placement version

**Declarations:**

```cpp
void* operator new  ( std::size_t count, void* ptr ) noexcept;
void* operator new[]( std::size_t count, void* ptr ) noexcept;
void  operator delete  ( void* ptr, void* place ) noexcept;
void  operator delete[]( void* ptr, void* place ) noexcept;
```

> **Note:** The non-allocating placement `operator delete` is called when the initialization throws in a `new (buffer) T` expression that selected the standard non-allocating `operator new`; it does nothing (no-op), because that function allocated nothing.

#### 3. Properties

1. **Global:** The library's versions of `operator new`/`delete` (including the placement forms) are declared in the global namespace (`::`), not within the `std` namespace.
2. **Implicit:**
   - The plain forms of `operator new`, `operator new[]`, `operator delete` and `operator delete[]` (including their sized-deallocation (C++14) and alignment-aware (C++17) variants) are implicitly declared in every translation unit. The `nothrow` and placement forms are not; they require `#include <new>`. So `new T` and `delete p` need no header, whereas `new (std::nothrow) T` and `new (p) T` do.
3. **Replaceable:** The global allocation/deallocation functions are replaceable.
   - **Replace:** define a function with the same signature at global scope; the replacement is then used by every call that selects that global function, including calls from inside the standard library (e.g. `std::allocator`). Only one such definition is allowed per program. Not allowed for the standard non-allocating placement forms (see Special case below).
   - **Overload:** add a version with different parameters, e.g. `operator new(size_t, Pool&)`, and call it as `new (pool) T`. The original version is unaffected.
   - **Class-specific:** declare `operator new`/`operator delete` as static members of a class, with whatever extra parameters you need. They apply only to that class and its derived classes. A class-specific `operator new` hides all the global ones, including the placement forms; `::new ((void*)p) T(...)` bypasses it (libstdc++'s `std::allocator::construct` is written this way).
   - **Special case:** the standard non-allocating placement forms (`operator new`/`new[]` with `(size_t, void*)` and `operator delete`/`delete[]` with `(void*, void*)`) are reserved: a program may not replace them by defining global functions with the same signatures. The ban covers replacement only: overloads and class-specific versions are not affected, even a class-specific one with the same signature (e.g. `void* T::operator new(size_t, void*)`).

### iv. Usage

#### 1. Ordinary new

**Initialization forms:**

```cpp
new type                              // default-initialized
new type ()                           // value-initialized
new type (initializers)               // direct-initialized
new type {}                           // list-initialized
new type [size]                       // each element default-initialized
new type [size] ()                    // each element value-initialized
new type [size] { initializer list }  // aggregate-initialized
```

**Array type alias:**

```cpp
typedef int arrT[42];            // arrT names the type "array of 42 ints"
int *p = new arrT;               // allocates an array of 42 ints; p points to the first one
delete [] p;                     // brackets are necessary because we allocated an array
```

**Zero-length array:**

```cpp
char *p = new char[0];           // legal, returns a valid non-null pointer, but the array has no elements: never dereference it, but still free it with delete[]
```

#### 2. Placement new

```cpp
new (place_address) type                                   // default-initialized
new (place_address) type (initializers)                    // direct-initialized
new (place_address) type [size]                            // each element default-initialized
new (place_address) type [size] { initializer list }       // aggregate-initialized
```

### v. Related std lib

#### 1. allocator class

The library `allocator` class (defined in `<memory>`) separates allocation from construction.

> **Version note:** The examples use the older allocator interface. The `construct()` and `destroy()` member functions of `std::allocator` were removed in C++20. The underlying separation between storage allocation and object lifetime remains relevant.

##### a. Operations

| Expression | Description |
|---|---|
| `allocator<T> a` | Defines an allocator object `a` that can allocate memory for objects of type `T`. |
| `a.allocate(n)` | Allocates raw, unconstructed memory to hold `n` objects of type `T`. |
| `a.deallocate(p, n)` | Deallocates memory that held `n` objects of type `T` starting at pointer `p`. `p` must have been returned by `allocate`, and `n` must be the size requested when `p` was created. Must run `destroy` on any constructed objects before calling `deallocate`. |
| `a.construct(p, args)` | `p` must point to raw memory; `args` are passed to a constructor for type `T`, which constructs an object at `p`. |
| `a.destroy(p)` | Runs the destructor on the object pointed to by `p`. |

##### b. Example: construct and destroy

```cpp
auto q = p;  // points to the end of the constructed range

alloc.construct(q++);            // constructs an empty string at the old q
alloc.construct(q++, 10, 'c');   // constructs "cccccccccc" at the old q
alloc.construct(q++, "hi");      // constructs "hi" at the old q

while (q != p)
    alloc.destroy(--q);          // destroy the constructed strings in reverse order
```

#### 2. Algorithms to copy and fill uninitialized memory

Defined in `<memory>`. These functions **construct** elements in the destination, rather than assigning to them.

| Function | Description |
|---|---|
| `uninitialized_copy(b, e, b2)` | Copies elements from the input range `[b, e)` into unconstructed, raw memory at `b2`. The memory must be large enough. |
| `uninitialized_copy_n(b, n, b2)` | Copies `n` elements starting from `b` into raw memory starting at `b2`. |
| `uninitialized_fill(b, e, t)` | Constructs objects in the range `[b, e)` of raw memory as copies of `t`. |
| `uninitialized_fill_n(b, n, t)` | Constructs `n` objects starting at `b`. `b` must denote unconstructed, raw memory large enough. |

Example: copy and fill

```cpp
// allocate twice as many elements as vi holds
auto p = alloc.allocate(vi.size() * 2);
// construct elements starting at p as copies of elements in vi
auto q = uninitialized_copy(vi.begin(), vi.end(), p);
// initialize the remaining elements to 42
uninitialized_fill_n(q, vi.size(), 42);
```

## Relation

### i. Issues

When manually managing dynamic objects with raw pointers and `new`/`delete`, the programmer must ensure that objects are destroyed and their storage is released correctly. Mistakes can lead to memory leaks, dangling pointers, or double deallocation. A raw pointer stores an address but does not express ownership or guarantee how long the pointed-to object remains valid.

### ii. RAII (Resource Acquisition Is Initialization)

RAII ties the acquisition of a resource to an object's lifetime: the resource is usually acquired in the constructor and released in the destructor. For a local RAII object that manages its resource correctly, the destructor still releases the resource automatically even if the function returns early or an exception triggers stack unwinding. This greatly reduces the risk of leaks from manual resource management, provided that resource ownership and object lifetimes are managed correctly.

**See also:** [Array](../01-basic/array.md): `new T[n]` returns a bare pointer with no size, and `vector` wraps `new[]` + size + RAII (see its Why section).

## Example

<!-- TODO: a complete program, its output, and a "Verifies:" list. Question to answer: when the initialization throws inside a new-expression, which operator delete runs? -->

Think of a new-expression as the code below. "Matching" means the deallocation function receives the same extra arguments as the allocation function.

```cpp
// pseudocode: new T(args) with extra placement arguments extra...
p = operator new(sizeof(T), extra...);
try {
    // construct T at p
} catch (...) {
    operator delete(p, extra...);   // only if a matching function is found
    throw;
}
```

- The deallocation function is looked up with the same extra arguments (the parameters after `void*` match those after `size_t` in the allocation function): class scope first, then global. If no match is found, nothing is called and the storage is not released.
- This cleanup exists only for storage that was obtained but not yet used: if the allocation function itself throws, nothing was allocated and no deallocation function is called; with a non-throwing allocation function (`nothrow`) that returns null, initialization is skipped and the expression yields null.
- It belongs to the new-expression alone. After successful construction, `delete p` always selects the ordinary `operator delete`, never the placement-form one, so a placement-form `operator delete` runs only when initialization throws.

## Reference

- cppreference: [new expression](https://en.cppreference.com/w/cpp/language/new), [delete expression](https://en.cppreference.com/w/cpp/language/delete), [`operator new`](https://en.cppreference.com/w/cpp/memory/new/operator_new), [`operator delete`](https://en.cppreference.com/w/cpp/memory/new/operator_delete), [`std::allocator`](https://en.cppreference.com/w/cpp/memory/allocator)
- C++ standard (draft), by claim:
  - [expr.new](https://eel.is/c++draft/expr.new): allocation, initialization, and the matching deallocation function when initialization throws
  - [expr.delete](https://eel.is/c++draft/expr.delete): the destructor (if any), then the deallocation function
  - [new.delete.placement](https://eel.is/c++draft/new.delete.placement): the non-allocating placement forms are reserved
  - [replacement.functions](https://eel.is/c++draft/replacement.functions): which functions a program may replace
  - [allocator.members](https://eel.is/c++draft/allocator.members): `std::allocator::allocate` obtains storage through `::operator new`
