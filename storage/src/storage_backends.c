/*
 * Kryon Storage Plugin - Platform Backends
 *
 * Platform-specific file I/O for storage persistence.
 */

#include "kryon_storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define KRYON_PLATFORM_WINDOWS
#elif defined(__EMSCRIPTEN__)
    #define KRYON_PLATFORM_EMSCRIPTEN
#else
    #define KRYON_PLATFORM_POSIX
#endif

// Platform-specific includes
#ifdef KRYON_PLATFORM_WINDOWS
    #include <windows.h>
    #include <shlobj.h>
    #include <fileapi.h>
#elif defined(KRYON_PLATFORM_POSIX)
    #include <sys/stat.h>
    #include <unistd.h>
    #include <errno.h>
#endif

// ============================================================================
// External global state from storage.c
// ============================================================================

extern struct {
    void* storage;
    char* app_name;
    bool initialized;
    bool auto_save;
} g_storage_state;

// ============================================================================
// POSIX Backend (Linux, macOS, BSD)
// ============================================================================

#ifdef KRYON_PLATFORM_POSIX

static kryon_storage_result_t posix_get_path(char* path, size_t size) {
    const char* data_home = getenv("XDG_DATA_HOME");
    char base_path[512];

    if (data_home && data_home[0] == '/') {
        // Use XDG_DATA_HOME
        snprintf(base_path, sizeof(base_path), "%s", data_home);
    } else {
        // Fallback to ~/.local/share
        const char* home = getenv("HOME");
        if (!home) {
            return KRYON_STORAGE_ERROR_IO;
        }
        snprintf(base_path, sizeof(base_path), "%s/.local/share", home);
    }

    // Create directory if it doesn't exist
    struct stat st;
    if (stat(base_path, &st) != 0) {
        if (errno == ENOENT) {
            // Create directory
            (void)system("mkdir -p ~/.local/share 2>/dev/null");
        }
    }

    // App-specific subdirectory
    snprintf(path, size, "%s/%s/storage.json", base_path, g_storage_state.app_name);

    // Create app directory
    char app_dir[600];
    snprintf(app_dir, sizeof(app_dir), "%s/%s", base_path, g_storage_state.app_name);
    if (stat(app_dir, &st) != 0) {
        if (errno == ENOENT) {
            char mkdir_cmd[700];
            snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", app_dir);
            (void)system(mkdir_cmd);
        }
    }

    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t posix_load(char** out_json, size_t* out_size) {
    char path[512];
    kryon_storage_result_t result = posix_get_path(path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    FILE* file = fopen(path, "r");
    if (!file) {
        if (errno == ENOENT) {
            return KRYON_STORAGE_ERROR_NOT_FOUND;
        }
        return KRYON_STORAGE_ERROR_IO;
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

static kryon_storage_result_t posix_save(const char* json, size_t size) {
    char path[512];
    kryon_storage_result_t result = posix_get_path(path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    FILE* file = fopen(path, "w");
    if (!file) {
        return KRYON_STORAGE_ERROR_IO;
    }

    size_t written = fwrite(json, 1, size, file);
    fclose(file);

    if (written != size) {
        return KRYON_STORAGE_ERROR_IO;
    }

    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t posix_get_collection_path(const char* collection_name, char* path, size_t size) {
    // Get base directory
    const char* data_home = getenv("XDG_DATA_HOME");
    char base_path[512];

    if (data_home && data_home[0] == '/') {
        snprintf(base_path, sizeof(base_path), "%s", data_home);
    } else {
        const char* home = getenv("HOME");
        if (!home) {
            return KRYON_STORAGE_ERROR_IO;
        }
        snprintf(base_path, sizeof(base_path), "%s/.local/share", home);
    }

    // Create app directory if needed
    char app_dir[600];
    snprintf(app_dir, sizeof(app_dir), "%s/%s", base_path, g_storage_state.app_name);

    struct stat st;
    if (stat(app_dir, &st) != 0) {
        if (errno == ENOENT) {
            char mkdir_cmd[700];
            snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\"", app_dir);
            (void)system(mkdir_cmd);
        }
    }

    // Collection file path
    snprintf(path, size, "%s/%s/%s.json", base_path, g_storage_state.app_name, collection_name);

    return KRYON_STORAGE_OK;
}

#endif // KRYON_PLATFORM_POSIX

// ============================================================================
// Windows Backend
// ============================================================================

#ifdef KRYON_PLATFORM_WINDOWS

static kryon_storage_result_t windows_get_path(char* path, size_t size) {
    WCHAR wide_path[MAX_PATH];

    // Get %APPDATA% path
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wide_path) != S_OK) {
        return KRYON_STORAGE_ERROR_IO;
    }

    // Convert to UTF-8
    char app_data[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, app_data, sizeof(app_data), NULL, NULL);

    // Construct app-specific path
    snprintf(path, size, "%s\\%s\\storage.json", app_data, g_storage_state.app_name);

    // Create directory if needed
    char app_dir[MAX_PATH];
    snprintf(app_dir, sizeof(app_dir), "%s\\%s", app_data, g_storage_state.app_name);

    WIN32_FILE_ATTRIBUTE_DATA attrs;
    if (GetFileAttributesExA(app_dir, GetFileExInfoStandard, &attrs)) {
        if (!(attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            return KRYON_STORAGE_ERROR_IO;
        }
    } else {
        // Create directory
        if (!CreateDirectoryA(app_dir, NULL)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return KRYON_STORAGE_ERROR_IO;
            }
        }
    }

    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t windows_load(char** out_json, size_t* out_size) {
    char path[MAX_PATH];
    kryon_storage_result_t result = windows_get_path(path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    // Open file for reading
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    // Get file size
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        CloseHandle(file);
        return KRYON_STORAGE_ERROR_IO;
    }

    if (file_size.QuadPart == 0) {
        CloseHandle(file);
        return KRYON_STORAGE_ERROR_NOT_FOUND;
    }

    // Allocate buffer
    char* content = (char*)malloc(file_size.QuadPart + 1);
    if (!content) {
        CloseHandle(file);
        return KRYON_STORAGE_ERROR_NO_MEMORY;
    }

    // Read file
    DWORD bytes_read;
    if (!ReadFile(file, content, (DWORD)file_size.QuadPart, &bytes_read, NULL)) {
        free(content);
        CloseHandle(file);
        return KRYON_STORAGE_ERROR_IO;
    }

    CloseHandle(file);
    content[bytes_read] = '\0';

    *out_json = content;
    *out_size = bytes_read;

    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t windows_save(const char* json, size_t size) {
    char path[MAX_PATH];
    kryon_storage_result_t result = windows_get_path(path, sizeof(path));
    if (result != KRYON_STORAGE_OK) {
        return result;
    }

    // Open file for writing
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return KRYON_STORAGE_ERROR_IO;
    }

    // Write file
    DWORD bytes_written;
    if (!WriteFile(file, json, (DWORD)size, &bytes_written, NULL)) {
        CloseHandle(file);
        return KRYON_STORAGE_ERROR_IO;
    }

    CloseHandle(file);

    if (bytes_written != size) {
        return KRYON_STORAGE_ERROR_IO;
    }

    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t windows_get_collection_path(const char* collection_name, char* path, size_t size) {
    WCHAR wide_path[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wide_path) != S_OK) {
        return KRYON_STORAGE_ERROR_IO;
    }

    char app_data[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, app_data, sizeof(app_data), NULL, NULL);

    // Create app directory
    char app_dir[MAX_PATH];
    snprintf(app_dir, sizeof(app_dir), "%s\\%s", app_data, g_storage_state.app_name);
    CreateDirectoryA(app_dir, NULL);  // Ignore error if exists

    // Collection file path
    snprintf(path, size, "%s\\%s\\%s.json", app_data, g_storage_state.app_name, collection_name);

    return KRYON_STORAGE_OK;
}

#endif // KRYON_PLATFORM_WINDOWS

// ============================================================================
// Emscripten/Web Backend
// ============================================================================

#ifdef KRYON_PLATFORM_EMSCRIPTEN

#include <emscripten.h>

// JavaScript interface functions (defined in JS glue)
extern void js_storage_setItem(const char* key, const char* value);
extern const char* js_storage_getItem(const char* key);
extern void js_storage_removeItem(const char* key);
extern void js_storage_clear(void);

static kryon_storage_result_t emscripten_get_path(char* path, size_t size) {
    snprintf(path, size, "localStorage://%s", g_storage_state.app_name);
    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t emscripten_load(char** out_json, size_t* out_size) {
    // For localStorage, we don't load everything at startup
    // The storage is accessed on-demand via localStorage APIs
    *out_json = NULL;
    *out_size = 0;
    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t emscripten_save(const char* json, size_t size) {
    // For localStorage, each key is saved individually
    // This is handled in the JSON serialization layer
    return KRYON_STORAGE_OK;
}

static kryon_storage_result_t emscripten_get_collection_path(const char* collection_name, char* path, size_t size) {
    // Use localStorage key with collection name
    snprintf(path, size, "localStorage://%s/%s", g_storage_state.app_name, collection_name);
    return KRYON_STORAGE_OK;
}

#endif // KRYON_PLATFORM_EMSCRIPTEN

// ============================================================================
// Public Backend Interface
// ============================================================================

kryon_storage_result_t storage_backend_load(char** out_json, size_t* out_size) {
#ifdef KRYON_PLATFORM_POSIX
    return posix_load(out_json, out_size);
#elif defined(KRYON_PLATFORM_WINDOWS)
    return windows_load(out_json, out_size);
#elif defined(KRYON_PLATFORM_EMSCRIPTEN)
    return emscripten_load(out_json, out_size);
#else
    #error "Unsupported platform"
#endif
}

kryon_storage_result_t storage_backend_save(const char* json, size_t size) {
#ifdef KRYON_PLATFORM_POSIX
    return posix_save(json, size);
#elif defined(KRYON_PLATFORM_WINDOWS)
    return windows_save(json, size);
#elif defined(KRYON_PLATFORM_EMSCRIPTEN)
    return emscripten_save(json, size);
#else
    #error "Unsupported platform"
#endif
}

kryon_storage_result_t storage_backend_get_path(char* path, size_t size) {
#ifdef KRYON_PLATFORM_POSIX
    return posix_get_path(path, size);
#elif defined(KRYON_PLATFORM_WINDOWS)
    return windows_get_path(path, size);
#elif defined(KRYON_PLATFORM_EMSCRIPTEN)
    return emscripten_get_path(path, size);
#else
    #error "Unsupported platform"
#endif
}

const char* storage_backend_get_name(void) {
#ifdef KRYON_PLATFORM_POSIX
    #ifdef __APPLE__
        return "macos";
    #else
        return "posix";
    #endif
#elif defined(KRYON_PLATFORM_WINDOWS)
    return "windows";
#elif defined(KRYON_PLATFORM_EMSCRIPTEN)
    return "emscripten";
#else
    return "unknown";
#endif
}

kryon_storage_result_t storage_backend_get_collection_path(const char* name, char* path, size_t size) {
#ifdef KRYON_PLATFORM_POSIX
    return posix_get_collection_path(name, path, size);
#elif defined(KRYON_PLATFORM_WINDOWS)
    return windows_get_collection_path(name, path, size);
#elif defined(KRYON_PLATFORM_EMSCRIPTEN)
    return emscripten_get_collection_path(name, path, size);
#else
    #error "Unsupported platform"
#endif
}
