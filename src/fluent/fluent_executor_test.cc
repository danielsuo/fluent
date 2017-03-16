#include "fluent/fluent_executor.h"

#include <cstddef>

#include <set>
#include <tuple>
#include <utility>

#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "zmq.hpp"

#include "fluent/channel.h"
#include "fluent/fluent_builder.h"
#include "fluent/infix.h"
#include "ra/all.h"
#include "testing/captured_stdout.h"

using ::testing::UnorderedElementsAreArray;

namespace fluent {

TEST(FluentExecutor, SimpleProgram) {
  zmq::context_t context(1);

  // clang-format off
  auto f = fluent("inproc://yolo", &context)
    .table<int>("t")
    .scratch<int, int, float>("s")
    .channel<std::string, float, char>("c")
    .RegisterRules([](auto& t, auto& s, auto& c) {
      using namespace fluent::infix;
      return std::make_tuple(
        t <= (t.Iterable() | ra::count()),
        t <= (s.Iterable() | ra::count()),
        t <= (c.Iterable() | ra::count())
      );
    });
  // clang-format on

  using T = std::set<std::tuple<int>>;
  using S = std::set<std::tuple<int, int, float>>;
  using C = std::set<std::tuple<std::string, float, char>>;

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));
  EXPECT_THAT(f.Get<2>().Get(), UnorderedElementsAreArray(C{}));

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}, {1}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));
  EXPECT_THAT(f.Get<2>().Get(), UnorderedElementsAreArray(C{}));

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}, {1}, {2}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));
  EXPECT_THAT(f.Get<2>().Get(), UnorderedElementsAreArray(C{}));
}

TEST(FluentExecutor, AllOperations) {
  zmq::context_t context(1);

  auto int_tuple_to_string = [](const std::tuple<int>& t) {
    return std::tuple<std::string>(std::to_string(std::get<0>(t)));
  };

  // clang-format off
  auto f = fluent("inproc://yolo", &context)
    .table<int>("t")
    .scratch<int>("s")
    .stdout()
    .RegisterRules([&int_tuple_to_string](auto& t, auto& s, auto& stdout) {
      using namespace fluent::infix;
      auto a = t <= (t.Iterable() | ra::count());
      auto b = t += t.Iterable();
      auto c = t -= s.Iterable();
      auto d = s <= (t.Iterable() | ra::count());
      auto e = stdout <= (s.Iterable() | ra::map(int_tuple_to_string));
      auto f = stdout += (s.Iterable() | ra::map(int_tuple_to_string));
      return std::make_tuple(a, b, c, d, e, f);
    });
  // clang-format on

  using T = std::set<std::tuple<int>>;
  CapturedStdout captured;

  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_STREQ("", captured.Get().c_str());

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_STREQ("1\n1\n", captured.Get().c_str());

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}, {1}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_STREQ("1\n1\n2\n2\n", captured.Get().c_str());
}

TEST(FluentExecutor, SimpleCommunication) {
  auto reroute = [](const std::string& s) {
    return [s](const std::tuple<std::string, int>& t) {
      return std::make_tuple(s, std::get<1>(t));
    };
  };

  zmq::context_t context(1);
  // clang-format off
  auto ping = fluent("inproc://ping", &context)
    .channel<std::string, int>("c")
    .RegisterRules([&reroute](auto& c) {
      using namespace fluent::infix;
      return std::make_tuple(
        c <= (c.Iterable() | ra::map(reroute("inproc://pong")))
      );
    });
  auto pong = fluent("inproc://pong", &context)
    .channel<std::string, int>("c")
    .RegisterRules([&reroute](auto& c) {
      using namespace fluent::infix;
      return std::make_tuple(
        c <= (c.Iterable() | ra::map(reroute("inproc://ping")))
      );
    });
  // clang-format on

  using C = std::set<std::tuple<std::string, int>>;
  C catalyst = {{"inproc://pong", 42}};
  ping.MutableGet<0>().Merge(ra::make_iterable(&catalyst));

  for (int i = 0; i < 3; ++i) {
    pong.Receive();
    EXPECT_THAT(pong.Get<0>().Get(),
                UnorderedElementsAreArray(C{{"inproc://pong", 42}}));
    pong.Tick();
    EXPECT_THAT(pong.Get<0>().Get(), UnorderedElementsAreArray(C{}));

    ping.Receive();
    EXPECT_THAT(ping.Get<0>().Get(),
                UnorderedElementsAreArray(C{{"inproc://ping", 42}}));
    ping.Tick();
    EXPECT_THAT(ping.Get<0>().Get(), UnorderedElementsAreArray(C{}));
  }
}

TEST(FluentExecutor, ComplexProgram) {
  using Tuple = std::tuple<int>;
  using T = std::set<Tuple>;
  using S = std::set<Tuple>;

  zmq::context_t context(1);

  auto plus_one_times_two = [](const std::tuple<int>& t) {
    return std::tuple<int>((1 + std::get<0>(t)) * 2);
  };
  auto is_even = [](const auto& t) { return std::get<0>(t) % 2 == 0; };
  auto f = fluent("inproc://yolo", &context)
               .table<int>("t")
               .scratch<int>("s")
               .RegisterRules([plus_one_times_two, is_even](auto& t, auto& s) {
                 using namespace fluent::infix;
                 auto a = t += (s.Iterable() | ra::count());
                 auto b = t <= (t.Iterable() | ra::map(plus_one_times_two));
                 auto c = s <= t.Iterable();
                 auto d = t -= (s.Iterable() | ra::filter(is_even));
                 return std::make_tuple(a, b, c, d);
               });

  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));

  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(T{{0}}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(S{}));
}

TEST(FluentExecutor, SimpleBootstrap) {
  zmq::context_t context(1);

  using Tuples = std::set<std::tuple<int>>;
  Tuples xs = {{1}, {2}, {3}, {4}, {5}};

  // clang-format off
  auto f = fluent("inproc://yolo", &context)
    .table<int>("t")
    .scratch<int>("s")
    .RegisterBootstrapRules([&xs](auto& t, auto& s) {
      using namespace fluent::infix;
      return std::make_tuple(
        t <= ra::make_iterable(&xs),
        s <= ra::make_iterable(&xs)
      );
    })
    .RegisterRules([&xs](auto&, auto&) {
      return std::make_tuple();
    });
  // clang-format on

  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(Tuples{}));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(Tuples{}));
  f.BootstrapTick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(xs));
  EXPECT_THAT(f.Get<1>().Get(), UnorderedElementsAreArray(Tuples{}));
}


TEST(FluentExecutor, Lattices) {
  zmq::context_t context(1);

  std::set<std::tuple<int>> xs = {{1}, {2}, {3}, {4}, {5}};
  std::set<std::tuple<int>> ys = {{4}, {5}, {3}, {1}, {2}};
  std::set<std::tuple<int, int>> zs = {{1, 5}, {1, 4}};
  std::set<std::tuple<bool>> b = {{false}, {true}, {false}};

  MaxLattice<int> omaxl("l", 10);
  MapLattice<int, MaxLattice<int>> omapl;

  // clang-format off
  auto f = fluent("inproc://yolo", &context)
    .table<int>("t")
    .lattice<BoolLattice>("bl")
    .lattice<MaxLattice<int>>("maxl")
    .lattice<MinLattice<int>>("minl")
    .lattice<MapLattice<int, MaxLattice<int>>>("mapl")
    .RegisterRules([&xs, &ys, &zs, &b, &omaxl, &omapl](auto& t, auto& bl, auto& maxl, auto& minl, auto& mapl) {
      using namespace fluent::infix;
      return std::make_tuple(
        t <= ra::make_iterable(&xs),
        bl <= omapl.size().gt_eq(0),
        maxl <= omaxl,
        minl <= ra::make_iterable(&ys),
        mapl <= ra::make_iterable(&zs)
      );
    });
  // clang-format on

  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(std::set<std::tuple<int>>{}));
  EXPECT_THAT(f.Get<1>().Reveal(), false);
  EXPECT_THAT(f.Get<2>().Reveal(), 0);
  EXPECT_THAT(f.Get<3>().Reveal(), 1000000);
  f.Tick();
  EXPECT_THAT(f.Get<0>().Get(), UnorderedElementsAreArray(xs));
  EXPECT_THAT(f.Get<1>().Reveal(), true);
  EXPECT_THAT(f.Get<2>().Reveal(), 10);
  EXPECT_THAT(f.Get<3>().Reveal(), 1);
  std::unordered_map<int, MaxLattice<int>> res = f.Get<4>().Reveal();
  for (auto it = res.begin(); it != res.end(); it++) {
    EXPECT_THAT((it->second).Reveal(), 5);
  }
}


}  // namespace fluent

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
