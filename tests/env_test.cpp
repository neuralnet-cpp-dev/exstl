// env_test — exstl::env 平台读取（flag / str / integer）
// 退出码：0 = 通过，1 = 失败。
#include <cstdio>
#include <cstdlib>
#include <string>

#include <exstl/env.hpp>

#include "test_common.hpp"

int main()
{
    std::printf("env:\n");
#ifdef _WIN32
    _putenv_s("EXSTL_TEST_FLAG", "1");
    _putenv_s("EXSTL_TEST_STR", "hello");
    _putenv_s("EXSTL_TEST_INT", "42");
    _putenv_s("EXSTL_TEST_ZERO", "0");
    _putenv_s("EXSTL_TEST_BAD", "not-a-number");
#else
    setenv("EXSTL_TEST_FLAG", "1", 1);
    setenv("EXSTL_TEST_STR", "hello", 1);
    setenv("EXSTL_TEST_INT", "42", 1);
    setenv("EXSTL_TEST_ZERO", "0", 1);
    setenv("EXSTL_TEST_BAD", "not-a-number", 1);
#endif

    CHECK(exstl::env::flag("EXSTL_TEST_FLAG"), "flag set");
    CHECK(!exstl::env::flag("EXSTL_TEST_ZERO"), "flag 0 = off");
    CHECK(!exstl::env::flag("EXSTL_TEST_MISSING"), "missing flag = false");
    CHECK(exstl::env::str("EXSTL_TEST_STR") == "hello", "str value");
    CHECK(exstl::env::str("EXSTL_TEST_MISSING").empty(), "missing str = empty");
    CHECK(exstl::env::integer("EXSTL_TEST_INT", -1) == 42, "int value");
    CHECK(exstl::env::integer("EXSTL_TEST_BAD", -1) == -1, "bad int = fallback");
    CHECK(exstl::env::integer("EXSTL_TEST_MISSING", 7) == 7, "missing int = fallback");

    return test_summary("env_test");
}
