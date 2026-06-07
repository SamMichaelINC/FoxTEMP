#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Archive Stub - Minimal interface for Desktop/internal services
 * 
 * The full Archive app runs as an external .fap file.
 * These stubs provide compatibility for code that needs Archive records/symbols.
 */

// Stub for archive favorites functionality
void archive_favorites_handle_setting_pin_unpin(const char* app_name, const char* setting);
bool archive_is_favorite(const char* format, ...) __attribute__((__format__(__printf__, 1, 2)));
void archive_add_to_favorites(const char* file_path);
bool archive_favorites_delete(const char* format, ...) __attribute__((__format__(__printf__, 1, 2)));
bool archive_favorites_rename(const char* src, const char* dst);

// Archive app record - set to NULL since it runs external
extern const void* FLIPPER_ARCHIVE = NULL;

#ifdef __cplusplus
}
#endif
