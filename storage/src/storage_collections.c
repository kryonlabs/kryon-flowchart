/*
 * Kryon Storage Plugin - Collections API
 *
 * Provides collection-based storage where each collection is saved as a
 * separate JSON file ({collection_name}.json).
 */

#include "kryon_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// External global state from storage.c
extern struct {
    void* storage;
    char* app_name;
    bool initialized;
    bool auto_save;
} g_storage_state;

// External backend function (implemented in storage_backends.c)
extern kryon_storage_result_t storage_backend_get_collection_path(const char* name, char* path, size_t size);

// ============================================================================
// Collection Management Functions
// ============================================================================

/**
 * Save a collection to disk as a JSON file
 */
kryon_storage_result_t kryon_storage_save_collection(const char* name, const char* json_data) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!name || !json_data) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    // Get collection file path
    char path[512];
    kryon_storage_result_t result = storage_backend_get_collection_path(name, path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    // Write to file
    FILE* file = fopen(path, "w");
    if (!file) {
        return KRYON_STORAGE_ERROR_IO;
    }

    size_t json_len = strlen(json_data);
    size_t written = fwrite(json_data, 1, json_len, file);
    fclose(file);

    if (written != json_len) {
        return KRYON_STORAGE_ERROR_IO;
    }

    return KRYON_STORAGE_OK;
}

/**
 * Load a collection from disk
 */
kryon_storage_result_t kryon_storage_load_collection(const char* name, char** out_json, size_t* out_size) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!name || !out_json || !out_size) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    // Get collection file path
    char path[512];
    kryon_storage_result_t result = storage_backend_get_collection_path(name, path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    // Open file
    FILE* file = fopen(path, "r");
    if (!file) {
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    // Allocate buffer
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Read file
    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(content);
        return KRYON_STORAGE_ERROR_IO;
    }

    content[read_size] = '\0';

    *out_json = content;
    *out_size = read_size;

    return KRYON_STORAGE_OK;
}

/**
 * Remove a collection file
 */
kryon_storage_result_t kryon_storage_remove_collection(const char* name) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!name) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    char path[512];
    kryon_storage_result_t result = storage_backend_get_collection_path(name, path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    if (remove(path) != 0) {
        return KRYON_STORAGE_ERROR_IO;
    }

    return KRYON_STORAGE_OK;
}

/**
 * Check if a collection exists
 */
kryon_storage_result_t kryon_storage_collection_exists(const char* name, bool* out_exists) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!name || !out_exists) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    char path[512];
    kryon_storage_result_t result = storage_backend_get_collection_path(name, path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    struct stat st;
    *out_exists = (stat(path, &st) == 0);

    return KRYON_STORAGE_OK;
}

/**
 * Get the file path for a collection
 */
kryon_storage_result_t kryon_storage_get_collection_path(const char* name, char* out_path, size_t out_size) {
    if (!g_storage_state.initialized) {
        return KRYON_STORAGE_ERROR_NOT_INITIALIZED;
    }
    if (!name || !out_path || out_size == 0) {
        return KRYON_STORAGE_ERROR_INVALID_ARG;
    }

    return storage_backend_get_collection_path(name, out_path, out_size);
}
