#include "instrumenter.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <memory>
#include "strutils.h"
#include "linemapper.h"

typedef std::vector<std::string> lines_vec;

static bool instrumentation_enabled = true;

void disable_instrumentation()
{
	instrumentation_enabled = false;
	Singleton<LineMapper>::Instance().disable();
}

lines_vec load_module_lines(const char* name)
{
	lines_vec res;
	std::string filename = std::string(name) + ".wren";
	std::ifstream file(filename);
	if (!file.fail())
	{
		std::string line;
		while (std::getline(file, line))
		{
			res.push_back(line);
		}
	}
	return res;
}


std::string unite(const lines_vec& lines)
{
	std::string code;
	for (const auto& line : lines)
	{
		code += line + "\n";
	}
	return code;
}

std::regex class_regex(R"(\s*class\s+(\w+)\s*\{)");
std::regex method_regex(R"(\s*(?:static\s+)?(\w+)\s*\(([^)]*)\)\s*\{)");
std::regex var_regex(R"(\s*var\s+(\w+)\s*=\s*.+)");

class Module : public IModule, public std::enable_shared_from_this<Module>
{
	xstring		m_Name;
	lines_vec	m_CodeLines;
	lines_vec	m_InstrumentedCode;

	struct Block
	{
		std::vector<std::string> variables;
	};

	std::string format_variables_string(const std::vector<Block>& block_stack)
	{
		std::ostringstream os;
		bool first = true;
		for(const auto& block : block_stack)
		{
			for (const auto& var_name : block.variables)
			{
				if (!first)
				{
					os << "+\"|";
				}
				else
					os << "\"";
				os << var_name << "=\"+" << var_name << ".toString";
				first = false;
			}
		}
		std::string res = os.str();
		if (res.empty())
			return "\"\""; // Return empty string if no variables
		return res;
	}

	void add_debugger_line(const std::string& ws, const std::string& class_name, const std::string& method_name, size_t line_index, const std::vector<Block>& block_stack)
	{
		size_t line_id;
		std::string instrumented_line;
		{
			std::ostringstream os;
			os << class_name << '.' << method_name << '.' << line_index;
			line_id = (std::hash<std::string>()(os.str())) & ((1ULL << 52)-1);
		}
		size_t instrumented_line_index = m_InstrumentedCode.size();
		Singleton<LineMapper>::Instance().add_line(line_id, shared_from_this(), 
												   instrumented_line_index, line_index);
		{
			std::ostringstream os;
			os << ws << "Gubedder.callback(" << line_id << ", " << format_variables_string(block_stack) << ")";
			instrumented_line = os.str();
		}
		m_InstrumentedCode.push_back(instrumented_line);
	}
public:
	Module(const xstring& name, const lines_vec& lines) 
		: m_Name(name)
		, m_CodeLines(lines)
		, m_InstrumentedCode(lines)
	{}

	virtual const xstring& get_name() const override
	{
		return m_Name;
	}

	virtual size_t get_line_count() const override
	{
		return m_CodeLines.size();
	}

	virtual const std::string& get_line(size_t index) const override
	{
		if (index < m_CodeLines.size())
		{
			return m_CodeLines[index];
		}
		static const std::string empty_line;
		return empty_line; // Return an empty string if index is out of bounds
	}

	char* allocate_code()
	{
		// Code for debugging purposes
		// 
		//for(size_t i=0;i<m_InstrumentedCode.size(); ++i)
		//{
		//	std::cout << (i + 1) << "  " << m_InstrumentedCode[i] << std::endl;
		//}
		std::string code = unite(m_InstrumentedCode);
		const size_t n = code.size();
		char* buffer = new char[n + 1];
		std::copy(code.begin(), code.end(), buffer);
		buffer[n] = 0;
		return buffer;
	}

	void instrument()
	{
		m_InstrumentedCode.clear();
		m_InstrumentedCode.push_back("import \"gubed\" for Gubedder");
		std::string class_name;
		std::string method_name;
		int brace_count = 0;
		std::vector<Block> block_stack;
		for (size_t i = 0; i < m_CodeLines.size(); ++i)
		{
			std::string line = m_CodeLines[i];
			{
				auto it = line.find("//");
				if (it != std::string::npos)
				{
					line = line.substr(0, it); // Remove comments
				}
			}
			const std::string ws = get_leading_white_space(line);
			std::string sline = trim(line);
			std::smatch match;
			if (std::regex_search(line, match, class_regex))
			{
				class_name = match[1].str();
			}
			if (!class_name.empty() && brace_count==1)
			{
				if (std::regex_search(line, match, method_regex))
				{
					method_name = match[1].str();
					block_stack.push_back(Block());
					size_t n=match.size();
					if (n > 2)
					{
						auto vars = tokenize(match[2].str(), ",", false);
						for(auto var : vars)
						{
							var.trim();
							if (!var.empty())
							{
								block_stack.back().variables.push_back(var);
							}
						}
					}
					++brace_count;
					m_InstrumentedCode.push_back(line);
					continue;
				}
			}
			if (!method_name.empty())
			{
				if (brace_count>=2)
					add_debugger_line(ws, class_name, method_name, i, block_stack);
				if (std::regex_search(line, match, var_regex))
				{
					std::string var_name = match[1].str();
					if (!var_name.empty() && !block_stack.empty())
					{
						block_stack.back().variables.push_back(var_name);
					}
				}
			}
			if (startswith(sline, "}"))
			{
				--brace_count;
				if (!block_stack.empty())
				{
					block_stack.pop_back();
				}
				if (brace_count == 1) 
					method_name = "";
				if (brace_count == 0)
					class_name = "";
			}
			if (endswith(sline, "{"))
			{
				++brace_count;
				block_stack.push_back(Block());
			}
			m_InstrumentedCode.push_back(line);
		}
	}
};

typedef std::shared_ptr<Module> ModulePtr;

std::unordered_map<std::string, ModulePtr> modules;

const char* load_module_code(const char* name)
{
	auto it = modules.find(name);
	if (it != modules.end())
	{
		return it->second->allocate_code(); // Return cached code if module is already loaded
	}
	auto lines = load_module_lines(name);
	if (lines.empty())
	{
		return nullptr; // No code found for the module
	}
	auto module = std::make_shared<Module>(name, lines);
	modules[name] = module;

	if (instrumentation_enabled)
		module->instrument();

	return module->allocate_code();
}
