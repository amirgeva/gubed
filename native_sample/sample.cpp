#include <iostream>
#include <wren.hpp>
#include <singleton.h>
#include <binder.h>
#include "sample.h"

void Sample::Print(int a)
{
	std::cout << a << std::endl;
}

//////////////////////////////////////////////////
//AUTOGENERATED CODE - DO NOT EDIT

void sample_Sample_Print_wrapper(WrenVM* vm)
{
	int value = static_cast<int>(wrenGetSlotDouble(vm, 1));
	Sample::Print(value);
}


void sample_Sample_Binder()
{
	Singleton<ForeignFunctions>::Instance().bind("sample.Sample.Print(_)", sample_Sample_Print_wrapper);
	const char* module_code=R"(
class Sample {
	foreign static Print(value)
}
)";
	Singleton<ForeignFunctions>::Instance().set_module_code("sample", module_code);
}

