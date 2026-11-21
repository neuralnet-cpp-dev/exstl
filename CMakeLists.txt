cmake_minimum_required(VERSION 3.20)
project(exstl VERSION 0.1.0 LANGUAGES CXX)

# ── 门面：header-only INTERFACE 库 ─────────────────────────────────────────
# 被主仓 add_subdirectory 时：只创建 exstl::exstl 目标，不拉子目录外任何东西。
# standalone 时：额外开测试。
add_library(exstl INTERFACE)
add_library(exstl::exstl ALIAS exstl)

target_include_directories(exstl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(exstl INTERFACE cxx_std_23)  # std::expected

# threadpool 用 std::thread；Windows 上 MSVC/clang 自动链 CRT，POSIX 需 pthread
find_package(Threads REQUIRED)
target_link_libraries(exstl INTERFACE Threads::Threads)

# ── 双模式：standalone 才建测试 ────────────────────────────────────────────
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(EXSTL_STANDALONE ON)
else()
    set(EXSTL_STANDALONE OFF)
endif()

option(EXSTL_ENABLE_TESTS "Build exstl tests" ${EXSTL_STANDALONE})

if(EXSTL_ENABLE_TESTS)
    enable_testing()
    foreach(t IN ITEMS
        error_macro_test
        assert_test
        threadpool_test
        parallel_test
        env_test
        observer_ptr_test
    )
        add_executable(${t} tests/${t}.cpp)
        target_link_libraries(${t} PRIVATE exstl::exstl)
        target_compile_features(${t} PRIVATE cxx_std_23)
        add_test(NAME ${t} COMMAND ${t})
    endforeach()
endif()

# ── 安装（供 find_package / 打包）──────────────────────────────────────────
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS exstl EXPORT exstlTargets)
install(EXPORT exstlTargets
    NAMESPACE exstl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/exstl
)
