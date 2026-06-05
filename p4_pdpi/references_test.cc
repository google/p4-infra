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

#include "p4_pdpi/references.h"

#include "absl/hash/hash_testing.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/status_matchers.h"
#include "gutil/testing.h"
#include "p4_pdpi/ir.h"
#include "p4_pdpi/test_p4info.h"
namespace pdpi {
namespace {

using ::gutil::StatusIs;

// -- Concrete Reference Field Test --------------------------------------------

TEST(ConcreteFieldReferenceTest, FieldsWithSrcNameDiffAreNotEqual) {
  EXPECT_NE((ConcreteFieldReference{
                .source_field = "Src1",
                .destination_field = "Dst",
                .value = "v",
            }),
            (ConcreteFieldReference{
                .source_field = "Src2",
                .destination_field = "Dst",
                .value = "v",
            }));
}

TEST(ConcreteFieldReferenceTest, FieldsWithDstNameDiffAreNotEqual) {
  EXPECT_NE((ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst1",
                .value = "v",
            }),
            (ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst2",
                .value = "v",
            }));
}

TEST(ConcreteFieldReferenceTest, FieldsWithValueDiffAreNotEqual) {
  EXPECT_NE((ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst",
                .value = "v1",
            }),
            (ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst",
                .value = "v2",
            }));
}

TEST(ConcreteFieldReferenceTest, IdenticalFieldsAreEqual) {
  EXPECT_EQ((ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst",
                .value = "v",
            }),
            (ConcreteFieldReference{
                .source_field = "Src",
                .destination_field = "Dst",
                .value = "v",
            }));
}

// The hash value check is provided by VerifyTypeImplementsAbslHashCorrectly
// that checks `ReferenceField`s that are equal have the same hash values and
// `ReferenceField`s that are not equal have different hash values.
TEST(ConcreteFieldReferenceTest, HashingTest) {
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      (ConcreteFieldReference{
          .source_field = "Src",
          .destination_field = "Dst",
          .value = "v",
      }),
      (ConcreteFieldReference{
          .source_field = "Src1",
          .destination_field = "Dst",
          .value = "v",
      }),
      (ConcreteFieldReference{
          .source_field = "Src",
          .destination_field = "Dst1",
          .value = "v",
      }),
      (ConcreteFieldReference{
          .source_field = "Src",
          .destination_field = "Dst",
          .value = "v1",
      }),
      (ConcreteFieldReference{
          .source_field = "Src",
          .destination_field = "Dst",
          .value = "v",
      }),
  }));
}

// -- Concrete Reference Entry Test --------------------------------------------

TEST(ConcreteTableReferenceTest, EntriesWithSrcNameDiffAreNotEqual) {
  EXPECT_NE((ConcreteTableReference{
                .source_table = "Src1",
                .destination_table = "Dst",
            }),
            (ConcreteTableReference{
                .source_table = "Src2",
                .destination_table = "Dst",
            }));
}

TEST(ConcreteTableReferenceTest, EntriesWithDstNameDiffAreNotEqual) {
  EXPECT_NE((ConcreteTableReference{
                .source_table = "Src",
                .destination_table = "Dst1",
            }),
            (ConcreteTableReference{
                .source_table = "Src",
                .destination_table = "Dst2",
            }));
}

TEST(ConcreteTableReferenceTest, EntriesWithFieldsDiffAreNotEqual) {
  EXPECT_NE((ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields =
                                        {
                                            ConcreteFieldReference{
                                                .source_field = "Src1",
                                                .destination_field = "Dst1",
                                                .value = "v1",
                                            },
                                        }}),
            (ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields = {
                                        ConcreteFieldReference{
                                            .source_field = "Src2",
                                            .destination_field = "Dst2",
                                            .value = "v2",
                                        },
                                    }}));
}

TEST(ConcreteTableReferenceTest, IdenticalEntriesAreEqual) {
  EXPECT_EQ((ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields =
                                        {
                                            ConcreteFieldReference{
                                                .source_field = "Src1",
                                                .destination_field = "Dst1",
                                                .value = "v1",
                                            },
                                        }}),
            (ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields = {
                                        ConcreteFieldReference{
                                            .source_field = "Src1",
                                            .destination_field = "Dst1",
                                            .value = "v1",
                                        },
                                    }}));
}

TEST(ConcreteTableReferenceTest, EntriesWithSameFieldsInDiffOrderAreEqual) {
  EXPECT_EQ((ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields =
                                        {
                                            ConcreteFieldReference{
                                                .source_field = "Src2",
                                                .destination_field = "Dst2",
                                                .value = "v2",
                                            },
                                            ConcreteFieldReference{
                                                .source_field = "Src1",
                                                .destination_field = "Dst1",
                                                .value = "v1",
                                            },
                                        }}),
            (ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields = {
                                        ConcreteFieldReference{
                                            .source_field = "Src1",
                                            .destination_field = "Dst1",
                                            .value = "v1",
                                        },
                                        ConcreteFieldReference{
                                            .source_field = "Src2",
                                            .destination_field = "Dst2",
                                            .value = "v2",
                                        },
                                    }}));
}

TEST(ConcreteTableReferenceTest, EntriesWithSubsetFieldsAreNotEqual) {
  EXPECT_NE((ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields =
                                        {
                                            ConcreteFieldReference{
                                                .source_field = "Src1",
                                                .destination_field = "Dst1",
                                                .value = "v1",
                                            },
                                        }}),
            (ConcreteTableReference{.source_table = "Src",
                                    .destination_table = "Dst",
                                    .fields = {
                                        ConcreteFieldReference{
                                            .source_field = "Src1",
                                            .destination_field = "Dst1",
                                            .value = "v1",
                                        },
                                        ConcreteFieldReference{
                                            .source_field = "Src2",
                                            .destination_field = "Dst2",
                                            .value = "v2",
                                        },
                                    }}));
}

// The hash value check is provided by VerifyTypeImplementsAbslHashCorrectly
// that checks `ReferenceField`s that are equal have the same hash values and
// `ReferenceField`s that are not equal have different hash values.
TEST(ConcreteTableReferenceTest, HashingTest) {
  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly(
      {(ConcreteTableReference{.source_table = "Src",
                               .destination_table = "Dst",
                               .fields =
                                   {
                                       ConcreteFieldReference{
                                           .source_field = "Src1",
                                           .destination_field = "Dst1",
                                           .value = "v1",
                                       },
                                   }}),
       (ConcreteTableReference{.source_table = "Src",
                               .destination_table = "Dst",
                               .fields =
                                   {
                                       ConcreteFieldReference{
                                           .source_field = "Src2",
                                           .destination_field = "Dst2",
                                           .value = "v1",
                                       },
                                   }}),
       (ConcreteTableReference{.source_table = "Src",
                               .destination_table = "Dst",
                               .fields =
                                   {
                                       ConcreteFieldReference{
                                           .source_field = "Src1",
                                           .destination_field = "Dst1",
                                           .value = "v1",
                                       },
                                       ConcreteFieldReference{
                                           .source_field = "Src2",
                                           .destination_field = "Dst2",
                                           .value = "v1",
                                       },
                                   }}),
       (ConcreteTableReference{.source_table = "Src",
                               .destination_table = "Dst",
                               .fields =
                                   {
                                       ConcreteFieldReference{
                                           .source_field = "Src2",
                                           .destination_field = "Dst2",
                                           .value = "v1",
                                       },
                                       ConcreteFieldReference{
                                           .source_field = "Src1",
                                           .destination_field = "Dst1",
                                           .value = "v1",
                                       },
                                   }}),
       (ConcreteTableReference{.source_table = "OtherSrc",
                               .destination_table = "Dst",
                               .fields =
                                   {
                                       ConcreteFieldReference{
                                           .source_field = "Src2",
                                           .destination_field = "Dst2",
                                           .value = "v1",
                                       },
                                       ConcreteFieldReference{
                                           .source_field = "Src1",
                                           .destination_field = "Dst1",
                                           .value = "v1",
                                       },
                                   }}),
       (ConcreteTableReference{.source_table = "Src",
                               .destination_table = "OtherDst",
                               .fields = {
                                   ConcreteFieldReference{
                                       .source_field = "Src2",
                                       .destination_field = "Dst2",
                                       .value = "v1",
                                   },
                                   ConcreteFieldReference{
                                       .source_field = "Src1",
                                       .destination_field = "Dst1",
                                       .value = "v1",
                                   },
                               }})}));
}

// Helper to construct a table entry that can be referred to by entry
// constructed by `MakeReferringByActionTableEntry`.
p4::v1::Entity MakeOneMatchFieldTableEntry(absl::string_view referred_to_key) {
  return *IrEntityToPi(GetTestIrP4Info(),
                       gutil::ParseProtoOrDie<pdpi::IrEntity>(absl::Substitute(
                           R"pb(
                             table_entry {
                               table_name: "one_match_field_table"
                               matches {
                                 name: "id"
                                 exact { str: "$0" }
                               }
                               action { name: "do_thing_4" }
                             }
                           )pb",
                           referred_to_key)));
}

// Helper to construct a table entry that refers to entry constructed by
// `MakeOneMatchFieldTableEntry`.
p4::v1::Entity MakeReferringByActionTableEntry(
    absl::string_view key, absl::string_view referring_param) {
  return *IrEntityToPi(GetTestIrP4Info(),
                       gutil::ParseProtoOrDie<pdpi::IrEntity>(absl::Substitute(
                           R"pb(
                             table_entry {
                               table_name: "referring_by_action_table"
                               matches {
                                 name: "val"
                                 exact { hex_str: "$0" }
                               }
                               action {
                                 name: "referring_to_one_match_field_action"
                                 params {
                                   name: "referring_id_1"
                                   value { str: "$1" }
                                 }
                               }
                             }
                           )pb",
                           key, referring_param)));
}

// Helper to construct a table entry for `two_match_fields_table`.
p4::v1::Entity MakeTwoMatchFieldsTableEntry(absl::string_view id_1,
                                            absl::string_view id_2) {
  return *IrEntityToPi(GetTestIrP4Info(),
                       gutil::ParseProtoOrDie<pdpi::IrEntity>(absl::Substitute(
                           R"pb(
                             table_entry {
                               table_name: "two_match_fields_table"
                               matches {
                                 name: "id_1"
                                 exact { str: "$0" }
                               }
                               matches {
                                 name: "id_2"
                                 exact { hex_str: "$1" }
                               }
                               action { name: "do_thing_4" }
                             }
                           )pb",
                           id_1, id_2)));
}

// Helper to construct a table entry for `referring_by_match_field_table`.
p4::v1::Entity MakeReferringByMatchFieldTableEntry(
    absl::string_view referring_id_1, absl::string_view referring_id_2) {
  return *IrEntityToPi(GetTestIrP4Info(),
                       gutil::ParseProtoOrDie<pdpi::IrEntity>(absl::Substitute(
                           R"pb(
                             table_entry {
                               table_name: "referring_by_match_field_table"
                               matches {
                                 name: "referring_id_1"
                                 exact { str: "$0" }
                               }
                               matches {
                                 name: "referring_id_2"
                                 exact { hex_str: "$1" }
                               }
                               action { name: "do_thing_4" }
                             }
                           )pb",
                           referring_id_1, referring_id_2)));
}

// Helper to construct a table entry for
// `referring_to_referring_by_match_field_table`.
p4::v1::Entity MakeReferringToReferringByMatchFieldTableEntry(
    absl::string_view referring_to_referring_id_1) {
  return *IrEntityToPi(
      GetTestIrP4Info(),
      gutil::ParseProtoOrDie<pdpi::IrEntity>(absl::Substitute(
          R"pb(
            table_entry {
              table_name: "referring_to_referring_by_match_field_table"
              matches {
                name: "referring_to_referring_id_1"
                exact { str: "$0" }
              }
              action { name: "do_thing_4" }
            }
          )pb",
          referring_to_referring_id_1)));
}

TEST(StatefulReferenceCheckerTest, ReturnsAlreadyExistsOnDuplicateAdd) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  ASSERT_OK(checker.AddEntity(entity));

  EXPECT_THAT(checker.AddEntity(entity),
              StatusIs(absl::StatusCode::kAlreadyExists));
}

TEST(StatefulReferenceCheckerTest, ReturnsNotFoundOnMissingRemoveAndUpdate) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"0x001");

  EXPECT_THAT(checker.RemoveEntity(entity),
              StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(checker.UpdateEntity(entity),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(StatefulReferenceCheckerTest,
     AddEntityReturnsFailedPreconditionOnMissingReferences) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity referenced_entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  p4::v1::Entity referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-a");

  // Returns FailedPrecondition error on missing references
  EXPECT_THAT(checker.AddEntity(referring_entity),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  // Add the referenced entity, now the addition succeeds
  ASSERT_OK(checker.AddEntity(referenced_entity));
  EXPECT_OK(checker.AddEntity(referring_entity));

  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest,
     RemoveEntityReturnsFailedPreconditionOnInUseReferences) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity referenced_entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  p4::v1::Entity referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-a");
  ASSERT_OK(checker.AddEntity(referenced_entity));
  ASSERT_OK(checker.AddEntity(referring_entity));

  // Return FailedPrecondition when removing referenced entity!
  EXPECT_THAT(checker.RemoveEntity(referenced_entity),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  // Removing referring entity succeeds
  ASSERT_OK(checker.RemoveEntity(referring_entity));
  EXPECT_OK(checker.RemoveEntity(referenced_entity));

  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest,
     UpdateEntitySucceedsOrReturnsFailedPrecondition) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity referenced_entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  p4::v1::Entity referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-a");
  ASSERT_OK(checker.AddEntity(referenced_entity));
  ASSERT_OK(checker.AddEntity(referring_entity));

  // Try to update to refer to non-existent key-b: should return
  // FailedPrecondition
  p4::v1::Entity modified_referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-b");
  ASSERT_THAT(checker.UpdateEntity(modified_referring_entity),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  // Now add destination key-b
  p4::v1::Entity new_referenced_entity = MakeOneMatchFieldTableEntry(
      /*referred_to_key=*/"key-b");
  ASSERT_OK(checker.AddEntity(new_referenced_entity));

  // Now retry update: should succeed!
  EXPECT_OK(checker.UpdateEntity(modified_referring_entity));

  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest, CannotAddReferrerAfterRemovingReferee) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity referenced_entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  p4::v1::Entity referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-a");

  ASSERT_OK(checker.AddEntity(referenced_entity));
  ASSERT_OK(checker.RemoveEntity(referenced_entity));

  EXPECT_THAT(checker.AddEntity(referring_entity),
              StatusIs(absl::StatusCode::kFailedPrecondition));
  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest, CannotRemoveEntityTwice) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity entity =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");

  ASSERT_OK(checker.AddEntity(entity));
  ASSERT_OK(checker.RemoveEntity(entity));

  EXPECT_THAT(checker.RemoveEntity(entity),
              StatusIs(absl::StatusCode::kNotFound));
  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest, CanRemoveRefereeAfterModifyingAwayReferrer) {
  StatefulReferenceChecker checker(GetTestIrP4Info());
  p4::v1::Entity referenced_entity_a =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-a");
  p4::v1::Entity referenced_entity_b =
      MakeOneMatchFieldTableEntry(/*referred_to_key=*/"key-b");
  p4::v1::Entity referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-a");

  ASSERT_OK(checker.AddEntity(referenced_entity_a));
  ASSERT_OK(checker.AddEntity(referenced_entity_b));
  ASSERT_OK(checker.AddEntity(referring_entity));

  // Cannot remove A because it is referenced.
  ASSERT_THAT(checker.RemoveEntity(referenced_entity_a),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  // Update referrer to point to B instead of A.
  p4::v1::Entity modified_referring_entity = MakeReferringByActionTableEntry(
      /*key=*/"0x001", /*referring_param=*/"key-b");
  ASSERT_OK(checker.UpdateEntity(modified_referring_entity));

  // Now we should be able to remove A.
  EXPECT_OK(checker.RemoveEntity(referenced_entity_a));
  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest,
     RedundantReferenceRemovalDoesNotImpactReference) {
  StatefulReferenceChecker checker(GetTestIrP4Info());

  // We need to satisfy references for referring_by_match_field_table.
  // referring_by_match_field_table references two_match_fields_table.
  p4::v1::Entity referee_1 = MakeTwoMatchFieldsTableEntry("foo", "0x001");
  p4::v1::Entity referee_2 = MakeTwoMatchFieldsTableEntry("foo", "0x002");

  // Entries in referring_by_match_field_table that satisfy the same reference
  // (referring_id_1 = "foo") for referring_to_referring_by_match_field_table.
  p4::v1::Entity referrer_shared_1 =
      MakeReferringByMatchFieldTableEntry("foo", "0x001");
  p4::v1::Entity referrer_shared_2 =
      MakeReferringByMatchFieldTableEntry("foo", "0x002");

  // Entry that uses the shared reference.
  p4::v1::Entity final_referrer =
      MakeReferringToReferringByMatchFieldTableEntry("foo");

  // Add all referees and referrers.
  ASSERT_OK(checker.AddEntity(referee_1));
  ASSERT_OK(checker.AddEntity(referee_2));
  ASSERT_OK(checker.AddEntity(referrer_shared_1));
  ASSERT_OK(checker.AddEntity(referrer_shared_2));

  // Remove one of the sharing referrers.
  // This should not make the reference "foo" invalid because referrer_shared_2
  // still exists.
  ASSERT_OK(checker.RemoveEntity(referrer_shared_1));

  // Add the final referrer that uses the "foo" reference.
  // This should succeed.
  EXPECT_OK(checker.AddEntity(final_referrer));

  EXPECT_OK(checker.CheckInvariantsForTesting());
}

TEST(StatefulReferenceCheckerTest, RemovingRedundantSatisfyingEntrySucceeds) {
  StatefulReferenceChecker checker(GetTestIrP4Info());

  // We need to satisfy references for referring_by_match_field_table.
  p4::v1::Entity referee_1 = MakeTwoMatchFieldsTableEntry("foo", "0x001");
  p4::v1::Entity referee_2 = MakeTwoMatchFieldsTableEntry("foo", "0x002");

  // Entries in referring_by_match_field_table that satisfy the same reference
  // (referring_id_1 = "foo") for referring_to_referring_by_match_field_table.
  p4::v1::Entity referrer_shared_1 =
      MakeReferringByMatchFieldTableEntry("foo", "0x001");
  p4::v1::Entity referrer_shared_2 =
      MakeReferringByMatchFieldTableEntry("foo", "0x002");

  // Entry that uses the shared reference.
  p4::v1::Entity final_referrer =
      MakeReferringToReferringByMatchFieldTableEntry("foo");

  // Add all.
  ASSERT_OK(checker.AddEntity(referee_1));
  ASSERT_OK(checker.AddEntity(referee_2));
  ASSERT_OK(checker.AddEntity(referrer_shared_1));
  ASSERT_OK(checker.AddEntity(referrer_shared_2));
  ASSERT_OK(checker.AddEntity(final_referrer));

  // Remove one of the satisfying entries.
  // This should succeed because referrer_shared_2 still exists to satisfy
  // "foo".
  EXPECT_OK(checker.RemoveEntity(referrer_shared_1));

  // Removing the other satisfying entry should fail because final_referrer
  // still relies on "foo".
  EXPECT_THAT(checker.RemoveEntity(referrer_shared_2),
              StatusIs(absl::StatusCode::kFailedPrecondition));

  EXPECT_OK(checker.CheckInvariantsForTesting());
}

}  // namespace
}  // namespace pdpi
