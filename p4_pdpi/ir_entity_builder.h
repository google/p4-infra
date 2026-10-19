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

#ifndef P4_INFRA_P4_PDPI_IR_ENTITY_BUILDER_H_
#define P4_INFRA_P4_PDPI_IR_ENTITY_BUILDER_H_

#include <concepts>
#include <cstdint>
#include <utility>

#include "absl/strings/string_view.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {

// Returns an IrValue holding `value` in the named format.
IrValue IrStr(absl::string_view value);
IrValue IrHexStr(absl::string_view value);
IrValue IrIpv4(absl::string_view value);
IrValue IrIpv6(absl::string_view value);
IrValue IrMac(absl::string_view value);

// Returns an action parameter `name` with `value`, for `IrAction`.
IrActionInvocation::IrActionParam IrParam(absl::string_view name,
                                          IrValue value);

// Returns an IrActionInvocation of action `name` with `params` in order, e.g.
// `IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-1")))`.
template <typename... Params>
  requires(std::same_as<Params, IrActionInvocation::IrActionParam> && ...)
IrActionInvocation IrAction(absl::string_view name, Params... params) {
  IrActionInvocation action;
  action.set_name(name);
  ((*action.add_params() = std::move(params)), ...);
  return action;
}

// Builds a pdpi::IrEntity by chaining setters, e.g.
//
//   IrEntity entity = IrEntityBuilder("vrf_table")
//                         .Exact("vrf_id", IrStr("vrf-1"))
//                         .Action(IrAction("no_action"))
//                         .Build();
//
// It builds table entries today. Packet replication entries (multicast groups
// and clone sessions), which are also IrEntity, may be added later.
//
// NOTE: Use it for P4 programs without PD protos, or to build partial (e.g.
// key-only) or invalid entries: it does no validation against a P4Info or the
// P4Runtime spec and sets exactly the fields it is given. For example, Build()
// returns an entity with no action or priority if none was set.
class IrEntityBuilder {
 public:
  // Constructs an empty builder.
  IrEntityBuilder() = default;
  // Constructs a builder targeting the specified P4 table name.
  explicit IrEntityBuilder(absl::string_view table_name);

  // Sets the table name.
  IrEntityBuilder& TableName(absl::string_view table_name);

  // Sets the entry priority (required for ternary/optional tables; 0 for
  // exact/LPM).
  IrEntityBuilder& Priority(int32_t priority);

  // Appends an exact match.
  IrEntityBuilder& Exact(absl::string_view field_name, IrValue value);

  // Appends an LPM match with the given prefix length.
  IrEntityBuilder& Lpm(absl::string_view field_name, IrValue value,
                       int32_t prefix_length);

  // Appends a ternary match with value and mask.
  IrEntityBuilder& Ternary(absl::string_view field_name, IrValue value,
                           IrValue mask);

  // Appends an optional match.
  IrEntityBuilder& Optional(absl::string_view field_name, IrValue value);

  // Sets the entry's single action.
  //
  // WARNING: Replaces any earlier action or action set, since an IrTableEntry
  // holds one or the other.
  IrEntityBuilder& Action(IrActionInvocation action);

  // Appends a member to the entry's WCMP action set with the given weight and
  // optional watch port.
  //
  // WARNING: Discards the single action set by Action(), if any.
  IrEntityBuilder& ActionSetMember(IrActionInvocation action, int32_t weight,
                                   absl::string_view watch_port = "");

  // Returns the constructed IrEntity. Leaves the builder intact, so a caller
  // can build a key-only entity (e.g. for a lookup or DELETE), then add an
  // action and build again. Callers that need the bare entry use
  // `Build().table_entry()`.
  IrEntity Build() const;

 private:
  IrTableEntry entry_;
};

}  // namespace pdpi

#endif  // P4_INFRA_P4_PDPI_IR_ENTITY_BUILDER_H_
