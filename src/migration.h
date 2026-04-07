#ifndef MIGRATION_H
#define MIGRATION_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — One-Click Migration (Export/Import)
 *  Packages AnyClaw config + OpenClaw workspace for transfer
 * ═══════════════════════════════════════════════════════════════
 */

/* Export migration package.
 * output_path: where to save the zip file (user-chosen via dialog)
 * Returns true on success, false on failure.
 * err_msg: set to error description on failure (optional). */
bool migration_export(const char* output_path, char* err_msg = nullptr, int err_size = 0);

/* Import migration package.
 * input_path: path to the migration zip file
 * Returns true on success, false on failure.
 * err_msg: set to error description on failure (optional). */
bool migration_import(const char* input_path, char* err_msg = nullptr, int err_size = 0);

/* Pre-flight checks before import.
 * Returns true if all prerequisites are met.
 * err_msg: set to description if check fails. */
bool migration_preflight_check(char* err_msg = nullptr, int err_size = 0);

#endif /* MIGRATION_H */
