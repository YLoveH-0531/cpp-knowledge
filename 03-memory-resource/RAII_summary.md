# RAII 

## What
RAII actially is C++ programming technique, which bind the lift circle of resource which must be aquired before use to the lifetime of the object.

## Why
The constructor/destructor technique generalizes to any scoped use of a resource and the move technique handles transfers of owner-
ship between scopes. The key idea is that a resource is always owned by a local (scoped) object(called resource handle). The handle’s constructor acquires the resource and
the handle’s destructor releases it.

1.What is Resource.
  A resource is anything that a program acquires from another part of system and must (explicitly or implicitly) release back to its owner after use.
  eg, momory, files, sockets, locks, threads, and database transactions.

2.Why so dangourous when leak?
  If resources are not properly released, the system’s performance will suffer and eventually a long-running system will fail for lack of usable memory

3.What caused leak?
C++ mechanism itself does not make sure to release the resources. it is up to the user who acquire the resourse. But user may forget to release resources, and can not afford to do that in complex return context or even have nothing to do with exception context.

4.Solutions?
The core question: Without counting paths, **how can we guarantee that resources are released regardless of which exit path is taken?**
 1).finally: C++ has none, and it would need a cleanup clause at every acquisition site. There are far more acquisitions than kinds of resources, so it means more code than RAII, where each resource kind writes its release once in a handle class (Stroustrup FAQ: https://www.stroustrup.com/bs_faq2.html#finally).
 2).Bind the release operation to something that is guaranteed to execute no matter how the scope is exited: an object's destructor.


## How

### Key points
