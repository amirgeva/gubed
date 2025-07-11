#pragma once

// Call this to run the script without debugging
void disable_instrumentation();

// Caller owns the memory returned by this function.
// Must call delete[]
const char* load_module_code(const char* name);
