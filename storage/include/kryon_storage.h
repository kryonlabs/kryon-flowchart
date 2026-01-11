/*
 * Kryon Storage Plugin
 *
 * localStorage-like persistent storage for Kryon applications.
 * Works across all frontends (Nim, Lua, C) and platforms (Linux, macOS, Windows, Web).
 *
 * Public C API
 */

#ifndef KRYON_STORAGE_H
#define KRYON_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration for internal state (not exposed in public API)
typedef struct kryon_storage_state kryon_storage_state_t;

// ============================================================================
// Result Codes
// ============================================================================

typedef enum {
    KRYON_STORAGE_OK = 0,                  // Success
    KRYON_STORAGE_ERROR_NOT_FOUND,         // Key not found
    KRYON_STORAGE_ERROR_IO,                // File I/O error
    KRYON_STORAGE_ERROR_PARSE,             // JSON parse error
    KRYON_STORAGE_ERROR_NO_MEMORY,         // Memory allocation failed
    KRYON_STORAGE_ERROR_NOT_INITIALIZED,   // Storage not initialized
    KRYON_STORAGE_ERROR_INVALID_ARG        // Invalid argument
} kryon_storage_result_t;

// ============================================================================
// Lifecycle
// ============================================================================

/**
 * Initialize storage for an application.
 * Must be called before any other storage functions.
 * Loads existing data from disk if available.
 *
 * @param app_name Application name (used for storage file/namespace)
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_init(const char* app_name);

/**
 * Shutdown storage and free resources.
 * Saves data to disk before shutdown.
 *
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_shutdown(void);

// ============================================================================
// Key-Value Operations (String values only, like localStorage)
// ============================================================================

/**
 * Set a key-value pair.
 * Auto-saves to disk after setting (can be disabled).
 *
 * @param key Key string (must be null-terminated)
 * @param value Value string (must be null-terminated)
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_set_item(const char* key, const char* value);

/**
 * Get a value by key.
 *
 * @param key Key string (must be null-terminated)
 * @param out_value Output buffer for value
 * @param out_size Size of output buffer
 * @return KRYON_STORAGE_OK if key found, KRYON_STORAGE_ERROR_NOT_FOUND if not exists
 */
kryon_storage_result_t kryon_storage_get_item(const char* key, char* out_value, size_t out_size);

/**
 * Remove a key-value pair.
 *
 * @param key Key string to remove
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_remove_item(const char* key);

/**
 * Clear all key-value pairs.
 *
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_clear(void);

// ============================================================================
// Query Operations
// ============================================================================

/**
 * Check if a key exists.
 *
 * @param key Key string to check
 * @return true if key exists, false otherwise
 */
bool kryon_storage_has_key(const char* key);

/**
 * Get the number of stored key-value pairs.
 *
 * @return Number of items, or 0 if not initialized
 */
size_t kryon_storage_count(void);

/**
 * Get all keys.
 * Caller must free the returned array and strings.
 *
 * @param out_keys Output array of key strings (caller must free)
 * @param out_count Output: number of keys
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_keys(char*** out_keys, size_t* out_count);

/**
 * Free the keys array returned by kryon_storage_keys().
 *
 * @param keys Array of key strings
 * @param count Number of keys
 */
void kryon_storage_free_keys(char** keys, size_t count);

// ============================================================================
// Persistence Control
// ============================================================================

/**
 * Enable or disable auto-save.
 * Auto-save is enabled by default (like localStorage).
 *
 * @param enable true to enable auto-save, false to disable
 * @return Previous auto-save state
 */
bool kryon_storage_set_auto_save(bool enable);

/**
 * Manually save to disk.
 * Only needed if auto-save is disabled.
 *
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_save(void);

/**
 * Get the storage file path.
 *
 * @param path Output buffer for path
 * @param size Size of output buffer
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_get_path(char* path, size_t size);

// ============================================================================
// Backend Info
// ============================================================================

/**
 * Get the name of the active backend.
 *
 * @return Backend name (e.g., "posix", "windows", "emscripten")
 */
const char* kryon_storage_get_backend_name(void);

// ============================================================================
// Collection Storage API
// ============================================================================

/**
 * Save a collection to a separate JSON file.
 * Each collection is stored as {collection_name}.json in the app directory.
 *
 * @param name Collection name (used as filename)
 * @param json_data JSON string to save (should be {"collection_name": [...]})
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_save_collection(const char* name, const char* json_data);

/**
 * Load a collection from a JSON file.
 *
 * @param name Collection name
 * @param out_json Output buffer for JSON data (caller must free)
 * @param out_size Output size of JSON data
 * @return KRYON_STORAGE_OK on success, KRYON_STORAGE_ERROR_NOT_FOUND if not exists
 */
kryon_storage_result_t kryon_storage_load_collection(const char* name, char** out_json, size_t* out_size);

/**
 * Remove a collection file.
 *
 * @param name Collection name
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_remove_collection(const char* name);

/**
 * Check if a collection exists.
 *
 * @param name Collection name
 * @param out_exists Output: true if exists, false otherwise
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_collection_exists(const char* name, bool* out_exists);

/**
 * Get the file path for a collection.
 *
 * @param name Collection name
 * @param out_path Output buffer for path
 * @param out_size Size of output buffer
 * @return KRYON_STORAGE_OK on success, error code otherwise
 */
kryon_storage_result_t kryon_storage_get_collection_path(const char* name, char* out_path, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif // KRYON_STORAGE_H
