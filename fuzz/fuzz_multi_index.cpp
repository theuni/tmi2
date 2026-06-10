// Copyright (c) 2026 Bitcoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tmi.h>

#include <fuzzer/FuzzedDataProvider.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>

namespace {

struct Elem {
  uint16_t ordered_unique;
  uint16_t hashed_unique;
  uint8_t non_unique;
};

struct OrderedUniqueKey {
  using result_type = uint16_t;
  [[maybe_unused]] uint16_t operator()(const Elem &e) const {
    return e.ordered_unique;
  }
};
struct HashedUniqueKey {
  using result_type = uint16_t;
  [[maybe_unused]] uint16_t operator()(const Elem &e) const {
    return e.hashed_unique;
  }
};
struct NonUniqueKey {
  using result_type = uint8_t;
  [[maybe_unused]] uint8_t operator()(const Elem &e) const {
    return e.non_unique;
  }
};

using Container = tmi::multi_index_container<
    Elem, tmi::indexed_by<tmi::ordered_unique<OrderedUniqueKey>,
                          tmi::hashed_unique<HashedUniqueKey>,
                          tmi::ordered_non_unique<NonUniqueKey>,
                          tmi::hashed_non_unique<NonUniqueKey>>>;

void check_invariants(
    const Container &c, const std::map<uint16_t, Elem> &by_ordered_unique,
    const std::unordered_map<uint16_t, uint16_t> &by_hashed_unique) {
  const auto &ordered_unique_view = c.template get<0>();
  const auto &hashed_unique_view = c.template get<1>();
  const auto &ordered_nonunique_view = c.template get<2>();
  const auto &hashed_nonunique_view = c.template get<3>();

  const size_t n = by_ordered_unique.size();
  assert(ordered_unique_view.size() == n);
  assert(hashed_unique_view.size() == n);
  assert(ordered_nonunique_view.size() == n);
  assert(hashed_nonunique_view.size() == n);
  assert(by_hashed_unique.size() == n);
  assert(ordered_unique_view.empty() == by_ordered_unique.empty());

  // The std::map oracle iterates in ascending key order, so an elementwise
  // match against ordered_unique_view simultaneously verifies content and
  // ordering of that index.
  assert(std::ranges::equal(
      ordered_unique_view, by_ordered_unique,
      [](const Elem &a, const auto &kv) {
        return a.ordered_unique == kv.first &&
               a.hashed_unique == kv.second.hashed_unique &&
               a.non_unique == kv.second.non_unique;
      }));

  // ordered_non_unique traverses in non-decreasing key order.
  assert(std::ranges::is_sorted(ordered_nonunique_view, {},
                                &Elem::non_unique));
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  Container c;
  auto &ordered_unique_view = c.template get<0>();
  auto &hashed_unique_view = c.template get<1>();

  // Oracles: ordered_unique key -> full element, and hashed_unique key ->
  // ordered_unique key. Together they let us predict insert/erase outcomes.
  std::map<uint16_t, Elem> by_ordered_unique;
  std::unordered_map<uint16_t, uint16_t> by_hashed_unique;

  while (fdp.remaining_bytes() > 0) {
    uint8_t op = fdp.ConsumeIntegralInRange<uint8_t>(0, 7);
    switch (op) {
    default:
      break;
    case 0:
    case 1: { // insert
      uint16_t ou = fdp.ConsumeIntegral<uint16_t>();
      uint16_t hu = fdp.ConsumeIntegral<uint16_t>();
      uint8_t nu = fdp.ConsumeIntegral<uint8_t>();
      Elem e{ou, hu, nu};

      bool ou_hit = by_ordered_unique.find(ou) != by_ordered_unique.end();
      bool hu_hit = by_hashed_unique.find(hu) != by_hashed_unique.end();
      bool expect_inserted = !ou_hit && !hu_hit;

      auto [it, inserted] = ordered_unique_view.insert(e);
      assert(inserted == expect_inserted);
      if (inserted) {
        by_ordered_unique.emplace(ou, e);
        by_hashed_unique.emplace(hu, ou);
        assert(it->ordered_unique == ou);
      }
      break;
    }
    case 2: { // emplace
      uint16_t ou = fdp.ConsumeIntegral<uint16_t>();
      uint16_t hu = fdp.ConsumeIntegral<uint16_t>();
      uint8_t nu = fdp.ConsumeIntegral<uint8_t>();

      bool expect_inserted =
          by_ordered_unique.find(ou) == by_ordered_unique.end() &&
          by_hashed_unique.find(hu) == by_hashed_unique.end();
      auto [it, inserted] = ordered_unique_view.emplace(Elem{ou, hu, nu});
      assert(inserted == expect_inserted);
      if (inserted) {
        by_ordered_unique.emplace(ou, Elem{ou, hu, nu});
        by_hashed_unique.emplace(hu, ou);
      }
      break;
    }
    case 3: { // erase via ordered_unique find
      uint16_t ou = fdp.ConsumeIntegral<uint16_t>();
      auto it = ordered_unique_view.find(ou);
      bool oracle_hit = by_ordered_unique.find(ou) != by_ordered_unique.end();
      assert((it != ordered_unique_view.end()) == oracle_hit);
      if (it != ordered_unique_view.end()) {
        uint16_t hu = it->hashed_unique;
        ordered_unique_view.erase(it);
        by_ordered_unique.erase(ou);
        by_hashed_unique.erase(hu);
      }
      break;
    }
    case 4: { // erase via hashed_unique find
      uint16_t hu = fdp.ConsumeIntegral<uint16_t>();
      auto it = hashed_unique_view.find(hu);
      auto oracle_it = by_hashed_unique.find(hu);
      assert((it != hashed_unique_view.end()) ==
             (oracle_it != by_hashed_unique.end()));
      if (it != hashed_unique_view.end()) {
        uint16_t ou = it->ordered_unique;
        assert(ou == oracle_it->second);
        hashed_unique_view.erase(it);
        by_ordered_unique.erase(ou);
        by_hashed_unique.erase(hu);
      }
      break;
    }
    case 5: { // find/count by ordered_unique key
      uint16_t ou = fdp.ConsumeIntegral<uint16_t>();
      auto it = ordered_unique_view.find(ou);
      size_t cnt = ordered_unique_view.count(ou);
      bool oracle_hit = by_ordered_unique.find(ou) != by_ordered_unique.end();
      assert((it != ordered_unique_view.end()) == oracle_hit);
      assert(cnt == (oracle_hit ? 1u : 0u));
      break;
    }
    case 6: { // count by non_unique key across both non-unique indices
      uint8_t nu = fdp.ConsumeIntegral<uint8_t>();
      size_t expect = 0;
      for (const auto &kv : by_ordered_unique)
        if (kv.second.non_unique == nu)
          ++expect;
      assert(c.template get<2>().count(nu) == expect);
      assert(c.template get<3>().count(nu) == expect);
      break;
    }
    case 7: { // clear (rarely, to exercise teardown mid-stream)
      if ((fdp.ConsumeIntegral<uint8_t>() & 0x1f) == 0) {
        c.template get<0>().clear();
        by_ordered_unique.clear();
        by_hashed_unique.clear();
      }
      break;
    }
    }

    check_invariants(c, by_ordered_unique, by_hashed_unique);
  }

  return 0;
}
