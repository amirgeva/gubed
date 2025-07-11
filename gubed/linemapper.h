#pragma once

#include <unordered_map>
#include <memory>
#include <singleton.h>
#include "xstring.h"

class IModule
{
public:
	virtual ~IModule() = default;
	virtual const xstring& get_name() const = 0;
	virtual size_t get_line_count() const = 0;
	virtual const std::string& get_line(size_t index) const = 0;
};

struct LineDetails
{
	std::shared_ptr<IModule> module;
	size_t instrumented_line_index;
	size_t line_index;
};

typedef size_t LineId;

class LineMapper
{
	std::unordered_map<LineId, LineDetails>		m_LineMap;
	bool										m_Disabled = false;

	LineMapper() = default;
	LineMapper(const LineMapper&) = delete;
	LineMapper& operator=(const LineMapper&) = delete;
	friend class Singleton<LineMapper>;
public:
	void disable()
	{
		m_Disabled = true;
	}

	void add_line(LineId id, std::shared_ptr<IModule> module, 
				  size_t instrumented_line_index, size_t line_index)
	{
		m_LineMap[id] = { module, instrumented_line_index, line_index };
	}

	bool search_line_details(const std::string& module_name, 
							 size_t instrumented_line_index, 
							 LineDetails& details) const
	{
		if (m_Disabled)
		{
			details.line_index = instrumented_line_index;
			return true;
		}
		for (const auto& pair : m_LineMap)
		{
			const LineDetails& detail = pair.second;
			if (detail.module->get_name() == module_name && 
				detail.instrumented_line_index == instrumented_line_index)
			{
				details = detail;
				return true;
			}
		}
		return false;
	}

	bool get_line_details(LineId id, LineDetails& details) const
	{
		auto it = m_LineMap.find(id);
		if (it != m_LineMap.end())
		{
			details = it->second;
			return true;
		}
		return false;
	}
};
