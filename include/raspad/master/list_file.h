/**
 * @file list_file.h
 * @brief Load `servers.json` into the registry. No HTTP.
 *
 * Application file I/O. The GoldSrc client never reads this file; UDP 27010
 * serves the in-memory copy.
 */

#ifndef RASPAD_MASTER_LIST_FILE_H
#define RASPAD_MASTER_LIST_FILE_H

#include <lh/core/bool.h>
#include <lh/core/char.h>
#include <lh/core/compiler/extern/c.h>
#include <lh/core/numeric/fixed/types.h>
#include <lh/core/numeric/types.h>
#include <raspad/master/registry.h>

LH_COMPILER_EXTERN_C_BEGIN

/**
 * @brief Read the whole file @p path into @p buf.
 *
 * Opens @p path with ::lh_os_fs_file_open_cstr and calls ::lh_os_fs_file_read
 * until end of file. The buffer is not terminated.
 *
 * @param path     File path.
 * @param buf      Destination, @p capacity bytes.
 * @param capacity Size of @p buf.
 * @param size     Receives the number of bytes read (also on failure).
 * @return ::lh_bool_true if the whole file was read; ::lh_bool_false if it
 *         could not be opened or read, or is larger than @p capacity.
 */
lh_bool_t
raspad_master_list_file_read(const lh_char_t *path, lh_char_t *buf, lh_usize_t capacity,
                             lh_usize_t *size);

/**
 * @brief Replace @p registry with entries from @p path.
 *
 * Expected JSON:
 * `{"servers":[{"ip":"1.2.3.4","port":27015}, ...]}`
 * or a top-level array of the same objects.
 *
 * On parse/read failure the registry is left unchanged.
 *
 * @param path     File path (typically `servers.json` beside the executable).
 * @param registry Destination; cleared only after a successful parse.
 * @return ::lh_bool_true if the file was read and applied.
 */
lh_bool_t
raspad_master_list_file_load(const lh_char_t *path, raspad_master_registry_t *registry);

/**
 * @brief Modification time of @p path.
 *
 * @param path File path.
 * @return Seconds since epoch, or `0` if the file is missing.
 */
lh_s64_t
raspad_master_list_file_mtime(const lh_char_t *path);

LH_COMPILER_EXTERN_C_END

#endif /* RASPAD_MASTER_LIST_FILE_H */
