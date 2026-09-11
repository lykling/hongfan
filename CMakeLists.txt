cmake_minimum_required(VERSION 3.24)
project(hongfan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion)

add_library(hongfan_core STATIC
  src/value.cpp
  src/world.cpp
  src/yaml.cpp
  src/yaml_flow.cpp
  src/expr.cpp
  src/expr_lexer.cpp
  src/expr_parse.cpp
  src/rule.cpp
  src/ruleset.cpp
  src/ruleset_check.cpp
  src/state_load.cpp
  src/policy.cpp
  src/chronicle.cpp
  src/engine.cpp
  src/interpolate.cpp
)
target_include_directories(hongfan_core PUBLIC include)
target_include_directories(hongfan_core SYSTEM PRIVATE third_party)

add_executable(hongfan src/main.cpp)
target_link_libraries(hongfan PRIVATE hongfan_core)

enable_testing()

foreach(t expr_test yaml_test world_test engine_test interpolate_test)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE hongfan_core)
  target_include_directories(${t} SYSTEM PRIVATE third_party)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # doctest 2.4.12 触发 clang 22 新告警：废弃头 <ciso646> 的 #warning 与 __COUNTER__ 宏（宏展开点在用户文件，SYSTEM 抑制不了）
    target_compile_options(${t} PRIVATE "-Wno-#warnings" -Wno-c2y-extensions)
  endif()
  add_test(NAME ${t} COMMAND ${t})
endforeach()

add_test(NAME e2e COMMAND bash ${CMAKE_SOURCE_DIR}/tests/e2e.sh $<TARGET_FILE:hongfan> ${CMAKE_SOURCE_DIR})
