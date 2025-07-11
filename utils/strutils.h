#pragma once

#include <string>
#include <vector>

std::vector<std::string> tokenize(const std::string& line, const std::string& delimiter_characters, bool include_empty_tokens);
bool endswith(const std::string& line, const std::string& suffix);
bool startswith(const std::string& line, const std::string& prefix);
std::string get_leading_white_space(const std::string& line);
std::string trim(const std::string line);
