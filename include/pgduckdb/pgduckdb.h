#pragma once

/* Values for the backslash_quote GUC */
typedef enum {
	MOTHERDUCK_OFF,
	MOTHERDUCK_ON,
	MOTHERDUCK_AUTO,
} MotherDuckEnabled;

// pgduckdb.cpp
// Note: on Windows, fmgr.h declares _PG_init with PGDLLEXPORT. Any earlier
// declaration without that attribute causes C2375 (different linkage). fmgr.h's
// comment explicitly says to include fmgr.h before any extension declaration of
// _PG_init, or compilation for Windows will fail. We therefore omit our own
// declaration here and rely on the one in fmgr.h.
#ifndef _WIN32
extern "C" void _PG_init(void);
#endif

// pgduckdb_hooks.c
void DuckdbInitHooks(void);
