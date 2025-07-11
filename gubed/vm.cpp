#include <iostream>
#include <string>
#include <sstream>
#include "vm.h"
#include "foreigns.h"
#include "instrumenter.h"
#include "linemapper.h"
#include "ui.h"

class QuitException : public std::exception {};

std::shared_ptr<IUserInterface> UI = IUserInterface::Create();

const char* debugger_class_code = R"(
class Gubedder {
	foreign static callback(line_id, var_data)
}
)";

const std::string callback_key = "gubed.Gubedder.callback(_,_)";

IUserInterface::Action action = IUserInterface::STEP;

extern "C" {

	static void DebugCallback(WrenVM* vm)
	{
		size_t line_id = size_t(wrenGetSlotDouble(vm, 1));
		const char* vars = wrenGetSlotString(vm, 2);
		LineDetails details;
		if (Singleton<LineMapper>::Instance().get_line_details(line_id, details))
		{
			const std::shared_ptr<IModule>& module = details.module;
			size_t line_index = details.line_index;
			if (action == IUserInterface::CONTINUE)
			{
				if (!UI->IsBreakpoint(module->get_name(), line_index))
					return;
			}
			if (line_index < module->get_line_count())
			{
				UI->LoadModule(module->get_name());
				UI->HighlightLine(line_index);
				UI->SetVariables(vars ? vars : "");
				action = UI->UILoop();
				if (action == IUserInterface::QUIT)
				{
					throw QuitException();
				}
			}
		}
	}

	static WrenForeignMethodFn bind_foreign_method(
		WrenVM* vm,
		const char* module_name,
		const char* className,
		bool isStatic,
		const char* signature)
	{
		if (!isStatic) return nullptr;
		std::ostringstream os;
		os << module_name << "." << className << "." << signature;
		std::string key = os.str();
		if (key == callback_key)
		{
			return DebugCallback;
		}
		return (WrenForeignMethodFn)find_foreign_method(key);
	}

	static void system_print(WrenVM* vm, const char* text)
	{
		std::cout << text;
	}



	static void report_error(WrenVM* vm,
							 WrenErrorType type,
							 const char* module,
							 int line,
							 const char* message)
	{
		LineDetails	details;
		if (Singleton<LineMapper>::Instance().search_line_details(module, line, details))
		{
			line = details.line_index;
		}
		else
			return;
		switch (type)
		{
			case WREN_ERROR_COMPILE:
				std::cerr << "Compile error in module '" << module
					<< "' at line " << line << ": " << message << std::endl;
				break;

			case WREN_ERROR_RUNTIME:
				if (module)
				{
					std::cerr << "Runtime error in module '" << module
						<< "' at line " << line << ": " << message << std::endl;
				}
				else
					std::cerr << message << std::endl;
				break;

			case WREN_ERROR_STACK_TRACE:
				std::cerr << "Stack trace in module '" << module
					<< "' at line " << line << ": " << message << std::endl;
				break;
		}
	}

	void release_buffer(WrenVM* vm, const char* name, WrenLoadModuleResult result)
	{
		delete[] result.source;
	}

	WrenLoadModuleResult load_module(WrenVM* vm, const char* name)
	{
		WrenLoadModuleResult result = { nullptr,nullptr,nullptr };
		result.onComplete = release_buffer;
		result.source = load_module_code(name);
		return result;
	}


	void* malloc_wren_realloc(void* memory, size_t size, void* user_data)
	{
		if (!memory && size == 0) return nullptr;
		if (memory == nullptr && size > 0)
		{
			return malloc(size);
		}
		else
			if (memory && size == 0)
			{
				free(memory);
				return nullptr;
			}
			else
			{
				return realloc(memory, size);
			}
	}

}


VMWrapper::VMWrapper()
	: vm(nullptr)
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.reallocateFn = malloc_wren_realloc;
	config.loadModuleFn = load_module;
	config.bindForeignMethodFn = bind_foreign_method;
	config.writeFn = system_print;
	config.errorFn = report_error;
	{
		vm = wrenNewVM(&config);
		InitializeForeignModules(".", vm);

		WrenInterpretResult result = wrenInterpret(vm, "gubed", debugger_class_code);
		if (result != WREN_RESULT_SUCCESS)
		{
			throw std::runtime_error("Failed to load debugger code");
		}
	}
}

VMWrapper::~VMWrapper()
{
	if (vm)
		wrenFreeVM(vm);
	ShutdownForeignModules();
}

void VMWrapper::run_module(const std::string& module_name)
{
	std::ostringstream os;
	os << "import \"" << module_name << "\"";
	std::string code_str = os.str();
	const char* code = code_str.c_str();
	try
	{
		auto result = wrenInterpret(vm, "main", code);
		if (result != WREN_RESULT_SUCCESS)
		{
			throw std::runtime_error("Failed to interpret module: " + module_name);
		}
	}
	catch (const QuitException&)
	{

	}
	
}

