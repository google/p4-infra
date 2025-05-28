![build](https://github.com/pins/pins-infra/workflows/build/badge.svg)

# P4 Infrastructure

This repository contains infrastructure and libraries that assist in testing P4.

The PDPI library provides a human readable format the intermediate
representation (IR) of program-dependent (PD) and program-independent (PI) and
the code to convert between the different representations. P4Runtime generally
uses a program-independent representation (or PI) for P4 entities such as table
entries, counters, etc. This is achieved by using numeric IDs instead of names.
The downside of this is that the representation is hard to read by humans. In
contrast, a program-dependent (or PD) representation uses names and is generally
more friendly to humans.

This is a work in progress.

This is not an officially supported Google product.
