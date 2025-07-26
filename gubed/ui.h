#pragma once

#include <unordered_map>
#include <set>
#include <memory>
#include "xstring.h"


class IUserInterface
{
public:
	virtual ~IUserInterface() = default;

	virtual void load_module(const xstring& module_name) = 0;
	virtual void highlight_line(size_t line_index) = 0;
	virtual void set_variables(const std::string& variables) = 0;
	virtual bool is_breakpoint(const xstring& module_name, size_t line_index) = 0;
	virtual void print(const char* text) = 0;

	enum Action
	{
		NONE,
		STEP,
		CONTINUE,
		QUIT
	};

	virtual Action ui_loop() = 0;

	static std::shared_ptr<IUserInterface> Create();
};

