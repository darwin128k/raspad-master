/**
 * @file list_file.h
 * @brief Load `servers.json` into the registry. No HTTP.
 *
 * Application file I/O. The GoldSrc client never reads this file; UDP 27010
 * serves the in-memory copy.
 */

#ifndef RASPAD_MASTER_LIST_FILE_H
#define RASPAD_MASTER_LIST_FILE_H

#include <lh/bool.h>
#include <lh/char.h>
#include <lh/compiler/extern/c.h>
#include <lh/numeric/fixed/types.h>
#include <raspad/master/registry.h>

LH_COMPILER_EXTERN_C_BEGIN

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
