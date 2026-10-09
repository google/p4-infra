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

#include "p4_pdpi/ir_entry_builder.h"

#include <cstdint>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {

IrTableEntryBuilder::IrTableEntryBuilder(absl::string_view table_name) {
  entry_.set_table_name(std::string(table_name));
}

IrTableEntryBuilder& IrTableEntryBuilder::TableName(
    absl::string_view table_name) {
  entry_.set_table_name(std::string(table_name));
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::Priority(int32_t priority) {
  entry_.set_priority(priority);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::Exact(absl::string_view field_name,
                                                IrValue value) {
  // Append an exact match field with the provided IrValue.
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_exact() = std::move(value);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::ExactStr(absl::string_view field_name,
                                                   absl::string_view value) {
  IrValue ir_value;
  ir_value.set_str(std::string(value));
  return Exact(field_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ExactHexStr(
    absl::string_view field_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_hex_str(std::string(value));
  return Exact(field_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ExactIpv4(
    absl::string_view field_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv4(std::string(value));
  return Exact(field_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ExactIpv6(
    absl::string_view field_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv6(std::string(value));
  return Exact(field_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ExactMac(absl::string_view field_name,
                                                   absl::string_view value) {
  IrValue ir_value;
  ir_value.set_mac(std::string(value));
  return Exact(field_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::Lpm(absl::string_view field_name,
                                              IrValue value,
                                              int32_t prefix_length) {
  // Append an LPM match field with value and prefix length.
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_lpm()->mutable_value() = std::move(value);
  match->mutable_lpm()->set_prefix_length(prefix_length);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::LpmIpv4(absl::string_view field_name,
                                                  absl::string_view value,
                                                  int32_t prefix_length) {
  IrValue ir_value;
  ir_value.set_ipv4(std::string(value));
  return Lpm(field_name, std::move(ir_value), prefix_length);
}

IrTableEntryBuilder& IrTableEntryBuilder::LpmIpv6(absl::string_view field_name,
                                                  absl::string_view value,
                                                  int32_t prefix_length) {
  IrValue ir_value;
  ir_value.set_ipv6(std::string(value));
  return Lpm(field_name, std::move(ir_value), prefix_length);
}

IrTableEntryBuilder& IrTableEntryBuilder::Ternary(absl::string_view field_name,
                                                  IrValue value, IrValue mask) {
  // Append a ternary match field with value and bitmask.
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_ternary()->mutable_value() = std::move(value);
  *match->mutable_ternary()->mutable_mask() = std::move(mask);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::Optional(absl::string_view field_name,
                                                   IrValue value) {
  // Append an optional match field with value.
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_optional()->mutable_value() = std::move(value);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::Action(
    absl::string_view action_name) {
  entry_.mutable_action()->set_name(std::string(action_name));
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::AddActionSetMember(
    absl::string_view action_name, int32_t weight,
    absl::string_view watch_port) {
  // Append a new weighted member action to the WCMP action set.
  IrActionSetInvocation* member = entry_.mutable_action_set()->add_actions();
  member->mutable_action()->set_name(std::string(action_name));
  member->set_weight(weight);
  if (!watch_port.empty()) {
    member->set_watch_port(std::string(watch_port));
  }
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::AddActionSetMember(
    IrActionInvocation action, int32_t weight, absl::string_view watch_port) {
  // Append a pre-constructed member action to the WCMP action set.
  IrActionSetInvocation* member = entry_.mutable_action_set()->add_actions();
  *member->mutable_action() = std::move(action);
  member->set_weight(weight);
  if (!watch_port.empty()) {
    member->set_watch_port(std::string(watch_port));
  }
  return *this;
}

IrActionInvocation* IrTableEntryBuilder::MutableCurrentAction() {
  // Target the last action set member if present, or the single action.
  if (entry_.has_action_set() && !entry_.action_set().actions().empty()) {
    return entry_.mutable_action_set()
        ->mutable_actions(entry_.action_set().actions_size() - 1)
        ->mutable_action();
  }
  return entry_.mutable_action();
}

IrTableEntryBuilder& IrTableEntryBuilder::Param(absl::string_view param_name,
                                                IrValue value) {
  // Append parameter name and value to the currently active action.
  IrActionInvocation::IrActionParam* param =
      MutableCurrentAction()->add_params();
  param->set_name(std::string(param_name));
  *param->mutable_value() = std::move(value);
  return *this;
}

IrTableEntryBuilder& IrTableEntryBuilder::ParamStr(absl::string_view param_name,
                                                   absl::string_view value) {
  IrValue ir_value;
  ir_value.set_str(std::string(value));
  return Param(param_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ParamHexStr(
    absl::string_view param_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_hex_str(std::string(value));
  return Param(param_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ParamIpv4(
    absl::string_view param_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv4(std::string(value));
  return Param(param_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ParamIpv6(
    absl::string_view param_name, absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv6(std::string(value));
  return Param(param_name, std::move(ir_value));
}

IrTableEntryBuilder& IrTableEntryBuilder::ParamMac(absl::string_view param_name,
                                                   absl::string_view value) {
  IrValue ir_value;
  ir_value.set_mac(std::string(value));
  return Param(param_name, std::move(ir_value));
}

IrEntity IrTableEntryBuilder::BuildEntity() const& {
  // Wrap a copy of the constructed IrTableEntry into an IrEntity.
  IrEntity entity;
  *entity.mutable_table_entry() = entry_;
  return entity;
}

IrEntity IrTableEntryBuilder::BuildEntity() && {
  // Move the constructed IrTableEntry into an IrEntity.
  IrEntity entity;
  *entity.mutable_table_entry() = std::move(entry_);
  return entity;
}

}  // namespace pdpi
