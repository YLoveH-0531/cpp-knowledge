# RAII (Resource Acquisition Is Initialization)

## What

RAII is a C++ technique that binds responsibility for managing a resource to the lifetime of an object(so-called resource handle). Typically, such handle acquires  the resource during construction and releases it during destruction.

```text
 constructor   acquire the resource, if construction fails, see How ii
      |             
      |
      v
    in use     the object owns the resource
      |
      |        
      |        leave the scope normally or through an exception (stack unwinding)
      v                                  |
 destructor                              v
                release the resource (must not let exceptions escape)
```

## Why

### i. What is a resource?

A resource is anything that a program acquires from another part of the system and must (explicitly or implicitly) release back to the system after use.(e.g., memory, files, sockets, locks, threads, and database transactions. )


### ii. Why is a leak so dangerous?

If resources are not properly released, the system's performance will suffer and eventually a long-running system will fail, either by running out of the resource or, for a lock, by blocking other threads.

### iii. What causes a leak?

Destroying a raw pointer or a raw handle such as a `FILE*` releases nothing, so the release (`delete`, `fclose`, `unlock`) is a separate step that the programmer must write on every exit path. Early returns and exceptions can skip it. 

**Example:**

```cpp
void f() {
    FILE* p = fopen("a", "r");
    g();            // may throw: a hidden exit path
    fclose(p);      // skipped if g() throws
}
```

### iv. Solutions?

The core question: Without counting paths, **how can we guarantee that resources are released regardless of which exit path is taken?**

1. `finally`: C++ has no `finally` statement, and it would need a cleanup clause at every acquisition site. Acquisitions far outnumber resource kinds, so this takes more code than RAII ([Stroustrup FAQ](https://www.stroustrup.com/bs_faq2.html#finally)).
2. RAII: Encapsulate resource management in a handle object and put the release operation in its destructor. When the handle is destroyed, its destructor runs automatically.

## How

### i. Key points

#### 1. Class invariant

A class invariant is a condition that defines a valid object state. A successful constructor establishes it, and public member functions should preserve it whenever they return. If a function throws but the object remains alive, it should leave the object in a state that still satisfies the invariant. A function may temporarily violate the invariant internally, but must restore it before returning or exposing the object.

For an RAII handle, the invariant specifies whether the handle must own a resource or is also allowed to be empty. Its destructor must release the resource if one is owned and safely handle the empty state.

| State | Example |
| --- | --- |
| Must hold the resource after successful construction | `lock_guard` |
| May be empty | `unique_ptr`, `ofstream` |

#### 2. Ownership

Ownership is the responsibility for managing a resource. RAII binds this responsibility to an object's lifetime. Destroying an owning handle relinquishes its ownership.
> **Note:** Resource lifetime runs from creation to final release. Ownership is the responsibility for managing that lifetime; acquisition, transfer, and release are actions that may occur during it.

### ii. Construction: acquire, or leave no object behind

Constructor should establish the class invariant. On success, the object owns a valid resource or is in an explicitly permitted empty state. If a required resource cannot be acquired, the constructor should normally throw rather than leave the object partially initialized (Core Guidelines C.42).

If exceptions are not used, a factory function can return an explicit result, such as `std::optional` or `std::expected` (C++23). Avoid two-phase initialization unless an uninitialized state is an intentional part of the design.

**What happens when construction fails?** ([except.ctor](https://eel.is/c++draft/except.ctor))

1. If a non-delegating constructor throws, the object's destructor is not called.
2. Fully constructed base and member subobjects are destroyed in reverse order of construction.
3. If initialization takes place in a `new` expression, such as `new T(...)`, the expression calls a matching deallocation function for the allocated storage, if one is found.

**Design rule (Core Guidelines R.12):** Give each acquired resource to an RAII owner as soon as possible. 

### iii. Destruction: release, and never throw

Construction establishes the class invariant. The destructor must safely handle every state permitted by the invariant. This happens when the object leaves its scope normally or when an exception causes the scope to be exited. As each scope is exited, fully constructed local objects are destroyed in reverse order of construction. 

#### 1. Why must destructors not throw?

A destructor may run during stack unwinding, while another exception is already propagating.
Also there is generally no reliable way for the caller to handle a cleanup error at that point.

A destructor is implicitly `noexcept` unless a base or member destructor can throw, and an exception escaping a `noexcept` destructor terminates the program even when no other exception is in flight. A destructor may still call a function that can throw, or even throw an exception internally, provided the exception is caught before it escapes the destructor.

#### 2. What if cleanup can fail?

- **Best effort:** Catch the error, log it if useful, and return normally when the failure cannot be meaningfully recovered from.
- **Report the error explicitly:** Provide a `close()` function that lets the caller handle failure; keep the destructor as a fallback for cleanup.
- **Separate the fallible operation from cleanup:** For a transaction, let `commit()` report failure, while the destructor rolls back an uncommitted transaction.

**Note :**  RAII guarantee the execution of destructors, but does not guarantee that cleanup succeeds or that cleanup occurs when execution terminates without stack unwinding.

**Design rule:** A destructor should attempt cleanup without allowing exceptions to escape. If the caller must know whether an operation succeeded, provide an explicit function for that operation.

### iv. Ownership and the Rule of Three/Five/Zero

RAII class must define who owns each resource and what copying or moving means. These decisions determine how the resource is released and whether ownership is transferred, duplicated, or shared.

#### 1. Why do copy and move semantics matter?

A compiler-generated copy of a raw resource handle copies the handle value, not the underlying resource. If two objects then treat the same resource as exclusively owned, both may try to release it, causing a double release.

#### 2. Rule of Three/Five/Zero

1. **Rule of Three**

   If a class needs a user-defined destructor, copy constructor, or copy assignment operator, consider all three together. When a class directly manages a resource, default copying may duplicate the handle without duplicating the resource.

2. **Rule of Five**

   Since C++11, also consider the move constructor and move assignment operator. If any of these five special member functions is user-declared, including as `= delete`, consider all five and define, default, or delete each as appropriate.

   For exclusive ownership, moving transfers the resource to the destination. The source must remain safe to destroy, typically by being left in an empty state. Move assignment must also deal with any resource already owned by the destination.

3. **Rule of Zero**

   Prefer using existing RAII types to manage resources. If members such as `std::unique_ptr`, `std::vector`, or `std::string` already express the intended ownership policy, the enclosing class usually needs no user-defined destructor, copy operations, or move operations. The compiler-generated operations then follow the semantics of the member types.

```cpp
class Bad {
    int* a_;
    int* b_;

public:
    Bad() : a_(nullptr), b_(nullptr) {
        a_ = new int[99];
        b_ = new int[99];  // If this throws, a_ leaks.
    }
};

class Good {
    std::unique_ptr<int[]> a_, b_;

public:
    Good()
        : a_(std::make_unique<int[]>(99)),
          b_(std::make_unique<int[]>(99)) {}
};
```

**Design rule:** Prefer the Rule of Zero. Use the Rule of Three/Five when a class must directly manage a resource or define special ownership semantics.

### v. Standard handles: smart pointers and non-memory resources

The standard library already provides handles for common resources:

| Type | Managed resource | Ownership / semantics |
| --- | --- | --- |
| `vector`, `string` | dynamic storage and the elements in it | exclusive; copying copies the elements |
| `unique_ptr` | one dynamically allocated object or array | exclusive; move-only |
| `shared_ptr` | same | shared; reference-counted |
| `lock_guard` | the lock held on a mutex | exclusive, scope-bound; non-copyable, non-movable |
| `ofstream` | an open file | exclusive; movable |

When a resource requires a release operation other than the default `delete` or `delete[]`, supply a custom deleter.

```cpp
struct Closer { void operator()(std::FILE* f) const noexcept { std::fclose(f); } };
std::unique_ptr<std::FILE, Closer> p(std::fopen("a", "r"));   // fclose on destruction, not delete
```

See [smart-pointers-draft.md](smart-pointers-draft.md) for deleter rules, `make_unique`/`make_shared`, and the control block.

## Relation

### Connection

- **[new-delete](new-delete.md):** A `new` expression allocates storage and initializes an object; a `delete` expression destroys the object and releases its storage. RAII ties the appropriate cleanup to the owning handle's lifetime.
- **[array](../01-basic/array.md):** `new T[n]` returns a pointer without retaining the array's size. `std::vector` manages storage, element lifetime, and size together.
- **[Exception safety](../07-language-mechanics/exception-safety-draft.md):** RAII prevents resource leaks during stack unwinding. Exception-safety guarantees additionally specify the state of an object or program after an operation fails.
- **Move semantics:** Move operations can transfer ownership from one object to another.

**See also:** [copy-and-move-draft.md](copy-and-move-draft.md) for ownership transfer and special member functions; [smart-pointers-draft.md](smart-pointers-draft.md) for smart-pointer design and implementation details.

## Example


## Reference

- Stroustrup, *Foundations of C++* (ETAPS 2012), read sections 6–8 following the reading route in [RAII.md](RAII.md); this draft has not been checked against the original
- C++ Core Guidelines: R.1 (manage resources with resource handles and RAII), R.12 (hand the result of an explicit allocation to a manager object immediately), E.6 (use RAII to prevent leaks), C.31 (a class's destructor releases the resources the class acquires, with a note on non-owning), C.36 and C.37 (a destructor must not fail and should be `noexcept`), C.42 (throw an exception if a constructor cannot construct a valid object)
- cppreference: [Destructors](https://en.cppreference.com/w/cpp/language/destructor), [RAII](https://en.cppreference.com/w/cpp/language/raii)
- C++ standard (draft), by claim:
  - [class.dtor](https://eel.is/c++draft/class.dtor): when a block scope is left, destructors are invoked implicitly for the constructed objects of automatic storage duration
  - [except.ctor](https://eel.is/c++draft/except.ctor): stack unwinding; when construction fails, the already constructed subobjects are destroyed
  - [except.spec](https://eel.is/c++draft/except.spec): a destructor is implicitly `noexcept` unless a base or member destructor can throw
  - [except.terminate](https://eel.is/c++draft/except.terminate): `std::terminate` is called when a destructor exits with an exception during stack unwinding, or an exception escapes a `noexcept` function; with no matching handler, or when `noexcept` is violated, whether the stack is unwound first is implementation-defined
