[![build](https://github.com/google/p4-infra/actions/workflows/ci.yml/badge.svg)](https://github.com/google/p4-infra/actions/workflows/ci.yml)

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

The PacketLib library provides functionality to parse a raw byte string
representation of a network packet and represent it as a collection of C++
header objects. Conversely, it can serialize the packetlib representation back
into a raw byte string.

This is a work in progress.

This is not an officially supported Google product.

## Cutting a Release

The version of releases follows the year, month, date, and patch format.

1.  Create a new branch with the name `release-yyyymmdd`.
2.  In the branch, the version in the MODULE.bazel must be updated to match
    `yyyymmdd.p`, where p is the patch number (initially 0).
    *   Note that this branch should not be deleted to keep an archive of all
        releases and it makes it easier to add patches for a release.
    *   Tip: You can do steps 1 and 2 in one swoop by editing MODULE.bazel
        directly in the browser and committing the change to a new branch of the
        correct name.
3.  Create a new release on Github with the tag being in the form `yyyymmdd.p`,
    the title being `Gutil yyyymmdd.p (month year)`, and the corresponding
    release notes (can be auto-generated).
    *   The release archive must be added to the Github release for the Bazel
        Central Registry to achieve
        [archive checksum stability](https://blog.bazel.build/2023/02/15/github-archive-checksum.html)
    *   The release archive can be added after creating a release. Download the
        source zip and re-uploading it as the stable release archive.
4.  The
    [Bazel Central Registry for Gutil](https://github.com/bazelbuild/bazel-central-registry/tree/main/modules/gutil)
    must also be updated accordingly by following the
    [contribution guidelines](https://github.com/bazelbuild/bazel-central-registry/blob/main/docs/README.md).
