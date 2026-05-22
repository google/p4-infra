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

#include "p4_pdpi/pd.h"

#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/proto.h"
#include "gutil/status_matchers.h"
#include "p4/v1/p4runtime.pb.h"
#include "p4_pdpi/ir.pb.h"
#include "p4_pdpi/main_p4_pd.pb.h"

namespace pdpi {
namespace {

using ::gutil::IsOk;
using ::gutil::IsOkAndHolds;
using ::testing::HasSubstr;
using ::testing::Not;

TEST(PdTest, ShortDescriptionReturnsDescriptionForValidEntry) {
  ASSERT_OK_AND_ASSIGN(TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
                         exact_table_entry {
                           match { normal: "0x054" }
                           action { NoAction {} }
                         }
                       )pb"));

  ASSERT_OK_AND_ASSIGN(std::string desc, ShortDescription(entry));

  EXPECT_THAT(desc, HasSubstr("exact_table_entry"));
  EXPECT_THAT(desc, HasSubstr("normal: \"0x054\""));
}

TEST(PdTest, ShortDescriptionReturnsDescriptionForValidEntryWithCounters) {
  ASSERT_OK_AND_ASSIGN(TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
                         count_and_meter_table_entry {
                           # Skip match and action.
                           counter_data { byte_count: 100 packet_count: 200 }
                           meter_counter_data {
                             green { byte_count: 1000 packet_count: 2000 }
                             yellow { byte_count: 3000 packet_count: 4000 }
                             red { byte_count: 5000 packet_count: 6000 }
                           }
                           controller_metadata: "some_value"
                         }
                       )pb"));

  ASSERT_OK_AND_ASSIGN(std::string desc, ShortDescription(entry));

  // The table name is included in the description.
  EXPECT_THAT(desc, HasSubstr("count_and_meter_table_entry"));

  // None of the counter or controller metadata pieces are included.
  EXPECT_THAT(desc, Not(HasSubstr("counter_data")));
  EXPECT_THAT(desc, Not(HasSubstr("packet_count")));
  EXPECT_THAT(desc, Not(HasSubstr("meter_counter_data")));
  EXPECT_THAT(desc, Not(HasSubstr("green")));
  EXPECT_THAT(desc, Not(HasSubstr("controller_metadata")));
  EXPECT_THAT(desc, Not(HasSubstr("some_value")));
}

TEST(PdTest, ShortDescriptionReturnsErrorForWrongMessageType) {
  IrTableEntry entry;
  entry.set_table_name("test_table");

  EXPECT_THAT(ShortDescription(entry), Not(IsOk()));
}

TEST(PdTest, ShortDescriptionReturnsErrorForEmptyEntry) {
  TableEntry entry;

  EXPECT_THAT(ShortDescription(entry), Not(IsOk()));
}

TEST(PdTest, GetCounterDataReturnsDataIfSet) {
  ASSERT_OK_AND_ASSIGN(
      TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
        count_and_meter_table_entry {
          # Skip match and action since they're irrelevant for this test.
          counter_data { byte_count: 100 packet_count: 200 }
          meter_counter_data {
            green { byte_count: 1000 packet_count: 2000 }
            yellow { byte_count: 3000 packet_count: 4000 }
            red { byte_count: 5000 packet_count: 6000 }
          }
        }
      )pb"));

  ASSERT_OK_AND_ASSIGN(
      std::optional<const google::protobuf::Message*> counter_data_msg,
      GetCounterData(entry));
  ASSERT_TRUE(counter_data_msg.has_value());

  const auto& counter_data =
      static_cast<const BytesAndPacketsCounterData&>(**counter_data_msg);
  EXPECT_EQ(counter_data.byte_count(), 100);
  EXPECT_EQ(counter_data.packet_count(), 200);
}

TEST(PdTest, GetCounterDataReturnsNulloptIfUnset) {
  ASSERT_OK_AND_ASSIGN(
      TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
        count_and_meter_table_entry {
          # Skip match and action since they're irrelevant for this test.
          # Skip counter_data, so result is nullopt.
          meter_counter_data {
            green { byte_count: 1000 packet_count: 2000 }
            yellow { byte_count: 3000 packet_count: 4000 }
            red { byte_count: 5000 packet_count: 6000 }
          }
        }
      )pb"));

  EXPECT_THAT(GetCounterData(entry), IsOkAndHolds(std::nullopt));
}

TEST(PdTest, GetCounterDataReturnsNulloptIfTableHasNoCounterSupport) {
  TableEntry entry;
  entry.mutable_exact_table_entry();

  EXPECT_THAT(GetCounterData(entry), IsOkAndHolds(std::nullopt));
}

TEST(PdTest, GetCounterDataReturnsErrorForWrongMessageType) {
  IrTableEntry entry;

  EXPECT_THAT(GetCounterData(entry), Not(IsOk()));
}

TEST(PdTest, GetMeterCounterDataReturnsDataIfSet) {
  ASSERT_OK_AND_ASSIGN(
      TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
        count_and_meter_table_entry {
          # Skip match and action since they're irrelevant for this test.
          counter_data { byte_count: 100 packet_count: 200 }
          meter_counter_data {
            green { byte_count: 1000 packet_count: 2000 }
            yellow { byte_count: 3000 packet_count: 4000 }
            red { byte_count: 5000 packet_count: 6000 }
          }
        }
      )pb"));

  ASSERT_OK_AND_ASSIGN(
      std::optional<const google::protobuf::Message*> meter_counter_data_msg,
      GetMeterCounterData(entry));
  ASSERT_TRUE(meter_counter_data_msg.has_value());

  const auto& meter_counter_data =
      static_cast<const MeterBytesAndPacketsCounterData&>(
          **meter_counter_data_msg);
  EXPECT_EQ(meter_counter_data.green().byte_count(), 1000);
  EXPECT_EQ(meter_counter_data.green().packet_count(), 2000);
  EXPECT_EQ(meter_counter_data.yellow().byte_count(), 3000);
  EXPECT_EQ(meter_counter_data.yellow().packet_count(), 4000);
  EXPECT_EQ(meter_counter_data.red().byte_count(), 5000);
  EXPECT_EQ(meter_counter_data.red().packet_count(), 6000);
}

TEST(PdTest, GetMeterCounterDataReturnsNulloptIfUnset) {
  ASSERT_OK_AND_ASSIGN(
      TableEntry entry, gutil::ParseTextProto<TableEntry>(R"pb(
        count_and_meter_table_entry {
          # Skip match and action since they're irrelevant for this test.
          # Skip meter_counter_data, so result is nullopt.
          counter_data { byte_count: 100 packet_count: 200 }
        }
      )pb"));

  EXPECT_THAT(GetMeterCounterData(entry), IsOkAndHolds(std::nullopt));
}

TEST(PdTest, GetMeterCounterDataReturnsNulloptIfTableHasNoMeterSupport) {
  TableEntry entry;
  entry.mutable_exact_table_entry();

  EXPECT_THAT(GetMeterCounterData(entry), IsOkAndHolds(std::nullopt));
}

TEST(PdTest, GetMeterCounterDataReturnsErrorForWrongMessageType) {
  IrTableEntry entry;

  EXPECT_THAT(GetMeterCounterData(entry), Not(IsOk()));
}

}  // namespace
}  // namespace pdpi
