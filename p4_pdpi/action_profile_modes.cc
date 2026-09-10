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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/util/message_differencer.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"
#include "p4_pdpi/annotation_parser.h"

namespace pdpi {
namespace {

std::string CleanToken(absl::string_view s) {
  s = absl::StripAsciiWhitespace(s);
  while (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                           (s.front() == '{' && s.back() == '}') ||
                           (s.front() == '\'' && s.back() == '\''))) {
    s = s.substr(1, s.size() - 2);
    s = absl::StripAsciiWhitespace(s);
  }
  return std::string(s);
}

void ParseSizeSemantics(absl::string_view raw_val, ActionProfileMode& mode) {
  std::string cleaned = CleanToken(raw_val);
  if (cleaned.empty()) return;

  absl::StatusOr<std::vector<std::string>> parts =
      pdpi::annotation::ParseAsArgList(
          absl::StrReplaceAll(cleaned, {{":", ","}}));
  if (!parts.ok() || parts->empty()) {
    parts = std::vector<std::string>{cleaned};
  }

  for (size_t i = 0; i < parts->size(); ++i) {
    std::string part = std::string(absl::StripAsciiWhitespace((*parts)[i]));
    size_t eq_pos = part.find('=');
    if (eq_pos != std::string::npos) {
      std::string key = CleanToken(part.substr(0, eq_pos));
      std::string val = CleanToken(part.substr(eq_pos + 1));
      if (key == "semantics" || key == "size_semantics") {
        std::string upper_val = absl::AsciiStrToUpper(val);
        if (upper_val == "SUM_OF_WEIGHTS") {
          mode.mutable_sum_of_weights();
        } else if (upper_val == "SUM_OF_MEMBERS") {
          mode.mutable_sum_of_members();
        }
      } else if (key == "max_weight" || key == "max_member_weight") {
        int64_t w;
        if (absl::SimpleAtoi(val, &w)) {
          if (!mode.has_sum_of_weights() && !mode.has_sum_of_members()) {
            mode.mutable_sum_of_members();
          }
          if (mode.has_sum_of_members()) {
            mode.mutable_sum_of_members()->set_max_member_weight(w);
          }
        }
      } else if (key == "member_multiplier" ||
                 key == "member_usage_multiplier") {
        int64_t mult;
        if (absl::SimpleAtoi(val, &mult)) {
          mode.mutable_resource_usage_multipliers()
              ->set_member_usage_multiplier(mult);
        }
      }
    } else {
      std::string token = CleanToken(part);
      std::string upper_token = absl::AsciiStrToUpper(token);
      if (upper_token == "SUM_OF_WEIGHTS") {
        mode.mutable_sum_of_weights();
      } else if (upper_token == "SUM_OF_MEMBERS") {
        mode.mutable_sum_of_members();
      } else if (i > 0) {
        int64_t val;
        if (absl::SimpleAtoi(token, &val)) {
          if (mode.has_sum_of_members()) {
            if (val > 100) {
              mode.mutable_sum_of_members()->set_max_member_weight(val);
            } else {
              mode.mutable_resource_usage_multipliers()
                  ->set_member_usage_multiplier(val);
            }
          }
        }
      }
    }
  }
}

ActionProfileMode ParseActionProfileModeFromKvList(
    const p4::config::v1::KeyValuePairList& kv_list) {
  ActionProfileMode mode;
  for (const auto& kv : kv_list.kv_pairs()) {
    if (kv.key() == "action_selection_mode") {
      std::string val =
          absl::AsciiStrToUpper(CleanToken(kv.value().string_value()));
      if (val == "HASH") {
        mode.set_action_selection_mode(ActionProfileMode::HASH);
      } else if (val == "RANDOM") {
        mode.set_action_selection_mode(ActionProfileMode::RANDOM);
      }
    } else if (kv.key() == "size_semantics") {
      ParseSizeSemantics(kv.value().string_value(), mode);
    } else if (kv.key() == "sum_of_weights") {
      mode.mutable_sum_of_weights();
    } else if (kv.key() == "sum_of_members") {
      mode.mutable_sum_of_members();
    } else if (kv.key() == "member_multiplier" ||
               kv.key() == "member_usage_multiplier") {
      if (kv.value().has_int64_value()) {
        mode.mutable_resource_usage_multipliers()->set_member_usage_multiplier(
            kv.value().int64_value());
      }
    } else if (kv.key() == "max_member_weight" || kv.key() == "max_weight") {
      if (kv.value().has_int64_value()) {
        if (!mode.has_sum_of_weights() && !mode.has_sum_of_members()) {
          mode.mutable_sum_of_members();
        }
        if (mode.has_sum_of_members()) {
          mode.mutable_sum_of_members()->set_max_member_weight(
              kv.value().int64_value());
        }
      }
    }
  }
  return mode;
}

std::vector<ActionProfileMode> ParseActionProfileModesFromExpressionList(
    const p4::config::v1::ExpressionList& expression_list) {
  std::vector<ActionProfileMode> modes;
  const auto& exprs = expression_list.expressions();

  ActionProfileMode mode;
  for (size_t i = 0; i < exprs.size(); i += 3) {
    mode.Clear();

    std::string action_selection_mode =
        absl::AsciiStrToUpper(CleanToken(exprs[i].string_value()));
    if (action_selection_mode == "HASH") {
      mode.set_action_selection_mode(ActionProfileMode::HASH);
    } else if (action_selection_mode == "RANDOM") {
      mode.set_action_selection_mode(ActionProfileMode::RANDOM);
    }

    if (i + 1 < exprs.size()) {
      ParseSizeSemantics(exprs[i + 1].string_value(), mode);
    }

    if (i + 2 < exprs.size()) {
      int64_t val;
      if (absl::SimpleAtoi(CleanToken(exprs[i + 2].string_value()), &val)) {
        if (mode.has_sum_of_members()) {
          mode.mutable_sum_of_members()->set_max_member_weight(val);
        } else if (mode.has_sum_of_weights()) {
          mode.mutable_resource_usage_multipliers()
              ->set_member_usage_multiplier(val);
        }
      }
    }

    modes.push_back(mode);
  }

  return modes;
}

}  // namespace

absl::StatusOr<std::vector<ActionProfileMode>>
ParseRequiredModesFromActionProfile(
    const p4::config::v1::ActionProfile& action_profile) {
  std::vector<ActionProfileMode> modes;

  for (const auto& sa : action_profile.preamble().structured_annotations()) {
    if (sa.name() != "group_mode") {
      continue;
    }
    if (sa.has_kv_pair_list()) {
      modes.push_back(ParseActionProfileModeFromKvList(sa.kv_pair_list()));
    } else if (sa.has_expression_list()) {
      std::vector<ActionProfileMode> parsed_modes =
          ParseActionProfileModesFromExpressionList(sa.expression_list());
      modes.insert(modes.end(), parsed_modes.begin(), parsed_modes.end());
    }
  }

  if (auto bodies = GetAllAnnotationBodies(
          "group_mode", action_profile.preamble().annotations());
      bodies.ok()) {
    p4::config::v1::KeyValuePairList kv_list;
    for (const auto& raw_body : *bodies) {
      kv_list.Clear();
      std::string body = CleanToken(raw_body);
      auto parts = annotation::ParseAsArgList(body);
      if (!parts.ok()) continue;
      for (const auto& part : *parts) {
        size_t eq_pos = part.find('=');
        if (eq_pos != std::string::npos) {
          std::string key = CleanToken(part.substr(0, eq_pos));
          std::string val = CleanToken(part.substr(eq_pos + 1));
          auto* kv = kv_list.add_kv_pairs();
          kv->set_key(key);
          int64_t int_val;
          if (absl::SimpleAtoi(val, &int_val)) {
            kv->mutable_value()->set_int64_value(int_val);
          } else {
            kv->mutable_value()->set_string_value(val);
          }
        }
      }
      modes.push_back(ParseActionProfileModeFromKvList(kv_list));
    }
  }

  std::vector<ActionProfileMode> deduplicated_modes;
  for (const auto& mode : modes) {
    bool exists = false;
    for (const auto& existing : deduplicated_modes) {
      if (google::protobuf::util::MessageDifferencer::Equals(mode, existing)) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      deduplicated_modes.push_back(mode);
    }
  }

  return deduplicated_modes;
}

}  // namespace pdpi
