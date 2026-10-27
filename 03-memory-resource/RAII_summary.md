# RAII(Resource Acquisition Is Initialization)

## What

RAII is actually a C++ programming technique that binds the lifetime of a resource, which must be acquired before use, to the lifetime of an object. The key idea is that a resource is always owned by a local (scoped) object (called a resource handle). The handle’s constructor acquires the resource and the handle’s destructor releases it.

```text
 constructor   acquire the resource          if it fails: throws, no object exists
     |
     v
 in use        the object owns the resource
     |
     |         leave the scope by any exit: normal end, early return/break,
     |         or an exception (stack unwinding)
     v
 destructor    release the resource
```

## Why

The constructor/destructor technique generalizes to any scoped use of a resource.

### 1. What is a resource?

A resource is anything that a program acquires from another part of the system and must (explicitly or implicitly) release back to its owner after use.

e.g., memory, files, sockets, locks, threads, and database transactions.

### 2. Why is a leak so dangerous?

If resources are not properly released, the system’s performance will suffer and eventually a long-running system will fail for lack of usable memory.

### 3. What causes a leak?

C++ automatically manages object lifetimes, but it cannot automatically determine how to release every resource represented by a raw pointer or handle: destroying a raw pointer does nothing to the resource it points to. Manual cleanup is fragile because early returns, exceptions, and other control-flow paths may bypass the cleanup code.

**Example:**

```cpp
void f() {
    FILE* p = fopen("a", "r");
    g();            // may throw: a hidden exit path
    fclose(p);      // skipped if g() throws
}
```

### 4. Solutions?

The core question: Without counting paths, **how can we guarantee that resources are released regardless of which exit path is taken?**

1. `finally`: C++ has no `finally` statement, and it would need a cleanup clause at every acquisition site. There are far more acquisitions than kinds of resources, so it means more code than RAII, where each resource kind writes its release once in a handle class ([Stroustrup FAQ](https://www.stroustrup.com/bs_faq2.html#finally)).
2. Bind the release operation to something that is guaranteed to execute when the scope is exited normally or by an exception (stack unwinding): an object's destructor.

## How

### Key points

### i. What the language guarantees

With a handle object, the `f()` from Why needs no `fclose` on any exit path:

```cpp
void f() {
    File p("a", "r");   // the constructor acquires the resource
    g();                // if g() throws, ~File() still runs (stack unwinding)
}                       // on a normal exit, ~File() runs here
```

(`File` wraps a `FILE*`.) Two language rules make this work:

1. **Leaving a scope destroys its local objects.** Whether the scope ends normally or is left early by `return`, `break` or `goto`, every fully constructed local object in it (automatic storage duration) is destroyed.
2. **So does an exception.** It travels up the call stack until a handler catches it, and each scope it leaves destroys its fully constructed local objects. This is stack unwinding.

Both destroy objects in the reverse order of construction, so a resource acquired later (which often depends on an earlier one) is released first.

Together they give the guarantee stated in Why, within these limits:

- **The object must be fully constructed.** An object whose construction fails halfway is not destroyed (section ii).
- **It guarantees that the cleanup code runs, not that the cleanup succeeds.** The release inside a destructor can itself fail (section iii).
- **It protects resources, not object state.** Stack unwinding only calls destructors; it does not undo modifications already made. Whether an object's class invariant (section ii) still holds after an exception depends on how the code is structured (see [exception-safety-draft.md](../07-language-mechanics/exception-safety-draft.md)).
- **Some ways of ending the program skip destructors.** `abort`, `_exit` and crashes never run them; `std::exit` does not unwind the stack; and when no handler catches an exception, `std::terminate` is called and whether the stack is unwound first is implementation-defined.

### ii. Construction: acquire, or leave no object behind

Only fully constructed objects are destroyed (section i), so a constructor has two acceptable outcomes: it builds the object completely, or it fails cleanly, leaving neither an object nor a leaked resource.

**Why acquire in the constructor.** In Resource Acquisition Is Initialization, acquisition is part of initialization, so a handle is never in an accidental "not acquired yet" state: it either holds a valid resource or is in a designed empty state, which is exactly its class invariant (defined at the end of this section). The destructor only has to handle these two states.

This sharpens the "binding" in What: the time the object holds the resource lies within its lifetime but does not equal it, since the object can be empty or release early (`reset()`). So acquisition is usually, not necessarily, in the constructor.

**Reporting failure.** A constructor has no return value. If it cannot establish the class invariant, it must not leave an object that violates it, so the default is to throw (Core Guidelines C.42); the hidden exits this creates are what stack unwinding cleans up (section i). Where exceptions are unavailable, a factory returning `optional`/`expected` also leaves no object, but every caller must check the result. A `valid()` flag or a two-phase `init()` is sound only when "not acquired yet" is itself a designed state of the type (a "may be empty" handle such as a default-constructed `ofstream`); every user then has to handle that state. What must never exist is a half-initialized object outside the class invariant.

**When a constructor throws** ([except.ctor](https://eel.is/c++draft/except.ctor)):

1. Its own destructor is **not** called, because the object's lifetime never began.
2. The **fully constructed members and bases** are destroyed, in reverse order.
3. For `new T(...)`, the matching `operator delete` frees the storage (see [new-delete.md](new-delete.md)).

So a resource that no fully constructed object owns yet leaks if a later step of the constructor throws. `Bad` leaks `a_`; `Good`'s members clean up after themselves:

```cpp
struct Bad {
    int *a_, *b_;                   // raw pointers do nothing when destroyed
    Bad() {
        a_ = new int[100];
        b_ = new int[100];          // throws: a_ leaks, and ~Bad() is not called
    }
};

struct Good {
    std::unique_ptr<int[]> a_, b_;  // each resource has its own owner
    Good() : a_(new int[100]),
             b_(new int[100]) {}    // throws: the constructed a_ is destroyed
};
```

Hence the rule (Core Guidelines R.12): hand each resource to a fully constructed object the moment it is acquired. In practice, **each resource is held by one member object**, and cleanup lives in that member's destructor, not the enclosing object's. The lowest-level handle holds a single raw resource and does nothing that can throw after acquiring it.

#### Class invariant

A **class invariant** is a rule that must hold whenever an object is in a stable, usable state. The constructor establishes it. A member function may assume it holds on entry and must restore it before it returns (it may be broken temporarily in the middle). The class invariant and RAII are two different concepts: the invariant is the condition to be kept, and RAII is one design technique that helps keep it.

For a resource handle, the invariant can be stated as: the object either owns a valid resource or is in a designed empty state, and the destructor can safely handle both states.

| | Rule | Example |
|---|---|---|
| Never empty | the object exists ⇒ it holds a resource | `lock_guard` |
| May be empty | it holds a valid resource, or is in the designed empty state | `unique_ptr`, a movable `File` |

The empty state is fixed at design time and can be checked through the interface (a moved-from object ends up there). It is not the "invalid object left behind by a failed construction".

### iii. Destruction: release, and never throw

Destructors run during unwinding (section i). If one throws an exception that escapes it, two exceptions are in flight and C++ calls `std::terminate`. So a destructor **must not let an exception escape**: it is `noexcept` by default (unless a member or base destructor is not), and Core Guidelines C.36 and C.37 say the same. A `try/catch` inside the destructor is fine.

If the release itself can fail:

- **Best effort**: log it or swallow it, when the failure cannot be repaired anyway.
- **The caller must know**: provide an explicit `close()` and keep the destructor as a fallback (`ofstream` does this).
- **An operation that both releases and may fail**: split it. `commit()` may throw; the destructor only rolls back.

### iv. Ownership: who releases, and how many times

One handle now acquires and releases correctly, but a resource must be released exactly once, so each resource needs a clear ownership policy. **Whoever owns the resource is responsible for releasing it; whoever merely refers to it is not.** Owners: value members, `vector`, `unique_ptr`, `shared_ptr`. Non-owners: references, raw pointers, `string_view`, and `weak_ptr` (see [smart-pointers-draft.md](smart-pointers-draft.md)).

Ownership decides how copying works. A compiler-generated memberwise copy would make two objects hold one resource and release it twice, so the policy has to be explicit:

- exclusive ownership: forbid copying (`File`, `unique_ptr`);
- value semantics: copy deeply, so each object owns its own (`vector`);
- shared ownership: the last owner releases (`shared_ptr`).

**Move** transfers ownership, and the moved-from object must stay safely destructible. For a handle the usual design is the empty state from section ii (standard library objects are generally only guaranteed to be valid but unspecified after a move). **Rule of Zero**: let members (`unique_ptr`, `vector`) own the resources and write no special member functions; the generated ones are then correct. Details: [copy-and-move-draft.md](copy-and-move-draft.md).

### v. Standard handles: smart pointers and non-memory resources

The policies in section iv already exist as standard types:

| Type | Resource | Ownership |
|---|---|---|
| `vector`, `string` | dynamic storage and elements | exclusive, deep copy |
| `unique_ptr` | one dynamic object or array | exclusive, move-only |
| `shared_ptr` | same | shared, reference counted |
| `lock_guard` | a held mutex | non-copyable, non-movable |
| `ofstream` | an open file | exclusive, movable |

Smart pointers are only one application of RAII, which manages any resource that must be released after use: even a `FILE*` can be owned by a `unique_ptr` with a custom deleter (see [smart-pointers-draft.md](smart-pointers-draft.md)).

## Relation

- Move semantics: the move technique handles transfers of ownership between scopes.
- Exception safety: RAII releases resources during stack unwinding, which is the foundation of exception safety. Keeping the object's invariants (basic guarantee) or restoring its earlier state (strong guarantee, e.g., copy-and-swap) is not RAII's job; see [exception-safety-draft.md](../07-language-mechanics/exception-safety-draft.md).
