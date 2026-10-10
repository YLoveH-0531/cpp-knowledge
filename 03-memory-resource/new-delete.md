# New and Delete

## 1. Definition

`new`/`delete` and `new[]`/`delete[]` allow users to dynamically allocate and free heap memory.

> **Note:** All expressions and operators are defined in the global (`::`) scope.

## 2. Implementation

### a. new expression

**i. Ordinary new expression:**

1. Calls `operator new()` or `operator new[]()` to allocate raw, unconstructed memory.
2. Calls the appropriate constructor to construct the object(s) from the specified initializers.
3. Evaluates to the appropriate type and returns the pointer.

**ii. Placement new expression:**

1. Calls the appropriate placement `operator new` function, then the appropriate constructor to construct the objects with initializers.
2. Evaluates to the appropriate type and returns the pointer. (The placement `operator new` itself returns its second argument unchanged.)

### b. delete expression

1. The appropriate destructor is run on the object to which `ptr` points, or on the elements in the array to which `arr` points.
2. The compiler frees the memory by calling `operator delete` or `operator delete[]`.

## 3. Operator Functions

### a. Allocating Version

**Declarations:**

```cpp
void* operator new  ( std::size_t count );
void* operator new[]( std::size_t count );
void  operator delete  ( void* ptr ) noexcept;
void  operator delete[]( void* ptr ) noexcept;

void* operator new  ( std::size_t count, const std::nothrow_t& tag );
void* operator new[]( std::size_t count, const std::nothrow_t& tag );
void  operator delete  ( void* ptr, const std::nothrow_t& tag ) noexcept;
void  operator delete[]( void* ptr, const std::nothrow_t& tag ) noexcept;
```

**Properties:**

1. **Global:** All versions of `operator new`/`delete` (including placement) are declared in the global namespace (`::`) , not within the `std` namespace.
2. **Implicit:** The allocating/deallocating versions are implicitly declared in every translation unit, whether or not `<new>` is included.
3. **Replaceable:** The allocating versions are replaceable — a program may provide its own definition, or overload for a specific type. (Placement `operator new`/`delete` cannot be replaced.)

### b. Placement Version

**Declarations:**

```cpp
void* operator new  ( std::size_t count, void* ptr );
void* operator new[]( std::size_t count, void* ptr );
void  operator delete  ( void* ptr, void* place ) noexcept;
void  operator delete[]( void* ptr, void* place ) noexcept;
```

**Notes:**

1. The placement `operator new` is called by a placement new expression (or manually by the user).
2. The placement `operator delete` is called when the constructor in a placement new expression throws an exception; it does nothing (no-op).

## 4. Usage

### a. Allocating new

**i. Initialization forms:**

```cpp
new type                              // default-initialized
new type ()                           // value-initialized
new type (initializers)               // direct-initialized
new type {}                           // list-initialized
new type [size]                       // each element default-initialized
new type [size] ()                    // each element value-initialized
new type [size] { initializer list }  // aggregate-initialized
```

**ii. Array type alias:**

```cpp
typedef int arrT[42];            // arrT names the type "array of 42 ints"
int *p = new arrT;               // allocates an array of 42 ints; p points to the first one
delete [] p;                     // brackets are necessary because we allocated an array
```

**iii. Zero-length array:**

```cpp
char *p = new char[0];           // legal, returns a valid non-null pointer
```

### b. Placement new

```cpp
new (place_address) type
new (place_address) type (initializers)
new (place_address) type [size]
new (place_address) type [size] { braced initializer list }
```

## 5. allocator Class

The library `allocator` class (defined in `<memory>`) separates allocation from construction.

### a. Operations

| Expression | Description |
|---|---|
| `allocator<T> a` | Defines an allocator object `a` that can allocate memory for objects of type `T`. |
| `a.allocate(n)` | Allocates raw, unconstructed memory to hold `n` objects of type `T`. |
| `a.deallocate(p, n)` | Deallocates memory that held `n` objects of type `T` starting at pointer `p`. `p` must have been returned by `allocate`, and `n` must be the size requested when `p` was created. Must run `destroy` on any constructed objects before calling `deallocate`. |
| `a.construct(p, args)` | `p` must point to raw memory; `args` are passed to a constructor for type `T`, which constructs an object at `p`. |
| `a.destroy(p)` | Runs the destructor on the object pointed to by `p`. |

### b. Example

```cpp
auto q = p;  // q will point to one past the last constructed element
alloc.construct(q++);            // *q is the empty string
alloc.construct(q++, 10, 'c');   // *q is cccccccccc
alloc.construct(q++, "hi");      // *q is hi!

while (q != p)
    alloc.destroy(--q);          // free the strings we actually allocated
```

### c. Algorithms to Copy and Fill Uninitialized Memory

Defined in `<memory>`. These functions **construct** elements in the destination, rather than assigning to them.

| Function | Description |
|---|---|
| `uninitialized_copy(b, e, b2)` | Copies elements from the input range `[b, e)` into unconstructed, raw memory at `b2`. The memory must be large enough. |
| `uninitialized_copy_n(b, n, b2)` | Copies `n` elements starting from `b` into raw memory starting at `b2`. |
| `uninitialized_fill(b, e, t)` | Constructs objects in the range `[b, e)` of raw memory as copies of `t`. |
| `uninitialized_fill_n(b, n, t)` | Constructs `n` objects starting at `b`. `b` must denote unconstructed, raw memory large enough. |

### d. Example

```cpp
// allocate twice as many elements as vi holds
auto p = alloc.allocate(vi.size() * 2);
// construct elements starting at p as copies of elements in vi
auto q = uninitialized_copy(vi.begin(), vi.end(), p);
// initialize the remaining elements to 42
uninitialized_fill_n(q, vi.size(), 42);
```
