#pragma once

// Forward declaration avoids including nodes/extensible.h in an extern "C"
// block, which triggers MSVC C2059/C2143 due to its deep include chain.
// Consumers that need the complete type (e.g. to define the variable or
// access its members) must include "nodes/extensible.h" themselves in their
// own extern "C" block before using this header.
struct CustomScanMethods;

extern CustomScanMethods duckdb_scan_scan_methods;
extern "C" void DuckdbInitNode(void);
