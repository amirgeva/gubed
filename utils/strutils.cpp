#include "strutils.h"

std::vector<xstring> tokenize(const xstring& line, const xstring& delimiter_characters, bool include_empty_tokens)
{
	std::vector<xstring> tokens;

	if (line.empty())
		return tokens;

	size_t start = 0;
	size_t end = 0;

	while (end != std::string::npos)
	{
		// Find the next delimiter
		end = line.find_first_of(delimiter_characters, start);

		// Extract the token
		xstring token;
		if (end == std::string::npos)
			token = line.substr(start);
		else
			token = line.substr(start, end - start);

		// Add the token if it's not empty or if we're including empty tokens
		if (!token.empty() || include_empty_tokens)
			tokens.push_back(token);

		// Move past this delimiter
		if (end != std::string::npos)
			start = end + 1;
	}

	return tokens;
}

bool endswith(const std::string& line, const std::string& suffix)
{
	if (suffix.length() > line.length())
		return false;

	return line.compare(line.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool startswith(const std::string& line, const std::string& prefix)
{
	if (prefix.length() > line.length())
		return false;

	return line.compare(0, prefix.length(), prefix) == 0;
}

std::string get_leading_white_space(const std::string& line)
{
	if (line.empty())
		return "";

	size_t first_non_whitespace = line.find_first_not_of(" \t\n\r\f\v");

	// If the string is all whitespace or empty
	if (first_non_whitespace == std::string::npos)
		return line;

	// Return the leading whitespace substring
	return line.substr(0, first_non_whitespace);
}

std::string trim(const std::string line)
{
	std::string sline = line;
	if (!sline.empty())
	{
		size_t first = sline.find_first_not_of(" \t\n\r\f\v");
		if (first != std::string::npos)
		{
			size_t last = sline.find_last_not_of(" \t\n\r\f\v");
			sline = sline.substr(first, last - first + 1);
		}
		else
		{
			sline = "";
		}
	}
	return sline;
}
