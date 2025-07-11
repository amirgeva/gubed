#pragma once

#include <iostream>
#include <ostream>
#include <unordered_map>
#include <string>
#include <wren.hpp>
#include "singleton.h"

class ForeignFunctions
{
public:
	using wrapper = void(*)(WrenVM* vm);

	void set_module_code(const char* module_name, const char* code)
	{
		m_ModuleCode[module_name] = code;
	}

	void bind(const char* name, wrapper func)
	{
		m_Mapping[name] = func;
	}

	wrapper get(const std::string& name) const
	{
		auto it = m_Mapping.find(name);
		if (it != m_Mapping.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void register_modules(WrenVM* vm) const
	{
		for (const auto& [module_name, code] : m_ModuleCode)
		{
			WrenInterpretResult result = wrenInterpret(
				vm,
				module_name.c_str(),
				code.c_str());
			if (result != WREN_RESULT_SUCCESS)
			{
				std::cerr << "Failed to register native module " << module_name << std::endl;
			}
		}
	}

private:
	std::unordered_map<std::string, wrapper> m_Mapping;
	std::unordered_map<std::string, std::string> m_ModuleCode;
};
