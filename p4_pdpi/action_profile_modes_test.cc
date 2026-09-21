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

#include "p4_pdpi/action_profile_modes.h"

#include <vector>

#include "absl/status/status.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/proto_matchers.h"
#include "gutil/status_matchers.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"

namespace pdpi {
namespace {

using ::gutil::EqualsProto;
using ::testing::ElementsAre;

TEST(ParseRequiredModesFromActionProfileTest, ParsesSumOfWeightsMode) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "size_semantics = sum_of_weights, member_usage_multiplier = 1)");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  EXPECT_THAT(modes, ElementsAre(EqualsProto(R"pb(
                action_selection_mode: HASH
                sum_of_weights {}
                resource_usage_multipliers { member_usage_multiplier: 1 }
              )pb")));
}

TEST(ParseRequiredModesFromActionProfileTest, ParsesSumOfMembersMode) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "size_semantics = sum_of_members, max_member_weight = 4095, "
      "member_usage_multiplier = 4)");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  EXPECT_THAT(modes, ElementsAre(EqualsProto(R"pb(
                action_selection_mode: HASH
                sum_of_members { max_member_weight: 4095 }
                resource_usage_multipliers { member_usage_multiplier: 4 }
              )pb")));
}

TEST(ParseRequiredModesFromActionProfileTest, IgnoresWhitespace) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(  action_selection_mode=HASH ,"
      "size_semantics   =   sum_of_weights  )");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  EXPECT_THAT(modes, ElementsAre(EqualsProto(R"pb(
                action_selection_mode: HASH
                sum_of_weights {}
              )pb")));
}

TEST(ParseRequiredModesFromActionProfileTest, ParsesMultipleAnnotations) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "size_semantics = sum_of_weights)");
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = RANDOM, "
      "size_semantics = sum_of_members, max_member_weight = 4095, "
      "member_usage_multiplier = 4)");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  EXPECT_THAT(modes, ElementsAre(EqualsProto(R"pb(
                                   action_selection_mode: HASH
                                   sum_of_weights {}
                                 )pb"),
                                 EqualsProto(R"pb(
                                   action_selection_mode: RANDOM
                                   sum_of_members { max_member_weight: 4095 }
                                   resource_usage_multipliers {
                                     member_usage_multiplier: 4
                                   }
                                 )pb")));
}

TEST(ParseRequiredModesFromActionProfileTest, FailsOnBracedAnnotationBody) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee({ action_selection_mode = HASH, "
      "size_semantics = sum_of_weights })");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRequiredModesFromActionProfileTest, FailsOnArgumentWithoutValue) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "sum_of_weights)");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRequiredModesFromActionProfileTest, FailsOnUnknownKey) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "member_multiplier = 1)");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRequiredModesFromActionProfileTest, FailsOnUnknownValue) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = ROUND_ROBIN)");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRequiredModesFromActionProfileTest, FailsOnNonIntegerValue) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "size_semantics = sum_of_members, max_member_weight = many)");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ParseRequiredModesFromActionProfileTest,
     FailsOnMaxMemberWeightWithoutSumOfMembers) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode_resource_guarantee(action_selection_mode = HASH, "
      "size_semantics = sum_of_weights, max_member_weight = 4095)");

  EXPECT_THAT(ParseRequiredModesFromActionProfile(ap),
              gutil::StatusIs(absl::StatusCode::kInvalidArgument));
}

}  // namespace
}  // namespace pdpi
