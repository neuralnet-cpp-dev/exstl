# exSTL

Extended STL — 补标准库缺口的 header-only C++23 库：错误处理宏族、线程池、并行算法、`observer_ptr`、环境变量平台分支。

从 [neuralnet.cpp](https://github.com/neuralnet-cpp-dev/neuralnet.cpp) 的 foundation 头独立而来；命名空间 `exstl`，宏前缀 `EX_`。

## 内容

| 头 | 提供 |
|----|------|
| `<exstl/error_macros.hpp>` | 宏岛：`EX_TRY` / `EX_UNWRAP` / `EX_EXIT` / `EX_FAIL` / `EX_ASSERT*` / `EX_FATAL` / `EX_REQUIRE` / `EX_LIKELY`… |
| `<exstl/errors.hpp>` | `exstl::Error` / `Result<T>`（`std::expected`）/ `parse_number` |
| `<exstl/assert.hpp>` | `detail::fatal_at`（宏展开点支撑） |
| `<exstl/threadpool.hpp>` | `exstl::ThreadPool` / `global_thread_pool()` |
| `<exstl/parallel.hpp>` | `for_each` / `transform` / `transform_reduce` / `parallel_for_*` + `PARALLEL_THRESHOLD` |
| `<exstl/observer_ptr.hpp>` | `observer_ptr<T>` / `make_observer`（C++26 提案轻量替代） |
| `<exstl/env.hpp>` | `exstl::env::flag` / `str` / `integer`（MSVC `_dupenv_s` vs `getenv` 收敛） |

**不包含**：`Scalar` / `BLOCK_SIZE` / 探针门 `NN_PROBES_ENABLED` 等 neuralnet 构建配置——那些留在主仓。

## 用法

```cmake
add_subdirectory(path/to/exstl)          # 或 FetchContent
target_link_libraries(app PRIVATE exstl::exstl)
```

```cpp
#include <exstl/error_macros.hpp>
#include <exstl/threadpool.hpp>

exstl::Result<int> f() {
    EX_TRY(v, read_config());
    return *v;
}

exstl::global_thread_pool().parallel_for_each(v.begin(), v.end(), [](int& x){ ++x; });
```

## 构建与测试（standalone）

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

被主仓 `add_subdirectory` 时默认不建测试（`EXSTL_ENABLE_TESTS=OFF`）。

## 依赖

- C++23（`std::expected`）
- Threads（POSIX pthread；Windows 随 CRT）
- 零第三方库
