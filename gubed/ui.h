#pragma once

#include <unordered_map>
#include <set>
#include <memory>
#include "xstring.h"


class IUserInterface
{
public:
	virtual ~IUserInterface() = default;

	virtual void LoadModule(const xstring& module_name) = 0;
	virtual void HighlightLine(size_t line_index) = 0;
	virtual void SetVariables(const std::string& variables) = 0;
	virtual bool IsBreakpoint(const xstring& module_name, size_t line_index) = 0;

	enum Action
	{
		NONE,
		STEP,
		CONTINUE,
		QUIT
	};

	virtual Action UILoop() = 0;

	static std::shared_ptr<IUserInterface> Create();
};

