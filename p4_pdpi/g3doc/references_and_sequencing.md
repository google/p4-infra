<!-- disableFinding(LINK_RELATIVE_G3DOC) -->

# Entity References and Update Sequencing

## Overview

This page covers how PDPI models references between P4Runtime entities and uses
them to validate, garbage-collect, and order updates. P4Runtime targets reject
`INSERT` and `MODIFY` updates whose referenced entries do not exist yet, and
`DELETE` updates for entries that active entries still reference.
[`IrP4Info`](../ir.proto) records `@refers_to` and `@referenced_by` once, so
callers use the same `IrP4Info` to validate references (in a batch or
incrementally), find unreachable entries for garbage collection, and order
`WriteRequest` batches.

## Where Things Live

*   **Compile-time reference graph and entity keys**:
    *   [`reference_annotations.h`](../reference_annotations.h):
        [`ParseIrTableReferences`](../reference_annotations.h)
    *   [`built_ins.h`](../built_ins.h): packet-replication-engine (PRE)
        built-in tables, backed by `IrBuiltInTable` in [`ir.proto`](../ir.proto)
    *   [`entity_keys.h`](../entity_keys.h): [`EntityKey`](../entity_keys.h),
        [`TableEntryKey`](../entity_keys.h)
*   **Concrete reference validation**:
    *   [`references.h`](../references.h):
        [`UnsatisfiedOutgoingReferences`](../references.h) (stateless batch
        check) and [`StatefulReferenceChecker`](../references.h)
        ([`AddEntity`](../references.h), [`UpdateEntity`](../references.h),
        [`CanRemove`](../references.h), [`RemoveEntity`](../references.h),
        `ir_p4_info`)
    *   [`references.md`](../references.md): illustrated concrete-reference
        walkthroughs
*   **Reachability and update sequencing**:
    *   [`sequencing.h`](../sequencing.h):
        [`SequencePiUpdatesIntoWriteRequests`](../sequencing.h),
        [`SequencePiUpdatesInPlace`](../sequencing.h),
        [`StableSortUpdates`](../sequencing.h),
        [`GetEntitiesUnreachableFromRoots`](../sequencing.h)
    *   [`sequencing_util.h`](../sequencing_util.h): standalone table-entry
        reference helpers that `sequencing.h` does not use

### Key APIs

*   `UnsatisfiedOutgoingReferences`: Takes a `const std::vector<p4.v1.Entity>&`
    of canonical entities and an `IrP4Info`.
    *   Returns a `vector<EntityWithUnsatisfiedReferences>`: each entity that
        has dangling outgoing [`ConcreteTableReference`](../references.h)s,
        grouped with those references.
    *   Fails on an unknown `table_id`, a missing referenced `EXACT` field, an
        unset `TableAction` on a referring entry, a referenced field that is
        neither `EXACT` nor `OPTIONAL`, or an unsupported `Entity` variant.
    *   Ignores unknown match-field IDs, and treats an unknown action ID as
        having no outgoing references.
*   `StatefulReferenceChecker`: Tracks live entities incrementally against its
    `IrP4Info` (available through `ir_p4_info`).
    *   `AddEntity` and `UpdateEntity` reject unsatisfied outgoing references
        with `FailedPreconditionError`. `AddEntity` also rejects duplicate keys
        with `AlreadyExistsError`.
    *   `UpdateEntity`, `CanRemove`, and `RemoveEntity` return `NotFoundError`
        on untracked keys.
    *   For a removal that would orphan active incoming references, `CanRemove`
        returns `false` and `RemoveEntity` returns `FailedPreconditionError`.
    *   Error messages format entities with lenient `IR` translation
        (`allow_unsupported = true`). They fall back to key-only `IR`, then to
        raw `PI` textproto.
*   `SequencePiUpdatesIntoWriteRequests`: Takes an `IrP4Info`, canonical
    `p4.v1.Update` table updates, and an optional `max_batch_size` (default
    `5000`).
    *   Groups independent updates into dependency-ordered `WriteRequest`
        batches: referees first for `INSERT`, referrers first for `DELETE`.
*   `SequencePiUpdatesInPlace`: Takes an `IrP4Info` and the same updates, with
    no `max_batch_size`.
    *   Returns the batches as vectors of indices into the input updates.
*   Both sequencing functions fail on:
    *   unknown table or field IDs, or updates that are not table updates
    *   unknown action or parameter IDs (`NotFoundError`)
    *   an unset `TableAction` (`UnimplementedError`), so a key-only `DELETE`
        without an action fails
    *   a cyclic dependency between updates (`InvalidArgumentError`)
*   `GetEntitiesUnreachableFromRoots`: Takes canonical entities, an `IrP4Info`,
    and an `is_root_entity` predicate.
    *   Returns the non-root `TableEntry` entities that no root reaches through
        outgoing references. A non-root entry whose table has no
        `incoming_references` is always returned.
    *   Fails on every `clone_session_entry`, root or not, and on non-root
        multicast entries.

## Cross-File Invariants

*   **`@refers_to` vs. `@referenced_by` and `EXACT`-match-only destinations**:
    `ParseIrTableReferences` requires `@refers_to` on all P4-table-to-P4-table
    and P4-table-to-built-in edges, restricting `@referenced_by` strictly to
    built-in PRE sources. `builtin::clone_session_table` has no referenceable
    match fields or actions, so any reference that names it is rejected.
    *   Reference destinations must be `EXACT` match fields, never action
        parameters. `MODIFY` can change an entry's action parameters in place,
        but its match fields (its `EntityKey`) stay fixed until the entry is
        deleted, so incoming references stay valid across `MODIFY`.
    *   `OPTIONAL` match fields can be reference sources but not destinations.
        LPM, ternary, and range match fields can be neither.
    *   The table reference graph must be a strict DAG.
        [`CreateIrP4Info`](../ir.h), not `ParseIrTableReferences`, rejects all
        cycles, including self-loops.
*   **Canonical `PI` byte-string precondition across `entity_keys.h`,
    `references.h`, and `sequencing.h`**: `EntityKey`,
    [`ConcreteFieldReference`](../references.h), and the update dependency index
    in [`sequencing.cc`](../sequencing.cc) hash and compare raw `p4.v1` byte
    strings and never call [`ArbitraryToCanonicalByteString`](../ir_utils.h).
    *   [`references.cc`](../references.cc) does encode built-in PRE integer
        fields, such as `multicast_group_id` and `replica.instance`, with
        [`string_encodings::BitsetToP4RuntimeByteString`](../../string_encodings/byte_string.h).
    *   Non-canonical `PI` byte strings cause silent false negatives in update
        sequencing and false positives in `UnsatisfiedOutgoingReferences`.
*   **Dependency rank polarity (`rank[referee] > rank[referrer]`)**:
    `CreateIrP4Info` computes `dependency_rank_by_table_name` so referenced
    tables receive strictly higher ranks than referring tables.
    `StableSortUpdates` sorts by descending rank by default, which puts referees
    first and is safe for `INSERT`. Pass `reverse_ordering = true` for ascending
    rank. See [Known Limitations](#known-limitations) for its equal-rank caveat.
*   **Redundant satisfiers in `StatefulReferenceChecker`**: Because multiple
    destination entries can satisfy the same incoming
    [`ConcreteTableReference`](../references.h):
    *   `CanRemove` returns `true` when every active incoming reference that the
        entry satisfies still has at least one other tracked satisfier.
    *   `RemoveEntity` calls `CanRemove`, so read-only garbage collection checks
        and real removals give the same answer.

## Known Limitations

*   **`SequencePiUpdates*` ignores built-in PRE edges and rejects
    `packet_replication_engine_entry` updates**: `references.h` and
    `GetEntitiesUnreachableFromRoots` read [`IrTableReference`](../ir.proto).
    `SequencePiUpdatesIntoWriteRequests` and `SequencePiUpdatesInPlace` still
    build their dependency graph in [`sequencing.cc`](../sequencing.cc) from the
    deprecated [`IrMatchFieldReference`](../ir.proto) lists on `IrP4Info`, match
    field definitions, and action parameter definitions.
    *   A `packet_replication_engine_entry` update fails with
        `"Table with ID 0 does not exist"`, because
        `update.entity().table_entry()` defaults `table_id` to `0`.
    *   Sequence table updates separately from PRE updates.
*   **`StableSortUpdates` with `reverse_ordering = true` violates strict weak
    ordering**: [`sequencing.cc`](../sequencing.cc) implements
    `reverse_ordering = true` as `!b_may_depend_on_a` (`!(a > b)`). It returns
    `true` for equal-rank updates, which violates the strict weak ordering that
    `std::stable_sort` requires.
*   **Copy-heavy performance in `references.h`**:
    `UnsatisfiedOutgoingReferences` and `StatefulReferenceChecker` materialize
    exhaustive `btree_set<ConcreteTableReference>` combinations per entity, so
    they are slow on large forwarding tables. Keep them off latency-critical
    control-plane hot paths.
*   **Entity coverage across `EntityKey` and
    `GetEntitiesUnreachableFromRoots`**:
    *   `EntityKey` supports `TableEntry`, plus `MulticastGroupEntry` and
        `CloneSessionEntry` through
        [`PacketReplicationEntryKey`](../entity_keys.h). It returns
        `InvalidArgumentError` on other `p4.v1.Entity` variants.
    *   `GetEntitiesUnreachableFromRoots` accepts `multicast_group_entry` only
        when `is_root_entity` is `true`. It returns `UnimplementedError` on
        non-root multicast entries and on every `clone_session_entry`, because
        it rejects clone sessions before it calls `is_root_entity`.
    *   It silently reports every non-root entry whose table has no
        `incoming_references` as unreachable. Callers must not garbage-collect
        tables that P4 never references.

## End-to-End Path

1.  **Annotation parsing & DAG rank assignment**: During `CreateIrP4Info`,
    `ParseIrTableReferences` validates `@refers_to` and `@referenced_by`
    annotations against `IrP4Info` match/action definitions and `built_ins.h`,
    and returns the `IrTableReference` list. `CreateIrP4Info` then attaches
    `outgoing_references` and `incoming_references` to each table and built-in
    definition, verifies acyclicity, and assigns
    `dependency_rank_by_table_name`.
2.  **Concrete reference validation (`references.h`)**:
    [`PossibleIncomingConcreteTableReferences`](../references.h) and
    [`OutgoingConcreteTableReferences`](../references.h) project canonical
    `p4.v1.Entity` values onto `ConcreteTableReference` keys so
    `UnsatisfiedOutgoingReferences` (batch) or `StatefulReferenceChecker`
    (incremental) can verify that no active reference is dangling.
3.  **Root reachability (`GetEntitiesUnreachableFromRoots`)**: Given an
    `is_root_entity` callback, `GetEntitiesUnreachableFromRoots` runs BFS from
    root entities across [`GetOutgoingTableReferences`](../references.h) and
    `PossibleIncomingConcreteTableReferences` to return non-root table entries
    unreachable from any active root.
4.  **Update sequencing & batching (`SequencePiUpdatesIntoWriteRequests`)**:
    `SequencePiUpdatesIntoWriteRequests` builds a dependency graph over a `PI`
    table-update batch (ordering referee `INSERT` before referrer
    `INSERT`/`MODIFY`, and referrer `DELETE` before referee `DELETE`), groups
    independent updates by topological phase, and splits phases at
    `max_batch_size`. The default of `5000` updates keeps a batch within the 1
    MB gRPC status error-detail metadata limit of `P4RuntimeSession`.
