#pragma once

#include <wren.hpp>

class VMWrapper
{
	WrenVM* vm;

	VMWrapper(const VMWrapper&) = delete; // Disable copy constructor
	VMWrapper& operator=(const VMWrapper&) = delete; // Disable assignment operator
public:
	VMWrapper();
	~VMWrapper();

	void run_module(const std::string& module_name);
};
