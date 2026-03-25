/*
 * include/storage/fd.h — MSVC compatibility shim for PostgreSQL's storage/fd.h
 *
 * PostgreSQL 17's storage/fd.h defines FileRead() and FileWrite() as static
 * inline functions using C99 designated initializers ({.field = value}).
 * MSVC rejects designated initializers in C++17 mode (they require C++20 with
 * VS2019 16.8+, which is unavailable on VS2017).
 *
 * Strategy:
 *   _MSC_VER  — This file provides a complete replacement: all declarations
 *               from the upstream fd.h, plus MSVC-compatible FileRead/FileWrite
 *               inline functions (using sequential member assignment instead of
 *               designated initializers).
 *
 *               FD_H may already be defined before this shim is reached.
 *               win32_interlocked_compat.hpp (force-included via -FI) pre-defines
 *               FD_H so that the real fd.h becomes a no-op regardless of which
 *               file MSVC finds first via its "search the including header's
 *               directory" rule.  In that case the #ifndef FD_H block below is
 *               skipped, but typedef int File; was already provided there.
 *               The full declarations (externs, FileRead/FileWrite) are provided
 *               inside PGDUCKDB_FD_FULL_DECLS which uses its own guard.
 *
 *   !_MSC_VER — #include_next transparently delegates to the real fd.h found
 *               later in the include search path (GCC/Clang support this).
 *
 * This shim is found first because Makefile.win puts -Iinclude before
 * -I$(INCLUDEDIR_SERVER).  The Linux Makefile also has -Iinclude first, so
 * the #include_next path handles GCC/Clang builds correctly.
 */

#ifndef PGDUCKDB_STORAGE_FD_SHIM_H
#define PGDUCKDB_STORAGE_FD_SHIM_H

#ifdef _MSC_VER

/* Ensure FD_H is defined so the real fd.h is a no-op if encountered later. */
#ifndef FD_H
#define FD_H
#endif

/*
 * Provide the full set of fd.h declarations.  Use a separate guard so this
 * block runs even when FD_H was pre-defined by win32_interlocked_compat.hpp
 * (which only provides the minimal typedef int File;).
 */
#ifndef PGDUCKDB_FD_FULL_DECLS
#define PGDUCKDB_FD_FULL_DECLS

#include "port/pg_iovec.h"

#include <dirent.h>
#include <fcntl.h>

/* File typedef: provided by win32_interlocked_compat.hpp when force-included,
 * or here when this shim is the first to run. */
#ifndef PGDUCKDB_FD_FILE_TYPEDEF
#define PGDUCKDB_FD_FILE_TYPEDEF
typedef int File;
#endif


#define IO_DIRECT_DATA			0x01
#define IO_DIRECT_WAL			0x02
#define IO_DIRECT_WAL_INIT		0x04


/* GUC parameter */
extern PGDLLIMPORT int max_files_per_process;
extern PGDLLIMPORT bool data_sync_retry;
extern PGDLLIMPORT int recovery_init_sync_method;
extern PGDLLIMPORT int io_direct_flags;

/*
 * This is private to fd.c, but exported for save/restore_backend_variables()
 */
extern PGDLLIMPORT int max_safe_fds;

/*
 * On Windows, we have to interpret EACCES as possibly meaning the same as
 * ENOENT, because if a file is unlinked-but-not-yet-gone on that platform,
 * that's what you get.  Ugh.
 */
#ifndef WIN32
#define FILE_POSSIBLY_DELETED(err)	((err) == ENOENT)
#else
#define FILE_POSSIBLY_DELETED(err)	((err) == ENOENT || (err) == EACCES)
#endif

/*
 * O_DIRECT is not standard; on Windows we simulate it.
 */
#if defined(O_DIRECT) && defined(pg_attribute_aligned)
#define		PG_O_DIRECT O_DIRECT
#elif defined(F_NOCACHE)
#define		PG_O_DIRECT 0x80000000
#define		PG_O_DIRECT_USE_F_NOCACHE
#else
#define		PG_O_DIRECT 0
#endif

/*
 * prototypes for functions in fd.c
 */

/* Operations on virtual Files --- equivalent to Unix kernel file ops */
extern File PathNameOpenFile(const char *fileName, int fileFlags);
extern File PathNameOpenFilePerm(const char *fileName, int fileFlags, mode_t fileMode);
extern File OpenTemporaryFile(bool interXact);
extern void FileClose(File file);
extern int	FilePrefetch(File file, off_t offset, off_t amount, uint32 wait_event_info);
extern ssize_t FileReadV(File file, const struct iovec *iov, int iovcnt, off_t offset, uint32 wait_event_info);
extern ssize_t FileWriteV(File file, const struct iovec *iov, int iovcnt, off_t offset, uint32 wait_event_info);
extern int	FileSync(File file, uint32 wait_event_info);
extern int	FileZero(File file, off_t offset, off_t amount, uint32 wait_event_info);
extern int	FileFallocate(File file, off_t offset, off_t amount, uint32 wait_event_info);

extern off_t FileSize(File file);
extern int	FileTruncate(File file, off_t offset, uint32 wait_event_info);
extern void FileWriteback(File file, off_t offset, off_t nbytes, uint32 wait_event_info);
extern char *FilePathName(File file);
extern int	FileGetRawDesc(File file);
extern int	FileGetRawFlags(File file);
extern mode_t FileGetRawMode(File file);

/* Operations used for sharing named temporary files */
extern File PathNameCreateTemporaryFile(const char *path, bool error_on_failure);
extern File PathNameOpenTemporaryFile(const char *path, int mode);
extern bool PathNameDeleteTemporaryFile(const char *path, bool error_on_failure);
extern void PathNameCreateTemporaryDir(const char *basedir, const char *directory);
extern void PathNameDeleteTemporaryDir(const char *dirname);
extern void TempTablespacePath(char *path, Oid tablespace);

/* Operations that allow use of regular stdio --- USE WITH CAUTION */
extern FILE *AllocateFile(const char *name, const char *mode);
extern int	FreeFile(FILE *file);

/* Operations that allow use of pipe streams (popen/pclose) */
extern FILE *OpenPipeStream(const char *command, const char *mode);
extern int	ClosePipeStream(FILE *file);

/* Operations to allow use of the <dirent.h> library routines */
extern DIR *AllocateDir(const char *dirname);
extern struct dirent *ReadDir(DIR *dir, const char *dirname);
extern struct dirent *ReadDirExtended(DIR *dir, const char *dirname,
									  int elevel);
extern int	FreeDir(DIR *dir);

/* Operations to allow use of a plain kernel FD, with automatic cleanup */
extern int	OpenTransientFile(const char *fileName, int fileFlags);
extern int	OpenTransientFilePerm(const char *fileName, int fileFlags, mode_t fileMode);
extern int	CloseTransientFile(int fd);

/* If you've really really gotta have a plain kernel FD, use this */
extern int	BasicOpenFile(const char *fileName, int fileFlags);
extern int	BasicOpenFilePerm(const char *fileName, int fileFlags, mode_t fileMode);

/* Use these for other cases, and also for long-lived BasicOpenFile FDs */
extern bool AcquireExternalFD(void);
extern void ReserveExternalFD(void);
extern void ReleaseExternalFD(void);

/* Make a directory with default permissions */
extern int	MakePGDirectory(const char *directoryName);

/* Miscellaneous support routines */
extern void InitFileAccess(void);
extern void InitTemporaryFileAccess(void);
extern void set_max_safe_fds(void);
extern void closeAllVfds(void);
extern void SetTempTablespaces(Oid *tableSpaces, int numSpaces);
extern bool TempTablespacesAreSet(void);
extern int	GetTempTablespaces(Oid *tableSpaces, int numSpaces);
extern Oid	GetNextTempTableSpace(void);
extern void AtEOXact_Files(bool isCommit);
extern void AtEOSubXact_Files(bool isCommit, SubTransactionId mySubid,
							  SubTransactionId parentSubid);
extern void RemovePgTempFiles(void);
extern void RemovePgTempFilesInDir(const char *tmpdirname, bool missing_ok,
								   bool unlink_all);
extern bool looks_like_temp_rel_name(const char *name);

extern int	pg_fsync(int fd);
extern int	pg_fsync_no_writethrough(int fd);
extern int	pg_fsync_writethrough(int fd);
extern int	pg_fdatasync(int fd);
extern bool pg_file_exists(const char *name);
extern void pg_flush_data(int fd, off_t offset, off_t nbytes);
extern int	pg_truncate(const char *path, off_t length);
extern void fsync_fname(const char *fname, bool isdir);
extern int	fsync_fname_ext(const char *fname, bool isdir, bool ignore_perm, int elevel);
extern int	durable_rename(const char *oldfile, const char *newfile, int elevel);
extern int	durable_unlink(const char *fname, int elevel);
extern void SyncDataDirectory(void);
extern int	data_sync_elevel(int elevel);

/*
 * MSVC-compatible FileRead/FileWrite: identical logic to the upstream versions
 * but using sequential member assignment instead of C99 designated initializers.
 */
static inline ssize_t
FileRead(File file, void *buffer, size_t amount, off_t offset,
		 uint32 wait_event_info)
{
	struct iovec iov;
	iov.iov_base = buffer;
	iov.iov_len = amount;
	return FileReadV(file, &iov, 1, offset, wait_event_info);
}

static inline ssize_t
FileWrite(File file, const void *buffer, size_t amount, off_t offset,
		  uint32 wait_event_info)
{
	struct iovec iov;
	iov.iov_base = (void *) buffer;	/* unconstify */
	iov.iov_len = amount;
	return FileWriteV(file, &iov, 1, offset, wait_event_info);
}

#endif /* PGDUCKDB_FD_FULL_DECLS */

#else  /* !_MSC_VER */

/* GCC/Clang: delegate to the real storage/fd.h via #include_next */
#include_next "storage/fd.h"

#endif /* _MSC_VER */

#endif /* PGDUCKDB_STORAGE_FD_SHIM_H */
