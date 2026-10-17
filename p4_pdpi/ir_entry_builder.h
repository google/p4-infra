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

#ifndef P4_INFRA_P4_PDPI_IR_ENTRY_BUILDER_H_
#define P4_INFRA_P4_PDPI_IR_ENTRY_BUILDER_H_

#include <cstdint>
#include <utility>

#include "absl/strings/string_view.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {

// Fluent builder for constructing pdpi::IrTableEntry and pdpi::IrEntity
// objects with concise match, action, and action-set specifications.
class IrTableEntryBuilder {
 public:
  // Constructs an empty table entry builder.
  IrTableEntryBuilder() = default;
  // Constructs a builder targeting the specified P4 table name.
  explicit IrTableEntryBuilder(absl::string_view table_name);

  // Sets the table name.
  IrTableEntryBuilder& TableName(absl::string_view table_name);

  // Sets the entry priority (required for ternary/optional tables; 0 for
  // exact/LPM).
  IrTableEntryBuilder& Priority(int32_t priority);

  // Appends an exact match with the specified IrValue format.
  IrTableEntryBuilder& Exact(absl::string_view field_name, IrValue value);
  IrTableEntryBuilder& ExactStr(absl::string_view field_name,
                                absl::string_view value);
  IrTableEntryBuilder& ExactHexStr(absl::string_view field_name,
                                   absl::string_view value);
  IrTableEntryBuilder& ExactIpv4(absl::string_view field_name,
                                 absl::string_view value);
  IrTableEntryBuilder& ExactIpv6(absl::string_view field_name,
                                 absl::string_view value);
  IrTableEntryBuilder& ExactMac(absl::string_view field_name,
                                absl::string_view value);

  // Appends an LPM match with the specified IrValue format and prefix length.
  IrTableEntryBuilder& Lpm(absl::string_view field_name, IrValue value,
                           int32_t prefix_length);
  IrTableEntryBuilder& LpmIpv4(absl::string_view field_name,
                               absl::string_view value, int32_t prefix_length);
  IrTableEntryBuilder& LpmIpv6(absl::string_view field_name,
                               absl::string_view value, int32_t prefix_length);

  // Appends a ternary match with value and mask.
  IrTableEntryBuilder& Ternary(absl::string_view field_name, IrValue value,
                               IrValue mask);

  // Appends an optional match.
  IrTableEntryBuilder& Optional(absl::string_view field_name, IrValue value);

  // Sets the single action name for this entry. Subsequent Param* calls will
  // attach parameters to this action.
  IrTableEntryBuilder& Action(absl::string_view action_name);

  // Appends an action member to the entry's WCMP action set with the given
  // weight and optional watch port. Subsequent Param* calls will attach
  // parameters to this newly added member action.
  IrTableEntryBuilder& AddActionSetMember(absl::string_view action_name,
                                          int32_t weight,
                                          absl::string_view watch_port = "");
  IrTableEntryBuilder& AddActionSetMember(IrActionInvocation action,
                                          int32_t weight,
                                          absl::string_view watch_port = "");

  // Appends a parameter to the currently active action (either the single
  // entry action set by Action(), or the most recently added action set member
  // from AddActionSetMember()).
  IrTableEntryBuilder& Param(absl::string_view param_name, IrValue value);
  IrTableEntryBuilder& ParamStr(absl::string_view param_name,
                                absl::string_view value);
  IrTableEntryBuilder& ParamHexStr(absl::string_view param_name,
                                   absl::string_view value);
  IrTableEntryBuilder& ParamIpv4(absl::string_view param_name,
                                 absl::string_view value);
  IrTableEntryBuilder& ParamIpv6(absl::string_view param_name,
                                 absl::string_view value);
  IrTableEntryBuilder& ParamMac(absl::string_view param_name,
                                absl::string_view value);

  // Builds and returns the constructed IrTableEntry.
  IrTableEntry Build() const& { return entry_; }
  IrTableEntry Build() && { return std::move(entry_); }
  IrTableEntry BuildTableEntry() const& { return entry_; }
  IrTableEntry BuildTableEntry() && { return std::move(entry_); }

  // Builds and returns an IrEntity wrapping the constructed IrTableEntry.
  IrEntity BuildEntity() const&;
  IrEntity BuildEntity() &&;

 private:
  // === Internal Helpers ======================================================

  // Returns a mutable pointer to the active action invocation (either the
  // entry's single action, or the last member of action_set if present).
  IrActionInvocation* MutableCurrentAction();

  // === Mutable State =========================================================

  IrTableEntry entry_;
};

}  // namespace pdpi

#endif  // P4_INFRA_P4_PDPI_IR_ENTRY_BUILDER_H_
