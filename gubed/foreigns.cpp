#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <wren.hpp>
#include "foreigns.h"



std::vector<std::string> find_shared_libraries(const std::string& path);

#ifdef WIN32

#include <windows.h>

const std::string shared_extension=".dll";

class NativeModule
{
public:
	NativeModule(const std::string& path)
		: dll_path(path), hModule(nullptr), Initialize(nullptr), Shutdown(nullptr), GetFunction(nullptr)
	{
		try
		{
			std::cout << "Loading module: " << dll_path << std::endl;
			hModule = LoadLibraryA(dll_path.c_str());
			if (!hModule)
			{
				DWORD err = GetLastError();          // <-- grab it right away
				char msgBuf[512]{};
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
							   FORMAT_MESSAGE_IGNORE_INSERTS,
							   nullptr, err,
							   0,                     // language (0 = caller locale)
							   msgBuf, sizeof msgBuf,
							   nullptr);
				printf("LoadLibraryA failed (error %lu): %s\n", err, msgBuf);

				throw std::runtime_error("Failed to load module: " + dll_path);
			}
			Initialize = (void (*)(WrenVM*))GetProcAddress(hModule, "Initialize");
			if (!Initialize) throw std::runtime_error("No Initialize function found in " + dll_path);
			Shutdown = (void (*)())GetProcAddress(hModule, "Shutdown");
			if (!Shutdown) throw std::runtime_error("No Shutdown function found in " + dll_path);
			GetFunction = (void* (*)(const char*))GetProcAddress(hModule, "GetFunction");
			if (!GetFunction) throw std::runtime_error("No GetFunction found in " + dll_path);
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			if (hModule)
			{
				FreeLibrary(hModule);
				hModule = nullptr;
			}
			Initialize = nullptr;
			Shutdown = nullptr;
			GetFunction = nullptr;
		}
	}

	~NativeModule()
	{
		if (is_valid())
		{
			if (Shutdown) Shutdown();  // Call the Shutdown function to clean up the module
			FreeLibrary(hModule);
			hModule = nullptr;
		}
	}

	bool is_valid() const
	{
		return hModule != nullptr && Initialize != nullptr && Shutdown != nullptr && GetFunction != nullptr;
	}

	void* Get(const char* name) const
	{
		if (GetFunction)
		{
			return GetFunction(name);
		}
		return nullptr;
	}

	void Init(WrenVM* vm)
	{
		if (is_valid())
			Initialize(vm);
	}

private:
	std::string dll_path;
	HMODULE hModule;
	void (*Initialize)(WrenVM* vm);
	void (*Shutdown)();
	void* (*GetFunction)(const char* name);
};

#endif


#ifdef __linux__

#include <dlfcn.h>

const std::string shared_extension=".so";

class NativeModule
{
public:
	NativeModule(const std::string& path)
		: lib_path(path), hModule(nullptr), Initialize(nullptr), Shutdown(nullptr), GetFunction(nullptr)
	{
		try
		{
			hModule = dlopen(lib_path.c_str(), RTLD_LAZY);
			if (!hModule) throw std::runtime_error("Failed to load module: " + lib_path);
			Initialize = (void (*)(WrenVM*))dlsym(hModule, "Initialize");
			if (!Initialize) throw std::runtime_error("No Initialize function found in " + lib_path);
			Shutdown = (void (*)())dlsym(hModule, "Shutdown");
			if (!Shutdown) throw std::runtime_error("No Shutdown function found in " + lib_path);
			GetFunction = (void* (*)(const char*))dlsym(hModule, "GetFunction");
			if (!GetFunction) throw std::runtime_error("No GetFunction found in " + lib_path);
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			if (hModule)
			{
				dlclose(hModule);
				hModule = nullptr;
			}
			Initialize = nullptr;
			Shutdown = nullptr;
			GetFunction = nullptr;
		}
	}

	~NativeModule()
	{
		if (is_valid())
		{
			if (Shutdown) Shutdown();  // Call the Shutdown function to clean up the module
			dlclose(hModule);
			hModule = nullptr;
		}
	}

	bool is_valid() const
	{
		return hModule != nullptr && Initialize != nullptr && Shutdown != nullptr && GetFunction != nullptr;
	}

	void* Get(const char* name) const
	{
		if (GetFunction)
		{
			return GetFunction(name);
		}
		return nullptr;
	}

	void Init(WrenVM* vm)
	{
		if (is_valid())
			Initialize(vm);
	}
private:
	std::string lib_path;
	void* hModule;
	void (*Initialize)(WrenVM* vm);
	void (*Shutdown)();
	void* (*GetFunction)(const char* name);
};


#endif


std::vector<std::string> find_shared_libraries(const std::string& path)
{
	std::vector<std::string> libs;
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_regular_file() && entry.path().extension() == shared_extension)
		{
			libs.push_back(entry.path().string());
		}
	}
	return libs;
}

typedef std::shared_ptr<NativeModule> NativeModulePtr;

std::vector<NativeModulePtr> native_modules;

void load_shared_libraries(const std::string& path, WrenVM* vm)
{
	std::vector<std::string> libs = find_shared_libraries(path);
	for (const auto& lib : libs)
	{
		NativeModulePtr module = std::make_shared<NativeModule>(lib);
		if (module->is_valid())
		{
			native_modules.push_back(module);
			module->Init(vm);
		}
		else
		{
			std::cerr << "Invalid module: " << lib << std::endl;
		}
	}
}

void initialize_foreign_modules(const std::string& path, WrenVM* vm)
{
	native_modules.clear();
	load_shared_libraries(path, vm);
}

void shutdown_foreign_modules()
{
	native_modules.clear();
}

void* find_foreign_method(const std::string& key)
{
	for (auto& module : native_modules)
	{
		if (module->is_valid())
		{
			void* func = module->Get(key.c_str());
			if (func) return func;
		}
	}
	return nullptr;
}

