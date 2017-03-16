#include "fluent/max_lattice.h"

#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ra/all.h"

using ::testing::UnorderedElementsAreArray;

namespace fluent {

TEST(MaxLattice, SimpleMerge) {
  MaxLattice<int> l("l");
  EXPECT_THAT(l.Reveal(), 0);

  MaxLattice<int> o("o", 10);

  std::set<std::tuple<int>> s1 = {{2}, {3}, {1}};

  l.Merge(ra::make_iterable(&s1));
  EXPECT_THAT(l.Reveal(), 3);

  l.Merge(o);
  EXPECT_THAT(l.Reveal(), 10);
}

}  // namespace fluent

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
