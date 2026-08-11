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

#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/proto_matchers.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {
namespace {

using ::gutil::EqualsProto;

TEST(IrTableEntryBuilderTest, BuildsExactMatchAndSingleActionTableEntry) {
  IrTableEntry entry = IrTableEntryBuilder("router_interface_table")
                           .ExactStr("router_interface_id", "rif-1")
                           .Action("set_port_and_src_mac")
                           .ParamStr("port", "Ethernet0")
                           .ParamMac("src_mac", "00:11:22:33:44:55")
                           .Build();

  EXPECT_THAT(entry, EqualsProto(R"pb(
                table_name: "router_interface_table"
                matches {
                  name: "router_interface_id"
                  exact { str: "rif-1" }
                }
                action {
                  name: "set_port_and_src_mac"
                  params {
                    name: "port"
                    value { str: "Ethernet0" }
                  }
                  params {
                    name: "src_mac"
                    value { mac: "00:11:22:33:44:55" }
                  }
                }
              )pb"));
}

TEST(IrTableEntryBuilderTest, BuildsLpmIpv4AndIpv6RouteEntryAsEntity) {
  IrEntity ipv4_entity = IrTableEntryBuilder()
                             .TableName("ipv4_table")
                             .ExactStr("vrf_id", "vrf-default")
                             .LpmIpv4("ipv4_dst", "10.0.0.0", 24)
                             .Action("set_nexthop_id")
                             .ParamStr("nexthop_id", "nh-1")
                             .BuildEntity();

  EXPECT_THAT(ipv4_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "ipv4_table"
                  matches {
                    name: "vrf_id"
                    exact { str: "vrf-default" }
                  }
                  matches {
                    name: "ipv4_dst"
                    lpm {
                      value { ipv4: "10.0.0.0" }
                      prefix_length: 24
                    }
                  }
                  action {
                    name: "set_nexthop_id"
                    params {
                      name: "nexthop_id"
                      value { str: "nh-1" }
                    }
                  }
                }
              )pb"));

  IrEntity ipv6_entity = IrTableEntryBuilder("ipv6_table")
                             .ExactStr("vrf_id", "vrf-blue")
                             .LpmIpv6("ipv6_dst", "2001:db8::", 64)
                             .Action("set_wcmp_group_id")
                             .ParamStr("wcmp_group_id", "grp-1")
                             .BuildEntity();

  EXPECT_THAT(ipv6_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "ipv6_table"
                  matches {
                    name: "vrf_id"
                    exact { str: "vrf-blue" }
                  }
                  matches {
                    name: "ipv6_dst"
                    lpm {
                      value { ipv6: "2001:db8::" }
                      prefix_length: 64
                    }
                  }
                  action {
                    name: "set_wcmp_group_id"
                    params {
                      name: "wcmp_group_id"
                      value { str: "grp-1" }
                    }
                  }
                }
              )pb"));
}

TEST(IrTableEntryBuilderTest, BuildsWcmpActionSetEntry) {
  IrTableEntry wcmp_entry =
      IrTableEntryBuilder("wcmp_group_table")
          .ExactStr("wcmp_group_id", "grp-1")
          .AddActionSetMember("set_nexthop_id", /*weight=*/2,
                              /*watch_port=*/"1")
          .ParamStr("nexthop_id", "nh-1")
          .AddActionSetMember("set_nexthop_id", /*weight=*/3)
          .ParamStr("nexthop_id", "nh-2")
          .BuildTableEntry();

  EXPECT_THAT(wcmp_entry, EqualsProto(R"pb(
                table_name: "wcmp_group_table"
                matches {
                  name: "wcmp_group_id"
                  exact { str: "grp-1" }
                }
                action_set {
                  actions {
                    action {
                      name: "set_nexthop_id"
                      params {
                        name: "nexthop_id"
                        value { str: "nh-1" }
                      }
                    }
                    weight: 2
                    watch_port: "1"
                  }
                  actions {
                    action {
                      name: "set_nexthop_id"
                      params {
                        name: "nexthop_id"
                        value { str: "nh-2" }
                      }
                    }
                    weight: 3
                  }
                }
              )pb"));
}

TEST(IrTableEntryBuilderTest, BuildsAllValueFormatsAndTernaryOptionalMatches) {
  IrValue ternary_val;
  ternary_val.set_hex_str("0x0800");
  IrValue ternary_mask;
  ternary_mask.set_hex_str("0xffff");
  IrValue optional_val;
  optional_val.set_str("opt-1");

  IrTableEntry entry = IrTableEntryBuilder("acl_ingress_table")
                           .Priority(100)
                           .ExactHexStr("ether_type", "0x0800")
                           .ExactIpv4("src_ip", "192.168.1.1")
                           .ExactIpv6("dst_ip", "fe80::1")
                           .ExactMac("dst_mac", "aa:bb:cc:dd:ee:ff")
                           .Ternary("l4_port", ternary_val, ternary_mask)
                           .Optional("in_port", optional_val)
                           .Action("acl_forward")
                           .ParamHexStr("meter_id", "0x01")
                           .ParamIpv4("redirect_ipv4", "10.1.1.1")
                           .ParamIpv6("redirect_ipv6", "2001:db8::1")
                           .Build();

  EXPECT_THAT(entry, EqualsProto(R"pb(
                table_name: "acl_ingress_table"
                priority: 100
                matches {
                  name: "ether_type"
                  exact { hex_str: "0x0800" }
                }
                matches {
                  name: "src_ip"
                  exact { ipv4: "192.168.1.1" }
                }
                matches {
                  name: "dst_ip"
                  exact { ipv6: "fe80::1" }
                }
                matches {
                  name: "dst_mac"
                  exact { mac: "aa:bb:cc:dd:ee:ff" }
                }
                matches {
                  name: "l4_port"
                  ternary {
                    value { hex_str: "0x0800" }
                    mask { hex_str: "0xffff" }
                  }
                }
                matches {
                  name: "in_port"
                  optional { value { str: "opt-1" } }
                }
                action {
                  name: "acl_forward"
                  params {
                    name: "meter_id"
                    value { hex_str: "0x01" }
                  }
                  params {
                    name: "redirect_ipv4"
                    value { ipv4: "10.1.1.1" }
                  }
                  params {
                    name: "redirect_ipv6"
                    value { ipv6: "2001:db8::1" }
                  }
                }
              )pb"));
}

TEST(IrTableEntryBuilderTest, LvalueBuildAndBuildEntityPreserveBuilderState) {
  IrTableEntryBuilder builder = IrTableEntryBuilder("vrf_table")
                                    .ExactStr("vrf_id", "vrf-1")
                                    .Action("no_action");

  // Lvalue Build, BuildTableEntry, and BuildEntity preserve builder state.
  IrTableEntry copied_entry = builder.Build();
  IrEntity copied_entity = builder.BuildEntity();
  EXPECT_EQ(copied_entry.table_name(), "vrf_table");
  EXPECT_EQ(copied_entity.table_entry().table_name(), "vrf_table");

  IrTableEntry copied_table_entry = builder.BuildTableEntry();
  EXPECT_EQ(copied_table_entry.table_name(), "vrf_table");

  // Moving rvalue extraction still succeeds.
  IrTableEntry moved_entry = std::move(builder).Build();
  EXPECT_EQ(moved_entry.table_name(), "vrf_table");
}

TEST(IrTableEntryBuilderTest, SupportsActionSetMemberWithActionInvocation) {
  IrActionInvocation action;
  action.set_name("set_nexthop_id");
  IrActionInvocation::IrActionParam* param = action.add_params();
  param->set_name("nexthop_id");
  param->mutable_value()->set_str("nh-10");

  IrTableEntry entry =
      IrTableEntryBuilder("wcmp_group_table")
          .ExactStr("wcmp_group_id", "grp-10")
          .AddActionSetMember(action, /*weight=*/5, /*watch_port=*/"Ethernet4")
          .Build();

  EXPECT_THAT(entry, EqualsProto(R"pb(
                table_name: "wcmp_group_table"
                matches {
                  name: "wcmp_group_id"
                  exact { str: "grp-10" }
                }
                action_set {
                  actions {
                    action {
                      name: "set_nexthop_id"
                      params {
                        name: "nexthop_id"
                        value { str: "nh-10" }
                      }
                    }
                    weight: 5
                    watch_port: "Ethernet4"
                  }
                }
              )pb"));
}

}  // namespace
}  // namespace pdpi
