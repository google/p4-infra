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

// Parses ActionProfile modes from `@group_mode(...)` annotations in the
// ActionProfile's preamble.
absl::StatusOr<std::vector<ActionProfileMode>>
ParseRequiredModesFromActionProfile(
    const p4::config::v1::ActionProfile& action_profile);

}  // namespace pdpi

#endif  // P4_INFRA_P4_PDPI_ACTION_PROFILE_MODES_H_
