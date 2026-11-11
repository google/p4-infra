# PDPI

PDPI is a C++ library and Bazel codegen suite for P4Runtime. It translates
entities, packet I/O messages, RPC requests and responses, and batch write
statuses between three protobuf representations: Program-Independent (`PI`),
Intermediate (`IR`), and Program-Dependent (`PD`). It also unions `P4Info`s,
validates references, and orders updates so that dependencies are written first.

## Entry Points

*   **System context & consumers**: Inputs are compiled `p4.config.v1.P4Info`
    schemas, `p4.v1` messages, and generated `*_pd.proto` messages. Changes here
    affect the P4Runtime switch stacks, controllers, and P4 validation tools
    that build on this library.
*   **Primary interface**:
    *   [`ir.h`](ir.h), [`ir.proto`](ir.proto) ([`//p4_pdpi:ir`](BUILD.bazel)):
        `CreateIrP4Info`, `PiEntityToIr`, `IrEntityToPi`, `PiWriteRequestToIr`,
        `IrWriteRequestToPi`, `GrpcStatusToIrWriteRpcStatus`.
    *   [`pd.h`](pd.h) ([`:pd`](BUILD.bazel)): `PdTableEntryToPiEntity`,
        `PiEntityToPdTableEntry`, `PdWriteRequestToPi`, `PiReadResponseToPd`,
        `GrpcStatusToPd`.
    *   [`ir_entity_builder.h`](ir_entity_builder.h)
        ([`:ir_entity_builder`](BUILD.bazel)): `IrEntityBuilder`.
    *   [`ir_tools.h`](ir_tools.h) ([`:ir_tools`](BUILD.bazel)):
        `TransformValuesOfType`.
    *   [`pdgenlib.h`](pdgenlib.h), [`pdgen.bzl`](pdgen.bzl)
        ([`:pdgenlib`](BUILD.bazel)): `IrP4InfoToPdProto` and `p4_pd_proto`.
    *   [`p4info_union_lib.h`](p4info_union_lib.h)
        ([`:p4info_union_lib`](BUILD.bazel)): `UnionP4info`.
    *   [`references.h`](references.h) ([`:references`](BUILD.bazel)):
        `UnsatisfiedOutgoingReferences` and `StatefulReferenceChecker`.
    *   [`sequencing.h`](sequencing.h) ([`:sequencing`](BUILD.bazel)):
        `SequencePiUpdatesIntoWriteRequests`, `StableSortUpdates`, and
        `GetEntitiesUnreachableFromRoots`.
*   **Lore atlas & design docs**: [`README.md`](README.md),
    [`references.md`](references.md), and [`g3doc/index.md`](g3doc/index.md).
    Before you change cross-file behavior, read the matching page, and update it
    in the same CL:
    *   `IrP4Info`, `PI <-> IR <-> PD` entity/packet/RPC translation,
        `GrpcStatus` decoding, `TranslationOptions`, or `IrEntityBuilder`:
        `g3doc/entity_and_rpc_translation.md#cross-file-invariants`.
    *   `pdgen` (`*_pd.proto` schema and field numbers), `p4_pd_proto`, or
        `UnionP4info` / `@group_mode_resource_guarantee`:
        `g3doc/pd_codegen_and_p4info_union.md#cross-file-invariants`.
    *   `@refers_to` / `@referenced_by`, `EntityKey`, reference validation,
        reachability, or `WriteRequest` sequencing:
        `g3doc/references_and_sequencing.md#cross-file-invariants`.

## Build, Test, and Code Generation

*   **Unit and golden diff tests**: `bazel test //p4_pdpi:all`.
*   **Golden updates**: When an intended output change breaks a `*_diff_test`,
    run `bazel run //p4_pdpi:<name>_diff_test -- --update` for each affected
    `<name>` (`p4info`, `table_entry`, `packet_io`, `references`, `rpc`,
    `main_pd`, `sequencing`, `sequencing_util`).
*   **Diagrams**: For the `images/*.dot` diagrams in `references.md`, see
    [`images/README.md`](images/README.md).

## Domain Invariants, Traps, and Known Limitations

*   **Non-canonical `PI` byte strings silently miss references and
    dependencies**: `:entity_keys`, `:references`, and `:sequencing` compare raw
    `p4.v1` byte strings and never call `ArbitraryToCanonicalByteString`.
    *   `PI -> IR -> PI` strips leading zeros, so the same value can appear in
        two byte forms.
    *   Canonicalize `PI` entities before reference checking or sequencing. See
        `g3doc/references_and_sequencing.md#cross-file-invariants`.
*   **`pd.cc` finds generated fields by name**: `pd.cc` uses `proto2::Message`
    reflection to look up generated `*_pd.proto` message and field names (such
    as `match`, `action`, `wcmp_actions`, `group_action`,
    `multicast_group_table_entry`, `counter_data`, `meter_counter_data`).
    *   Renaming a generated field in `pdgenlib.cc` without updating `pd.cc`
        fails at runtime, not at build time. See
        `g3doc/entity_and_rpc_translation.md#cross-file-invariants`.
*   **`TranslationOptions` flags gate some inputs**:
    *   `DELETE` updates that omit actions or non-key fields fail unless
        `.key_only = true`.
    *   `@unsupported` elements fail unless `.allow_unsupported = true`.
    *   See `g3doc/entity_and_rpc_translation.md#cross-file-invariants`.
*   **`UnionP4info` does not mark non-universal elements**: It unions elements
    that only some programs have without adding `@unsupported`.
    *   Callers must still validate against each switch's own `IrP4Info`. See
        `g3doc/pd_codegen_and_p4info_union.md#cross-file-invariants`.
*   **Write status decoding trusts the caller's update count**:
    `GrpcStatusToIrWriteRpcStatus` and `GrpcStatusToPd` take
    `number_of_updates_in_write_request`.
    *   On `OK`, they return that many `OK` statuses, so a wrong count silently
        gives a wrong result.
    *   On `UNKNOWN` with `error_details`, they fail with `InvalidArgumentError`
        unless `details` unpacks to exactly that many `p4.v1.Error` entries,
        with at least one non-`OK` entry. `UNKNOWN` with empty `error_details`
        becomes an `rpc_wide_error`.
    *   See `g3doc/entity_and_rpc_translation.md#cross-file-invariants`.
*   **Sequencing rejects PRE updates**: `SequencePiUpdatesIntoWriteRequests` and
    `SequencePiUpdatesInPlace` use the deprecated `IrMatchFieldReference`.
    *   They fail with `"Table with ID 0 does not exist"` on
        `packet_replication_engine_entry` updates. See
        `g3doc/references_and_sequencing.md#known-limitations`.
*   **Reverse `StableSortUpdates` breaks on equal ranks**: With
    `reverse_ordering = true`, it returns `true` for equal ranks, which violates
    the strict weak ordering that `std::stable_sort` requires. See
    `g3doc/references_and_sequencing.md#known-limitations`.
*   **Deprecated `*TableEntry*` APIs**: Use `PdTableEntryToPiEntity`,
    `PdTableEntriesToPiEntities`, `PdTableEntryToIrEntity`,
    `PdTableEntriesToIrEntities`, and `IrEntities`.
    *   Don't use the `ABSL_DEPRECATED` `PartialPdTableEntry*` functions or the
        `[deprecated = true]` `IrTableEntries` message. See
        `g3doc/entity_and_rpc_translation.md#known-limitations`.
*   **C++ `PdGenOptions` leaves the multicast field number unset**: Set
    `PdGenOptions::multicast_table_field_number` when you call
    `IrP4InfoToPdProto` from C++.
    *   `:pdgen` and `p4_pd_proto` default it to `2047`. See
        `g3doc/pd_codegen_and_p4info_union.md#cross-file-invariants`.
*   **Reference annotation rules**:
    *   `@referenced_by` is only for `builtin::` PRE sources. Use `@refers_to`
        everywhere else.
    *   Destinations must be `EXACT` match fields, never action parameters or
        ternary, LPM, or optional matches.
    *   The table graph must be acyclic.
    *   See `g3doc/references_and_sequencing.md#cross-file-invariants`.
*   **Keep `:references` and `CreateIrP4Info` off hot paths**:
    *   `:references` builds every `ConcreteTableReference` and copies heavily
        on large tables. See
        `g3doc/references_and_sequencing.md#known-limitations`.
    *   Call `CreateIrP4Info` once per stream or instantiation, outside packet
        I/O loops. See
        `g3doc/entity_and_rpc_translation.md#cross-file-invariants`.
*   **Open `p4c` and `P4Info` workarounds**:
    *   `ir.cc` hardcodes `weights_disallowed` for 3 named ARS tables only. A
        new or renamed ARS table allows weights unless `p4c` sets the field.
    *   `pdgenlib.cc` emits `group_action` only when `@groupaction` is present,
        and does not check `ActionRef::scope == GROUP_ONLY`.
    *   See `g3doc/entity_and_rpc_translation.md#known-limitations` and
        `g3doc/pd_codegen_and_p4info_union.md#known-limitations`.

## Best Practices & Presubmit Conventions

*   **Internal libraries pass `TranslationOptions` explicitly**: An internal
    `cc_library` in this package's `BUILD` file that calls `TranslationOptions`
    APIs must set
    `local_defines = ["PDPI_DISABLE_TRANSLATION_OPTIONS_DEFAULT"]`. The define
    removes the `= {}` default, so a forgotten forward fails at compile time
    instead of silently resetting the options to `{}`.
