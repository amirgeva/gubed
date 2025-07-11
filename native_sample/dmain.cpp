#include "binder.h"
#include <wren.hpp>
#include "initializers.h"

#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C" {

	EXPORT void Initialize(WrenVM* vm)
	{
		initialize_bindings();
		Singleton<ForeignFunctions>::Instance().register_modules(vm);
	}

	EXPORT void Shutdown()
	{
	}

	EXPORT ForeignFunctions::wrapper GetFunction(const char* name)
	{
		return Singleton<ForeignFunctions>::Instance().get(name);
	}

}
