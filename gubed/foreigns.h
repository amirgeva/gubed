#pragma once

void InitializeForeignModules(const std::string& path, WrenVM* vm);
void ShutdownForeignModules();
void* find_foreign_method(const std::string& key);
