# Smart Pointer Demo (C++)

一个用于学习和练习的 C++ 智能指针小项目，手写实现了简化版的 `unique_ptr` / `shared_ptr` / `weak_ptr` 核心机制，并提供了可直接运行的测试程序。

## 项目目标

- 理解 RAII（资源获取即初始化）与自动资源释放
- 掌握独占所有权与共享所有权模型
- 练习移动语义、拷贝控制与引用计数实现
- 熟悉 CMake 构建流程

## 功能概览

当前主要代码位于 `smartptr.h`：

- `uniquePtr<T, Deleter>`
  - 独占所有权，禁止拷贝，支持移动构造/移动赋值
  - 支持 `get()`、`reset()`、`operator*`、`operator->`
- `uniquePtr<T[], Deleter>`
  - 提供数组特化，支持 `operator[]`
- `sharedPtr<T>`
  - 通过控制块维护 `sharedCount` / `weakCount`
  - 支持拷贝共享、移动转移、`useCount()`、`reset()`
- `weakPtr<T>`
  - 提供弱引用接口（`expired()` / `lock()` 语义）
- 工具函数
  - `make_unique<T>(...)`
  - `make_shared<T>(...)`

## 项目结构

- `smartptr.h`：智能指针实现
- `test.cpp`：断言测试与示例入口（`main`）
- `CMakeLists.txt`：CMake 构建配置

## 环境要求

- C++ 编译器（建议支持 C++11 及以上）
- CMake 3.20+

## 构建与运行

在项目根目录执行：

```powershell
cmake -S . -B build
cmake --build build
./build/smartptr
```

Windows + MinGW 常见可执行文件路径为：

```powershell
.\build\smartptr.exe
```

程序成功时输出：

```text
All tests passed.
```

## 已覆盖测试（`test.cpp`）

- `testUniquePtrBasic()`：`uniquePtr` 基础行为（构造、移动、reset）
- `testUniquePtrArray()`：数组特化访问与修改
- `testMakeUnique()`：`make_unique` 构造对象
- `testSharedPtrCount()`：`sharedPtr` 引用计数与生命周期
- `testMakeShared()`：`make_shared` 构造对象

## 当前限制与后续可改进点

这是教学/练习性质实现，不等同于标准库实现，建议继续完善：

- 补充 `weakPtr` 相关完整可用性与测试用例
- 增加异常安全细节与更严格边界检查
- 支持更多标准接口（如比较运算、swap、自定义分配策略等）
- 增加多线程场景下更系统的压力测试

## 许可证

当前仓库未声明许可证。如需开源发布，建议补充 `LICENSE` 文件（如 MIT）。
