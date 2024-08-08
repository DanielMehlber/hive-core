#pragma once

#include <boost/chrono/duration.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <chrono>

namespace hive::services {

using namespace std::chrono_literals;

/**
 * Describes the behavior of a caller when the called service executor is busy.
 */
struct ExecutorBusyBehavior {
  /** maximum amount of retries before simply returning the busy response */
  size_t max_retries;

  /** time to wait before attempting another retry */
  std::chrono::nanoseconds retry_interval;

  /** retry using the next executor in line (if there is one) */
  bool try_next_executor{false};
};

/**
 * Does not retry and directly returns the response.
 */
constexpr ExecutorBusyBehavior BUSY_RESPONSE_RETURN{
    .max_retries = 0, .retry_interval = 0s, .try_next_executor = false};

/**
 * Continues immediately with the next executor in line for another 3 times
 * before returning the busy response.
 */
constexpr ExecutorBusyBehavior BUSY_RESPONSE_NEXT{
    .max_retries = 3,
    .retry_interval = std::chrono::milliseconds(0),
    .try_next_executor = true};

/**
 * Retries the same executor 3 times before returning the busy response.
 */
constexpr ExecutorBusyBehavior BUSY_RESPONSE_RETRY_SAME{
    .max_retries = 3,
    .retry_interval = std::chrono::milliseconds(1000),
    .try_next_executor = false};

} // namespace hive::services
