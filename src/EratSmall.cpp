///
/// @file   EratSmall.cpp
/// @brief  EratSmall is a segmented sieve of Eratosthenes
///         implementation optimized for small sieving primes. Since
///         each small sieving prime has many multiple occurrences per
///         segment the initialization overhead of the sieving primes
///         at the beginning of each segment is not really important
///         for performance. What matters is that crossing off
///         multiples uses as few instructions as possible since there
///         are so many multiples.
///
/// Copyright (C) 2020 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve/EratSmall.hpp>
#include <primesieve/bits.hpp>
#include <primesieve/Bucket.hpp>
#include <primesieve/CpuInfo.hpp>
#include <primesieve/pmath.hpp>
#include <primesieve/primesieve_error.hpp>
#include <primesieve/Wheel.hpp>
#include <primesieve/unlikely.hpp>

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <vector>

/// Update the current sieving prime's multipleIndex
/// and wheelIndex after sieving has finished.
///
#define CHECK_FINISHED(wheelIndex) \
  if_unlikely(p >= sieveEnd) \
  { \
    multipleIndex = (uint64_t) (p - sieveEnd); \
    prime.set(multipleIndex, wheelIndex); \
    break; \
  }

namespace primesieve {

/// @stop:        Upper bound for sieving
/// @l1CacheSize: CPU L1 cache size
/// @maxPrime:    Sieving primes <= maxPrime
///
void EratSmall::init(uint64_t stop, uint64_t l1CacheSize, uint64_t maxPrime)
{
  if (maxPrime > l1CacheSize * 3)
    throw primesieve_error("EratSmall: maxPrime > l1CacheSize * 3");

  enabled_ = true;
  maxPrime_ = maxPrime;
  l1CacheSize_ = l1CacheSize;
  Wheel::init(stop, l1CacheSize);

  size_t count = primeCountApprox(maxPrime);
  primes_.reserve(count);
}

/// Add a new sieving prime to EratSmall
void EratSmall::storeSievingPrime(uint64_t prime, uint64_t multipleIndex, uint64_t wheelIndex)
{
  assert(prime <= maxPrime_);
  uint64_t sievingPrime = prime / 30;
  primes_.emplace_back(sievingPrime, multipleIndex, wheelIndex);
}

/// Use the CPU's L1 cache size as
/// sieveSize in EratSmall.
///
uint64_t EratSmall::getL1CacheSize(uint64_t sieveSize)
{
  if (!cpuInfo.hasL1Cache())
    return sieveSize;

  uint64_t size = cpuInfo.l1CacheSize();
  uint64_t minSize = 8 << 10;
  uint64_t maxSize = 4096 << 10;

  size = std::min(size, sieveSize);
  size = inBetween(minSize, size, maxSize);

  return size;
}

/// Both EratMedium and EratBig run fastest using a sieve size
/// that matches the CPU's L2 cache size (or slightly less).
/// However, proportionally EratSmall does a lot more memory
/// writes than both EratMedium and EratBig and hence EratSmall
/// runs fastest using a smaller sieve size that matches the
/// CPU's L1 cache size.
///
/// @sieveSize:   CPU L2 cache size / 2
/// @l1CacheSize: CPU L1 cache size
///
void EratSmall::crossOff(uint8_t* sieve, uint64_t sieveSize)
{
  for (uint64_t i = 0; i < sieveSize; i += l1CacheSize_)
  {
    uint64_t end = i + l1CacheSize_;
    end = std::min(end, sieveSize);
    crossOff(&sieve[i], &sieve[end]);
  }
}

/// Segmented sieve of Eratosthenes with wheel factorization
/// optimized for small sieving primes that have many multiples
/// per segment. This algorithm uses a hardcoded modulo 30
/// wheel that skips multiples of 2, 3 and 5.
///
void EratSmall::crossOff(uint8_t* sieve, uint8_t* sieveEnd)
{
  for (auto& prime : primes_)
  {
    uint64_t sievingPrime = prime.getSievingPrime();
    uint64_t multipleIndex = prime.getMultipleIndex();
    uint64_t wheelIndex = prime.getWheelIndex();
    uint64_t maxLoopDist = sievingPrime * 28 + 27;
    uint8_t* loopEnd = std::max(sieveEnd, sieve + maxLoopDist) - maxLoopDist;

    // Pointer to the byte containing the first multiple of
    // sievingPrime within the current segment.
    uint8_t* p = &sieve[multipleIndex];

    // By using switch(n & 63) we let the compiler know that
    // n is always within [0, 63]. This trick reduces the
    // number of instructions as the compiler now does not
    // add a check if (n > 63) which skips to the end of the
    // switch statement.
    switch (wheelIndex & 63)
    {
      // sievingPrime % 30 == 7
      for (;;)
      {
        case 0: // Each iteration removes the next 8
                // multiples of the sievingPrime.
                for (; p < loopEnd; p += sievingPrime * 30 + 7)
                {
                  p[sievingPrime *  0 + 0] &= BIT0;
                  p[sievingPrime *  6 + 1] &= BIT4;
                  p[sievingPrime * 10 + 2] &= BIT3;
                  p[sievingPrime * 12 + 2] &= BIT7;
                  p[sievingPrime * 16 + 3] &= BIT6;
                  p[sievingPrime * 18 + 4] &= BIT2;
                  p[sievingPrime * 22 + 5] &= BIT1;
                  p[sievingPrime * 28 + 6] &= BIT5;
                }
                CHECK_FINISHED(0);
                *p &= BIT0; p += sievingPrime * 6 + 1;
        case 1: CHECK_FINISHED(1);
                *p &= BIT4; p += sievingPrime * 4 + 1;
        case 2: CHECK_FINISHED(2);
                *p &= BIT3; p += sievingPrime * 2 + 0;
        case 3: CHECK_FINISHED(3);
                *p &= BIT7; p += sievingPrime * 4 + 1;
        case 4: CHECK_FINISHED(4);
                *p &= BIT6; p += sievingPrime * 2 + 1;
        case 5: CHECK_FINISHED(5);
                *p &= BIT2; p += sievingPrime * 4 + 1;
        case 6: CHECK_FINISHED(6);
                *p &= BIT1; p += sievingPrime * 6 + 1;
        case 7: CHECK_FINISHED(7);
                *p &= BIT5; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 11
      for (;;)
      {
        case  8: for (; p < loopEnd; p += sievingPrime * 30 + 11)
                 {
                   p[sievingPrime *  0 +  0] &= BIT1;
                   p[sievingPrime *  6 +  2] &= BIT3;
                   p[sievingPrime * 10 +  3] &= BIT7;
                   p[sievingPrime * 12 +  4] &= BIT5;
                   p[sievingPrime * 16 +  6] &= BIT0;
                   p[sievingPrime * 18 +  6] &= BIT6;
                   p[sievingPrime * 22 +  8] &= BIT2;
                   p[sievingPrime * 28 + 10] &= BIT4;
                 }
                 CHECK_FINISHED(8);
                 *p &= BIT1; p += sievingPrime * 6 + 2;
        case  9: CHECK_FINISHED(9);
                 *p &= BIT3; p += sievingPrime * 4 + 1;
        case 10: CHECK_FINISHED(10);
                 *p &= BIT7; p += sievingPrime * 2 + 1;
        case 11: CHECK_FINISHED(11);
                 *p &= BIT5; p += sievingPrime * 4 + 2;
        case 12: CHECK_FINISHED(12);
                 *p &= BIT0; p += sievingPrime * 2 + 0;
        case 13: CHECK_FINISHED(13);
                 *p &= BIT6; p += sievingPrime * 4 + 2;
        case 14: CHECK_FINISHED(14);
                 *p &= BIT2; p += sievingPrime * 6 + 2;
        case 15: CHECK_FINISHED(15);
                 *p &= BIT4; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 13
      for (;;)
      {
        case 16: for (; p < loopEnd; p += sievingPrime * 30 + 13)
                 {
                   p[sievingPrime *  0 +  0] &= BIT2;
                   p[sievingPrime *  6 +  2] &= BIT7;
                   p[sievingPrime * 10 +  4] &= BIT5;
                   p[sievingPrime * 12 +  5] &= BIT4;
                   p[sievingPrime * 16 +  7] &= BIT1;
                   p[sievingPrime * 18 +  8] &= BIT0;
                   p[sievingPrime * 22 +  9] &= BIT6;
                   p[sievingPrime * 28 + 12] &= BIT3;
                 }
                 CHECK_FINISHED(16);
                 *p &= BIT2; p += sievingPrime * 6 + 2;
        case 17: CHECK_FINISHED(17);
                 *p &= BIT7; p += sievingPrime * 4 + 2;
        case 18: CHECK_FINISHED(18);
                 *p &= BIT5; p += sievingPrime * 2 + 1;
        case 19: CHECK_FINISHED(19);
                 *p &= BIT4; p += sievingPrime * 4 + 2;
        case 20: CHECK_FINISHED(20);
                 *p &= BIT1; p += sievingPrime * 2 + 1;
        case 21: CHECK_FINISHED(21);
                 *p &= BIT0; p += sievingPrime * 4 + 1;
        case 22: CHECK_FINISHED(22);
                 *p &= BIT6; p += sievingPrime * 6 + 3;
        case 23: CHECK_FINISHED(23);
                 *p &= BIT3; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 17
      for (;;)
      {
        case 24: for (; p < loopEnd; p += sievingPrime * 30 + 17)
                 {
                   p[sievingPrime *  0 +  0] &= BIT3;
                   p[sievingPrime *  6 +  3] &= BIT6;
                   p[sievingPrime * 10 +  6] &= BIT0;
                   p[sievingPrime * 12 +  7] &= BIT1;
                   p[sievingPrime * 16 +  9] &= BIT4;
                   p[sievingPrime * 18 + 10] &= BIT5;
                   p[sievingPrime * 22 + 12] &= BIT7;
                   p[sievingPrime * 28 + 16] &= BIT2;
                 }
                 CHECK_FINISHED(24);
                 *p &= BIT3; p += sievingPrime * 6 + 3;
        case 25: CHECK_FINISHED(25);
                 *p &= BIT6; p += sievingPrime * 4 + 3;
        case 26: CHECK_FINISHED(26);
                 *p &= BIT0; p += sievingPrime * 2 + 1;
        case 27: CHECK_FINISHED(27);
                 *p &= BIT1; p += sievingPrime * 4 + 2;
        case 28: CHECK_FINISHED(28);
                 *p &= BIT4; p += sievingPrime * 2 + 1;
        case 29: CHECK_FINISHED(29);
                 *p &= BIT5; p += sievingPrime * 4 + 2;
        case 30: CHECK_FINISHED(30);
                 *p &= BIT7; p += sievingPrime * 6 + 4;
        case 31: CHECK_FINISHED(31);
                 *p &= BIT2; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 19
      for (;;)
      {
        case 32: for (; p < loopEnd; p += sievingPrime * 30 + 19)
                 {
                   p[sievingPrime *  0 +  0] &= BIT4;
                   p[sievingPrime *  6 +  4] &= BIT2;
                   p[sievingPrime * 10 +  6] &= BIT6;
                   p[sievingPrime * 12 +  8] &= BIT0;
                   p[sievingPrime * 16 + 10] &= BIT5;
                   p[sievingPrime * 18 + 11] &= BIT7;
                   p[sievingPrime * 22 + 14] &= BIT3;
                   p[sievingPrime * 28 + 18] &= BIT1;
                 }
                 CHECK_FINISHED(32);
                 *p &= BIT4; p += sievingPrime * 6 + 4;
        case 33: CHECK_FINISHED(33);
                 *p &= BIT2; p += sievingPrime * 4 + 2;
        case 34: CHECK_FINISHED(34);
                 *p &= BIT6; p += sievingPrime * 2 + 2;
        case 35: CHECK_FINISHED(35);
                 *p &= BIT0; p += sievingPrime * 4 + 2;
        case 36: CHECK_FINISHED(36);
                 *p &= BIT5; p += sievingPrime * 2 + 1;
        case 37: CHECK_FINISHED(37);
                 *p &= BIT7; p += sievingPrime * 4 + 3;
        case 38: CHECK_FINISHED(38);
                 *p &= BIT3; p += sievingPrime * 6 + 4;
        case 39: CHECK_FINISHED(39);
                 *p &= BIT1; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 23
      for (;;)
      {
        case 40: for (; p < loopEnd; p += sievingPrime * 30 + 23)
                 {
                   p[sievingPrime *  0 +  0] &= BIT5;
                   p[sievingPrime *  6 +  5] &= BIT1;
                   p[sievingPrime * 10 +  8] &= BIT2;
                   p[sievingPrime * 12 +  9] &= BIT6;
                   p[sievingPrime * 16 + 12] &= BIT7;
                   p[sievingPrime * 18 + 14] &= BIT3;
                   p[sievingPrime * 22 + 17] &= BIT4;
                   p[sievingPrime * 28 + 22] &= BIT0;
                 }
                 CHECK_FINISHED(40);
                 *p &= BIT5; p += sievingPrime * 6 + 5;
        case 41: CHECK_FINISHED(41);
                 *p &= BIT1; p += sievingPrime * 4 + 3;
        case 42: CHECK_FINISHED(42);
                 *p &= BIT2; p += sievingPrime * 2 + 1;
        case 43: CHECK_FINISHED(43);
                 *p &= BIT6; p += sievingPrime * 4 + 3;
        case 44: CHECK_FINISHED(44);
                 *p &= BIT7; p += sievingPrime * 2 + 2;
        case 45: CHECK_FINISHED(45);
                 *p &= BIT3; p += sievingPrime * 4 + 3;
        case 46: CHECK_FINISHED(46);
                 *p &= BIT4; p += sievingPrime * 6 + 5;
        case 47: CHECK_FINISHED(47);
                 *p &= BIT0; p += sievingPrime * 2 + 1;
      }
      break;

      // sievingPrime % 30 == 29
      for (;;)
      {
        case 48: for (; p < loopEnd; p += sievingPrime * 30 + 29)
                 {
                   p[sievingPrime *  0 +  0] &= BIT6;
                   p[sievingPrime *  6 +  6] &= BIT5;
                   p[sievingPrime * 10 + 10] &= BIT4;
                   p[sievingPrime * 12 + 12] &= BIT3;
                   p[sievingPrime * 16 + 16] &= BIT2;
                   p[sievingPrime * 18 + 18] &= BIT1;
                   p[sievingPrime * 22 + 22] &= BIT0;
                   p[sievingPrime * 28 + 27] &= BIT7;
                 }
                 CHECK_FINISHED(48);
                 *p &= BIT6; p += sievingPrime * 6 + 6;
        case 49: CHECK_FINISHED(49);
                 *p &= BIT5; p += sievingPrime * 4 + 4;
        case 50: CHECK_FINISHED(50);
                 *p &= BIT4; p += sievingPrime * 2 + 2;
        case 51: CHECK_FINISHED(51);
                 *p &= BIT3; p += sievingPrime * 4 + 4;
        case 52: CHECK_FINISHED(52);
                 *p &= BIT2; p += sievingPrime * 2 + 2;
        case 53: CHECK_FINISHED(53);
                 *p &= BIT1; p += sievingPrime * 4 + 4;
        case 54: CHECK_FINISHED(54);
                 *p &= BIT0; p += sievingPrime * 6 + 5;
        case 55: CHECK_FINISHED(55);
                 *p &= BIT7; p += sievingPrime * 2 + 2;
      }
      break;

      // sievingPrime % 30 == 1
      for (;;)
      {
        case 56: for (; p < loopEnd; p += sievingPrime * 30 + 1)
                 {
                   p[sievingPrime *  0 + 0] &= BIT7;
                   p[sievingPrime *  6 + 1] &= BIT0;
                   p[sievingPrime * 10 + 1] &= BIT1;
                   p[sievingPrime * 12 + 1] &= BIT2;
                   p[sievingPrime * 16 + 1] &= BIT3;
                   p[sievingPrime * 18 + 1] &= BIT4;
                   p[sievingPrime * 22 + 1] &= BIT5;
                   p[sievingPrime * 28 + 1] &= BIT6;
                 }
                 CHECK_FINISHED(56);
                 *p &= BIT7; p += sievingPrime * 6 + 1;
        case 57: CHECK_FINISHED(57);
                 *p &= BIT0; p += sievingPrime * 4 + 0;
        case 58: CHECK_FINISHED(58);
                 *p &= BIT1; p += sievingPrime * 2 + 0;
        case 59: CHECK_FINISHED(59);
                 *p &= BIT2; p += sievingPrime * 4 + 0;
        case 60: CHECK_FINISHED(60);
                 *p &= BIT3; p += sievingPrime * 2 + 0;
        case 61: CHECK_FINISHED(61);
                 *p &= BIT4; p += sievingPrime * 4 + 0;
        case 62: CHECK_FINISHED(62);
                 *p &= BIT5; p += sievingPrime * 6 + 0;
        case 63: CHECK_FINISHED(63);
                 *p &= BIT6; p += sievingPrime * 2 + 0;
      }
    }
  }
}

} // namespace
