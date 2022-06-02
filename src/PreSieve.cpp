///
/// @file   PreSieve.cpp
/// @brief  Pre-sieve multiples of small primes < 100 to speed up the
///         sieve of Eratosthenes. The idea is to allocate several
///         arrays (buffers_) and remove the multiples of small primes
///         from them at initialization. Each buffer is assigned
///         different primes, for example:
///
///         Buffer 0 removes multiplies of: {  7, 67, 71 }
///         Buffer 1 removes multiplies of: { 11, 41, 73 }
///         Buffer 2 removes multiplies of: { 13, 43, 59 }
///         Buffer 3 removes multiplies of: { 17, 37, 53 }
///         Buffer 4 removes multiplies of: { 19, 29, 61 }
///         Buffer 5 removes multiplies of: { 23, 31, 47 }
///         Buffer 6 removes multiplies of: { 79, 97 }
///         Buffer 7 removes multiplies of: { 83, 89 }
///
///         Then whilst sieving, we perform a bitwise AND on the
///         buffers_ arrays and store the result in the sieve array.
///         Pre-sieving provides a speedup of up to 30% when
///         sieving the primes < 10^10 using primesieve.
///
/// Copyright (C) 2022 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve/PreSieve.hpp>
#include <primesieve/EratSmall.hpp>

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

using std::copy_n;
using primesieve::pod_array;

namespace {

/// Pre-sieve with the primes <= 13
const pod_array<uint8_t, 7*11*13> buffer_7_11_13 =
{
  0xf8, 0xef, 0x77, 0x3f, 0xdb, 0xed, 0x9e, 0xfc, 0xea, 0x37,
  0xaf, 0xf9, 0xf5, 0xd3, 0x7e, 0x4f, 0x77, 0x9e, 0xeb, 0xf9,
  0xdd, 0xee, 0xad, 0x77, 0xb7, 0x73, 0xd9, 0xdf, 0x3e, 0xef,
  0x53, 0xaf, 0xeb, 0xfd, 0xde, 0xb6, 0x6f, 0x57, 0xb7, 0xba,
  0xfd, 0x5b, 0xfe, 0xcf, 0x65, 0xbf, 0xf1, 0x7c, 0x9f, 0xfe,
  0xae, 0x77, 0xbb, 0xfb, 0x6d, 0xdd, 0xde, 0xe7, 0x77, 0x9d,
  0xfa, 0xbc, 0xdf, 0xfa, 0xe7, 0x63, 0xbd, 0x7b, 0xf5, 0x5f,
  0xce, 0xef, 0x34, 0xbe, 0xbb, 0xfd, 0xcf, 0xf4, 0xeb, 0x77,
  0x3f, 0xdb, 0xdd, 0x8e, 0xfe, 0xe9, 0x76, 0xaf, 0xf9, 0xfd,
  0xd7, 0x7a, 0xcf, 0x77, 0xbe, 0xdb, 0xe9, 0xdf, 0xec, 0xec,
  0x37, 0xb7, 0x7b, 0xd5, 0xdb, 0xbe, 0x6f, 0x73, 0x9f, 0xeb,
  0xfd, 0xdd, 0xf6, 0x2f, 0x57, 0xbf, 0xb2, 0xf9, 0xdb, 0x7e,
  0xef, 0x55, 0xaf, 0xf3, 0x7d, 0xde, 0xbe, 0xae, 0x77, 0xb3,
  0xfb, 0xed, 0x5d, 0xfe, 0xc7, 0x67, 0x9f, 0xf9, 0xbc, 0x9f,
  0xfa, 0xef, 0x67, 0xb9, 0xfb, 0x75, 0x5f, 0xde, 0xef, 0x36,
  0xbd, 0xfa, 0xbd, 0xcf, 0xfc, 0xe7, 0x73, 0x3f, 0x5b, 0xfd,
  0x9e, 0xee, 0xeb, 0x75, 0xae, 0xb9, 0xfd, 0xd7, 0x76, 0xcb,
  0x77, 0x3e, 0xfb, 0xd9, 0xcf, 0xee, 0xed, 0x76, 0xb7, 0x7b,
  0xdd, 0xd7, 0xba, 0xef, 0x73, 0xbf, 0xcb, 0xed, 0xdf, 0xf4,
  0x6e, 0x17, 0xbf, 0xba, 0xf5, 0xdb, 0xfe, 0x6f, 0x75, 0x9f,
  0xe3, 0x7d, 0xdd, 0xfe, 0xae, 0x77, 0xbb, 0xf3, 0xe9, 0xdd,
  0x7e, 0xe7, 0x57, 0x8f, 0xfb, 0xbc, 0xde, 0xba, 0xef, 0x67,
  0xb5, 0xfb, 0xf5, 0x5f, 0xde, 0xcf, 0x26, 0xbf, 0xf9, 0xfc,
  0x8f, 0xfc, 0xef, 0x77, 0x3b, 0xdb, 0x7d, 0x9e, 0xde, 0xeb,
  0x77, 0xad, 0xf8, 0xbd, 0xd7, 0x7e, 0xc7, 0x73, 0xbe, 0x7b,
  0xf9, 0xdf, 0xee, 0xed, 0x75, 0xb6, 0x3b, 0xdd, 0xdf, 0xb6,
  0xeb, 0x73, 0x3f, 0xeb, 0xdd, 0xcf, 0xf6, 0x6d, 0x56, 0xbf,
  0xba, 0xfd, 0xd3, 0xfa, 0xef, 0x75, 0xbf, 0xd3, 0x6d, 0xdf,
  0xfc, 0xae, 0x37, 0xbb, 0xfb, 0xe5, 0xd9, 0xfe, 0x67, 0x77,
  0x9f, 0xeb, 0xbc, 0xdd, 0xfa, 0xaf, 0x67, 0xbd, 0xf3, 0xf1,
  0x5f, 0x5e, 0xef, 0x16, 0xaf, 0xfb, 0xfd, 0xce, 0xbc, 0xef,
  0x77, 0x37, 0xdb, 0xfd, 0x1e, 0xfe, 0xcb, 0x67, 0xaf, 0xf9,
  0xfc, 0x97, 0x7e, 0xcf, 0x77, 0xba, 0xfb, 0x79, 0xdf, 0xce,
  0xed, 0x77, 0xb5, 0x7a, 0x9d, 0xdf, 0xbe, 0xe7, 0x73, 0xbf,
  0x6b, 0xfd, 0xdf, 0xe6, 0x6f, 0x55, 0xbe, 0xba, 0xfd, 0xdb,
  0xf6, 0xeb, 0x75, 0x3f, 0xf3, 0x5d, 0xcf, 0xfe, 0xac, 0x76,
  0xbb, 0xfb, 0xed, 0xd5, 0xfa, 0xe7, 0x77, 0x9f, 0xdb, 0xac,
  0xdf, 0xf8, 0xee, 0x27, 0xbd, 0xfb, 0xf5, 0x5b, 0xde, 0x6f,
  0x36, 0x9f, 0xeb, 0xfd, 0xcd, 0xfc, 0xaf, 0x77, 0x3f, 0xd3,
  0xf9, 0x9e, 0x7e, 0xeb, 0x57, 0xaf, 0xf9, 0xfd, 0xd6, 0x3e,
  0xcf, 0x77, 0xb6, 0xfb, 0xf9, 0x5f, 0xee, 0xcd, 0x67, 0xb7,
  0x79, 0xdc, 0x9f, 0xbe, 0xef, 0x73, 0xbb, 0xeb, 0x7d, 0xdf,
  0xd6, 0x6f, 0x57, 0xbd, 0xba, 0xbd, 0xdb, 0xfe, 0xe7, 0x71,
  0xbf, 0x73, 0x7d, 0xdf, 0xee, 0xae, 0x75, 0xba, 0xbb, 0xed,
  0xdd, 0xf6, 0xe3, 0x77, 0x1f, 0xfb, 0x9c, 0xcf, 0xfa, 0xed,
  0x66, 0xbd, 0xfb, 0xf5, 0x57, 0xda, 0xef, 0x36, 0xbf, 0xdb,
  0xed, 0xcf, 0xfc, 0xee, 0x37, 0x3f, 0xdb, 0xf5, 0x9a, 0xfe,
  0x6b, 0x77, 0x8f, 0xe9, 0xfd, 0xd5, 0x7e, 0x8f, 0x77, 0xbe,
  0xf3, 0xf9, 0xdf, 0x6e, 0xed, 0x57, 0xa7, 0x7b, 0xdd, 0xde,
  0xbe, 0xef, 0x73, 0xb7, 0xeb, 0xfd, 0x5f, 0xf6, 0x4f, 0x47,
  0xbf, 0xb8, 0xfc, 0x9b, 0xfe, 0xef, 0x75, 0xbb, 0xf3, 0x7d,
  0xdf, 0xde, 0xae, 0x77, 0xb9, 0xfa, 0xad, 0xdd, 0xfe, 0xe7,
  0x73, 0x9f, 0x7b, 0xbc, 0xdf, 0xea, 0xef, 0x65, 0xbc, 0xbb,
  0xf5, 0x5f, 0xd6, 0xeb, 0x36, 0x3f, 0xfb, 0xdd, 0xcf, 0xfc,
  0xed, 0x76, 0x3f, 0xdb, 0xfd, 0x96, 0xfa, 0xeb, 0x77, 0xaf,
  0xd9, 0xed, 0xd7, 0x7c, 0xce, 0x37, 0xbe, 0xfb, 0xf1, 0xdb,
  0xee, 0x6d, 0x77, 0x97, 0x6b, 0xdd, 0xdd, 0xbe, 0xaf, 0x73,
  0xbf, 0xe3, 0xf9, 0xdf, 0x76, 0x6f, 0x57, 0xaf, 0xba, 0xfd,
  0xda, 0xbe, 0xef, 0x75, 0xb7, 0xf3, 0x7d, 0x5f, 0xfe, 0x8e,
  0x67, 0xbb, 0xf9, 0xec, 0x9d, 0xfe, 0xe7, 0x77, 0x9b, 0xfb,
  0x3c, 0xdf, 0xda, 0xef, 0x67, 0xbd, 0xfa, 0xb5, 0x5f, 0xde,
  0xe7, 0x32, 0xbf, 0x7b, 0xfd, 0xcf, 0xec, 0xef, 0x75, 0x3e,
  0x9b, 0xfd, 0x9e, 0xf6, 0xeb, 0x77, 0x2f, 0xf9, 0xdd, 0xc7,
  0x7e, 0xcd, 0x76, 0xbe, 0xfb, 0xf9, 0xd7, 0xea, 0xed, 0x77,
  0xb7, 0x5b, 0xcd, 0xdf, 0xbc, 0xee, 0x33, 0xbf, 0xeb, 0xf5,
  0xdb, 0xf6, 0x6f, 0x57, 0x9f, 0xaa, 0xfd, 0xd9, 0xfe, 0xaf,
  0x75, 0xbf, 0xf3, 0x79, 0xdf, 0x7e, 0xae, 0x57, 0xab, 0xfb,
  0xed, 0xdc, 0xbe, 0xe7, 0x77, 0x97, 0xfb, 0xbc, 0x5f, 0xfa,
  0xcf, 0x67, 0xbd, 0xf9, 0xf4, 0x1f, 0xde, 0xef, 0x36, 0xbb,
  0xfb, 0x7d, 0xcf, 0xdc, 0xef, 0x77, 0x3d, 0xda, 0xbd, 0x9e,
  0xfe, 0xe3, 0x73, 0xaf, 0x79, 0xfd, 0xd7, 0x6e, 0xcf, 0x75,
  0xbe, 0xbb, 0xf9, 0xdf, 0xe6, 0xe9, 0x77, 0x37, 0x7b, 0xdd,
  0xcf, 0xbe, 0xed, 0x72, 0xbf, 0xeb, 0xfd, 0xd7, 0xf2, 0x6f,
  0x57, 0xbf, 0x9a, 0xed, 0xdb, 0xfc, 0xee, 0x35, 0xbf, 0xf3,
  0x75, 0xdb, 0xfe, 0x2e, 0x77, 0x9b, 0xeb, 0xed, 0xdd, 0xfe,
  0xa7, 0x77, 0x9f, 0xf3, 0xb8, 0xdf, 0x7a, 0xef, 0x47, 0xad,
  0xfb, 0xf5, 0x5e, 0x9e, 0xef, 0x36, 0xb7, 0xfb, 0xfd, 0x4f,
  0xfc, 0xcf, 0x67, 0x3f, 0xd9, 0xfc, 0x9e, 0xfe, 0xeb, 0x77,
  0xab, 0xf9, 0x7d, 0xd7, 0x5e, 0xcf, 0x77, 0xbc, 0xfa, 0xb9,
  0xdf, 0xee, 0xe5, 0x73, 0xb7, 0x7b, 0xdd, 0xdf, 0xae, 0xef,
  0x71, 0xbe, 0xab, 0xfd, 0xdf, 0xf6, 0x6b, 0x57, 0x3f, 0xba,
  0xdd, 0xcb, 0xfe, 0xed, 0x74, 0xbf, 0xf3, 0x7d, 0xd7, 0xfa,
  0xae, 0x77, 0xbb, 0xdb, 0xed, 0xdd, 0xfc, 0xe6, 0x37, 0x9f,
  0xfb, 0xb4, 0xdb, 0xfa, 0x6f, 0x67, 0x9d, 0xeb, 0xf5, 0x5d,
  0xde, 0xaf, 0x36, 0xbf, 0xf3, 0xf9, 0xcf, 0x7c, 0xef, 0x57,
  0x2f, 0xdb, 0xfd, 0x9e, 0xbe, 0xeb, 0x77, 0xa7, 0xf9, 0xfd,
  0x57, 0x7e, 0xcf, 0x67, 0xbe, 0xf9, 0xf8, 0x9f, 0xee, 0xed,
  0x77, 0xb3, 0x7b, 0x5d, 0xdf, 0x9e, 0xef, 0x73, 0xbd, 0xea,
  0xbd, 0xdf, 0xf6, 0x67, 0x53, 0xbf, 0x3a, 0xfd, 0xdb, 0xee,
  0xef, 0x75, 0xbe, 0xb3, 0x7d, 0xdf, 0xf6, 0xaa, 0x77, 0x3b,
  0xfb, 0xcd, 0xcd, 0xfe, 0xe5, 0x76, 0x9f, 0xfb, 0xbc, 0xd7,
  0xfa, 0xef, 0x67, 0xbd, 0xdb, 0xe5, 0x5f, 0xdc, 0xee, 0x36,
  0xbf, 0xfb, 0xf5, 0xcb, 0xfc, 0x6f, 0x77, 0x1f, 0xcb, 0xfd,
  0x9c, 0xfe, 0xab, 0x77, 0xaf, 0xf1, 0xf9, 0xd7, 0x7e, 0xcf,
  0x57, 0xae, 0xfb, 0xf9, 0xde, 0xae, 0xed, 0x77, 0xb7, 0x7b,
  0xdd, 0x5f, 0xbe, 0xcf, 0x63, 0xbf, 0xe9, 0xfc, 0x9f, 0xf6,
  0x6f, 0x57, 0xbb, 0xba, 0x7d, 0xdb, 0xde, 0xef, 0x75, 0xbd,
  0xf2, 0x3d, 0xdf, 0xfe, 0xa6, 0x73, 0xbb, 0x7b, 0xed, 0xdd,
  0xee, 0xe7, 0x75, 0x9e, 0xbb, 0xbc, 0xdf, 0xf2, 0xeb, 0x67,
  0x3d, 0xfb, 0xd5, 0x4f, 0xde, 0xed, 0x36, 0xbf, 0xfb, 0xfd,
  0xc7
};

/// Pre-sieve with the primes < 100
const pod_array<std::vector<uint64_t>, 8> bufferPrimes =
{{
  {  7, 67, 71 },  // 32 KiB
  { 11, 41, 73 },  // 32 KiB
  { 13, 43, 59 },  // 32 KiB
  { 17, 37, 53 },  // 32 KiB
  { 19, 29, 61 },  // 32 KiB
  { 23, 31, 47 },  // 32 KiB
      { 79, 97 },  //  8 KiB
      { 83, 89 }   //  7 KiB
}};

/// Each byte represents an interval of 30 integers
const uint64_t buffersDist =
  ( 7 * 67 * 71) * 30 +
  (11 * 41 * 73) * 30 +
  (13 * 43 * 59) * 30 +
  (17 * 37 * 53) * 30 +
  (19 * 29 * 61) * 30 +
  (23 * 31 * 47) * 30 +
       (79 * 97) * 30 +
       (83 * 89) * 30;

void andBuffers(const uint8_t* __restrict buf1,
                const uint8_t* __restrict buf2,
                const uint8_t* __restrict buf3,
                const uint8_t* __restrict buf4,
                const uint8_t* __restrict buf5,
                const uint8_t* __restrict buf6,
                const uint8_t* __restrict buf7,
                const uint8_t* __restrict buf8,
                uint8_t* __restrict output,
                std::size_t bytes)
{
  // This loop should be auto-vectorized
  for (std::size_t i = 0; i < bytes; i++)
    output[i] = buf1[i] & buf2[i] & buf3[i] & buf4[i]
              & buf5[i] & buf6[i] & buf7[i] & buf8[i];
}

} // namespace

namespace primesieve {

void PreSieve::init(uint64_t start,
                    uint64_t stop)
{
  // Already initialized
  if (!buffers_[0].empty())
    return;

  // To reduce the initialization overhead, we only enable
  // pre-sieving if the sieving distance is at least 20x
  // larger than the distance (size) of the pre-sieve buffers.
  uint64_t dist = std::max(start, stop) - start;
  uint64_t sqrtStop = (uint64_t) std::sqrt(stop);
  dist = std::max(dist, sqrtStop);

  // When sieving backwards using primesieve::iterator.prev_prime()
  // the sieving distance is subdivided into smaller chunks. By
  // keeping track of the total sieving distance here, we can
  // figure out if the total sieving distance is large and if it
  // is worth enabling pre-sieving.
  totalDist_ += dist;

  // For small intervals we pre-sieve using the
  // static buffer_7_11_13 lookup table. In this
  // case no initialization is required.
  if (totalDist_ < buffersDist * 20)
    return;

  initBuffers();
}

/// We are sieving a large interval, we have to
/// initialize all pre-sieve buffers.
///
void PreSieve::initBuffers()
{
  for (std::size_t i = 0; i < buffers_.size(); i++)
  {
    uint64_t product = 30;

    for (uint64_t prime : bufferPrimes[i])
      product *= prime;

    uint64_t start = product;
    uint64_t stop = start + product;
    buffers_[i].resize(product / 30);
    std::fill(buffers_[i].begin(), buffers_[i].end(), 0xff);
    uint64_t maxPrime = bufferPrimes[i].back();
    assert(maxPrime == *std::max_element(bufferPrimes[i].begin(), bufferPrimes[i].end()));
    assert(start >= maxPrime * maxPrime);
    maxPrime_ = std::max(maxPrime_, maxPrime);

    EratSmall eratSmall;
    eratSmall.init(stop, buffers_[i].size(), maxPrime);

    for (uint64_t prime : bufferPrimes[i])
      eratSmall.addSievingPrime(prime, start);

    eratSmall.crossOff(buffers_[i]);
  }
}

void PreSieve::preSieve(pod_vector<uint8_t>& sieve,
                        uint64_t segmentLow) const
{
  if (buffers_[0].empty())
    preSieveSmall(sieve, segmentLow);
  else
    preSieveLarge(sieve, segmentLow);

  // Pre-sieving removes the primes < 100. We
  // have to undo that work and reset these bits
  // to 1 (but 49 = 7 * 7 is not a prime).
  uint8_t bit49 = 1 << 4;
  uint8_t bit77 = 1 << 3;
  uint8_t bit91 = 1 << 7;
  uint8_t bit119 = 1 << 6;
  uint8_t bit121 = 1 << 7;

  std::size_t i = 0;

  if (segmentLow < 30)
    sieve[i++] = 0xff;
  if (segmentLow < 60)
    sieve[i++] = 0xff ^ bit49;
  if (segmentLow < 90)
    sieve[i++] = 0xff ^ bit77 ^ bit91;
  if (segmentLow < 120)
    sieve[i++] = 0xff ^ bit119 ^ bit121;
}

/// Pre-sieve with the primes <= 13
void PreSieve::preSieveSmall(pod_vector<uint8_t>& sieve,
                             uint64_t segmentLow)
{
  uint64_t size = buffer_7_11_13.size();
  uint64_t primeProduct = size * 30;
  uint64_t i = (segmentLow % primeProduct) / 30;
  uint64_t sizeLeft = size - i;
  auto buffer = buffer_7_11_13.data();

  if (sieve.size() <= sizeLeft)
    copy_n(&buffer[i], sieve.size(), sieve.begin());
  else
  {
    // Copy the last remaining bytes of buffer
    // to the beginning of the sieve array
    copy_n(&buffer[i], sizeLeft, sieve.begin());

    // Restart copying at the beginning of buffer
    for (i = sizeLeft; i + size < sieve.size(); i += size)
      copy_n(buffer, size, &sieve[i]);

    // Copy the last remaining bytes
    copy_n(buffer, sieve.size() - i, &sieve[i]);
  }
}

/// Pre-sieve with the primes < 100
void PreSieve::preSieveLarge(pod_vector<uint8_t>& sieve,
                             uint64_t segmentLow) const
{
  uint64_t offset = 0;
  pod_array<uint64_t, 8> pos;

  for (std::size_t i = 0; i < buffers_.size(); i++)
    pos[i] = (segmentLow % (buffers_[i].size() * 30)) / 30;

  while (offset < sieve.size()) {
    uint64_t bytesToCopy = sieve.size() - offset;

    for (std::size_t i = 0; i < buffers_.size(); i++) {
      uint64_t left = buffers_[i].size() - pos[i];
      bytesToCopy = std::min(left, bytesToCopy);
    }

    andBuffers(&buffers_[0][pos[0]],
               &buffers_[1][pos[1]],
               &buffers_[2][pos[2]],
               &buffers_[3][pos[3]],
               &buffers_[4][pos[4]],
               &buffers_[5][pos[5]],
               &buffers_[6][pos[6]],
               &buffers_[7][pos[7]],
               &sieve[offset],
               bytesToCopy);

    offset += bytesToCopy;

    for (std::size_t i = 0; i < pos.size(); i++) {
      pos[i] += bytesToCopy;
      if (pos[i] >= buffers_[i].size())
        pos[i] = 0;
    }
  }
}

} // namespace
