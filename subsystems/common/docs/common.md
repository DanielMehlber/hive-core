# Common & Base Functionality

## Exclusive Memory Ownership Model

> **TL;DR**: In Hive, using `std::shared_ptr<T>` for subsystems can cause issues in the multithreaded environment
> because
> it shares ownership of an object, which can lead to crashes during cleanup. Instead, Hive uses the `Owner<T>` class
> for subsystem management, which gives exclusive control to one instance, while others can only borrow the object
> temporarily. This approach prevents data races and ensures safe cleanup of shared objects and subsystems.

Many components, subsystems, plugins, and threads often need access to the same data or objects in the program.
Usually, `std::shared_ptr<T>` from the C++ STL is used for this purpose. It employs a **Shared Memory Ownership Model**
and every instance of `std::shared_ptr<T>` has the same _privileges_ towards the shared object. There is no real
_owner_. The shared object is destroyed, when the last `std::shared_ptr<T>` goes out of scope, in a "_the last one turns
off the lights_" manner.

For most cases, this is a valid solution and is also used in many subsystems of the Hive framework. However, when
working with more complex shared objects in multi-threaded environments, this can cause problems. Imagine, we put
the [JobManager](\ref hive::jobsystem::JobManager) subsystem into a shared pointer. It is created in the main-thread and
spawns its own
worker-threads (
like many subsystems do!). **Remember:** The [JobManager](\ref hive::jobsystem::JobManager) worker threads must be
joined **into the same thread that
spawned them** before its destruction. This is where the shared ownership model fails. When _some_ arbitrary thread (
which is not the main-thread) owns by
chance the last `std::shared_ptr<JobManager>` instance, joins its worker threads, a segmentation fault can occur on some
platforms. Especially when the last instance was inside the worker threads themselves, and they try to join themselves
as a result (sounds unlikely, but happened lots of times).

The main point is, that pure shared ownership causes **data races** in the clean-up process which may crash the program.
We need to prevent that. For subsystems, no `std::shared_ptr<T>` are used. Instead, we use
the [Owner<T>](\ref hive::common::memory::Owner<T>) class. It employs an **Exclusive Memory Ownership Model**:

* Only one instance of [Owner<T>](\ref hive::common::memory::Owner<T>) is allowed to exclusively own the object at a
  time. It can only be moved and is
  responsible
  for cleaning-up the object. It cannot be destroyed before all other references to the object are released.
* When other components, subsystems, or plugins want to use the owner's data, they have to _borrow_ it.
  The [Borrower<T>](\ref hive::common::memory::Borrower<T>) has access to the data (like a `std::shared_ptr<T>`).
  The [Owner<T>](\ref hive::common::memory::Owner<T>)'s destructor blocks until
  all [Borrower<T>](\ref hive::common::memory::Borrower<T>) instances have gone out of scope.
* A weak reference, like the `std::weak_ptr<T>` from the C++ STL, is implemented using
  the [Reference<T>](\ref hive::common::memory::Reference<T>) class. It
  allows to check if the object is still alive and to _borrow_ it. It does not increase the reference count.

This results in a more elaborate and hierarchical ownership model, where data is shared, but still owned by a single
instance. It
prevents data-races in the clean-up process, we mentioned before.

```cpp
using namespace hive::common::memory;

Owner<MySubsystem> subsystem_owner = Owner<MySubsystem>();

// does not increae the reference count
Referernce<MySubsystem> subsystem_ref = subsystem_owner.ToReference();

std::thread worker_thread([subsystem_ref]() {
    Borrower<MySubsystem> subsystem_borrower = subsystem_owner.TryBorrow();
    
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
does it provide access to
subsystems, but also
allows plugin developers to replace them or add their own ones. It also allows to separate interfaces from their
implementations.

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