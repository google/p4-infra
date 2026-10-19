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

#include "p4_pdpi/ir_entity_builder.h"

#include <cstdint>
#include <string>
#include <utility>

#include "absl/strings/string_view.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {

IrValue IrStr(absl::string_view value) {
  IrValue ir_value;
  ir_value.set_str(std::string(value));
  return ir_value;
}

IrValue IrHexStr(absl::string_view value) {
  IrValue ir_value;
  ir_value.set_hex_str(std::string(value));
  return ir_value;
}

IrValue IrIpv4(absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv4(std::string(value));
  return ir_value;
}

IrValue IrIpv6(absl::string_view value) {
  IrValue ir_value;
  ir_value.set_ipv6(std::string(value));
  return ir_value;
}

IrValue IrMac(absl::string_view value) {
  IrValue ir_value;
  ir_value.set_mac(std::string(value));
  return ir_value;
}

IrActionInvocation::IrActionParam IrParam(absl::string_view name,
                                          IrValue value) {
  IrActionInvocation::IrActionParam param;
  param.set_name(name);
  *param.mutable_value() = std::move(value);
  return param;
}

IrEntityBuilder::IrEntityBuilder(absl::string_view table_name) {
  entry_.set_table_name(std::string(table_name));
}

IrEntityBuilder& IrEntityBuilder::TableName(absl::string_view table_name) {
  entry_.set_table_name(std::string(table_name));
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Priority(int32_t priority) {
  entry_.set_priority(priority);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Exact(absl::string_view field_name,
                                        IrValue value) {
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_exact() = std::move(value);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Lpm(absl::string_view field_name,
                                      IrValue value, int32_t prefix_length) {
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_lpm()->mutable_value() = std::move(value);
  match->mutable_lpm()->set_prefix_length(prefix_length);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Ternary(absl::string_view field_name,
                                          IrValue value, IrValue mask) {
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_ternary()->mutable_value() = std::move(value);
  *match->mutable_ternary()->mutable_mask() = std::move(mask);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Optional(absl::string_view field_name,
                                           IrValue value) {
  IrMatch* match = entry_.add_matches();
  match->set_name(std::string(field_name));
  *match->mutable_optional()->mutable_value() = std::move(value);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::Action(IrActionInvocation action) {
  *entry_.mutable_action() = std::move(action);
  return *this;
}

IrEntityBuilder& IrEntityBuilder::ActionSetMember(
    IrActionInvocation action, int32_t weight, absl::string_view watch_port) {
  IrActionSetInvocation* member = entry_.mutable_action_set()->add_actions();
  *member->mutable_action() = std::move(action);
  member->set_weight(weight);
  if (!watch_port.empty()) {
    member->set_watch_port(std::string(watch_port));
  }
  return *this;
}

IrEntity IrEntityBuilder::Build() const {
  IrEntity entity;
  *entity.mutable_table_entry() = entry_;
  return entity;
}

}  // namespace pdpi
