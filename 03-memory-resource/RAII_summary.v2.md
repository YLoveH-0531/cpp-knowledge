# RAII

## What
RAII is actually a C++ programming technique that binds the lifetime of a resource, which must be acquired before use, to the lifetime of an object.

## Why
The constructor/destructor technique generalizes to any scoped use of a resource and the move technique handles transfers of ownership between scopes. The key idea is that a resource is always owned by a local (scoped) object (called a resource handle). The handle’s constructor acquires the resource and the handle’s destructor releases it.

1.What is Resource.
  A resource is anything that a program acquires from another part of system and must (explicitly or implicitly) release back to its owner after use.
  eg, memory, files, sockets, locks, threads, and database transactions.

2.Why so dangerous when leak?
  If resources are not properly released, the system’s performance will suffer and eventually a long-running system will fail for lack of usable memory

3.What caused leak?
  C++ manages the lifetime of objects, but not of what a raw pointer or handle refers to: destroying a raw pointer does nothing to the resource it points to. So releasing is up to the user who acquired the resource. But the user may forget to release it, and cannot afford to cover every complex return path or exception path.

4.Solutions?
The core question: Without counting paths, **how can we guarantee that resources are released regardless of which exit path is taken?**
 1).finally: C++ has none, and it would need a cleanup clause at every acquisition site. There are far more acquisitions than kinds of resources, so it means more code than RAII, where each resource kind writes its release once in a handle class (Stroustrup FAQ: https://www.stroustrup.com/bs_faq2.html#finally).
 2).Bind the release operation to something that is guaranteed to execute when the scope is exited normally or by an exception (stack unwinding): an object's destructor.


## How

### Key points
