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

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"

namespace pdpi {
namespace {

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesStructuredAnnotationGroupModeKeyValuePairList) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* kv_list = sa->mutable_kv_pair_list();

  auto* kv1 = kv_list->add_kv_pairs();
  kv1->set_key("action_selection_mode");
  kv1->mutable_value()->set_string_value("HASH");

  auto* kv2 = kv_list->add_kv_pairs();
  kv2->set_key("size_semantics");
  kv2->mutable_value()->set_string_value("sum_of_weights");

  auto* kv3 = kv_list->add_kv_pairs();
  kv3->set_key("member_multiplier");
  kv3->mutable_value()->set_int64_value(1);

  auto* kv4 = kv_list->add_kv_pairs();
  kv4->set_key("max_member_weight");
  kv4->mutable_value()->set_int64_value(0);

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_weights());
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 1);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesStructuredAnnotationKeyValuePairList) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* kv_list = sa->mutable_kv_pair_list();

  auto* kv1 = kv_list->add_kv_pairs();
  kv1->set_key("action_selection_mode");
  kv1->mutable_value()->set_string_value("HASH");

  auto* kv2 = kv_list->add_kv_pairs();
  kv2->set_key("size_semantics");
  kv2->mutable_value()->set_string_value("sum_of_weights");

  auto* kv3 = kv_list->add_kv_pairs();
  kv3->set_key("member_multiplier");
  kv3->mutable_value()->set_int64_value(1);

  auto* kv4 = kv_list->add_kv_pairs();
  kv4->set_key("max_member_weight");
  kv4->mutable_value()->set_int64_value(0);

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_weights());
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 1);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesStructuredAnnotationExpressionList) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* expr_list = sa->mutable_expression_list();

  expr_list->add_expressions()->set_string_value("HASH");
  expr_list->add_expressions()->set_string_value("sum_of_members: 1");
  expr_list->add_expressions()->set_string_value("4095");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_members());
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 1);
  EXPECT_EQ(modes[0].sum_of_members().max_member_weight(), 4095);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesMultipleSeparateRequiredModeAnnotations) {
  p4::config::v1::ActionProfile ap;

  auto* sa1 = ap.mutable_preamble()->add_structured_annotations();
  sa1->set_name("group_mode");
  auto* kv_list1 = sa1->mutable_kv_pair_list();
  auto* kv1_1 = kv_list1->add_kv_pairs();
  kv1_1->set_key("action_selection_mode");
  kv1_1->mutable_value()->set_string_value("HASH");
  auto* kv1_2 = kv_list1->add_kv_pairs();
  kv1_2->set_key("size_semantics");
  kv1_2->mutable_value()->set_string_value("sum_of_weights");

  auto* sa2 = ap.mutable_preamble()->add_structured_annotations();
  sa2->set_name("group_mode");
  auto* kv_list2 = sa2->mutable_kv_pair_list();
  auto* kv2_1 = kv_list2->add_kv_pairs();
  kv2_1->set_key("action_selection_mode");
  kv2_1->mutable_value()->set_string_value("RANDOM");
  auto* kv2_2 = kv_list2->add_kv_pairs();
  kv2_2->set_key("size_semantics");
  kv2_2->mutable_value()->set_string_value("sum_of_members");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 2);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_weights());
  EXPECT_EQ(modes[1].action_selection_mode(), ActionProfileMode::RANDOM);
  EXPECT_TRUE(modes[1].has_sum_of_members());
}

TEST(ParseRequiredModesFromActionProfileTest,
     DeduplicatesStructuredAnnotations) {
  p4::config::v1::ActionProfile ap;

  auto add_structured_mode = [&](const std::string& mode_str,
                                 const std::string& size_str, int mult,
                                 int weight) {
    auto* sa = ap.mutable_preamble()->add_structured_annotations();
    sa->set_name("group_mode");
    auto* kv_list = sa->mutable_kv_pair_list();

    auto* kv1 = kv_list->add_kv_pairs();
    kv1->set_key("action_selection_mode");
    kv1->mutable_value()->set_string_value(mode_str);

    auto* kv2 = kv_list->add_kv_pairs();
    kv2->set_key("size_semantics");
    kv2->mutable_value()->set_string_value(size_str);

    auto* kv3 = kv_list->add_kv_pairs();
    kv3->set_key("member_multiplier");
    kv3->mutable_value()->set_int64_value(mult);

    auto* kv4 = kv_list->add_kv_pairs();
    kv4->set_key("max_member_weight");
    kv4->mutable_value()->set_int64_value(weight);
  };

  add_structured_mode("HASH", "sum_of_weights", 1, 0);
  add_structured_mode("HASH", "sum_of_weights", 1, 0);

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  EXPECT_EQ(modes.size(), 1);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesMultipleStructuredGroupModeAnnotations) {
  p4::config::v1::ActionProfile ap;
  auto* sa1 = ap.mutable_preamble()->add_structured_annotations();
  sa1->set_name("group_mode");
  auto* kv1 = sa1->mutable_kv_pair_list()->add_kv_pairs();
  kv1->set_key("action_selection_mode");
  kv1->mutable_value()->set_string_value("HASH");

  auto* sa2 = ap.mutable_preamble()->add_structured_annotations();
  sa2->set_name("group_mode");
  auto* kv2 = sa2->mutable_kv_pair_list()->add_kv_pairs();
  kv2->set_key("action_selection_mode");
  kv2->mutable_value()->set_string_value("RANDOM");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 2);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_EQ(modes[1].action_selection_mode(), ActionProfileMode::RANDOM);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesMultipleModesFromSingleExpressionList) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* expr_list = sa->mutable_expression_list();

  expr_list->add_expressions()->set_string_value("HASH");
  expr_list->add_expressions()->set_string_value("sum_of_weights");
  expr_list->add_expressions()->set_string_value("0");

  expr_list->add_expressions()->set_string_value("RANDOM");
  expr_list->add_expressions()->set_string_value("sum_of_members: 4");
  expr_list->add_expressions()->set_string_value("4095");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 2);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_weights());
  EXPECT_EQ(modes[1].action_selection_mode(), ActionProfileMode::RANDOM);
  EXPECT_TRUE(modes[1].has_sum_of_members());
  EXPECT_EQ(modes[1].resource_usage_multipliers().member_usage_multiplier(), 4);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesSymmetricSumOfWeightsAndKeyValuePairs) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* kv_list = sa->mutable_kv_pair_list();

  auto* kv1 = kv_list->add_kv_pairs();
  kv1->set_key("action_selection_mode");
  kv1->mutable_value()->set_string_value("HASH");

  auto* kv2 = kv_list->add_kv_pairs();
  kv2->set_key("size_semantics");
  kv2->mutable_value()->set_string_value(
      "{semantics=\"sum_of_members\", max_weight=4095}");

  auto* kv3 = kv_list->add_kv_pairs();
  kv3->set_key("member_usage_multiplier");
  kv3->mutable_value()->set_int64_value(4);

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_members());
  EXPECT_EQ(modes[0].sum_of_members().max_member_weight(), 4095);
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 4);
}

TEST(ParseRequiredModesFromActionProfileTest,
     ParsesSymmetricSumOfWeightsBracedSyntax) {
  p4::config::v1::ActionProfile ap;
  auto* sa = ap.mutable_preamble()->add_structured_annotations();
  sa->set_name("group_mode");
  auto* kv_list = sa->mutable_kv_pair_list();

  auto* kv1 = kv_list->add_kv_pairs();
  kv1->set_key("action_selection_mode");
  kv1->mutable_value()->set_string_value("HASH");

  auto* kv2 = kv_list->add_kv_pairs();
  kv2->set_key("size_semantics");
  kv2->mutable_value()->set_string_value("{\"sum_of_weights\"}");

  auto* kv3 = kv_list->add_kv_pairs();
  kv3->set_key("member_usage_multiplier");
  kv3->mutable_value()->set_int64_value(1);

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_weights());
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 1);
}

TEST(ParseRequiredModesFromActionProfileTest, ParsesStringAnnotations) {
  p4::config::v1::ActionProfile ap;
  ap.mutable_preamble()->add_annotations(
      "@group_mode({ action_selection_mode = HASH , size_semantics = "
      "sum_of_members , max_member_weight = 4095 , member_usage_multiplier = "
      "4 })");

  ASSERT_OK_AND_ASSIGN(std::vector<ActionProfileMode> modes,
                       ParseRequiredModesFromActionProfile(ap));

  ASSERT_EQ(modes.size(), 1);
  EXPECT_EQ(modes[0].action_selection_mode(), ActionProfileMode::HASH);
  EXPECT_TRUE(modes[0].has_sum_of_members());
  EXPECT_EQ(modes[0].sum_of_members().max_member_weight(), 4095);
  EXPECT_EQ(modes[0].resource_usage_multipliers().member_usage_multiplier(), 4);
}

}  // namespace
}  // namespace pdpi
