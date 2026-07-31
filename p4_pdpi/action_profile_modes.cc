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

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/util/message_differencer.h"
#include "gutil/status.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"
#include "p4_pdpi/annotation_parser.h"

namespace pdpi {
namespace {

constexpr absl::string_view kAnnotationLabel = "group_mode_resource_guarantee";

// Keys of the annotation's key-value pairs. They mirror the field names of
// `ActionProfileMode` and `p4::config::v1::ActionProfile`.
constexpr absl::string_view kActionSelectionModeKey = "action_selection_mode";
constexpr absl::string_view kSizeSemanticsKey = "size_semantics";
constexpr absl::string_view kMaxMemberWeightKey = "max_member_weight";
constexpr absl::string_view kMemberUsageMultiplierKey =
    "member_usage_multiplier";

// Values of the `size_semantics` key.
constexpr absl::string_view kSumOfWeights = "sum_of_weights";
constexpr absl::string_view kSumOfMembers = "sum_of_members";

// Parses a single `<key> = <value>` argument of the annotation body.
absl::StatusOr<std::pair<absl::string_view, absl::string_view>> ParseKeyValue(
    absl::string_view arg) {
  std::vector<absl::string_view> key_and_value =
      absl::StrSplit(arg, absl::MaxSplits('=', 1));
  if (key_and_value.size() != 2) {
    return gutil::InvalidArgumentErrorBuilder()
           << "expected argument of the form '<key> = <value>', but got '"
           << arg << "'";
  }
  return std::make_pair(absl::StripAsciiWhitespace(key_and_value[0]),
                        absl::StripAsciiWhitespace(key_and_value[1]));
}

absl::StatusOr<int32_t> ParseInt32(absl::string_view key,
                                   absl::string_view value) {
  int32_t parsed_value;
  if (!absl::SimpleAtoi(value, &parsed_value)) {
    return gutil::InvalidArgumentErrorBuilder()
           << "expected an integer value for key '" << key << "', but got '"
           << value << "'";
  }
  return parsed_value;
}

// Parses a single mode from the argument list of one annotation, e.g.
// {"action_selection_mode = HASH", "size_semantics = sum_of_members",
//  "max_member_weight = 4095"}.
absl::StatusOr<ActionProfileMode> ParseMode(
    absl::Span<const std::string> args) {
  ActionProfileMode mode;
  std::optional<int32_t> max_member_weight;
  for (absl::string_view arg : args) {
    // NOTE: The structured binding is declared outside of `ASSIGN_OR_RETURN`
    // because the open-source version of the macro does not support
    // parenthesized structured binding declarations.
    ASSIGN_OR_RETURN(const auto key_value, ParseKeyValue(arg));
    const auto& [key, value] = key_value;
    if (key == kActionSelectionModeKey) {
      if (value == "HASH") {
        mode.set_action_selection_mode(ActionProfileMode::HASH);
      } else if (value == "RANDOM") {
        mode.set_action_selection_mode(ActionProfileMode::RANDOM);
      } else {
        return gutil::InvalidArgumentErrorBuilder()
               << "unknown '" << kActionSelectionModeKey << "' value: '"
               << value << "'; expected 'HASH' or 'RANDOM'";
      }
    } else if (key == kSizeSemanticsKey) {
      if (value == kSumOfWeights) {
        mode.mutable_sum_of_weights();
      } else if (value == kSumOfMembers) {
        mode.mutable_sum_of_members();
      } else {
        return gutil::InvalidArgumentErrorBuilder()
               << "unknown '" << kSizeSemanticsKey << "' value: '" << value
               << "'; expected '" << kSumOfWeights << "' or '" << kSumOfMembers
               << "'";
      }
    } else if (key == kMaxMemberWeightKey) {
      ASSIGN_OR_RETURN(max_member_weight, ParseInt32(key, value));
    } else if (key == kMemberUsageMultiplierKey) {
      ASSIGN_OR_RETURN(int32_t multiplier, ParseInt32(key, value));
      mode.mutable_resource_usage_multipliers()->set_member_usage_multiplier(
          multiplier);
    } else {
      return gutil::InvalidArgumentErrorBuilder()
             << "unknown key: '" << key << "'";
    }
  }

  if (max_member_weight.has_value()) {
    if (!mode.has_sum_of_members()) {
      return gutil::InvalidArgumentErrorBuilder()
             << "'" << kMaxMemberWeightKey << "' requires '"
             << kSizeSemanticsKey << " = " << kSumOfMembers << "'";
    }
    mode.mutable_sum_of_members()->set_max_member_weight(*max_member_weight);
  }
  return mode;
}

}  // namespace

absl::StatusOr<std::vector<ActionProfileMode>>
ParseRequiredModesFromActionProfile(
    const p4::config::v1::ActionProfile& action_profile) {
  ASSIGN_OR_RETURN(
      std::vector<std::vector<std::string>> annotation_arg_lists,
      GetAllAnnotationsAsArgList(kAnnotationLabel,
                                 action_profile.preamble().annotations()));

  std::vector<ActionProfileMode> modes;
  modes.reserve(annotation_arg_lists.size());
  for (const std::vector<std::string>& args : annotation_arg_lists) {
    ASSIGN_OR_RETURN(ActionProfileMode mode, ParseMode(args),
                     _.SetPrepend()
                         << "failed to parse @" << kAnnotationLabel
                         << " annotation of ActionProfile '"
                         << action_profile.preamble().name() << "': ");
    for (const ActionProfileMode& existing_mode : modes) {
      if (google::protobuf::util::MessageDifferencer::Equals(mode,
                                                             existing_mode)) {
        return gutil::InvalidArgumentErrorBuilder()
               << "found duplicate action profile mode in ActionProfile '"
               << action_profile.preamble().name()
               << "': " << mode.ShortDebugString();
      }
    }
    modes.push_back(mode);
  }

  return modes;
}

}  // namespace pdpi
