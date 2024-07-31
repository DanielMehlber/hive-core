#pragma once

#include "common/assert/Assert.h"
#include "common/exceptions/ExceptionsBase.h"
#include "common/synchronization/ScopedLock.h"
#include "common/synchronization/SpinLock.h"
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <type_traits>

namespace common::memory {

// forward declared
template <typename T> class Owner;

/**
 * Shared state between the owner and its borrowers.
 */
struct OwnershipState {
  /** count of pending borrows */
  std::atomic_int borrows = 0;

  /** determines if owner can borrow its owned object. If this is false, the
   * owner is not allowed to borrow its contents. */
  std::atomic_bool alive = true;

  /** pointer to currently owning instance */
  std::atomic<void *> owner;

  /** can be used to lock the ownership state */
  common::sync::SpinLock state_lock;
};

template <typename T> class Reference;

/**
 * An Owner can temporarily lend its data to other users, like threads,
 * instances, or function calls, without violating or sharing its exclusive
 * ownership (in contrast to std::shared_ptr). A Borrower allows access to the
 * Owner's data and is not able to outlive the Owner. In other words: The
 * existence of a living Borrower instance blocks its Owner's destructor until
 * all instances have been destroyed. This prohibits the Owner from being
 * destroyed while its data is still accessed by other parties (like threads),
 * causing dangling Borrowers. Think of a Borrower as a lock of the Owner's
 * destructor.
 *
 * @attention Borrowing is only recommended for accessing the Owner's data in a
 * temporary, short-term, or scoped manner (e.g. borrow the Owner's data until
 * the end of the current function). Storing or keeping Borrowers for longer
 * than required (e.g. as class members) can lead to deadlocks because the
 * Owner's destructor will block until all Borrowers have been destroyed. If
 * that can't happen, it will block indefinitely. Take a look at the Reference
 * class for this purpose.
 *
 * @tparam T type of the borrowed object
 */
template <typename T> class Borrower {
protected:
  friend Owner<T>;
  friend Reference<T>;
  std::shared_ptr<OwnershipState> _shared_ownership_state;

#ifndef NDEBUG
  /** convenience pointer to the owned object
  (mainly for the use with debuggers, because they'd only see a void*). */
  T *_data;
#endif

  /**
   * Borrows from an Owner.
   * @param owner Owner to borrow from.
   * @param already_locked if true, the shared ownership state has already been
   * locked further up the callstack. This parameter is mandatory to avoid
   * recursive locks.
   */
  explicit Borrower(Owner<T> *owner, bool already_locked = false) {
    std::unique_ptr<common::sync::ScopedLock<common::sync::SpinLock>> lock;
    if (!already_locked) {
      lock = std::make_unique<common::sync::ScopedLock<common::sync::SpinLock>>(
          owner->_state->state_lock);
    }
    _shared_ownership_state = owner->_state;
    _shared_ownership_state->borrows.fetch_add(1, std::memory_order_relaxed);
#ifndef NDEBUG
    // get convenience pointer to owned data (mainly for debuggers).
    _data = owner->operator->();
#endif
  }

public:
  Borrower(Borrower<T> &other)
      : _shared_ownership_state(other._shared_ownership_state) {
    _shared_ownership_state->borrows.fetch_add(1, std::memory_order_relaxed);
#ifndef NDEBUG
    // get convenience pointer to owned data (mainly for debuggers).
    auto *owner = (Owner<T> *)_shared_ownership_state->owner.load();
    _data = owner->operator->();
#endif
  }

  Borrower(const Borrower<T> &other)
      : _shared_ownership_state(other._shared_ownership_state) {
    _shared_ownership_state->borrows.fetch_add(1, std::memory_order_relaxed);
#ifndef NDEBUG
    // get convenience pointer to owned data (mainly for debuggers).
    auto *owner = (Owner<T> *)_shared_ownership_state->owner.load();
    _data = owner->operator->();
#endif
  }

  ~Borrower() {
    _shared_ownership_state->borrows.fetch_sub(1, std::memory_order_relaxed);
  }

  T *operator->() const {
    common::sync::ScopedLock lock(_shared_ownership_state->state_lock);
    auto *owner = (Owner<T> *)_shared_ownership_state->owner.load();
    return owner->operator->();
  }

  T &operator*() { return *this->operator->(); }

  Reference<T> ToReference();
};

template <typename T> Reference<T> Borrower<T>::ToReference() {
  return Reference<T>(this);
}

DECLARE_EXCEPTION(BorrowFailedException);

/**
 * In contrast to a Borrower instance, a Reference does not prohibit the owner's
 * destruction. The Owner may go out of scope and die while Reference instances
 * of it are still alive. This means, that a Reference may outlive its Owner and
 * may thus be in an invalid state when its Owner has already died or gone out
 * of scope. Therefore, it is not capable of accessing the Owner's data directly
 * (because it might already be dead) and must first be converted to a Borrower.
 * Obviously, this conversion fails if the Owner instance has already been
 * destroyed. Think of this class as a weak user of the Owner's data (like a
 * destroyed. Think of this class as a weak user of the Owner's data (like a
 * std::weak_ptr).
 *
 * @tparam T type of borrowed object
 */
template <typename T> class Reference {
protected:
  friend Owner<T>;
  std::shared_ptr<OwnershipState> _shared_ownership_state;

  explicit Reference(Owner<T> *owner);

  /**
   * Attempts to convert the Reference to a Borrower instance. This operation
   * may fail, if the referenced Owner is not alive anymore.
   * @attention This call requires the Owner to be alive, or else an exception
   * is thrown. Checks whether or not this condition is met must be performed
   * before this call.
   * @param already_locked if true, the shared ownership state has already been
   * locked further up the callstack. This parameter is mandatory to avoid
   * recursive locks.
   * @exception BorrowFailedException if shared state was invalid or Owner was
   * not alive anymore.
   * @return Borrower instance
   */
  Borrower<T> PerformBorrow(bool already_locked);

public:
  Reference();
  explicit Reference(Borrower<T> *borrow);

  /**
   * Converts a reference of a certain type to another reference of a base-class
   * the original type inherits from.
   * @tparam Other other type that must inherit this type
   * @param other other reference
   * @attention This will not compile if the outcome type is not a base class of
   * the original type.
   */
  template <typename Other> Reference(Reference<Other> &other);

  template <typename Other> Reference(Reference<Other> &&other);

  /**
   * Checks if the Owner is still alive and its data can be borrowed.
   * @return true, if a borrow operation can be performed.
   */
  bool CanBorrow() const;

  /**
   * Try to borrow from the Owner. Depending on this operation's success, a
   * valid Borrower will be returned or nothing.
   * @return a valid Borrower or nothing, if the borrow operation failed.
   */
  std::optional<Borrower<T>> TryBorrow();

  /**
   * Forces the Reference to borrow. Obviously, this operation might fail if the
   * Owner is not alive anymore.
   * @throws BorrowFailedException if Owner is not alive anymore or shared state
   * is invalid.
   * @return Borrower if Owner is still alive and its shared state is valid.
   */
  Borrower<T> Borrow();

  explicit operator bool() const;

  std::shared_ptr<OwnershipState> GetState() const;
};

template <typename T> inline Borrower<T> Reference<T>::Borrow() {
  return PerformBorrow(false);
}

template <typename T>
inline std::shared_ptr<OwnershipState> Reference<T>::GetState() const {
  return _shared_ownership_state;
}

template <typename T>
template <typename Other>
Reference<T>::Reference(Reference<Other> &other)
    : _shared_ownership_state(other.GetState()) {
  static_assert(std::is_base_of<T, Other>());
}

template <typename T>
template <typename Other>
Reference<T>::Reference(Reference<Other> &&other)
    : _shared_ownership_state(std::move(other.GetState())) {
  static_assert(std::is_base_of<T, Other>());
}

template <typename T>
Reference<T>::Reference() : _shared_ownership_state(nullptr) {}

template <typename T> inline Reference<T>::operator bool() const {
  return CanBorrow();
}

template <typename T>
Reference<T>::Reference(Owner<T> *owner)
    : _shared_ownership_state(owner->_state) {
  DEBUG_ASSERT(owner != nullptr, "cannot create reference from null borrow")
  DEBUG_ASSERT(_shared_ownership_state, "shared state is broken")
  DEBUG_ASSERT(_shared_ownership_state->alive.load(),
               "cannot create reference to dead owner")
  DEBUG_ASSERT(_shared_ownership_state->owner != nullptr,
               "owner cannot be nullptr")
}

template <typename T>
Reference<T>::Reference(Borrower<T> *borrow)
    : _shared_ownership_state(borrow->_shared_ownership_state) {
  DEBUG_ASSERT(borrow != nullptr, "cannot create reference from null borrow")
  DEBUG_ASSERT(_shared_ownership_state, "shared state is broken")
  DEBUG_ASSERT(_shared_ownership_state->alive.load(),
               "cannot create reference to dead owner")
  DEBUG_ASSERT(_shared_ownership_state->owner != nullptr,
               "owner cannot be nullptr")
}

template <typename T> std::optional<Borrower<T>> Reference<T>::TryBorrow() {
  if (_shared_ownership_state) {
    common::sync::ScopedLock lock(_shared_ownership_state->state_lock);
    DEBUG_ASSERT(_shared_ownership_state->owner != nullptr,
                 "owner cannot be nullptr")
    if (_shared_ownership_state->alive.load()) {
      return PerformBorrow(true);
    }
  }

  return {};
}

template <typename T>
Borrower<T> Reference<T>::PerformBorrow(bool already_locked) {
  if (_shared_ownership_state) {
    std::unique_ptr<common::sync::ScopedLock<common::sync::SpinLock>> lock;
    if (!already_locked) {
      lock = std::make_unique<common::sync::ScopedLock<common::sync::SpinLock>>(
          _shared_ownership_state->state_lock);
    }
    if (CanBorrow()) {
      DEBUG_ASSERT(_shared_ownership_state, "shared state is broken")
      DEBUG_ASSERT(_shared_ownership_state->owner != nullptr,
                   "owner cannot be nullptr")
      auto owner = (Owner<T> *)_shared_ownership_state->owner.load();
      return owner->PerformBorrow(true);
    }
  }

  THROW_EXCEPTION(BorrowFailedException,
                  "owner is not alive or reference in an invalid state")
}

template <typename T> bool Reference<T>::CanBorrow() const {
  if (_shared_ownership_state) {
    DEBUG_ASSERT(_shared_ownership_state->owner != nullptr,
                 "owner cannot be nullptr")
    return _shared_ownership_state->alive.load();
  } else {
    return false;
  };
}

/**
 * The equivalent of std::enable_shared_from_this of std::shared_ptr, but for
 * the exclusive ownership model. It enables classes to borrow from themselves
 * if they are contained inside an Owner.
 *
 * @tparam T type of the owned data
 */
template <typename T> class EnableBorrowFromThis {
private:
  std::unique_ptr<Reference<T>> _this_reference{nullptr};

protected:
  /**
   * Checks if the class is currently handled by an Owner and can thus be
   * borrowed from.
   * @return true, if borrowing is possible for this instance.
   */
  bool HasOwner();

  /**
   * Creates a  from the current Owner.
   * @return newly created borrow
   */
  Borrower<T> BorrowFromThis();

  /**
   * Create a Reference to the current Owner.
   * @return newly created reference
   */
  Reference<T> ReferenceFromThis();

public:
  EnableBorrowFromThis() = default;

  void SetOwnerOfThis(Owner<T> *owner);
};

template <typename T>
Reference<T> EnableBorrowFromThis<T>::ReferenceFromThis() {
  DEBUG_ASSERT(_this_reference && _this_reference != nullptr,
               "cannot create reference because this is currently not owned")
  return *_this_reference;
}

template <typename T>
void EnableBorrowFromThis<T>::SetOwnerOfThis(Owner<T> *owner) {
  DEBUG_ASSERT(owner != nullptr, "owner cannot be nullptr")
  _this_reference =
      std::make_unique<Reference<T>>(owner->CreateReference(true));
}

template <typename T> Borrower<T> EnableBorrowFromThis<T>::BorrowFromThis() {
  DEBUG_ASSERT(_this_reference,
               "cannot borrow because there is no owner of this")
  if (auto maybe_borrower = _this_reference->TryBorrow()) {
    return maybe_borrower.value();
  } else /* if borrow operation failed */ {
    THROW_EXCEPTION(
        BorrowFailedException,
        "cannot borrow from this; are you sure this is currently owned?")
  }
}

template <typename T> inline bool EnableBorrowFromThis<T>::HasOwner() {
  if (_this_reference) {
    return _this_reference->CanBorrow();
  } else {
    return false;
  }
}

/**
 * Takes exclusive ownership over some data and only allows access to it through
 * borrowing. A owner cannot be destroyed while any of its borrows are still
 * alive: The owner's destructor is deferred/blocked while borrowers are
 * accessing its data. In essence, a borrow cannot outlive its owner. In
 * contrast to std::shared_ptr and std::weak_ptr, this a more exclusive memory
 * ownership model.
 *
 * @note This is useful in multi-threaded applications when a certain thread
 * owns a resource (like a thread pool) and must be responsible for cleaning it
 * up (i.e. joining its spawned threads). If the thread pool is joined by some
 * other thread because its ownership is not exclusive and can be transferred to
 * the "last user" (e.g. using std::shared_ptr and std::weak_ptr), errors or
 * unwanted behavior can occur.
 *
 * @tparam T type of owned object
 */
template <typename T> class Owner {
protected:
  friend Borrower<T>;
  friend Reference<T>;

  std::unique_ptr<T> _owned;
  std::shared_ptr<OwnershipState> _state;

  Borrower<T> PerformBorrow(bool already_locked);

public:
  Owner(const Owner &) = delete;
  Owner(Owner &) = delete;

  template <typename... Params> explicit Owner(Params... params);

  Owner(Owner &&other);
  explicit Owner(T &&owned);

  template <typename Other> Owner(Owner<Other> &&other);

  Borrower<T> Borrow() { return PerformBorrow(false); }

  Reference<T> CreateReference(bool already_locked = false) {

    // if there is no lock, lock it.
    std::unique_ptr<common::sync::ScopedLock<common::sync::SpinLock>> lock;
    if (!already_locked) {
      lock = std::make_unique<common::sync::ScopedLock<common::sync::SpinLock>>(
          _state->state_lock);
    }

    DEBUG_ASSERT(_state, "shared state is broken")
    DEBUG_ASSERT(_state->alive, "cannot borrow from a destroyed owner")
    DEBUG_ASSERT(_state->owner != nullptr, "owner cannot be a nullptr")
    return Reference<T>(this);
  }

  /**
   * Blocks until all Borrows have been destroyed.
   */
  ~Owner();

  T *operator->() const;
  T &operator*() const { return *this->operator->(); }
  Owner<T> &operator=(Owner<T> &&) = delete;

  std::shared_ptr<OwnershipState> &GetState();
  std::unique_ptr<T> &GetPointer();
};

template <typename T> inline T *Owner<T>::operator->() const {
  DEBUG_ASSERT(_state, "cannot access data because owner is invalid")
  DEBUG_ASSERT(_state->alive,
               "cannot access data because owner is already dead")
  return _owned.get();
}

template <typename T> Borrower<T> Owner<T>::PerformBorrow(bool already_locked) {
  // if there is no lock, lock it.
  std::unique_ptr<common::sync::ScopedLock<common::sync::SpinLock>> lock;
  if (!already_locked) {
    lock = std::make_unique<common::sync::ScopedLock<common::sync::SpinLock>>(
        _state->state_lock);
  }

  DEBUG_ASSERT(_state, "shared state is broken")
  DEBUG_ASSERT(_state->alive, "cannot borrow from a destroyed owner")
  DEBUG_ASSERT(_state->owner != nullptr, "owner cannot be a nullptr")
  return Borrower<T>(this, true);
}

template <typename T> std::unique_ptr<T> &Owner<T>::GetPointer() {
  return _owned;
}

template <typename T>
inline std::shared_ptr<OwnershipState> &Owner<T>::GetState() {
  return _state;
}

template <typename T>
Owner<T>::Owner(T &&owned)
    : _owned(std::make_unique<T>(owned)),
      _state(std::make_shared<OwnershipState>()) {
  common::sync::ScopedLock lock(_state->state_lock);
  _state->owner.store(this);

  if constexpr (std::is_base_of<EnableBorrowFromThis<T>, T>()) {
    _owned->SetOwnerOfThis(this);
  }
}

template <typename T>
template <typename Other>
Owner<T>::Owner(Owner<Other> &&other) {
  static_assert(std::is_base_of<T, Other>());
  common::sync::ScopedLock lock(other.GetState()->state_lock);
  _state = std::move(other.GetState());
  _owned = std::move(other.GetPointer());

  DEBUG_ASSERT(_state, "shared state is broken")
  DEBUG_ASSERT(_owned, "owned object should not be null")
  DEBUG_ASSERT(_state->alive, "cannot move dead owner")

  DEBUG_ASSERT(!other.GetState(),
               "other owner should be without state after move")
  DEBUG_ASSERT(!other.GetPointer(),
               "other owner should be without owned after move")

  _state->owner.store(this);

  if constexpr (std::is_base_of<EnableBorrowFromThis<Other>, T>()) {
    _owned->SetOwnerOfThis(this);
  }
}

template <typename T> Owner<T>::Owner(Owner<T> &&other) {
  common::sync::ScopedLock lock(other._state->state_lock);
  _state = std::move(other._state);
  _owned = std::move(other._owned);

  DEBUG_ASSERT(_state, "shared state is broken")
  DEBUG_ASSERT(_owned, "owned object should not be null")
  DEBUG_ASSERT(_state->alive, "cannot move dead owner")

  DEBUG_ASSERT(!other._state, "other owner should be without state after move")
  DEBUG_ASSERT(!other._owned, "other owner should be without owned after move")

  _state->owner.store(this);

  if constexpr (std::is_base_of<EnableBorrowFromThis<T>, T>()) {
    _owned->SetOwnerOfThis(this);
  }
}

template <typename T>
template <typename... Params>
Owner<T>::Owner(Params... params)
    : _state(std::make_shared<OwnershipState>()),
      _owned(std::make_unique<T>(std::forward<Params>(params)...)) {
  _state->owner.store(this);

  if constexpr (std::is_base_of<EnableBorrowFromThis<T>, T>()) {
    _owned->SetOwnerOfThis(this);
  }
}

template <typename T> Owner<T>::~Owner() {
  if (_state) {
    DEBUG_ASSERT(_state->alive, "owner should not be dead at this point")
    DEBUG_ASSERT(_state->owner != nullptr, "owner cannot be nullptr")

    while (_state->borrows > 0) {
      std::this_thread::yield();
    }

    _state->alive.store(false);
  }
}

} // namespace common::memory
