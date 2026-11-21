#pragma once

// ── exstl/env — 环境变量读取的唯一入口（平台分支收敛）──────────────────────
// 库内一切环境变量读取都经本头，不再各自写 getenv / _dupenv_s 样板。
// MSVC CRT 把 getenv 标记 deprecated（-Werror 下报错）→ 走 _dupenv_s；
// clang/gcc 走 std::getenv。分支只存在于此处。
//
// 取值约定：flag = "存在、非空、首字符 != '0'"（"0" = 关）。
// 调用点自行缓存（进程内恒定的开关用 `static const bool on = env::flag(...)`）。
// ─────────────────────────────────────────────────────────────────────────

#include <cstdlib>
#include <string>

namespace exstl::env
{

// 环境变量开关：存在、非空、首字符 != '0' → true（"0" = 关）。
[[nodiscard]] inline bool flag(const char* name) noexcept
{
#if defined(_MSC_VER)
    char* buf = nullptr;
    std::size_t len = 0;
    const bool ok = (_dupenv_s(&buf, &len, name) == 0 && buf != nullptr && buf[0] != '\0');
    const bool on = ok && buf[0] != '0';
    std::free(buf);
    return on;
#else
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
#endif
}

// 环境变量字符串：未设置 / 读取失败 → 空串。
[[nodiscard]] inline std::string str(const char* name)
{
#if defined(_MSC_VER)
    char* buf = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) return {};
    std::string s(buf);
    std::free(buf);
    return s;
#else
    const char* v = std::getenv(name);
    return v != nullptr ? std::string(v) : std::string{};
#endif
}

// 环境变量整数：strtol 十进制解析（跳过前导空白、接受正负号），要求整串合法
// （无尾随字符）；缺失 / 非数字 / 溢出 int → fallback。
[[nodiscard]] inline int integer(const char* name, int fallback) noexcept
{
    const std::string s = str(name);
    if (s.empty()) return fallback;
    const char* p = s.c_str();
    char* end = nullptr;
    const long v = std::strtol(p, &end, 10);
    if (end == p || *end != '\0') return fallback;
    if (v < -2147483647L - 1L || v > 2147483647L) return fallback;
    return static_cast<int>(v);
}

} // namespace exstl::env
