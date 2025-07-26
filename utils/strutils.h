#pragma once

#include <string>
#include <vector>
#include "xstring.h"

std::vector<xstring> tokenize(const xstring& line, const xstring& delimiter_characters, bool include_empty_tokens);
bool endswith(const std::string& line, const std::string& suffix);
bool startswith(const std::string& line, const std::string& prefix);
std::string get_leading_white_space(const std::string& line);
std::string trim(const std::string line);
