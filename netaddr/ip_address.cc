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

#include "netaddr/ip_address.h"

#include <bitset>
#include <optional>
#include <string>
#include <variant>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "netaddr/ipv4_address.h"
#include "netaddr/ipv6_address.h"

namespace netaddr {
namespace {

// Low 32 bits of an IPv6 address, which hold the embedded IPv4 address.
constexpr std::bitset<128> kIpv4Mask(0xffffffffULL);
// The RFC 4291 IPv4-mapped prefix ::ffff:0:0/96.
constexpr std::bitset<128> kIpv4MappedPrefix(0xffffULL << 32);

}  // namespace

absl::StatusOr<IpAddress> IpAddress::OfString(absl::string_view address) {
  // Try IPv4 first: IPv6 notation always contains ':', so the two parses
  // never both succeed.
  if (absl::StatusOr<Ipv4Address> ipv4 = Ipv4Address::OfString(address);
      ipv4.ok()) {
    return *ipv4;
  }
  if (absl::StatusOr<Ipv6Address> ipv6 = Ipv6Address::OfString(address);
      ipv6.ok()) {
    return *ipv6;
  }
  return absl::InvalidArgumentError(
      absl::StrCat("Invalid IP address: '", address, "'"));
}

std::string IpAddress::ToString() const {
  return std::visit([](const auto& address) { return address.ToString(); },
                    address_);
}

bool IpAddress::IsAllZeros() const {
  return std::visit([](const auto& address) { return address.IsAllZeros(); },
                    address_);
}

std::optional<Ipv4Address> IpAddress::AsIpv4() const {
  if (const auto* ipv4 = std::get_if<Ipv4Address>(&address_)) return *ipv4;
  return std::nullopt;
}

std::optional<Ipv6Address> IpAddress::AsIpv6() const {
  if (const auto* ipv6 = std::get_if<Ipv6Address>(&address_)) return *ipv6;
  return std::nullopt;
}

Ipv6Address IpAddress::ToIpv4MappedIpv6() const {
  if (const auto* ipv6 = std::get_if<Ipv6Address>(&address_)) return *ipv6;
  const auto& ipv4 = std::get<Ipv4Address>(address_);
  return Ipv6Address(kIpv4MappedPrefix |
                     std::bitset<128>(ipv4.ToBitset().to_ulong()));
}

IpAddress IpAddress::UnmapIpv4MappedIpv6() const {
  const auto* ipv6 = std::get_if<Ipv6Address>(&address_);
  if (ipv6 == nullptr) return *this;

  // An IPv4-mapped address has 0xffff in bits [32, 48) and zeros above.
  std::bitset<128> bits = ipv6->ToBitset();
  if ((bits & ~kIpv4Mask) != kIpv4MappedPrefix) return *this;
  return Ipv4Address(std::bitset<32>((bits & kIpv4Mask).to_ulong()));
}

}  // namespace netaddr
