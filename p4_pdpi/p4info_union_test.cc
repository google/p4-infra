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

#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/proto_matchers.h"
#include "gutil/status_matchers.h"  // IWYU pragma: keep
#include "gutil/testing.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"
#include "p4_pdpi/p4info_union_lib.h"

namespace pdpi {
namespace {

using ::gutil::EqualsProto;
using ::gutil::Partially;
using ::gutil::StatusIs;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::SizeIs;

// Test that all new_types in each P4Info are included in the unioned P4Info,
// whether they are unique among all P4Infos or shared by some/all P4Infos.
TEST(P4InfoUnionTest, UnionTypeInfo) {
  constexpr int kNumberOfInfos = 50;
  constexpr absl::string_view kSharedNewTypeKey = "type_shared_by_all_p4infos";
  constexpr absl::string_view kSharedNewType = R"pb(
    translated_type { sdn_string {} }
    annotations: "shared_by_all_p4infos"
  )pb";

  std::vector<p4::config::v1::P4Info> infos;

  for (int i = 0; i < kNumberOfInfos; ++i) {
    infos.push_back(gutil::ParseProtoOrDie<p4::config::v1::P4Info>(
        absl::Substitute(R"pb(
                           type_info {
                             new_types {
                               key: "type_$0_t"
                               value { translated_type { sdn_string {} } }
                             }
                             new_types { key: "$1" value { $2 } }
                           }
                         )pb",
                         i, kSharedNewTypeKey, kSharedNewType)));
  }

  ASSERT_OK_AND_ASSIGN(p4::config::v1::P4Info unioned_p4info,
                       UnionP4info(infos));
  // Unioned P4Info should contain all kNumberOfInfo unique new_types plus
  // the one shared type.
  ASSERT_THAT(unioned_p4info.type_info().new_types(),
              SizeIs(kNumberOfInfos + 1));

  for (int i = 0; i < kNumberOfInfos; ++i) {
    std::string type_name = absl::Substitute("type_$0_t", i);
    ASSERT_TRUE(unioned_p4info.type_info().new_types().contains(type_name));
    EXPECT_THAT(unioned_p4info.type_info().new_types().at(type_name),
                EqualsProto(
                    R"pb(
                      translated_type { sdn_string {} }
                    )pb"));
  }
  ASSERT_TRUE(
      unioned_p4info.type_info().new_types().contains(kSharedNewTypeKey));
  EXPECT_THAT(unioned_p4info.type_info().new_types().at(kSharedNewTypeKey),
              EqualsProto(kSharedNewType));
}

TEST(P4InfoUnionTest, PlatformPropertiesUseMax) {
  // Any values work here as long as kMaxSize > kSmallerSize.
  constexpr int kMaxSize = 500;
  constexpr int kSmallerSize = 100;

  std::vector<p4::config::v1::P4Info> infos;

  p4::config::v1::PlatformProperties& platform_properties1 =
      *infos.emplace_back().mutable_pkg_info()->mutable_platform_properties();
  // multicast_group_table_size is unset.
  platform_properties1.set_multicast_group_table_total_replicas(kSmallerSize);
  platform_properties1.set_multicast_group_table_max_replicas_per_entry(
      kMaxSize);

  p4::config::v1::PlatformProperties& platform_properties2 =
      *infos.emplace_back().mutable_pkg_info()->mutable_platform_properties();
  platform_properties2.set_multicast_group_table_size(kMaxSize);
  // multicast_group_table_total_replicas is unset.
  platform_properties2.set_multicast_group_table_max_replicas_per_entry(
      kSmallerSize);

  p4::config::v1::PlatformProperties& platform_properties3 =
      *infos.emplace_back().mutable_pkg_info()->mutable_platform_properties();
  platform_properties3.set_multicast_group_table_size(kSmallerSize);
  platform_properties3.set_multicast_group_table_total_replicas(kMaxSize);
  // multicast_group_table_max_replicas_per_entry is unset.

  p4::config::v1::P4Info expected_p4info;
  p4::config::v1::PlatformProperties& expected_platform_properties =
      *expected_p4info.mutable_pkg_info()->mutable_platform_properties();
  expected_platform_properties.set_multicast_group_table_size(kMaxSize);
  expected_platform_properties.set_multicast_group_table_total_replicas(
      kMaxSize);
  expected_platform_properties.set_multicast_group_table_max_replicas_per_entry(
      kMaxSize);

  ASSERT_OK_AND_ASSIGN(p4::config::v1::P4Info unioned_p4info,
                       UnionP4info(infos));

  EXPECT_THAT(unioned_p4info, Partially(EqualsProto(expected_p4info)));
}

TEST(P4InfoUnionTest, UnionActionProfileSizesAndSelectorSemantics) {
  std::vector<p4::config::v1::P4Info> infos;
  infos.push_back(gutil::ParseProtoOrDie<p4::config::v1::P4Info>(R"pb(
    action_profiles {
      preamble {
        id: 1
        name: "ap"
        annotations: "@group_mode_resource_guarantee(action_selection_mode = HASH, size_semantics = sum_of_weights)"
      }
      size: 100
      max_group_size: 10
    }
  )pb"));
  infos.push_back(gutil::ParseProtoOrDie<p4::config::v1::P4Info>(R"pb(
    action_profiles {
      preamble {
        id: 1
        name: "ap"
        annotations: "@group_mode_resource_guarantee(action_selection_mode = HASH, size_semantics = sum_of_weights)"
        annotations: "@group_mode_resource_guarantee(action_selection_mode = RANDOM, size_semantics = sum_of_members)"
      }
      size: 200
      max_group_size: 5
    }
  )pb"));

  ASSERT_OK_AND_ASSIGN(p4::config::v1::P4Info unioned_p4info,
                       UnionP4info(infos));

  p4::config::v1::P4Info expected_p4info = gutil::ParseProtoOrDie<
      p4::config::v1::P4Info>(R"pb(
    action_profiles {
      preamble {
        id: 1
        name: "ap"
        annotations: "@group_mode_resource_guarantee(action_selection_mode = HASH, size_semantics = sum_of_weights)"
        annotations: "@group_mode_resource_guarantee(action_selection_mode = RANDOM, size_semantics = sum_of_members)"
      }
      size: 200
      max_group_size: 10
    }
  )pb");

  EXPECT_THAT(unioned_p4info, Partially(EqualsProto(expected_p4info)));
}

TEST(P4InfoUnionTest, UnionMatchFieldSaiUdfAnnotations) {
  const auto info1 = gutil::ParseProtoOrDie<p4::config::v1::P4Info>(R"pb(
    tables {
      preamble { id: 1 name: "acl_table" }
      match_fields {
        id: 17
        name: "psp_next_header"
        annotations: "@unsupported"
        annotations: "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1)"
        annotation_locations { file: "a.p4" line: 10 }
        bitwidth: 8
        match_type: TERNARY
      }
    }
  )pb");
  const auto info2 = gutil::ParseProtoOrDie<p4::config::v1::P4Info>(R"pb(
    tables {
      preamble { id: 1 name: "acl_table" }
      match_fields {
        id: 17
        name: "psp_next_header"
        annotations: "@unsupported"
        annotations: "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1 , container_select = SAI_UDF_CONTAINER_SELECT_EXT_CONT_ID2)"
        annotation_locations { file: "b.p4" line: 20 }
        bitwidth: 8
        match_type: TERNARY
      }
    }
  )pb");

  ASSERT_OK_AND_ASSIGN(p4::config::v1::P4Info unioned_12,
                       UnionP4info({info1, info2}));
  EXPECT_THAT(
      unioned_12.tables(0).match_fields(0).annotations(),
      ElementsAre("@unsupported",
                  "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1)",
                  "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1 , "
                  "container_select = SAI_UDF_CONTAINER_SELECT_EXT_CONT_ID2)"));

  // Output order follows input order.
  ASSERT_OK_AND_ASSIGN(p4::config::v1::P4Info unioned_21,
                       UnionP4info({info2, info1}));
  EXPECT_THAT(
      unioned_21.tables(0).match_fields(0).annotations(),
      ElementsAre(
          "@unsupported",
          "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1 , "
          "container_select = SAI_UDF_CONTAINER_SELECT_EXT_CONT_ID2)",
          "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1)"));
}

TEST(P4InfoUnionTest, RejectsIncompatibleMatchFields) {
  auto make_info = [](absl::string_view match_field_pb) {
    return gutil::ParseProtoOrDie<p4::config::v1::P4Info>(absl::Substitute(
        R"pb(
          tables {
            preamble { id: 1 name: "acl_table" }
            match_fields { $0 }
          }
        )pb",
        match_field_pb));
  };

  const p4::config::v1::P4Info base = make_info(R"pb(
    id: 17
    name: "psp_next_header"
    annotations: "@sai_udf(base = SAI_UDF_BASE_L4 , offset = 8 , length = 1)"
    bitwidth: 8
    match_type: TERNARY
  )pb");

  // Non-`@sai_udf` annotations must match identically.
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     annotations: "@unsupported"
                     bitwidth: 8
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("@unsupported")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     annotations: "@format(IPV4_ADDRESS)"
                     bitwidth: 8
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("@format")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     annotations: "@refers_to(other_table, f)"
                     bitwidth: 8
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("@refers_to")));
  EXPECT_THAT(UnionP4info({base, make_info(R"pb(
                             id: 17
                             name: "psp_next_header"
                             annotations: "@referenced_by(caller_table, g)"
                             bitwidth: 8
                             match_type: TERNARY
                           )pb")}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("@referenced_by")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     annotations: "@sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL)"
                     bitwidth: 8
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("@sai_field")));

  // Other proto fields must match identically.
  EXPECT_THAT(UnionP4info({base, make_info(R"pb(
                             id: 17
                             name: "other_name"
                             bitwidth: 8
                             match_type: TERNARY
                           )pb")}),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("name")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     bitwidth: 16
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("bitwidth")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     bitwidth: 8
                     match_type: OPTIONAL
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("match_type")));
  EXPECT_THAT(UnionP4info({base, make_info(R"pb(
                             id: 17
                             name: "psp_next_header"
                             doc { brief: "different" }
                             bitwidth: 8
                             match_type: TERNARY
                           )pb")}),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("doc")));
  EXPECT_THAT(
      UnionP4info({base, make_info(R"pb(
                     id: 17
                     name: "psp_next_header"
                     type_name { name: "custom_t" }
                     bitwidth: 8
                     match_type: TERNARY
                   )pb")}),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("type_name")));
  EXPECT_THAT(UnionP4info({base, make_info(R"pb(
                             id: 17
                             name: "psp_next_header"
                             structured_annotations { name: "custom" }
                             bitwidth: 8
                             match_type: TERNARY
                           )pb")}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("structured_annotations")));
}

}  // namespace
}  // namespace pdpi
