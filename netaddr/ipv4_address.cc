// Copyright 2025 Google LLC
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

#include "netaddr/ipv4_address.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdint>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "gutil/status.h"

namespace netaddr {

absl::StatusOr<Ipv4Address> Ipv4Address::OfString(absl::string_view address) {
  std::string bytes(32 / 8, '\x0');
  if (inet_pton(AF_INET, std::string(address).c_str(), bytes.data()) == 1) {
    return Ipv4Address::OfByteString(bytes);
  }
  return gutil::InvalidArgumentErrorBuilder()
         << "Invalid IPv4 address: '" << address << "'";
}

std::string Ipv4Address::ToString() const {
  uint8_t byte4 = (bits_ >> 24).to_ulong() & 0xFFu;
  uint8_t byte3 = (bits_ >> 16).to_ulong() & 0xFFu;
  uint8_t byte2 = (bits_ >> 8).to_ulong() & 0xFFu;
  uint8_t byte1 = (bits_ >> 0).to_ulong() & 0xFFu;
  return absl::StrFormat("%d.%d.%d.%d", byte4, byte3, byte2, byte1);
}

}  // namespace netaddr
