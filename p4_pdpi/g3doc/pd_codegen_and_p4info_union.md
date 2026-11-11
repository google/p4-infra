<!-- disableFinding(LINK_RELATIVE_G3DOC) -->

# PD Proto Generation and P4Info Union

## Overview

This page covers two independent build-time `P4Info` tools. `p4_pd_proto` and
`pdgen` generate a typed `*_pd.proto` schema from one compiled `P4Info`, so C++
callers get compile-time field names and types instead of building raw `p4.v1`
protos. Separately, [`UnionP4info`](../p4info_union_lib.h) can merge
per-instantiation `P4Info` schemas into one superset `P4Info` for controllers
that span several switch roles in one binary. Runtime translation still
validates every entity against each switch's concrete [`IrP4Info`](../ir.proto).

## Where Things Live

*   **PD proto generation**:
    *   [`pdgenlib.h`](../pdgenlib.h): [`IrP4InfoToPdProto`](../pdgenlib.h),
        [`PdGenOptions`](../pdgenlib.h)
    *   [`pdgen.cc`](../pdgen.cc) ([`:pdgen`](../BUILD.bazel)): CLI that runs
        [`CreateIrP4Info`](../ir.h), then `IrP4InfoToPdProto`
    *   [`pdgen.bzl`](../pdgen.bzl): `p4_pd_proto` macro that runs `:pdgen` to
        emit a `*_pd.proto` schema from a compiled `P4Info`
*   **Multi-instantiation `P4Info` union (optional)**:
    *   [`p4info_union_lib.h`](../p4info_union_lib.h): `UnionP4info`
    *   [`p4info_union.cc`](../p4info_union.cc)
        ([`:p4info_union`](../BUILD.bazel)): CLI over `UnionP4info`
    *   [`action_profile_modes.h`](../action_profile_modes.h):
        [`ParseRequiredModesFromActionProfile`](../action_profile_modes.h)

### Key APIs

*   `IrP4InfoToPdProto`: Takes an `IrP4Info` and `PdGenOptions` (proto package
    name, role filter, and optional
    [`PdGenOptions::multicast_table_field_number`](../pdgenlib.h)).
    *   Returns a complete `.proto` schema string with per-table entries,
        actions, packet I/O, and RPC wrappers.
    *   Fails with `InvalidArgumentError` when no table matches
        [`PdGenOptions::roles`](../pdgenlib.h) (the C++ default `{}` matches
        none, not even roleless tables), when there are no actions, or on a
        duplicate `@proto_id` in a table, a `RANGE` match, or an unsupported
        meter or counter unit.
    *   It checks neither the package name nor `multicast_table_field_number`. A
        zero or colliding field number fails later, when `protoc` compiles the
        generated `.proto`.
*   `UnionP4info`: Takes a `const std::vector<p4.config.v1.P4Info>&`.
    *   Returns one superset `P4Info` that unites tables, actions, action
        profiles, counters, meters, and the other top-level `P4Info`
        collections, plus `new_types`.
    *   Fails on incompatible same-ID element definitions, invalid
        `@group_mode_resource_guarantee` annotations on any action profile, or
        unsupported `externs`. Same-ID tables, actions, or action profiles also
        fail on `structured_annotations` or table `other_properties`.
*   `ParseRequiredModesFromActionProfile`: Takes an `ActionProfile` definition
    and parses each `@group_mode_resource_guarantee` annotation into one
    [`ActionProfileMode`](../action_profile_mode.proto). Fails with
    `InvalidArgumentError` on an argument not of the form `<key> = <value>`, an
    unknown key or value, a non-integer numeric value, or `max_member_weight`
    without `size_semantics = sum_of_members`. It does not reject two
    annotations that describe the same mode, although the header comment says it
    does.

## Cross-File Invariants

*   **Deterministic `P4Info`-derived proto field numbers**: `IrP4InfoToPdProto`
    computes `TableEntry` oneof field numbers from `table_id & 0xffffff`
    (masking off the 8-bit P4Runtime resource-type prefix), match and
    action-parameter field numbers directly from their P4 `@id`, and action
    oneof field numbers from `ActionRef` `@proto_id` annotations. Changing `@id`
    or `@proto_id` in P4 source alters wire/textproto field numbers in generated
    PD protos, and omitting `@proto_id` on any non-`DEFAULT_ONLY` table action
    ref fails in [`CreateIrP4Info`](../ir.h).
*   **`multicast_group_table_entry` default split between `:pdgen` and C++
    `PdGenOptions`**:
    *   `:pdgen` (`pdgen.cc`, used by `p4_pd_proto`) defaults
        `--multicast_table_field_number` to `2047`, so it emits
        `multicast_group_table_entry = 2047` (as in
        [`main_p4_pd.expected`](../main_p4_pd.expected)).
    *   Direct C++ callers of `IrP4InfoToPdProto` get
        [`PdGenOptions::multicast_table_field_number`](../pdgenlib.h) as
        `std::nullopt`. Unless they set it, the generated proto has no
        `MulticastGroupTableEntry`, and multicast translation in
        [`pd.cc`](../pd.cc) fails.
    *   `IrP4InfoToPdProto` emits any set value unchecked. Pick a non-zero
        number that does not collide with any `table_id & 0xffffff`, or `protoc`
        rejects the generated `.proto`.
*   **Schema lockstep between `pdgenlib.cc` and `pd.cc`**: Message suffixes
    (`"*Entry"`, `"*Action"`), field names (`match`, `action`, `wcmp_actions`,
    `group_action`, `counter_data`, `meter_counter_data`), and RPC wrapper
    layouts emitted by [`pdgenlib.cc`](../pdgenlib.cc) are reflected by name in
    `pd.cc` ([entity_and_rpc_translation.md](entity_and_rpc_translation.md)).
    Any layout change must update `pd.cc` and regenerate `main_p4_pd.expected`
    in the same CL.
*   **Superset merging rules in `UnionP4info`**: `UnionP4info` unions distinct
    IDs across the input `P4Info`s into one superset schema.
    *   It adds no `@unsupported` annotations. Callers must still translate
        against each switch's own `IrP4Info` to reject absent elements at
        runtime.
    *   Shared elements must have matching IDs and types.
    *   Shared tables union `match_fields`, `action_refs`,
        `direct_resource_ids`, and `preamble.annotations`, and take the max
        `size`. A `MatchField` or `ActionRef` with the same ID must be identical
        in every input, annotations included. Shared actions, by contrast, union
        their preamble `annotations`.
    *   `@group_mode_resource_guarantee` strings on every action profile, shared
        or not, are validated up front with
        `ParseRequiredModesFromActionProfile`.
    *   Shared action profiles set-union `preamble.annotations` and take the max
        `size` and `max_group_size`.
    *   When shared action profiles disagree on selector semantics, the result
        uses `sum_of_weights`, the more restrictive mode. Capacities widen, but
        selector semantics narrow. When every input uses `sum_of_members`, it
        takes the max `max_member_weight`.

## Known Limitations

*   **`@groupaction` annotation vs. `ActionRef::scope` in `pdgen`**:
    `IrP4InfoToPdProto` emits the `group_action` field on any table whose action
    refs carry `@groupaction`, checking the `ActionRef` annotations rather than
    `ActionRef::scope == GROUP_ONLY`. A `GROUP_ONLY` action ref without
    `@groupaction` lands in the regular member action oneof instead of
    `group_action`.
*   **`UnionP4info` rejects `externs`, `structured_annotations`, and
    non-`new_types` `P4TypeInfo`**:
    *   It merges `pkg_info` (taking the max multicast `PlatformProperties`),
        `tables`, `actions`, `action_profiles`, `counters`, `direct_counters`,
        `meters`, `direct_meters`, `registers`, `digests`, `value_sets`,
        `controller_packet_metadata`, and `type_info.new_types`.
    *   It returns `UnimplementedError` on non-empty `externs`, and on table
        `other_properties` or preamble `structured_annotations` of elements that
        share an ID across inputs. Elements with unique IDs are copied
        unchecked.
    *   It returns `InvalidArgumentError` on `P4TypeInfo` fields other than
        `new_types`.

## End-to-End Path

1.  **Optional multi-instantiation union (`UnionP4info`)**: Multi-switch build
    rules pass per-instantiation `p4.config.v1.P4Info` files to `UnionP4info`,
    which validates action-profile mode annotations and type compatibility,
    unions per-switch elements and `preamble.annotations`, widens table and
    action-profile capacities, and narrows differing selector semantics to
    `sum_of_weights`.
2.  **`P4Info` to `IrP4Info` lowering**: `:pdgen` lowers the single or unioned
    `P4Info` via `CreateIrP4Info`, resolving `@format` types, `@proto_id`
    numbers, `@padding` packet metadata, and `@unsupported` flags.
3.  **PD proto synthesis (`IrP4InfoToPdProto`)**: `IrP4InfoToPdProto` emits
    per-table `"*Entry"` messages, per-action `"*Action"` messages,
    `MulticastGroupTableEntry` (when `multicast_table_field_number` is set,
    default `2047` in `:pdgen`), `PacketIn` and `PacketOut` messages, and
    `TableEntry` / `TableEntries` / `ReadRequest` / `ReadResponse` /
    `WriteRequest` / `WriteRpcStatus` wrappers, prefixing `@unsupported`
    elements with `// CAUTION:` doc comments.
4.  **Bazel compilation (`p4_pd_proto`)**: `pdgen.bzl` invokes `:pdgen` in a
    `genrule` whose `out` names the generated `.proto`, which downstream
    `proto_library` and `cc_proto_library` targets compile into C++ types
    consumed with [`pd.h`](../pd.h).
