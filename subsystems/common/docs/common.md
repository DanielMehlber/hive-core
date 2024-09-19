# Common & Base Functionality

## Exclusive Memory Ownership Model

The following section may seem complicated and one might ask if this feature is really justified, but bare with me. I
had to introduce the exclusive ownership model to avoid segmentation faults in various subsystems.

> **TL;DR**: In multi-threaded programs, using `std::shared_ptr<T>` can lead to crashes during clean-ups due to data
> races. To prevent this, the Hive framework sometimes uses an **exclusive ownership model** with the `Owner<T>` class.
> Only one `Owner<T>` controls the object, while others can borrow it using `Borrower<T>`. A weak reference is available
> through `Reference<T>`. This approach ensures safe clean-up and avoids segmentation faults when joining threads on
> clean-up.

### Critical Niche Problems with the Shared Ownerhip Model

> **TL;DR**: Clean-up processes of shared objects are susceptible to data races using `std::shared_ptr<T>`. This is
> usually not a problem in 99%, but it renders subsystems which maintain their own set of worker threads unstable.

In many programs, components or threads often need access to shared data.
Typically, the C++ STL's `std::shared_ptr<T>` can be used for this purpose. It follows a Shared Memory Ownership
Model, where all instances of `std::shared_ptr<T>` have equal access to the shared object, distributing ownership
between multiple instances and eliminating a single "owner." The shared object is destroyed by the last
`std::shared_ptr<T>` instance after it went out of scope, following a "last one turns off the lights" approach.

For most scenarios, this model works well and is widely used, including in various subsystems of the Hive framework.
However, in some niche scenarios this can lead to issues and even crashes. Hive subsystems often spawn and manage
threads themselves, like the job system which spawns its own worker threads. Threads need to be joined when the
subsystem is destroyed. Following rules apply:

1. A thread must not be joined into itself. This causes undefined behavior, like a segfault on linux or a deadlock on
   windows.
2. A thread **should** be joined by the thread that spawned it. Joining arbitrary threads into each other can cause data
   races or deadlocks as well.

When using `std::shared_ptr<T>` to manage subsystems, we can't control which thread calls the subsystem's destructor and
tries to join its worker threads. It will be the thread in which the last `std::shared_ptr<T>` has gone out of scope,
and we can't control that! With good luck, it may be the thread that initially constructed the subsystem and spawned its
threads. But sometimes it can be one of the worker threads themselves, leading to self-joining scenarios we must
prevent. It happened a lot during unit testing.

Consider
placing the [JobManager](\ref hive::jobsystem::JobManager) subsystem inside a shared pointer. The `JobManager` is
created in the main thread and spawns worker threads (as many subsystems do). When the `JobManager` destructor is
called, its worker threads are joined into the calling thread. In the `std::shared_ptr<T>` scenario, this is the thread
where the last instance has gone out of scope. If a worker thread held the last instance, it will try to join itself.
This scenario is rare, but it happened 1/100 times during unit testing, rendering subsystems unstable.

The key issue is that the clean-up process of shared ownership `std::shared_ptr<T>` is susceptible to data races. In our
program, however, this is critical. We want to shut down subsystems without causing crashes. Instead, we employ
the [Owner<T>](\ref hive::common::memory::Owner<T>) class, which follows an Exclusive Memory
Ownership Model.

### Key Features and Idea

The main idea is to separate ownership and access. A single, exclusive owner instance is responsible for controlling and
eventually destroying the shared object, while others can still access it without inheriting this responsibility.
Therefore, we introduce three
objects: [Owner<T>](\ref hive::common::memory::Owner<T>), [Borrower<T>](\ref hive::common::memory::Borrower<T>),
and [Reference<T>](\ref hive::common::memory::Reference<T>).

* **Single Ownership**: Shared data is exclusively owned by a single [Owner<T>](\ref hive::common::memory::Owner<T>)
  instance, which is move-only. It exclusively maintains the object's lifecycle: Being the only one to create, hold, and
  eventually destroy the shared object. The [Owner<T>](\ref hive::common::memory::Owner<T>) can grant access to its
  object to others by borrowing it. It does not destroy its object until all borrows have been released. Therefore,
  the [Owner<T>](\ref hive::common::memory::Owner<T>) is guaranteed to always outlive
  its [Borrower<T>](\ref hive::common::memory::Borrower<T>) instances.
* **Multiple Temporary Access**: Other components or threads can gain temporary access to the owner's object by
  borrowing it. The [Borrower<T>](\ref hive::common::memory::Borrower<T>) class allows access to the data in a manner
  similar to `std::shared_ptr<T>`, but is not responsible to influence the objects life-cycle.
  The [Owner<T>](\ref hive::common::memory::Owner<T>) ensures its destructor only runs after
  all [Borrower<T>](\ref hive::common::memory::Borrower<T>) instances have gone out of scope. **This ensures that the
  object cannot be destroyed while it is still used somewhere else**.
* **Weak References**: A weak reference, like `std::weak_ptr<T>`, is provided through
  the [Reference<T>](\ref hive::common::memory::Reference<T>) class. It allows checking if the object is still alive and
  provides the ability to borrow it. Importantly, it doesn't increase the reference count.

This model introduces a structured and hierarchical approach to ownership, where the data is temporarily shared but
ultimately controlled by a single instance. It clearly defines which thread or component is supposed to destroy and
clean-up the shared object and ensures that it won't be destroyed while still in use somewhere.

### Important Notes and Correct Use

* Avoid storing or keeping [Borrower<T>](\ref hive::common::memory::Borrower<T>) alive for too long and only use them
  for temporary access. Especially avoid storing them in long-living member variables. Remember that
  the [Owner<T>](\ref hive::common::memory::Owner<T>) destructor blocks until all borrowing instances have gone out of
  scope. Not following this rule can lead to deadlocks. Use [Reference<T>](\ref hive::common::memory::Reference<T>) type
  instead if you want to store a reference to the owned data somewhere.

### Examples

```cpp
using namespace hive::common::memory;

Owner<MySubsystem> subsystem_owner = Owner<MySubsystem>();

// does not increae the reference count
Referernce<MySubsystem> subsystem_ref = subsystem_owner.ToReference();

std::thread worker_thread([subsystem_ref]() {
    // try to borrow from reference
    Borrower<MySubsystem> subsystem_borrower = subsystem_ref.TryBorrow();
    
    // do something with the subsystem
    subsystem_borrower->DoLongRunningAction();
});

// while thread is running in borrow is still alive, the owner cannot be destroyed
worker_thread.join();

// now the owner can go out of scope without blocking the thread
```

## Managing Subsystems and their Implementations

The Hive Core consists of many subsystems, like the job-system, networking system, service infrastructure, and more. To
access these subsystems, we introduce the [SubsystemManager](\ref hive::common::subsystems::SubsystemManager). Not only
does it provide access to subsystems, but also allows plugin developers to replace them or add their own ones. It also
allows to separate interfaces from their implementations.

```cpp
Borrower<SubsystemManager> subsystem_manager = BorrowSubsystemManager();

/* load a subsystem implementation directly */
Borrower<JobManager> job_system = subsystem_manager->RequireSubsystem<JobManager>();

/* load an interface implementation implicitely. The caller does not need to know the
 underlying implementation and only cares about the provided functionality */
Borrower<IEventBroker> event_broker = subsystem_manager->RequireSubsystem<IEventBroker>();

/* replace the current registered implementation with your own. All subsequent
 requests for a subsystem will retrurn your implementation */
Owner<MyOwnEventBroker> my_event_broker_impl = Owner<MyOwnEventBroker>(...);
subsystem_manager->AddOrReplaceSubsystem<IEventBroker>(my_event_broker_impl);
```