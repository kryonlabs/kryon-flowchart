/*
 * Kryon Storage Plugin - Core Implementation
 *
 * Implements in-memory hash map storage with JSON persistence.
 */

#include "kryon_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Hash Map Implementation
// ============================================================================

#define HASH_MAP_CAPACITY 256

typedef struct hash_entry {
    char* key;
    char* value;
    struct hash_entry* next;
} hash_entry_t;

typedef struct {
    hash_entry_t* buckets[HASH_MAP_CAPACITY];
    size_t count;
} hash_map_t;

// Simple djb2 hash function
static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    char c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static hash_map_t* hash_map_create(void) {
    hash_map_t* map = (hash_map_t*)calloc(1, sizeof(hash_map_t));
    return map;
}

static void hash_map_destroy(hash_map_t* map) {
    if (!map) return;

    for (int i = 0; i < HASH_MAP_CAPACITY; i++) {
        hash_entry_t* entry = map->buckets[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(map);
}

static hash_entry_t* hash_map_get(hash_map_t* map, const char* key) {
    if (!map || !key) return NULL;

    uint32_t hash = hash_string(key);
    uint32_t index = hash % HASH_MAP_CAPACITY;

    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static bool hash_map_set(hash_map_t* map, const char* key, const char* value) {
    if (!map || !key || !value) return false;

    uint32_t hash = hash_string(key);
    uint32_t index = hash % HASH_MAP_CAPACITY;

    // Check if key already exists
    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            // Update existing value
            free(entry->value);
            entry->value = strdup(value);
            return entry->value != NULL;
        }
        entry = entry->next;
    }

    // Create new entry
    hash_entry_t* new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t));
    if (!new_entry) return false;

    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    if (!new_entry->key || !new_entry->value) {
        free(new_entry->key);
        free(new_entry->value);
        free(new_entry);
        return false;
    }

    // Insert at head of bucket
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->count++;
    return true;
}

static bool hash_map_remove(hash_map_t* map, const char* key) {
    if (!map || !key) return false;

    uint32_t hash = hash_string(key);
    uint32_t index = hash % HASH_MAP_CAPACITY;

    hash_entry_t* entry = map->buckets[index];
    hash_entry_t* prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            free(entry->key);
            free(entry->value);
            free(entry);
            map->count--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}

static void hash_map_clear(hash_map_t* map) {
    if (!map) return;

    for (int i = 0; i < HASH_MAP_CAPACITY; i++) {
        hash_entry_t* entry = map->buckets[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    map->count = 0;
}

// ============================================================================
// Global State
// ============================================================================

struct {
    hash_map_t* storage;
    char* app_name;
    bool initialized;
    bool auto_save;
} g_storage_state = {
    .storage = NULL,
    .app_name = NULL,
    .initialized = false,
    .auto_save = true
};

// Forward declarations (functions defined in storage_backends.c and storage_json.c)
extern kryon_storage_result_t storage_backend_load(char** out_json, size_t* out_size);
extern kryon_storage_result_t storage_backend_save(const char* json, size_t size);
extern kryon_storage_result_t storage_backend_get_path(char* path, size_t size);
extern const char* storage_backend_get_name(void);

extern kryon_storage_result_t storage_load_from_json(const char* json, size_t size);
extern kryon_storage_result_t storage_save_to_json(char** out_json, size_t* out_size);

// ============================================================================
// Lifecycle Implementation
// ============================================================================

kryon_storage_result_t kryon_storage_init(const char* app_name) {
    if (!app_name) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    if (g_storage_state.initialized) {
        return KRYON_STORAGE_OK; // Already initialized
    }

    // Create hash map
    g_storage_state.storage = hash_map_create();
    if (!g_storage_state.storage) {
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Store app name
    g_storage_state.app_name = strdup(app_name);
    if (!g_storage_state.app_name) {
        hash_map_destroy(g_storage_state.storage);
        g_storage_state.storage = NULL;
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Load existing data from disk
    char* json_content = NULL;
    size_t json_size = 0;
    kryon_storage_result_t result = storage_backend_load(&json_content, &json_size);

    if (result == KRYON_STORAGE_OK && json_content) {
        result = storage_load_from_json(json_content, json_size);
        free(json_content);
    }

    // If file doesn't exist, that's OK (first run)
    if (result == KRYON_STORAGE_ERROR_NOT_FOUND || result == KRYON_STORAGE_ERROR_IO) {
        result = KRYON_STORAGE_OK;
    }

    if (result != KRYON_STORAGE_OK) {
        hash_map_destroy(g_storage_state.storage);
        free(g_storage_state.app_name);
        g_storage_state.storage = NULL;
        g_storage_state.app_name = NULL;
        return result;
    }

    g_storage_state.initialized = true;
    return KRYON_STORAGE_OK;
}

kryon_storage_result_t kryon_storage_shutdown(void) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }

    // Save before shutdown
    kryon_storage_result_t result = kryon_storage_save();

    // Clean up
    hash_map_destroy(g_storage_state.storage);
    free(g_storage_state.app_name);

    g_storage_state.storage = NULL;
    g_storage_state.app_name = NULL;
    g_storage_state.initialized = false;

    return result;
}

// ============================================================================
// Key-Value Operations
// ============================================================================

kryon_storage_result_t kryon_storage_set_item(const char* key, const char* value) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!key || !value) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    if (!hash_map_set(g_storage_state.storage, key, value)) {
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Auto-save if enabled
    if (g_storage_state.auto_save) {
        kryon_storage_result_t save_result = kryon_storage_save();
        if (save_result != KRYON_STORAGE_OK) {
            return save_result;
        }
    }

    return KRYON_STORAGE_OK;
}

kryon_storage_result_t kryon_storage_get_item(const char* key, char* out_value, size_t out_size) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!key || !out_value || out_size == 0) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    hash_entry_t* entry = hash_map_get(g_storage_state.storage, key);
    if (!entry) {
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    strncpy(out_value, entry->value, out_size - 1);
    out_value[out_size - 1] = '\0';

    return KRYON_STORAGE_OK;
}

kryon_storage_result_t kryon_storage_remove_item(const char* key) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!key) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    if (!hash_map_remove(g_storage_state.storage, key)) {
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    // Auto-save if enabled
    if (g_storage_state.auto_save) {
        kryon_storage_result_t save_result = kryon_storage_save();
        if (save_result != KRYON_STORAGE_OK) {
            return save_result;
        }
    }

    return KRYON_STORAGE_OK;
}

kryon_storage_result_t kryon_storage_clear(void) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }

    hash_map_clear(g_storage_state.storage);

    // Auto-save if enabled
    if (g_storage_state.auto_save) {
        return kryon_storage_save();
    }

    return KRYON_STORAGE_OK;
}

// ============================================================================
// Query Operations
// ============================================================================

bool kryon_storage_has_key(const char* key) {
    if (!g_storage_state.initialized || !key) {
        return false;
    }
    return hash_map_get(g_storage_state.storage, key) != NULL;
}

size_t kryon_storage_count(void) {
    if (!g_storage_state.initialized) {
        return 0;
    }
    return g_storage_state.storage->count;
}

kryon_storage_result_t kryon_storage_keys(char*** out_keys, size_t* out_count) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!out_keys || !out_count) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    size_t count = g_storage_state.storage->count;
    if (count == 0) {
        *out_keys = NULL;
        *out_count = 0;
        return KRYON_STORAGE_OK;
    }

    char** keys = (char**)malloc(count * sizeof(char*));
    if (!keys) {
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    size_t index = 0;
    for (int i = 0; i < HASH_MAP_CAPACITY; i++) {
        hash_entry_t* entry = g_storage_state.storage->buckets[i];
        while (entry && index < count) {
            keys[index] = strdup(entry->key);
            if (!keys[index]) {
                // Cleanup on failure
                for (size_t j = 0; j < index; j++) {
                    free(keys[j]);
                }
                free(keys);
                return KRYON_STORAGE_ERROR_NO_MEMORY;
            }
            index++;
            entry = entry->next;
        }
    }

    *out_keys = keys;
    *out_count = count;
    return KRYON_STORAGE_OK;
}

void kryon_storage_free_keys(char** keys, size_t count) {
    if (!keys) return;
    for (size_t i = 0; i < count; i++) {
        free(keys[i]);
    }
    free(keys);
}

// ============================================================================
// Persistence Control
// ============================================================================

bool kryon_storage_set_auto_save(bool enable) {
    bool previous = g_storage_state.auto_save;
    g_storage_state.auto_save = enable;
    return previous;
}

kryon_storage_result_t kryon_storage_save(void) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }

    char* json_content = NULL;
    size_t json_size = 0;

    kryon_storage_result_t result = storage_save_to_json(&json_content, &json_size);
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    result = storage_backend_save(json_content, json_size);
    free(json_content);

    return result;
}

kryon_storage_result_t kryon_storage_get_path(char* path, size_t size) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!path || size == 0) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    return storage_backend_get_path(path, size);
}

// ============================================================================
// Backend Info
// ============================================================================

const char* kryon_storage_get_backend_name(void) {
    return storage_backend_get_name();
}
