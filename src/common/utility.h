#pragma once

#include <Eigen/Eigen>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include "common/logging.h"

#define MY_PI 3.1415926f

// For 16-byte-aligned Eigen types under Win32
template <typename T>
using STLVectorOfEigenTypes = std::vector<T, Eigen::aligned_allocator<T>>;
template <typename T0, typename T1>
using STLMapOfEigenTypes =
    std::map<T0, T1, std::less<T0>,
             Eigen::aligned_allocator<std::pair<T0, T1>>>;
template <typename T>
using STLDequeOfEigenTypes = std::deque<T, Eigen::aligned_allocator<T>>;
template <typename T, typename... Args>
std::shared_ptr<T> STLMakeSharedOfEigenTypes(const Args&... args) {
  return std::allocate_shared<T, Eigen::aligned_allocator<T>, const Args&...>(
      Eigen::aligned_allocator<T>(), args...);
}