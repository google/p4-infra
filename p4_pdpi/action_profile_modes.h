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

#ifndef P4_INFRA_P4_PDPI_ACTION_PROFILE_MODES_H_
#define P4_INFRA_P4_PDPI_ACTION_PROFILE_MODES_H_

#include <vector>

#include "absl/status/statusor.h"
#include "p4/config/v1/p4info.pb.h"
#include "p4_pdpi/action_profile_mode.pb.h"

namespace pdpi {

// Parses the modes required by an ActionProfile from the
// `@group_mode_resource_guarantee` annotations in its preamble. Each annotation
// describes one mode as a flat list of key-value pairs, e.g.
//
//   @group_mode_resource_guarantee(action_selection_mode = HASH,
//                                  size_semantics = sum_of_members,
//                                  max_member_weight = 4095,
//                                  member_usage_multiplier = 4)
//
// Supported keys are `action_selection_mode` (HASH or RANDOM),
// `size_semantics` (sum_of_weights or sum_of_members), `max_member_weight`
// (only valid with `size_semantics = sum_of_members`) and
// `member_usage_multiplier`.
//
// Returns InvalidArgument if an annotation is malformed, uses an unknown key or
// value, or if two annotations describe the same mode.
absl::StatusOr<std::vector<ActionProfileMode>>
ParseRequiredModesFromActionProfile(
    const p4::config::v1::ActionProfile& action_profile);

}  // namespace pdpi

#endif  // P4_INFRA_P4_PDPI_ACTION_PROFILE_MODES_H_
