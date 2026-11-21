#pragma once

// ══════════════════════════════════════════════════════════════════════════
//  exstl/parallel — 顶层并行算法（线程池 + 阈值串行回落的薄封装）
//
//  PARALLEL_THRESHOLD 与 global_thread_pool 调度组合成对齐
//  std::for_each / std::transform 的顶层并行算法。阈值常量随本头自持。
//
//  依赖：threadpool.hpp（调度）。
// ══════════════════════════════════════════════════════════════════════════

#include <cstddef>
#include <iterator>  // for std::ranges::distance
#include <utility>   // for std::forward

#include <exstl/threadpool.hpp> // global_thread_pool

namespace exstl
{
// ── 并行阈值 ─────────────────────────────────────────────────────
// 元素数 >= 此值时启用线程池并行；低于则串行执行（避免调度开销）。
// 线程池调度开销约 50-200μs（Windows mutex+cv），仅当计算量足够时并行才有收益。
//
// 实测结果（32 核 CPU, Release -O3）：
//   元素数   串行μs  并行μs  加速比   建议
//   16384     9.6    9.8     0.98x   无差别
//   32768    18.8   19.9     0.94x   无差别
//   65536    38.7  121.7     0.32x   串行优（调度开销显现）
//   131072   76.4  182.8     0.42x   串行优
//   262144  151.6  196.0     0.77x   串行优
//   524288  308.7  201.5     1.53x   ← 首次稳定 > 1.5x
//   1048576 620.3  186.7     3.32x   并行优（稳定）
//   2097152 1239.5 217.3     5.70x   并行优
//   4194304 2657.3 340.6     7.80x   并行优
// 取 524288 (512K) 作为阈值：在加速比首次稳定 > 1.5x 的位置。
// 注：1024-4096 元素数测出的高加速比是亚微秒级测量噪声，不可靠。
inline constexpr std::size_t PARALLEL_THRESHOLD = 524288;

// ── 顶层并行算法函数 ─────────────────────────────────────────────────────
// 用法与 std::for_each / std::transform 完全一致，内部自动选择串行/并行。
// 替代 std::execution::par 的线程创建开销，也替代手写 for 循环。

// for_each
template <typename Iterator, typename Func>
inline void for_each(Iterator first, Iterator last, Func&& func)
{
    const auto n = static_cast<std::size_t>(std::ranges::distance(first, last));
    if (n >= PARALLEL_THRESHOLD)
    {
        global_thread_pool().parallel_for_each(first, last, std::forward<Func>(func));
    }
    else
    {
        for (auto it = first; it != last; ++it)
            func(*it);
    }
}

// transform（一元）
template <typename InputIt, typename OutputIt, typename UnaryOp>
inline void transform(InputIt first, InputIt last, OutputIt d_first, UnaryOp&& op)
{
    const auto n = static_cast<std::size_t>(std::ranges::distance(first, last));
    if (n >= PARALLEL_THRESHOLD)
    {
        global_thread_pool().parallel_transform(first, last, d_first, std::forward<UnaryOp>(op));
    }
    else
    {
        for (; first != last; ++first, ++d_first)
            *d_first = op(*first);
    }
}

// transform（二元）
template <typename InputIt1, typename InputIt2, typename OutputIt, typename BinaryOp>
inline void transform(InputIt1 first1, InputIt1 last1, InputIt2 first2, OutputIt d_first,
                      BinaryOp&& op)
{
    const auto n = static_cast<std::size_t>(std::ranges::distance(first1, last1));
    if (n >= PARALLEL_THRESHOLD)
    {
        global_thread_pool().parallel_transform(first1, last1, first2, d_first,
                                                std::forward<BinaryOp>(op));
    }
    else
    {
        for (; first1 != last1; ++first1, ++first2, ++d_first)
            *d_first = op(*first1, *first2);
    }
}

// transform_reduce（一元 transform）
template <typename InputIt, typename T, typename BinaryReduceOp, typename UnaryTransformOp>
inline T transform_reduce(InputIt first, InputIt last, T init, BinaryReduceOp&& reduce_op,
                          UnaryTransformOp&& transform_op)
{
    const auto n = static_cast<std::size_t>(std::ranges::distance(first, last));
    if (n >= PARALLEL_THRESHOLD)
    {
        return global_thread_pool().parallel_transform_reduce(
            first, last, init, std::forward<BinaryReduceOp>(reduce_op),
            std::forward<UnaryTransformOp>(transform_op));
    }
    else
    {
        T acc = init;
        for (; first != last; ++first)
            acc = reduce_op(acc, transform_op(*first));
        return acc;
    }
}

// transform_reduce（二元 transform）
template <typename InputIt1, typename InputIt2, typename T, typename BinaryReduceOp,
          typename BinaryTransformOp>
inline T transform_reduce(InputIt1 first1, InputIt1 last1, InputIt2 first2, T init,
                          BinaryReduceOp&& reduce_op, BinaryTransformOp&& transform_op)
{
    const auto n = static_cast<std::size_t>(std::ranges::distance(first1, last1));
    if (n >= PARALLEL_THRESHOLD)
    {
        return global_thread_pool().parallel_transform_reduce(
            first1, last1, first2, init, std::forward<BinaryReduceOp>(reduce_op),
            std::forward<BinaryTransformOp>(transform_op));
    }
    else
    {
        T acc = init;
        for (; first1 != last1; ++first1, ++first2)
            acc = reduce_op(acc, transform_op(*first1, *first2));
        return acc;
    }
}

// parallel_for_blocks（块级并行，无阈值保护）
template <typename Iterator, typename Func>
inline void parallel_for_blocks(Iterator first, Iterator last, Func&& func)
{ global_thread_pool().parallel_for_blocks(first, last, std::forward<Func>(func)); }

// ── parallel_for_samples（样本级并行）────────────────────────────────────
// 样本是"完整子任务"（一次前向/整段编码），每样本工作量未知，故提供两个口径：
//   · 带 work_per_sample 的版本：total_work = num_samples × work_per_sample
//     （单位 = 等效标量元素数）低于 PARALLEL_THRESHOLD 即串行回落，
//     与 for_each / transform 的门控口径一致。
//   · 不带工作量估算的版本：由调用方保证每样本为重量级任务（如整段文本
//     编码、一次前向），仅在样本数 ≤ 1 时串行。
// 动机（#P12）：小算子密集路径（窄 N GEMV 等）每"样本"只有一块 64×K
// 乘加，仅按样本数进池会让 2–8 片的小并行区反复唤醒 worker；实测
// p50 数百 µs、p99 毫秒级（计算本身仅十几 µs）→ 毫秒级抖动。
template <typename Func>
inline void parallel_for_samples(std::size_t num_samples, std::size_t work_per_sample,
                                 Func&& func)
{
    // 串行回落：用 (THRESHOLD-1) 整除比较，避免 num_samples × work_per_sample 溢出
    if (num_samples <= 1 || work_per_sample <= (PARALLEL_THRESHOLD - 1) / num_samples)
    {
        for (std::size_t i = 0; i < num_samples; ++i)
            func(i);
        return;
    }
    global_thread_pool().parallel_for_samples(num_samples, std::forward<Func>(func));
}

template <typename Func> inline void parallel_for_samples(std::size_t num_samples, Func&& func)
{
    if (num_samples <= 1)
    {
        for (std::size_t i = 0; i < num_samples; ++i)
            func(i);
    }
    else
    {
        global_thread_pool().parallel_for_samples(num_samples, std::forward<Func>(func));
    }
}
} // namespace exstl
