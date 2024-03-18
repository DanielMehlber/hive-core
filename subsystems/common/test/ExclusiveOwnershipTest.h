#ifndef SIMULATION_FRAMEWORK_EXCLUSIVEOWNERSHIPTEST_H
#define SIMULATION_FRAMEWORK_EXCLUSIVEOWNERSHIPTEST_H

#include "common/memory/ExclusiveOwnership.h"
#include <chrono>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(ExclusiveOwnershipTests, owner_borrow) {
  common::memory::Owner<std::string> owner("hallo");

  auto start_time = std::chrono::high_resolution_clock::now();

  std::thread borrow_thread([borrow = owner.Borrow()]() mutable {
    std::string value = *borrow;
    ASSERT_EQ(value, "hallo");
    borrow->clear();
    ASSERT_TRUE(borrow->empty());
    std::this_thread::sleep_for(0.2s);
  });

  std::thread owner_thread([_owner = std::move(owner)]() mutable {
    // owner will go out of scope here
  });

  owner_thread.join();

  auto duration = std::chrono::high_resolution_clock::now() - start_time;

  borrow_thread.join();

  ASSERT_GE(duration, 0.2s);
}

TEST(ExclusiveOwnershipTests, weak_borrow) {
  std::unique_ptr<common::memory::Reference<std::string>> weak_borrow;
  {
    common::memory::Owner<std::string> owner("hallo");
    weak_borrow = std::make_unique<common::memory::Reference<std::string>>(
        owner.CreateReference());
    ASSERT_TRUE(weak_borrow->CanBorrow());
    ASSERT_TRUE(weak_borrow->TryBorrow().has_value());
    ASSERT_NO_THROW(weak_borrow->Borrow());

    auto borrow = weak_borrow->Borrow();
    ASSERT_EQ(*borrow, "hallo");
    borrow->clear();
    ASSERT_TRUE(borrow->empty());
  }

  ASSERT_FALSE(weak_borrow->CanBorrow());
  ASSERT_FALSE(weak_borrow->TryBorrow().has_value());
  ASSERT_THROW(weak_borrow->Borrow(), common::memory::BorrowFailedException);
}

TEST(ExclusiveOwnershipTests, destroy_after_expiration) {
  auto shared_string = std::make_shared<std::string>("hallo");
  std::weak_ptr<std::string> weak_string = shared_string;

  {
    auto string_pointer = std::move(shared_string);
    auto string_owner = common::memory::Owner<std::shared_ptr<std::string>>(
        std::move(string_pointer));

    ASSERT_FALSE(weak_string.expired());
    ASSERT_EQ(std::string("hallo"), *weak_string.lock());
    ASSERT_EQ(std::string("hallo"), *(*string_owner));
  }

  ASSERT_TRUE(weak_string.expired());
}

class Borrowable : public common::memory::EnableBorrowFromThis<Borrowable> {
public:
  int _i;

  Borrowable() = delete;
  explicit Borrowable(int i) : _i(i){};
  bool CanBorrow() { return HasOwner(); }

  int TestBorrow() {
    auto borrow = BorrowFromThis();
    return borrow->_i;
  }
};

TEST(ExclusiveOwnershipTests, enable_borrow_from_this) {
  Borrowable borrowable(5);
  ASSERT_FALSE(borrowable.CanBorrow());

  common::memory::Owner<Borrowable> another_borrowable(10);
  ASSERT_TRUE(another_borrowable->CanBorrow());
  ASSERT_EQ(another_borrowable->TestBorrow(), 10);

  common::memory::Owner<Borrowable> owner(5);
  ASSERT_TRUE(owner->CanBorrow());
  ASSERT_EQ(owner->TestBorrow(), 5);

  auto start_time = std::chrono::high_resolution_clock::now();

  std::thread borrow_thread(
      [_borrow = owner.Borrow()] { std::this_thread::sleep_for(0.2s); });

  std::thread owner_thread([_owner = std::move(owner)] {
    ASSERT_TRUE(_owner->CanBorrow());
    ASSERT_EQ(_owner->TestBorrow(), 5);
  });

  owner_thread.join();
  auto duration = std::chrono::high_resolution_clock::now() - start_time;

  borrow_thread.join();

  ASSERT_GE(duration, 0.2s);
}

class Derived : public Borrowable {
public:
  explicit Derived(int a) : Borrowable(a) {}
};

TEST(ExclusiveOwnershipTests, base_conversion) {
  common::memory::Owner<Derived> derived(5);
  ASSERT_EQ(derived.Borrow()->_i, 5);

  common::memory::Owner<Borrowable> base(std::move(derived));
  ASSERT_EQ(base.Borrow()->_i, 5);
}

TEST(ExclusiveOwnershipTests, reference_state_validity) {
  common::memory::Reference<Borrowable> reference;
  ASSERT_FALSE(reference);

  {
    common::memory::Owner<Borrowable> owner(5);
    reference = owner.CreateReference();
    ASSERT_TRUE(reference);
  }

  ASSERT_FALSE(reference);
}

#endif // SIMULATION_FRAMEWORK_EXCLUSIVEOWNERSHIPTEST_H
