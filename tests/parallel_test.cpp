// parallel_test — 顶层并行算法（阈值回落语义）
// 退出码：0 = 通过，1 = 失败。
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>

#include <exstl/parallel.hpp>

#include "test_common.hpp"

namespace
{
constexpr std::size_t kSmall = exstl::PARALLEL_THRESHOLD / 16;
constexpr std::size_t kLarge = exstl::PARALLEL_THRESHOLD + exstl::PARALLEL_THRESHOLD / 8;

void test_for_each()
{
    std::printf("for_each:\n");
    for (std::size_t kN : {kSmall, kLarge})
    {
        std::vector<int> hits(kN, 0);
        exstl::for_each(hits.begin(), hits.end(), [](int& v) { ++v; });
        bool full = true;
        for (std::size_t i = 0; i < kN; ++i)
            if (hits[i] != 1) full = false;
        CHECK(full, kN >= exstl::PARALLEL_THRESHOLD ? "大输入并行覆盖" : "小输入串行覆盖");
    }
    std::vector<int> empty;
    exstl::for_each(empty.begin(), empty.end(), [](int& v) { ++v; });
    CHECK(true, "空区间");
    std::vector<int> one{0};
    exstl::for_each(one.begin(), one.end(), [](int& v) { ++v; });
    CHECK(one[0] == 1, "单元素");
}

void test_transform_reduce()
{
    std::printf("transform_reduce:\n");
    std::vector<int> data(kSmall);
    std::iota(data.begin(), data.end(), 1);
    const long long got = exstl::transform_reduce(
        data.begin(), data.end(), 0LL, [](long long a, long long b) { return a + b; },
        [](int x) { return static_cast<long long>(x); });
    long long want = 0;
    for (int x : data) want += x;
    CHECK(got == want, "小输入 transform_reduce == 串行");
}

void test_threshold()
{
    std::printf("PARALLEL_THRESHOLD:\n");
    CHECK(exstl::PARALLEL_THRESHOLD == 524288u, "阈值 = 524288");
}
} // namespace

int main()
{
    test_for_each();
    test_transform_reduce();
    test_threshold();
    return test_summary("parallel_test");
}
