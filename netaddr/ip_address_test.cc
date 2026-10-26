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

#include <optional>
#include <sstream>
#include <vector>

#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/status_matchers.h"
#include "netaddr/ipv4_address.h"
#include "netaddr/ipv6_address.h"

namespace netaddr {
namespace {

using ::gutil::IsOkAndHolds;
using ::gutil::StatusIs;
using ::testing::Eq;
using ::testing::Optional;

TEST(IpAddressTest, DefaultIsIpv4Zero) {
  EXPECT_EQ(IpAddress(), IpAddress(Ipv4Address()));
}

TEST(IpAddressTest, OfStringParsesIpv4) {
  EXPECT_THAT(IpAddress::OfString("10.0.0.1"),
              IsOkAndHolds(Eq(IpAddress(Ipv4Address(10, 0, 0, 1)))));
}

TEST(IpAddressTest, OfStringParsesIpv6) {
  EXPECT_THAT(IpAddress::OfString("2001:db8::1"),
              IsOkAndHolds(Eq(
                  IpAddress(Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1)))));
}

TEST(IpAddressTest, OfStringParsesIpv4MappedIpv6AsIpv6) {
  EXPECT_THAT(IpAddress::OfString("::ffff:10.0.0.1"),
              IsOkAndHolds(Eq(IpAddress(
                  Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001)))));
}

TEST(IpAddressTest, OfStringRejectsInvalidAddresses) {
  // Empty string.
  EXPECT_THAT(IpAddress::OfString(""),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Non-IP text.
  EXPECT_THAT(IpAddress::OfString("notanip"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // IPv4 octet > 255.
  EXPECT_THAT(IpAddress::OfString("10.0.0.256"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Too few IPv4 octets.
  EXPECT_THAT(IpAddress::OfString("10.0.0"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Triple colon in IPv6.
  EXPECT_THAT(IpAddress::OfString("2001:db8:::1"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  // Prefix length attached to address.
  EXPECT_THAT(IpAddress::OfString("10.0.0.1/24"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(IpAddressTest, ToStringReturnsCanonicalNotation) {
  EXPECT_EQ(IpAddress(Ipv4Address(10, 0, 0, 1)).ToString(), "10.0.0.1");
  EXPECT_EQ(
      IpAddress(Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1)).ToString(),
      "2001:db8::1");
}

TEST(IpAddressTest, ToStringCanonicalizesNonCanonicalIpv6) {
  ASSERT_OK_AND_ASSIGN(
      IpAddress ip,
      IpAddress::OfString("2001:0DB8:0000:0000:0000:0000:0000:0001"));
  EXPECT_EQ(ip.ToString(), "2001:db8::1");
}

TEST(IpAddressTest, OstreamPrintsToString) {
  std::ostringstream os;
  os << IpAddress(Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1));
  EXPECT_EQ(os.str(), "2001:db8::1");
}

TEST(IpAddressTest, FamilyChecksAndConversions) {
  const IpAddress ipv4 = Ipv4Address(10, 0, 0, 1);
  EXPECT_TRUE(ipv4.IsIpv4());
  EXPECT_FALSE(ipv4.IsIpv6());
  EXPECT_THAT(ipv4.AsIpv4(), Optional(Ipv4Address(10, 0, 0, 1)));
  EXPECT_EQ(ipv4.AsIpv6(), std::nullopt);

  const IpAddress ipv6 = Ipv6Address(0x2001, 0x0db8);
  EXPECT_FALSE(ipv6.IsIpv4());
  EXPECT_TRUE(ipv6.IsIpv6());
  EXPECT_EQ(ipv6.AsIpv4(), std::nullopt);
  EXPECT_THAT(ipv6.AsIpv6(), Optional(Ipv6Address(0x2001, 0x0db8)));
}

TEST(IpAddressTest, IsAllZeros) {
  EXPECT_TRUE(IpAddress(Ipv4Address()).IsAllZeros());
  EXPECT_TRUE(IpAddress(Ipv6Address()).IsAllZeros());
  EXPECT_FALSE(IpAddress(Ipv4Address(0, 0, 0, 1)).IsAllZeros());
  EXPECT_FALSE(IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0, 0, 0x1)).IsAllZeros());
}

TEST(IpAddressTest, FamilyIsPartOfTheValue) {
  // 0.0.0.0 and :: have the same (zero) bits but differ in family.
  EXPECT_NE(IpAddress(Ipv4Address()), IpAddress(Ipv6Address()));
}

TEST(IpAddressTest, OrdersIpv4BeforeIpv6ThenByValue) {
  const IpAddress ipv4_low = Ipv4Address(10, 0, 0, 1);
  const IpAddress ipv4_high = Ipv4Address(255, 255, 255, 255);
  const IpAddress ipv6_low = Ipv6Address();
  const IpAddress ipv6_high = Ipv6Address(0x2001, 0x0db8);
  EXPECT_LT(ipv4_low, ipv4_high);
  EXPECT_LT(ipv4_high, ipv6_low);
  EXPECT_LT(ipv6_low, ipv6_high);
  EXPECT_LE(ipv4_low, ipv4_low);
  EXPECT_GT(ipv6_high, ipv4_high);
  EXPECT_GE(ipv6_high, ipv6_high);
}

TEST(IpAddressTest, ImplementsAbslHashCorrectly) {
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      IpAddress(Ipv4Address()),
      IpAddress(Ipv6Address()),
      IpAddress(Ipv4Address(10, 0, 0, 1)),
      IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0, 0x0a00, 0x0001)),
      IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001)),
  }));
}

TEST(IpAddressTest, ToIpv4MappedIpv6WrapsIpv4) {
  EXPECT_EQ(IpAddress(Ipv4Address(10, 0, 0, 1)).ToIpv4MappedIpv6(),
            Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001));
  EXPECT_EQ(IpAddress(Ipv4Address(10, 0, 0, 1)).ToIpv4MappedIpv6().ToString(),
            "::ffff:10.0.0.1");
}

TEST(IpAddressTest, ToIpv4MappedIpv6WrapsBoundaryIpv4) {
  EXPECT_EQ(IpAddress(Ipv4Address(0, 0, 0, 0)).ToIpv4MappedIpv6(),
            Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0, 0));
  EXPECT_EQ(IpAddress(Ipv4Address(255, 255, 255, 255)).ToIpv4MappedIpv6(),
            Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff));
}

TEST(IpAddressTest, ToIpv4MappedIpv6ReturnsIpv6Unchanged) {
  const Ipv6Address ipv6(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1);
  EXPECT_EQ(IpAddress(ipv6).ToIpv4MappedIpv6(), ipv6);
  const Ipv6Address mapped(0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001);
  EXPECT_EQ(IpAddress(mapped).ToIpv4MappedIpv6(), mapped);
}

TEST(IpAddressTest, UnmapIpv4MappedIpv6UnwrapsMappedAddress) {
  EXPECT_EQ(IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0x0a00, 0x0001))
                .UnmapIpv4MappedIpv6(),
            IpAddress(Ipv4Address(10, 0, 0, 1)));
}

TEST(IpAddressTest, UnmapIpv4MappedIpv6UnwrapsBoundaryMappedAddresses) {
  EXPECT_EQ(IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0xffff)).UnmapIpv4MappedIpv6(),
            IpAddress(Ipv4Address(0, 0, 0, 0)));
  EXPECT_EQ(IpAddress(Ipv6Address(0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff))
                .UnmapIpv4MappedIpv6(),
            IpAddress(Ipv4Address(255, 255, 255, 255)));
}

TEST(IpAddressTest, UnmapIpv4MappedIpv6ReturnsNonMappedIpv6Unchanged) {
  for (const Ipv6Address& ipv6 : {
           // Native IPv6.
           Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1),
           // ::
           Ipv6Address(),
           // ::1
           Ipv6Address(0, 0, 0, 0, 0, 0, 0, 0x1),
           // ::fffe:10.0.0.1
           Ipv6Address(0, 0, 0, 0, 0, 0xfffe, 0x0a00, 0x0001),
           // ::1:ffff:10.0.0.1
           Ipv6Address(0, 0, 0, 0, 0x1, 0xffff, 0x0a00, 0x0001),
           // ::10.0.0.1 (deprecated IPv4-compatible form).
           Ipv6Address(0, 0, 0, 0, 0, 0, 0x0a00, 0x0001),
       }) {
    EXPECT_EQ(IpAddress(ipv6).UnmapIpv4MappedIpv6(), IpAddress(ipv6)) << ipv6;
  }
}

TEST(IpAddressTest, UnmapIpv4MappedIpv6ReturnsIpv4Unchanged) {
  EXPECT_EQ(IpAddress(Ipv4Address(10, 0, 0, 1)).UnmapIpv4MappedIpv6(),
            IpAddress(Ipv4Address(10, 0, 0, 1)));
}

TEST(IpAddressTest, Ipv4MappedIpv6RoundTrips) {
  for (const IpAddress& ip : std::vector<IpAddress>{
           Ipv4Address(0, 0, 0, 0),
           Ipv4Address(10, 0, 0, 1),
           Ipv4Address(255, 255, 255, 255),
           Ipv6Address(0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x1),
       }) {
    EXPECT_EQ(IpAddress(ip.ToIpv4MappedIpv6()).UnmapIpv4MappedIpv6(), ip) << ip;
  }
}

}  // namespace
}  // namespace netaddr
