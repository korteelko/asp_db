/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef UTILS__THREADWRAP_H
#define UTILS__THREADWRAP_H

#include "Common.h"

#include <mutex>
#if defined(CXX17)
#  include <shared_mutex>
#endif  // CXX17


template <class T>
class MutexTemplate {
public:
  MutexTemplate() = default;
  MutexTemplate(const MutexTemplate &) {}
    // /*mutex_(T())*/ { (void)m; }

  T &Get() { return mutex_; }

  void lock() { mutex_.lock(); }
  void unlock() { mutex_.unlock(); }
  bool try_lock() { return mutex_.try_lock(); }

public:
  T mutex_;
};

/** \brief Обёртка над обычным мьютексом */
using Mutex = MutexTemplate<std::mutex>;
/** \brief Обёртка над рекурсивным мьютексом */
using RecursiveMutex = MutexTemplate<std::recursive_mutex>;
#if defined(CXX17)
/** \brief Обёртка над разделяемым мьютексом
  * \note В С++11 его нет, реализовать отвельным классом */
template<>
class MutexTemplate<std::shared_mutex> {
public:
  MutexTemplate() = default;
  MutexTemplate(const MutexTemplate &) {}
    // /*mutex_(T())*/ { (void)m; }

  std::shared_mutex &Get() { return mutex_; }

  void lock() { mutex_.lock(); }
  void unlock() { mutex_.unlock(); }
  bool try_lock() { return mutex_.try_lock(); }

  void lock_shared() { mutex_.lock_shared(); }
  void unlock_shared() { mutex_.unlock_shared(); }

public:
  std::shared_mutex mutex_;
};
using SharedMutex = MutexTemplate<std::shared_mutex>;
#endif  // CXX17

#endif  // !UTILS__THREADWRAP_H
