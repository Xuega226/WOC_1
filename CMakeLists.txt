# 指定CMake的最低版本要求，建议与你的安装版本一致或稍低
cmake_minimum_required(VERSION 3.20)

# 定义项目名称。这个名字会用在后续的变量中，如 `PROJECT_NAME`
project(smart_ptr)

# 告诉CMake：我要创建名为 smartptr 的可执行文件，它的源代码是 test.cpp
add_executable(smartptr test.cpp)
