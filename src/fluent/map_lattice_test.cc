#include "fluent/map_lattice.h"
#include "fluent/max_lattice.h"

#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "common/function_traits.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ra/all.h"

using ::testing::UnorderedElementsAreArray;

namespace fluent {

namespace {

template <typename A, typename B, typename C>
const A& fst(const std::tuple<A, B, C>& t) {
  return std::get<0>(t);
}

template <typename A, typename B, typename C>
const B& snd(const std::tuple<A, B, C>& t) {
  return std::get<1>(t);
}

template <typename A, typename B, typename C>
const C& thd(const std::tuple<A, B, C>& t) {
  return std::get<2>(t);
}

}  // namespace

TEST(MapLattice, SimpleMerge) {
  std::set<std::tuple<int, int>> s1 = {{1, 5}, {1, 4}};
  MapLattice<int, MaxLattice<int>> mapl("mapl");

  mapl.Merge(ra::make_iterable(&s1));
  std::unordered_map<int, MaxLattice<int>> res = mapl.Reveal();
  for (auto it = res.begin(); it != res.end(); it++) {
    EXPECT_THAT((it->second).Reveal(), 5);
  }
}

TEST(MapLattice, HasIterableCheck) {
  bool res = fluent::has_Iterable<MapLattice<int, MaxLattice<int>>>::value;
  EXPECT_THAT(res, true);
  res = fluent::has_Iterable<std::unordered_map<int, int>>::value;
  EXPECT_THAT(res, false);
}

}  // namespace fluent

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}