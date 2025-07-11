cmake_minimum_required(VERSION 3.16)

# Tests folder
set(TESTS_FOLDER "Tests")

# Add test subdirectories
add_subdirectory(string.test)

# Set folder for all test targets
set_target_properties(string.test PROPERTIES FOLDER ${TESTS_FOLDER})