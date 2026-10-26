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

#include <sstream>

#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/status_matchers.h"
#include "netaddr/ip_address.h"
#include "netaddr/ipv4_address.h"
#include "netaddr/ipv6_address.h"

namespace netaddr {
namespace {

using ::gutil::IsOk;
using ::gutil::IsOkAndHolds;
using ::gutil::StatusIs;
using ::testing::Eq;

TEST(IpPrefixTest, DefaultIsIpv4Zero) {
  EXPECT_EQ(IpPrefix(), IpPrefix(Ipv4Address(), 0));
}

TEST(IpPrefixTest, Accessors) {
  const IpPrefix prefix(Ipv6Address(0x2001, 0x0db8), 32);
  EXPECT_EQ(prefix.address(), IpAddress(Ipv6Address(0x2001, 0x0db8)));
  EXPECT_EQ(prefix.prefix_length(), 32);
}

TEST(IpPrefixTest, OfStringParsesIpv4) {
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/24"),
              IsOkAndHolds(Eq(IpPrefix(Ipv4Address(10, 0, 0, 0), 24))));
}

TEST(IpPrefixTest, OfStringParsesIpv6) {
  EXPECT_THAT(IpPrefix::OfString("2001:db8::/32"),
              IsOkAndHolds(Eq(IpPrefix(Ipv6Address(0x2001, 0x0db8), 32))));
}

TEST(IpPrefixTest, OfStringDefaultsMissingLengthToFullWidth) {
  EXPECT_THAT(IpPrefix::OfString("10.0.0.1"),
              IsOkAndHolds(Eq(IpPrefix(Ipv4Address(10, 0, 0, 1), 32))));
  EXPECT_THAT(
      IpPrefix::OfString("::1"),
      IsOkAndHolds(Eq(IpPrefix(Ipv6Address(0, 0, 0, 0, 0, 0, 0, 0x1), 128))));
}

TEST(IpPrefixTest, OfStringParsesBoundaryLengths) {
  EXPECT_THAT(IpPrefix::OfString("0.0.0.0/0"),
              IsOkAndHolds(Eq(IpPrefix(Ipv4Address(), 0))));
  EXPECT_THAT(IpPrefix::OfString("::/0"),
              IsOkAndHolds(Eq(IpPrefix(Ipv6Address(), 0))));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.1/32"), IsOk());
  EXPECT_THAT(IpPrefix::OfString("2001:db8::1/128"), IsOk());
}

TEST(IpPrefixTest, OfStringKeepsNonZeroHostBits) {
  ASSERT_OK_AND_ASSIGN(IpPrefix prefix, IpPrefix::OfString("10.0.0.1/24"));
  EXPECT_EQ(prefix, IpPrefix(Ipv4Address(10, 0, 0, 1), 24));
  EXPECT_TRUE(prefix.HasNonZeroHostBits());
  EXPECT_EQ(prefix.ToString(), "10.0.0.1/24");
}

TEST(IpPrefixTest, OfStringRejectsInvalidPrefixes) {
  // Empty string.
  EXPECT_THAT(IpPrefix::OfString(""),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Missing IP address before '/'.
  EXPECT_THAT(IpPrefix::OfString("/24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Malformed IPv4 or IPv6 address.
  EXPECT_THAT(IpPrefix::OfString("notanip/24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.256/24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("2001:db8:::/32"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Empty prefix length after '/'.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Non-numeric prefix length.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/abc"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Extra '/' in prefix length.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/24/8"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Negative or signed prefix length.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/-1"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/+24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Whitespace in prefix length.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/ 24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/24 "),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Leading zeros in prefix length.
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/0024"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/024"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/00"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("2001:db8::/064"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Prefix length out of range (> 32 for IPv4, > 128 for IPv6).
  EXPECT_THAT(IpPrefix::OfString("10.0.0.0/33"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(IpPrefix::OfString("2001:db8::/129"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(IpPrefixTest, HasNonZeroHostBits) {
  EXPECT_FALSE(IpPrefix(Ipv4Address(10, 0, 0, 0), 24).HasNonZeroHostBits());
  EXPECT_TRUE(IpPrefix(Ipv4Address(10, 0, 0, 1), 24).HasNonZeroHostBits());
  EXPECT_FALSE(IpPrefix(Ipv4Address(10, 0, 0, 1), 32).HasNonZeroHostBits());
  EXPECT_TRUE(IpPrefix(Ipv4Address(10, 0, 0, 0), 0).HasNonZeroHostBits());
  EXPECT_FALSE(IpPrefix(Ipv6Address(0x2001, 0x0db8), 32).HasNonZeroHostBits());
  EXPECT_TRUE(
      IpPrefix(Ipv6Address(0x2001, 0x0db8, 0x1), 32).HasNonZeroHostBits());
  EXPECT_FALSE(IpPrefix(Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 1), 128)
                   .HasNonZeroHostBits());
}

TEST(IpPrefixTest, ToStringReturnsCanonicalNotation) {
  EXPECT_EQ(IpPrefix(Ipv4Address(10, 0, 0, 0), 24).ToString(), "10.0.0.0/24");
  EXPECT_EQ(IpPrefix(Ipv6Address(0x2001, 0x0db8), 32).ToString(),
            "2001:db8::/32");
  EXPECT_EQ(IpPrefix(Ipv4Address(), 0).ToString(), "0.0.0.0/0");
  EXPECT_EQ(IpPrefix(Ipv6Address(), 0).ToString(), "::/0");
}

TEST(IpPrefixTest, OstreamPrintsToString) {
  std::ostringstream os;
  os << IpPrefix(Ipv6Address(0x2001, 0x0db8), 32);
  EXPECT_EQ(os.str(), "2001:db8::/32");
}

TEST(IpPrefixTest, ImplementsAbslHashCorrectly) {
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      IpPrefix(Ipv4Address(), 0),
      IpPrefix(Ipv6Address(), 0),
      IpPrefix(Ipv4Address(10, 0, 0, 0), 24),
      IpPrefix(Ipv4Address(10, 0, 0, 1), 24),
      IpPrefix(Ipv4Address(10, 0, 0, 0), 25),
      IpPrefix(Ipv6Address(0x2001, 0x0db8), 32),
      IpPrefix(Ipv6Address(0x2001, 0x0db8, 0x1), 32),
      IpPrefix(Ipv6Address(0x2001, 0x0db8), 64),
  }));
}

}  // namespace
}  // namespace netaddr
