/*
 * Kryon Storage Plugin - JSON Serialization
 *
 * Uses cJSON for serialization/deserialization of storage data.
 */

#include "kryon_storage.h"
#include "../deps/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Hash Map Types (must match storage.c)
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

// Simple djb2 hash function (same as in storage.c)
static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    char c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// ============================================================================
// External declarations from storage.c
// ============================================================================

extern struct {
    hash_map_t* storage;
    char* app_name;
    bool initialized;
    bool auto_save;
} g_storage_state;

// ============================================================================
// Save to JSON
// ============================================================================

kryon_storage_result_t storage_save_to_json(char** out_json, size_t* out_size) {
    if (!g_storage_state.storage || !out_json || !out_size) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    hash_map_t* map = (hash_map_t*)g_storage_state.storage;

    // Create JSON object
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Add metadata
    cJSON_AddStringToObject(root, "_app", g_storage_state.app_name);
    cJSON_AddNumberToObject(root, "_version", 1);

    // Create items object
    cJSON* items = cJSON_CreateObject();
    if (!items) {
        cJSON_Delete(root);
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Iterate over hash map and add each key-value pair
    for (int i = 0; i < HASH_MAP_CAPACITY; i++) {
        hash_entry_t* entry = map->buckets[i];
        while (entry) {
            // Check if value looks like JSON (starts with { or [)
            char first_char = entry->value[0];
            bool success = false;

            if (first_char == '{' || first_char == '[') {
                // Try to parse as JSON
                cJSON* parsed = cJSON_Parse(entry->value);
                if (parsed) {
                    // Successfully parsed - add as JSON object/array
                    if (cJSON_AddItemToObject(items, entry->key, parsed)) {
                        success = true;
                    } else {
                        cJSON_Delete(parsed);
                    }
                }
                // If parse failed, fall through to string handling
            }

            if (!success) {
                // Add as literal string (non-JSON or parse failed)
                if (!cJSON_AddStringToObject(items, entry->key, entry->value)) {
                    cJSON_Delete(items);
                    cJSON_Delete(root);
                    return KRYON_STORAGE_ERROR_NO_MEMORY;
                }
            }

            entry = entry->next;
        }
    }

    cJSON_AddItemToObject(root, "items", items);

    // Print to string
    char* json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) {
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    *out_json = json_str;
    *out_size = strlen(json_str);

    return KRYON_STORAGE_OK;
}

// ============================================================================
// Load from JSON
// ============================================================================

kryon_storage_result_t storage_load_from_json(const char* json, size_t size) {
    if (!json || size == 0) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    // Parse JSON
    cJSON* root = cJSON_Parse(json);
    if (!root) {
        return KRYON_STORAGE_ERROR_PARSE;
    }

    // Get items object
    cJSON* items = cJSON_GetObjectItem(root, "items");
    if (!items || !cJSON_IsObject(items)) {
        cJSON_Delete(root);
        return KRYON_STORAGE_ERROR_PARSE;
    }

    // Clear existing storage
    hash_map_t* map = (hash_map_t*)g_storage_state.storage;
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

    // Load items
    cJSON* item = NULL;
    cJSON_ArrayForEach(item, items) {
        if (item->string) {
            // Add to hash map (using internal hash_map_set from storage.c)
            uint32_t hash = hash_string(item->string);
            uint32_t index = hash % HASH_MAP_CAPACITY;

            // Create new entry
            hash_entry_t* new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t));
            if (!new_entry) continue;

            new_entry->key = strdup(item->string);

            // Serialize the item to JSON string (handles objects, arrays, numbers, booleans, etc.)
            char* json_str = cJSON_PrintUnformatted(item);
            if (!json_str) {
                free(new_entry->key);
                free(new_entry);
                continue;
            }
            new_entry->value = json_str;

            if (!new_entry->key || !new_entry->value) {
                free(new_entry->key);
                free(new_entry->value);
                free(new_entry);
                continue;
            }

            // Insert at head of bucket
            new_entry->next = map->buckets[index];
            map->buckets[index] = new_entry;
            map->count++;
        }
    }

    cJSON_Delete(root);
    return KRYON_STORAGE_OK;
}
