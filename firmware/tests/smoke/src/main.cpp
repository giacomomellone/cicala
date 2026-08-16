
#include <zephyr/ztest.h>

ZTEST_SUITE(kveld_smoke, NULL, NULL, NULL, NULL, NULL);

ZTEST(kveld_smoke, test_harness_runs)
{
    zassert_true(true, "ztest runs on native_sim");
}

ZTEST(kveld_smoke, test_cpp17_available)
{
    constexpr struct {
        int deck;
        int depth;
    } q{5, 2};
    const auto [deck, depth] = q;

    zassert_equal(deck, 5);
    zassert_equal(depth, 2);
}
