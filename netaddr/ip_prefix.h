// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef P4_INFRA_NETADDR_IP_PREFIX_H_
#define P4_INFRA_NETADDR_IP_PREFIX_H_

#include <ostream>
#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "netaddr/ip_address.h"

namespace netaddr {

// An IPv4 or IPv6 address with a prefix length, e.g. 10.0.0.0/24 or
// 2001:db8::/32.
//
// The address may have bits set beyond the prefix length (e.g. 10.0.0.1/24, an
// interface address with its subnet); such bits are kept rather than cleared,
// so 10.0.0.1/24 and 10.0.0.0/24 are different values. See
// `HasNonZeroHostBits`.
class IpPrefix {
 public:
  // The default constructor returns 0.0.0.0/0.
  constexpr IpPrefix() = default;

  // Constructs the prefix `address`/`prefix_length`. The caller must ensure
  // that `prefix_length` is in [0, 32] for an IPv4 address and [0, 128] for an
  // IPv6 address; this is not checked. Use `OfString` for untrusted input.
  constexpr IpPrefix(IpAddress address, int prefix_length)
      : address_(address), prefix_length_(prefix_length) {}

  // Constructs an IpPrefix from a string of the form "<address>[/<length>]",
  // e.g. "10.0.0.0/24" or "2001:db8::/32". The address accepts any notation
  // that `IpAddress::OfString` accepts. The length must be a decimal number
  // without sign or leading zeros, in [0, 32] for IPv4 and [0, 128] for IPv6; a
  // missing length defaults to the full address width (/32 or /128). Bits
  // beyond the prefix length are kept.
  static absl::StatusOr<IpPrefix> OfString(absl::string_view prefix);

  // Returns the prefix as "<address>/<length>" with the address in canonical
  // notation, e.g. "2001:db8::/32".
  std::string ToString() const;

  // -- Accessors --
  const IpAddress& address() const { return address_; }
  int prefix_length() const { return prefix_length_; }

  // -- Checks --
  // Returns true if the address has any bits set beyond the prefix length, e.g.
  // for 10.0.0.1/24.
  bool HasNonZeroHostBits() const;

  // -- Comparisons --
  friend bool operator==(const IpPrefix& left, const IpPrefix& right) {
    return left.address_ == right.address_ &&
           left.prefix_length_ == right.prefix_length_;
  }

  // Hashing (https://abseil.io/docs/cpp/guides/hash).
  template <typename H>
  friend H AbslHashValue(H h, const IpPrefix& prefix) {
    return H::combine(std::move(h), prefix.address_, prefix.prefix_length_);
  }

 private:
  IpAddress address_;
  int prefix_length_ = 0;
};

// Pretty printing.
inline std::ostream& operator<<(std::ostream& os, const IpPrefix& prefix) {
  return os << prefix.ToString();
}

}  // namespace netaddr

#endif  // P4_INFRA_NETADDR_IP_PREFIX_H_
