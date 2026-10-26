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

#ifndef P4_INFRA_NETADDR_IP_ADDRESS_H_
#define P4_INFRA_NETADDR_IP_ADDRESS_H_

#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <variant>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "netaddr/ipv4_address.h"
#include "netaddr/ipv6_address.h"

namespace netaddr {

// An IPv4 or IPv6 address, for values whose family is only known at runtime
// (e.g., an address parsed from text that may be either).
//
// The family is part of the value: `::ffff:10.0.0.1` is an IPv6 address and
// differs from the IPv4 address `10.0.0.1` (see `UnmapIpv4MappedIpv6` to
// convert between them).
class IpAddress {
 public:
  // The default constructor returns the IPv4 address 0.0.0.0.
  constexpr IpAddress() = default;

  // Implicitly converts a per-family address, which represents the same value.
  constexpr IpAddress(
      Ipv4Address address)  // NOLINT(google-explicit-constructor)
      : address_(address) {}
  constexpr IpAddress(
      Ipv6Address address)  // NOLINT(google-explicit-constructor)
      : address_(address) {}

  // Constructs an IpAddress from a string in IPv4 dot-decimal notation (e.g.
  // "192.168.2.1") or IPv6 address notation (e.g. "2001:db8::1"). Accepts any
  // notation that `Ipv4Address::OfString` or `Ipv6Address::OfString` accepts,
  // such as uppercase hex or uncompressed zeros.
  static absl::StatusOr<IpAddress> OfString(absl::string_view address);

  // Returns the address in the canonical notation of its family, e.g.
  // "192.168.2.1" or "2001:db8::1".
  std::string ToString() const;

  // -- Checks --
  bool IsIpv4() const { return std::holds_alternative<Ipv4Address>(address_); }
  bool IsIpv6() const { return std::holds_alternative<Ipv6Address>(address_); }
  bool IsAllZeros() const;

  // -- Conversions --
  // Returns the IPv4 address, or nullopt if this is an IPv6 address.
  std::optional<Ipv4Address> AsIpv4() const;
  // Returns the IPv6 address, or nullopt if this is an IPv4 address.
  std::optional<Ipv6Address> AsIpv6() const;
  // Returns the address as an IPv6 address: an IPv4 address a.b.c.d becomes the
  // RFC 4291 IPv4-mapped address ::ffff:a.b.c.d, and an IPv6 address is
  // returned as is.
  Ipv6Address ToIpv4MappedIpv6() const;
  // Inverse of `ToIpv4MappedIpv6`: returns the IPv4 address a.b.c.d for the
  // IPv4-mapped address ::ffff:a.b.c.d, and any other address as is.
  IpAddress UnmapIpv4MappedIpv6() const;

  // -- Comparisons --
  // All IPv4 addresses order before all IPv6 addresses; addresses of the same
  // family order by value.
  friend bool operator==(const IpAddress& left, const IpAddress& right) {
    return left.address_ == right.address_;
  }
  friend bool operator<(const IpAddress& left, const IpAddress& right) {
    return left.address_ < right.address_;
  }
  friend bool operator<=(const IpAddress& left, const IpAddress& right) {
    return left.address_ <= right.address_;
  }
  friend bool operator>(const IpAddress& left, const IpAddress& right) {
    return left.address_ > right.address_;
  }
  friend bool operator>=(const IpAddress& left, const IpAddress& right) {
    return left.address_ >= right.address_;
  }

  // Hashing (https://abseil.io/docs/cpp/guides/hash).
  template <typename H>
  friend H AbslHashValue(H h, const IpAddress& address) {
    return H::combine(std::move(h), address.address_);
  }

 private:
  std::variant<Ipv4Address, Ipv6Address> address_;
};

// Pretty printing.
inline std::ostream& operator<<(std::ostream& os, const IpAddress& address) {
  return os << address.ToString();
}

}  // namespace netaddr

#endif  // P4_INFRA_NETADDR_IP_ADDRESS_H_
