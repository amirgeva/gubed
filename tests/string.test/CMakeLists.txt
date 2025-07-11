add_executable(string.test main.cpp)
target_link_libraries(string.test PRIVATE GTest::gtest utils)

# Folder is set in the parent tests/CMakeLists.txt
