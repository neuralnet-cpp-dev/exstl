// error_macro_test — EX_UNWRAP / EX_EXIT / EX_TRY / EX_TRY_CHECK / EX_TRY_MSG / EX_FAIL
// 子进程模式：--expect-abort / --expect-exit <n>
// 退出码：0 = 通过，1 = 失败。
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string>
#include <utility>

#include <exstl/error_macros.hpp>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "test_common.hpp"

namespace
{
constexpr int kSkip = -1;
std::string g_self;

exstl::Result<int> ok_int() { return 42; }
exstl::Result<int> bad_int() { return std::unexpected(exstl::Error{"inner failure"}); }
exstl::Result<void> ok_void() { return {}; }
exstl::Result<std::string> payload() { return std::string{"payload"}; }

exstl::Result<int> propagate_plain()
{
    EX_TRY(r, bad_int());
    return *r;
}
exstl::Result<int> propagate_ctx()
{
    EX_TRY_MSG(r, "读取配置", bad_int());
    return *r;
}
exstl::Result<int> fail_source() { EX_FAIL("源头错误"); }

int run_child(const char* mode, int expect_exit)
{
    std::string cmd = "\"" + g_self + "\" " + mode;
    const int rc = std::system(cmd.c_str());
#ifdef _WIN32
    const int code = rc;
#else
    if (WIFEXITED(rc)) code = WEXITSTATUS(rc);
    else return kSkip;
#endif
    if (code == kSkip) return kSkip;
    if (expect_exit < 0) return code != 0 ? 0 : 1; // any nonzero = terminated
    return code == expect_exit ? 0 : 1;
}
} // namespace

int main(int argc, char** argv)
{
    g_self = argv[0];
    if (argc >= 2)
    {
        if (std::string(argv[1]) == "--expect-abort")
        {
            (void)EX_UNWRAP(bad_int());
            return 0; // not reached
        }
        if (std::string(argv[1]) == "--expect-exit" && argc >= 3)
        {
            const int want = std::atoi(argv[2]);
            EX_EXIT(bad_int(), want);
            return 0; // not reached
        }
    }

    CHECK(*ok_int() == 42, "ok_int value");
    CHECK(ok_void().has_value(), "ok_void");
    CHECK(*payload() == "payload", "payload move");

    {
        auto r = propagate_plain();
        CHECK(!r && r.error().message == "inner failure", "EX_TRY propagates message");
    }
    {
        auto r = propagate_ctx();
        CHECK(!r && r.error().message == "读取配置: inner failure", "EX_TRY_MSG prefix");
    }
    {
        auto r = fail_source();
        CHECK(!r && r.error().message == "源头错误", "EX_FAIL source");
    }
    {
        int side = 0;
        auto counting = [&]() -> exstl::Result<int>
        {
            EX_TRY_CHECK((side += 1, bad_int()));
            return 0;
        };
        auto r = counting();
        CHECK(!r && side == 1, "EX_TRY_CHECK evaluates once");
    }
    {
        auto v = exstl::parse_number<int>("123");
        CHECK(v && *v == 123, "parse_number int");
        auto bad = exstl::parse_number<int>("12x");
        CHECK(!bad, "parse_number rejects trailing junk");
    }

    // abort 的退出码平台相关（Linux 134/SIGABRT，Windows 3 等）——只要非零即终止
    const int abort_rc = run_child("--expect-abort", /*any_nonzero=*/-1);
    if (abort_rc == kSkip)
        std::printf("[SKIP] abort child unavailable\n");
    else
        CHECK(abort_rc == 0, "EX_UNWRAP abort child");

    const int exit_rc = run_child("--expect-exit 7", 7);
    if (exit_rc == kSkip)
        std::printf("[SKIP] exit child unavailable\n");
    else
        CHECK(exit_rc == 0, "EX_EXIT code child");

    return test_summary("error_macro_test");
}
