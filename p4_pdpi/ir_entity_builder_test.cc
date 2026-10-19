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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gutil/proto_matchers.h"
#include "p4_pdpi/ir.pb.h"

namespace pdpi {
namespace {

using ::gutil::EqualsProto;

TEST(IrEntityBuilderTest, BuildsExactMatchAndSingleActionTableEntry) {
  IrTableEntry entry =
      IrEntityBuilder("router_interface_table")
          .Exact("router_interface_id", IrStr("rif-1"))
          .Action(IrAction("set_port_and_src_mac",
                           IrParam("port", IrStr("Ethernet0")),
                           IrParam("src_mac", IrMac("00:11:22:33:44:55"))))
          .Build()
          .table_entry();

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

TEST(IrEntityBuilderTest, BuildsLpmIpv4AndIpv6RouteEntries) {
  IrEntity ipv4_entity =
      IrEntityBuilder()
          .TableName("ipv4_table")
          .Exact("vrf_id", IrStr("vrf-default"))
          .Lpm("ipv4_dst", IrIpv4("10.0.0.0"), 24)
          .Action(
              IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-1"))))
          .Build();

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

  IrEntity ipv6_entity =
      IrEntityBuilder("ipv6_table")
          .Exact("vrf_id", IrStr("vrf-blue"))
          .Lpm("ipv6_dst", IrIpv6("2001:db8::"), 64)
          .Action(IrAction("set_wcmp_group_id",
                           IrParam("wcmp_group_id", IrStr("grp-1"))))
          .Build();

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

TEST(IrEntityBuilderTest, BuildsWcmpActionSetEntry) {
  IrTableEntry wcmp_entry =
      IrEntityBuilder("wcmp_group_table")
          .Exact("wcmp_group_id", IrStr("grp-1"))
          .ActionSetMember(
              IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-1"))),
              /*weight=*/2, /*watch_port=*/"1")
          .ActionSetMember(
              IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-2"))),
              /*weight=*/3)
          .Build()
          .table_entry();

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

TEST(IrEntityBuilderTest, BuildsAllValueFormatsAndTernaryOptionalMatches) {
  IrTableEntry entry =
      IrEntityBuilder("acl_ingress_table")
          .Priority(100)
          .Exact("ether_type", IrHexStr("0x0800"))
          .Exact("src_ip", IrIpv4("192.168.1.1"))
          .Exact("dst_ip", IrIpv6("fe80::1"))
          .Exact("dst_mac", IrMac("aa:bb:cc:dd:ee:ff"))
          .Ternary("l4_port", IrHexStr("0x0800"), IrHexStr("0xffff"))
          .Optional("in_port", IrStr("opt-1"))
          .Action(IrAction("acl_forward", IrParam("meter_id", IrHexStr("0x01")),
                           IrParam("redirect_ipv4", IrIpv4("10.1.1.1")),
                           IrParam("redirect_ipv6", IrIpv6("2001:db8::1"))))
          .Build()
          .table_entry();

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

TEST(IrEntityBuilderTest, LvalueBuildLeavesBuilderReusable) {
  // Build a key-only entity first, as a caller would for a lookup or DELETE.
  IrEntityBuilder builder =
      IrEntityBuilder("vrf_table").Exact("vrf_id", IrStr("vrf-1"));
  IrEntity key_entity = builder.Build();

  // Then add the action and build the full entity from the same builder.
  builder.Action(IrAction("no_action"));
  IrEntity full_entity = builder.Build();

  EXPECT_THAT(key_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "vrf_table"
                  matches {
                    name: "vrf_id"
                    exact { str: "vrf-1" }
                  }
                }
              )pb"));
  EXPECT_THAT(full_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "vrf_table"
                  matches {
                    name: "vrf_id"
                    exact { str: "vrf-1" }
                  }
                  action { name: "no_action" }
                }
              )pb"));
}

TEST(IrEntityBuilderTest, ActionAndActionSetReplaceEachOther) {
  // An IrTableEntry holds either a single action or an action set, so the
  // later call wins.
  IrEntity action_set_entity =
      IrEntityBuilder("wcmp_group_table")
          .Action(IrAction("no_action"))
          .ActionSetMember(
              IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-1"))),
              /*weight=*/1)
          .Build();
  IrEntity action_entity =
      IrEntityBuilder("wcmp_group_table")
          .ActionSetMember(
              IrAction("set_nexthop_id", IrParam("nexthop_id", IrStr("nh-1"))),
              /*weight=*/1)
          .Action(IrAction("no_action"))
          .Build();

  EXPECT_THAT(action_set_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "wcmp_group_table"
                  action_set {
                    actions {
                      action {
                        name: "set_nexthop_id"
                        params {
                          name: "nexthop_id"
                          value { str: "nh-1" }
                        }
                      }
                      weight: 1
                    }
                  }
                }
              )pb"));
  EXPECT_THAT(action_entity, EqualsProto(R"pb(
                table_entry {
                  table_name: "wcmp_group_table"
                  action { name: "no_action" }
                }
              )pb"));
}

TEST(IrActionTest, IrActionAttachesParamsToItsOwnAction) {
  EXPECT_THAT(IrAction("no_action"), EqualsProto(R"pb(name: "no_action")pb"));
  EXPECT_THAT(
      IrAction("set_port_and_src_mac", IrParam("port", IrStr("Ethernet0")),
               IrParam("src_mac", IrMac("00:11:22:33:44:55"))),
      EqualsProto(R"pb(
        name: "set_port_and_src_mac"
        params {
          name: "port"
          value { str: "Ethernet0" }
        }
        params {
          name: "src_mac"
          value { mac: "00:11:22:33:44:55" }
        }
      )pb"));
}

}  // namespace
}  // namespace pdpi
