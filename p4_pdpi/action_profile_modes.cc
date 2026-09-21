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
#include "gutil/status.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"
#include "p4_pdpi/annotation_parser.h"

namespace pdpi {
namespace {

constexpr absl::string_view kAnnotationLabel = "group_mode_resource_guarantee";

// Keys of the annotation's key-value pairs. They mirror the field names of
// `ActionProfileMode` and `p4::config::v1::ActionProfile`.
enum class Key {
  kActionSelectionMode,
  kSizeSemantics,
  kMaxMemberWeight,
  kMemberUsageMultiplier,
};

// All values of `Key`. Keep in sync with the enum above; only used so that
// `ParseStringToKey` does not need a second list of key spellings.
constexpr Key kAllKeys[] = {
    Key::kActionSelectionMode,
    Key::kSizeSemantics,
    Key::kMaxMemberWeight,
    Key::kMemberUsageMultiplier,
};

// Returns the spelling of `key` as it appears in the annotation.
constexpr absl::string_view KeyToString(Key key) {
  switch (key) {
    case Key::kActionSelectionMode:
      return "action_selection_mode";
    case Key::kSizeSemantics:
      return "size_semantics";
    case Key::kMaxMemberWeight:
      return "max_member_weight";
    case Key::kMemberUsageMultiplier:
      return "member_usage_multiplier";
  }
  // Unreachable: `Key` values are only ever produced by `ParseStringToKey`.
  return "";
}

absl::StatusOr<Key> ParseStringToKey(absl::string_view key) {
  for (Key candidate : kAllKeys) {
    if (key == KeyToString(candidate)) return candidate;
  }
  return gutil::InvalidArgumentErrorBuilder() << "unknown key: '" << key << "'";
}

// Values of the `size_semantics` key.
constexpr absl::string_view kSumOfWeights = "sum_of_weights";
constexpr absl::string_view kSumOfMembers = "sum_of_members";

// Parses a single `<key> = <value>` argument of the annotation body.
absl::StatusOr<std::pair<Key, absl::string_view>> ParseKeyValue(
    absl::string_view arg) {
  std::vector<absl::string_view> key_and_value =
      absl::StrSplit(arg, absl::MaxSplits('=', 1));
  if (key_and_value.size() != 2) {
    return gutil::InvalidArgumentErrorBuilder()
           << "expected argument of the form '<key> = <value>', but got '"
           << arg << "'";
  }
  ASSIGN_OR_RETURN(
      const Key key,
      ParseStringToKey(absl::StripAsciiWhitespace(key_and_value[0])));
  return std::make_pair(key, absl::StripAsciiWhitespace(key_and_value[1]));
}

absl::StatusOr<int32_t> ParseInt32(Key key, absl::string_view value) {
  int32_t parsed_value;
  if (!absl::SimpleAtoi(value, &parsed_value)) {
    return gutil::InvalidArgumentErrorBuilder()
           << "expected an integer value for key '" << KeyToString(key)
           << "', but got '" << value << "'";
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
    switch (key) {
      case Key::kActionSelectionMode: {
        if (value == "HASH") {
          mode.set_action_selection_mode(ActionProfileMode::HASH);
        } else if (value == "RANDOM") {
          mode.set_action_selection_mode(ActionProfileMode::RANDOM);
        } else {
          return gutil::InvalidArgumentErrorBuilder()
                 << "unknown '" << KeyToString(key) << "' value: '" << value
                 << "'; expected 'HASH' or 'RANDOM'";
        }
        break;
      }
      case Key::kSizeSemantics: {
        if (value == kSumOfWeights) {
          mode.mutable_sum_of_weights();
        } else if (value == kSumOfMembers) {
          mode.mutable_sum_of_members();
        } else {
          return gutil::InvalidArgumentErrorBuilder()
                 << "unknown '" << KeyToString(key) << "' value: '" << value
                 << "'; expected '" << kSumOfWeights << "' or '"
                 << kSumOfMembers << "'";
        }
        break;
      }
      case Key::kMaxMemberWeight: {
        ASSIGN_OR_RETURN(max_member_weight, ParseInt32(key, value));
        break;
      }
      case Key::kMemberUsageMultiplier: {
        ASSIGN_OR_RETURN(int32_t multiplier, ParseInt32(key, value));
        mode.mutable_resource_usage_multipliers()->set_member_usage_multiplier(
            multiplier);
        break;
      }
    }
  }

  if (max_member_weight.has_value()) {
    if (!mode.has_sum_of_members()) {
      return gutil::InvalidArgumentErrorBuilder()
             << "'" << KeyToString(Key::kMaxMemberWeight) << "' requires '"
             << KeyToString(Key::kSizeSemantics) << " = " << kSumOfMembers
             << "'";
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
    modes.push_back(mode);
  }

  return modes;
}

}  // namespace pdpi
