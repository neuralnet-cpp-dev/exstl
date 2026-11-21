#pragma once

#include <cstdio>
#include <string>
#include <expected>
#include <exstl/error_macros.hpp>   // 宏岛头（尾部拉入 errors.hpp）

namespace exstl
{

namespace detail
{

// ── 无条件 fail-fast 的公共出口（EX_ASSERT_ALWAYS / EX_FATAL 的实现支撑）────
// 打印 "<tag> failed at <file>:<line>: <msg>" 后经 fail_abort 终止
// （Windows 下 fail_abort 已把 CRT 的 abort 弹窗/ WER 上报静音，见 errors.hpp）。
// ⚠ 与 EX_ASSERT 的区别是**语义**，不是开关：EX_ASSERT 是"可选诊断"（NDEBUG 下
// 为空），只允许用在"失败也不影响后续正确性"的检查上；本函数的两个宏是
// "前置条件校验"（失败后继续执行 = UB），NDEBUG 下同样终止。
[[noreturn]] inline void fatal_at(const char* tag, const char* file, int line, const char* msg)
{
    std::string s(tag);
    s += " failed at ";
    s += (file != nullptr) ? file : "?";
    s += ':';
    s += std::to_string(line);
    s += ": ";
    s += (msg != nullptr) ? msg : "?";
    fail_abort(s);
}

} // namespace detail

// ── 断言/提示宏 → 迁至宏岛头 exstl/error_macros.hpp ──
// EX_ASSERT / EX_ASSERT_ALWAYS / EX_FATAL / EX_REQUIRE / EX_HOT_FN /
// EX_LIKELY / EX_UNLIKELY 的定义随宏段整体迁出；本头仍经
// `#include <exstl/errors.hpp>`（其内含岛头包含）在宏展开点提供
// detail::fatal_at 等实现支撑。直接 include 本头的消费方照旧拿得到全部宏。

} // namespace exstl
