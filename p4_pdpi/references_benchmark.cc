#include <cstdint>
#include <utility>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"
#include "gutil/proto.h"
#include "p4/v1/p4runtime.pb.h"
#include "p4_pdpi/main_p4_pd.pb.h"
#include "p4_pdpi/pd.h"
#include "p4_pdpi/references.h"
#include "p4_pdpi/test_p4info.h"

namespace pdpi {
namespace {

using ::p4::v1::Entity;
using ::p4::v1::Replica;

// Generates a list of entities containing `num_references` table entries that
// can be referred to, and `num_referrer` table entries that refer to the first
// set of entries via match fields.
// NOTE:
// 1. If `num_references` < `num_referrer` => Generates dangling
//    references.
// 2. If `num_references` > `num_referrer` => Generates garbage entries
std::vector<Entity> GenerateTableEntries(int num_references, int num_referrer) {
  pdpi::TableEntries entries = *gutil::ParseTextProto<pdpi::TableEntries>(R"pb(
    entries {
      two_match_fields_table_entry {
        match { id_1: "str_id" id_2: "0x343" }
        action { do_thing_4 {} }
      }
    }
    entries {
      referring_by_match_field_table_entry {
        match { referring_id_1: "str_id" referring_id_2: "0x343" }
        action { do_thing_4 {} }
      }
    }
  )pb");
  auto pi_entities_or =
      pdpi::PdTableEntriesToPiEntities(pdpi::GetTestIrP4Info(), entries);
  CHECK(pi_entities_or.ok()) << pi_entities_or.status();
  auto pi_entities = *pi_entities_or;

  std::vector<Entity> result;
  result.reserve(num_references + num_referrer);

  const Entity& base_referenced_entry = pi_entities[0];
  for (int i = 0; i < num_references; ++i) {
    Entity entity = base_referenced_entry;
    entity.mutable_table_entry()->mutable_match(0)->mutable_exact()->set_value(
        absl::StrCat("str_id_", i));
    result.push_back(std::move(entity));
  }

  const Entity& base_referring_entry = pi_entities[1];
  for (int i = 0; i < num_referrer; ++i) {
    Entity entity = base_referring_entry;
    entity.mutable_table_entry()->mutable_match(0)->mutable_exact()->set_value(
        absl::StrCat("str_id_", i));
    result.push_back(std::move(entity));
  }
  return result;
}

// Generates a list of entities containing `num_references` table entries that
// are referred to by multicast replicas, and `num_referrer` of
// `multicast_replicas` entries that refer to the first set of entries.
// `replicas_per_entry` is the number of replicas a multicast group entry
// can have.
// NOTE:
// 1. If `num_references` < `num_referrer` => Generates dangling
//    references.
// 2. If `num_references` > `num_referrer` => Generates garbage entries
// 3. Number of multicast groups = `num_referrer`/`replicas_per_entry`
std::vector<::p4::v1::Entity> GenerateMulticastGroupEntries(
    int num_references, int num_referrer, int replicas_per_entry) {
  pdpi::TableEntries entries = *gutil::ParseTextProto<pdpi::TableEntries>(R"pb(
    entries {
      referenced_by_multicast_replica_table_entry {
        match { port: "some_port" instance: "0xbeef" }
        action { do_thing_4 {} }
      }
    }
    entries {
      multicast_group_table_entry {
        match { multicast_group_id: "0xdead" }
        action {
          replicate { replicas { port: "some_port" instance: "0xbeef" } }
        }
      }
    }
  )pb");

  auto pi_entities_or =
      pdpi::PdTableEntriesToPiEntities(pdpi::GetTestIrP4Info(), entries);
  CHECK(pi_entities_or.ok()) << pi_entities_or.status();
  auto pi_entities = *pi_entities_or;

  std::vector<::p4::v1::Entity> result;
  result.reserve(num_references + num_referrer);

  const Entity& base_referenced_entry = pi_entities[0];
  for (int i = 0; i < num_references; ++i) {
    Entity entity = base_referenced_entry;
    entity.mutable_table_entry()->mutable_match(0)->mutable_exact()->set_value(
        absl::StrCat("some_port_", i));
    result.push_back(std::move(entity));
  }

  Entity base_multicast_group_entry = pi_entities[1];
  const Replica& base_referencing_replica =
      base_multicast_group_entry.packet_replication_engine_entry()
          .multicast_group_entry()
          .replicas(0);
  base_multicast_group_entry.mutable_packet_replication_engine_entry()
      ->mutable_multicast_group_entry()
      ->clear_replicas();
  Entity entity = base_multicast_group_entry;
  for (int i = 0; i < num_referrer; ++i) {
    if (entity.packet_replication_engine_entry()
            .multicast_group_entry()
            .replicas_size() == replicas_per_entry) {
      result.push_back(std::move(entity));
      entity = base_multicast_group_entry;
    }
    Replica replica = base_referencing_replica;
    replica.set_port(absl::StrCat("some_port_", i));
    *entity.mutable_packet_replication_engine_entry()
         ->mutable_multicast_group_entry()
         ->add_replicas() = std::move(replica);
  }
  result.push_back(std::move(entity));
  return result;
}

// Benchmarks the performance of `UnsatisfiedOutgoingReferences` when processing
// parameterized normal table entries.
void BM_UnsatisfiedOutgoingReferences_TableEntries(benchmark::State& state) {
  int entry_to_reference = state.range(0);
  int referring_entries = state.range(1);
  const pdpi::IrP4Info& info = pdpi::GetTestIrP4Info();
  auto entities = GenerateTableEntries(entry_to_reference, referring_entries);

  for (auto s : state) {
    auto res = pdpi::UnsatisfiedOutgoingReferences(entities, info);
    benchmark::DoNotOptimize(res);
  }
  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                          (entry_to_reference + referring_entries));
}
BENCHMARK(BM_UnsatisfiedOutgoingReferences_TableEntries)
    ->ArgNames({"num_referenced_entries", "num_referring_entries"})
    ->Args({10, 10})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({10000, 10000})
    ->Args({100000, 100000})
    ->Args({10, 100})
    ->Args({10, 100000});

// Benchmarks the performance of `UnsatisfiedOutgoingReferences` when processing
// multicast references with potentially dangling references.
void BM_UnsatisfiedOutgoingReferences_Multicast(benchmark::State& state) {
  int entry_to_reference = state.range(0);
  int referring_entries = state.range(1);
  int replicas_per_entry = state.range(2);
  const pdpi::IrP4Info& info = pdpi::GetTestIrP4Info();
  auto entities = GenerateMulticastGroupEntries(
      entry_to_reference, referring_entries, replicas_per_entry);

  for (auto s : state) {
    auto res = pdpi::UnsatisfiedOutgoingReferences(entities, info);
    benchmark::DoNotOptimize(res);
  }
  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                          (entry_to_reference + referring_entries));
}
BENCHMARK(BM_UnsatisfiedOutgoingReferences_Multicast)
    ->ArgNames({"num_referenced_entries", "num_referring_replicas",
                "replicas_per_multicast_group"})
    ->Args({10, 10, 100})
    ->Args({100, 100, 100})
    ->Args({1000, 1000, 100})
    ->Args({10000, 10000, 100})
    ->Args({100000, 100000, 100})
    ->Args({10, 100, 100})
    ->Args({10, 100000, 100})
    ->Args({10, 100000, 10});

}  // namespace
}  // namespace pdpi

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
}
