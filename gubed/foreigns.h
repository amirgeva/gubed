#pragma once

void initialize_foreign_modules(const std::string& path, WrenVM* vm);
void shutdown_foreign_modules();
void* find_foreign_method(const std::string& key);
