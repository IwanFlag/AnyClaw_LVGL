#ifndef LICENSE_H
#define LICENSE_H

/*
 * ═══════════════════════════════════════════════════════════════
 *  AnyClaw LVGL — License Validation
 *  Time-incrementing key system: each key's duration is 2x the previous
 * ═══════════════════════════════════════════════════════════════
 */

#include <cstdint>

/* Check if license is currently valid (not expired) */
bool license_is_valid();

/* Get remaining seconds (0 if expired) */
int64_t license_remaining_seconds();

/* Get current sequence number */
int license_get_seq();

/* Validate and activate a license key.
 * Returns true if key is valid and not expired.
 * Updates config with new seq/expiry if accepted.
 * On success: out_seq and out_expiry_hours are set.
 * On failure: out_error is set to error description. */
bool license_activate(const char* key, int* out_seq = nullptr,
                      int* out_expiry_hours = nullptr, const char** out_error = nullptr);

/* Initialize license from config (call at startup).
 * Loads stored seq/expiry from config.json. */
void license_init();

/* Get human-readable remaining time string (e.g., "11.5h remaining") */
void license_get_remaining_str(char* buf, int buf_size);

/* Get primary machine id (MAC-like string) for HW key flow. */
bool license_get_machine_id(char* buf, int buf_size);

#endif /* LICENSE_H */
