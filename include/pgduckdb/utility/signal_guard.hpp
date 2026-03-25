
#pragma once

#ifndef _WIN32
#include <signal.h>
#endif

namespace pgduckdb {

class ThreadSignalBlockGuard {
public:
	ThreadSignalBlockGuard(const ThreadSignalBlockGuard &) = delete;
	ThreadSignalBlockGuard(ThreadSignalBlockGuard &&) = delete;
	ThreadSignalBlockGuard &operator=(const ThreadSignalBlockGuard &) = delete;
	ThreadSignalBlockGuard &operator=(ThreadSignalBlockGuard &&) = delete;

	explicit ThreadSignalBlockGuard();
	~ThreadSignalBlockGuard();
	void unblock();

private:
	bool _blocked;
#ifndef _WIN32
	sigset_t _saved_set;
#endif
};

} // namespace pgduckdb
