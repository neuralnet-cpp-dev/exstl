// threadpool_test — ThreadPool 本体（construction / parallel_* / stability）
// 退出码：0 = 通过，1 = 失败。
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <numeric>
#include <vector>

#include <exstl/threadpool.hpp>

#include "test_common.hpp"

namespace
{
void test_construction()
{
    std::printf("构造/析构:\n");
    {
        exstl::ThreadPool pool(4);
        CHECK(pool.size() == 4, "指定 4 线程构造 → size()==4");
        std::atomic<int> done{0};
        pool.parallel_for_samples(4, [&](std::size_t) { done.fetch_add(1); });
        CHECK(done.load() == 4, "并行区 4 样本全覆盖");
    }
    {
        exstl::ThreadPool pool(0);
        CHECK(pool.size() == 1, "0 线程回落 1");
    }
    {
        exstl::ThreadPool pool(2);
        std::vector<int> hits(4096, 0);
        pool.parallel_for_each(hits.begin(), hits.end(), [](int& v) { ++v; });
        bool full = true;
        for (int v : hits)
            if (v != 1) full = false;
        CHECK(full, "析构后新池仍可全覆盖");
    }
}

void test_for_each()
{
    std::printf("parallel_for_each:\n");
    exstl::ThreadPool pool(4);
    std::vector<int> hits(10000, 0);
    pool.parallel_for_each(hits.begin(), hits.end(), [](int& v) { ++v; });
    bool full = true;
    for (int v : hits)
        if (v != 1) full = false;
    CHECK(full, "全覆盖每元素恰好一次");

    std::vector<int> empty;
    pool.parallel_for_each(empty.begin(), empty.end(), [](int& v) { ++v; });
    CHECK(true, "空区间不崩溃");

    std::vector<int> one{0};
    pool.parallel_for_each(one.begin(), one.end(), [](int& v) { ++v; });
    CHECK(one[0] == 1, "单元素");
}

void test_transform_reduce()
{
    std::printf("parallel_transform_reduce:\n");
    exstl::ThreadPool pool(4);
    std::vector<int> data(50000);
    std::iota(data.begin(), data.end(), 1);
    const long long par = pool.parallel_transform_reduce(
        data.begin(), data.end(), 0LL, [](long long a, long long b) { return a + b; },
        [](int x) { return static_cast<long long>(x); });
    long long serial = 0;
    for (int x : data) serial += x;
    CHECK(par == serial, "并行归约 == 串行参考");
}

void test_stability()
{
    std::printf("重复小并行区稳定性:\n");
    exstl::ThreadPool pool(4);
    std::atomic<int> total{0};
    for (int i = 0; i < 20000; ++i)
    {
        std::vector<int> v(64, 0);
        pool.parallel_for_each(v.begin(), v.end(), [](int& x) { ++x; });
        total.fetch_add(v[0]);
    }
    CHECK(total.load() == 20000, "2 万次小并行区无死锁/丢任务");
}
} // namespace

int main()
{
    test_construction();
    test_for_each();
    test_transform_reduce();
    test_stability();
    return test_summary("threadpool_test");
}
