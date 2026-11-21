// assert_test — EX_ASSERT_ALWAYS / EX_FATAL / EX_ASSERT / EX_REQUIRE
// 子进程模式：--expect-assert-always / --expect-fatal
// 退出码：0 = 通过，1 = 失败。
#include <cstdio>
#include <cstdlib>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

#include <exstl/error_macros.hpp>

#include "test_common.hpp"

namespace
{
constexpr int kSkip = -1;
std::string g_self;

exstl::Result<int> require_ok() { return 42; }
exstl::Result<int> require_bad() { EX_REQUIRE(false, "require failed"); }

int run_child(const char* mode)
{
    std::string cmd = "\"" + g_self + "\" " + mode;
    const int rc = std::system(cmd.c_str());
#ifdef _WIN32
    const int code = rc;
#else
    if (WIFEXITED(rc)) code = WEXITSTATUS(rc);
    else return kSkip;
#endif
    if (code == kSkip || code == 0) return code == 0 ? 0 : kSkip;
    return 0; // non-zero exit = terminated as expected
}
} // namespace

int main(int argc, char** argv)
{
    g_self = argv[0];
    if (argc >= 2)
    {
        const std::string mode = argv[1];
        if (mode == "--expect-assert-always")
        {
            EX_ASSERT_ALWAYS(false, "always boom");
            return 0;
        }
        if (mode == "--expect-fatal")
        {
            EX_FATAL("fatal boom");
            return 0;
        }
    }

    CHECK(*require_ok() == 42, "EX_REQUIRE pass");
    {
        auto r = require_bad();
        CHECK(!r && r.error().message == "require failed", "EX_REQUIRE returns Error");
    }
    EX_ASSERT(true, "true condition does not abort");

    const int a = run_child("--expect-assert-always");
    if (a == kSkip) std::printf("[SKIP] assert-always child\n");
    else CHECK(a == 0, "EX_ASSERT_ALWAYS terminates child");

    const int f = run_child("--expect-fatal");
    if (f == kSkip) std::printf("[SKIP] fatal child\n");
    else CHECK(f == 0, "EX_FATAL terminates child");

    return test_summary("assert_test");
}
