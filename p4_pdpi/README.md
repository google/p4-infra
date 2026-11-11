[![build](https://github.com/google/p4-infra/actions/workflows/ci.yml/badge.svg)](https://github.com/google/p4-infra/actions/workflows/ci.yml)

# P4 PDPI

P4Runtime generally uses a program-independent representation (or PI) for P4
entities such as table entries, counters, etc. This is achieved by using numeric
IDs instead of names. The downside of this is that the representation is hard to
read by humans. In contrast, a program-dependent (or PD) representation uses
names and is generally more friendly to humans.

This library provides several PD-like representations, and code to automatically
convert between them. It includes:

-   **Translation** (`ir.h`, `pd.h`): Converts P4Runtime entities, packet I/O
    messages, read and write requests, and write statuses between PI, an
    intermediate representation (IR) with named fields, and per-program PD
    protos. All conversions go through IR.
-   **P4Info validation** (`ir.h`): `CreateIrP4Info` validates a `P4Info` and
    its annotations once, and returns the `IrP4Info` that every translation call
    takes.
-   **PD proto generation** (`pdgen.bzl`): The `p4_pd_proto` build rule
    generates a PD proto for a P4 program from its `P4Info`.
-   **P4Info union** (`p4info_union_lib.h`): `UnionP4info` merges the `P4Info`s
    of several P4 programs into one.
-   **[Reference Library](references.md)**: Validates dependencies expressed
    through @refers_to and @referenced_by annotations across P4 entities.
-   **Sequencing** (`sequencing.h`): Orders PI updates into write requests that
    respect those dependencies.

This is not an officially supported Google product.
