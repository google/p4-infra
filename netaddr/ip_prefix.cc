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

#include "netaddr/ip_prefix.h"

#include <cstddef>
#include <optional>
#include <string>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "gutil/status.h"
#include "netaddr/ip_address.h"
#include "netaddr/ipv4_address.h"
#include "netaddr/ipv6_address.h"

namespace netaddr {

absl::StatusOr<IpPrefix> IpPrefix::OfString(absl::string_view prefix) {
  // Split "<address>[/<length>]" and parse the address.
  size_t slash_pos = prefix.find('/');
  ASSIGN_OR_RETURN(IpAddress address,
                   IpAddress::OfString(prefix.substr(0, slash_pos)),
                   _ << "Invalid IP prefix: '" << prefix << "'");

  // Parse the prefix length, defaulting to the full address width.
  int max_prefix_length = address.IsIpv4() ? 32 : 128;
  int prefix_length = max_prefix_length;
  if (slash_pos != absl::string_view::npos) {
    absl::string_view length_str = prefix.substr(slash_pos + 1);
    // Check for invalid characters, leading zeros, and successful
    // conversion.
    if (!absl::c_all_of(length_str, absl::ascii_isdigit) ||
        (length_str.size() > 1 && length_str[0] == '0') ||
        !absl::SimpleAtoi(length_str, &prefix_length)) {
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid prefix length in IP prefix: '", prefix, "'"));
    }
  }
  if (prefix_length > max_prefix_length) {
    return absl::InvalidArgumentError(
        absl::StrCat("Prefix length out of range [0, ", max_prefix_length,
                     "] in IP prefix: '", prefix, "'"));
  }

  return IpPrefix(address, prefix_length);
}

std::string IpPrefix::ToString() const {
  return absl::StrCat(address_.ToString(), "/", prefix_length_);
}

bool IpPrefix::HasNonZeroHostBits() const {
  // Shifting out the network bits leaves only the host bits.
  if (std::optional<Ipv4Address> ipv4 = address_.AsIpv4(); ipv4.has_value()) {
    return (ipv4->ToBitset() << prefix_length_).any();
  }
  return (address_.AsIpv6()->ToBitset() << prefix_length_).any();
}

}  // namespace netaddr
