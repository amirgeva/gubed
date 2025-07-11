#include <iostream>
#include "vm.h"
#include "instrumenter.h"
#include "cmdline.h"
#include "ui.h"

// In order to the script without debugging, use the following:
// gubed -di script.wren
COMMAND_LINE_OPTION(di, false, "Disable instrumentation")
{
	disable_instrumentation();
}

int main(int argc, char* argv[])
{
	try
	{
		PROCESS_COMMAND_LINE_P("<script>", 1, 1);
		xstring target_module;
		*cmd >> target_module;
		VMWrapper vm;
		vm.run_module(target_module);
	} catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "An unknown error occurred." << std::endl;
		return 1;
	}
	return 0;
}
