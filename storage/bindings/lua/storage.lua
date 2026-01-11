-- Kryon Storage Plugin - Lua Bindings
--
-- localStorage-like persistent storage for Kryon Lua applications.
-- Includes JSON encode/decode functionality using cJSON.

local ffi = require("ffi")

-- RTLD flags for loading libraries globally
local RTLD_NOW = 0x00002
local RTLD_GLOBAL = 0x00100

-- cJSON FFI definitions
ffi.cdef[[
typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

// Construction
cJSON* cJSON_CreateObject(void);
cJSON* cJSON_CreateArray(void);
cJSON* cJSON_CreateString(const char* string);
cJSON* cJSON_CreateNumber(double num);
cJSON* cJSON_CreateTrue(void);
cJSON* cJSON_CreateFalse(void);
cJSON* cJSON_CreateNull(void);

// Adding items
void cJSON_AddItemToObject(cJSON* object, const char* string, cJSON* item);
void cJSON_AddItemToArray(cJSON* array, cJSON* item);

// Parsing
cJSON* cJSON_Parse(const char* value);

// Printing
char* cJSON_Print(cJSON* item);
char* cJSON_PrintUnformatted(cJSON* item);
void cJSON_free(char* ptr);

// Cleanup
void cJSON_Delete(cJSON* item);

// Accessors
cJSON* cJSON_GetObjectItem(cJSON* object, const char* string);
cJSON* cJSON_GetArrayItem(cJSON* array, int index);
int cJSON_GetArraySize(cJSON* array);
int cJSON_IsArray(cJSON* item);
int cJSON_IsObject(cJSON* item);
int cJSON_IsString(cJSON* item);
int cJSON_IsNumber(cJSON* item);
int cJSON_IsTrue(cJSON* item);
int cJSON_IsFalse(cJSON* item);
int cJSON_IsNull(cJSON* item);

// Get values
char* cJSON_GetStringValue(cJSON* item);
double cJSON_GetNumberValue(cJSON* item);
]]

-- Load the storage shared library with RTLD_GLOBAL
-- This makes the statically-linked cJSON symbols available globally via ffi.C
local libname = "libkryon_storage"
local success, lib = pcall(ffi.load, libname, RTLD_NOW + RTLD_GLOBAL)
local err = lib  -- If failed, lib contains the error message

-- Fallback: try loading from build directories
if not success then
    local home = os.getenv("HOME") or ""
    local kryon_root = os.getenv("KRYON_ROOT") or (home .. "/.local/share/kryon")

    local paths = {
        kryon_root .. "/plugins/kryon-storage/build/libkryon_storage.so",
        home .. "/.local/lib/libkryon_storage.so",
        "../kryon-storage/build/libkryon_storage.so",  -- Development
        "./build/libkryon_storage.so",                 -- Local build
        "/usr/local/lib/libkryon_storage.so",          -- System install
        "/mnt/storage/Projects/kryon-storage/build/libkryon_storage.so",  -- Legacy fallback
    }
    for _, path in ipairs(paths) do
        success, lib = pcall(ffi.load, path, RTLD_NOW + RTLD_GLOBAL)
        if success then
            print("✓ Loaded library from: " .. path)
            break
        else
            print("✗ Failed to load " .. path .. ": " .. tostring(lib))
            err = lib
        end
    end
end

if not success then
    error("Failed to load libkryon_storage from all paths. Last error: " .. tostring(err))
end

-- Storage C function definitions
ffi.cdef[[
typedef enum {
    KRYON_STORAGE_OK = 0,
    KRYON_STORAGE_ERROR_NOT_FOUND,
    KRYON_STORAGE_ERROR_IO,
    KRYON_STORAGE_ERROR_PARSE,
    KRYON_STORAGE_ERROR_NO_MEMORY,
    KRYON_STORAGE_ERROR_NOT_INITIALIZED,
    KRYON_STORAGE_ERROR_INVALID_ARG
} kryon_storage_result_t;

kryon_storage_result_t kryon_storage_init(const char* app_name);
kryon_storage_result_t kryon_storage_shutdown(void);
kryon_storage_result_t kryon_storage_set_item(const char* key, const char* value);
kryon_storage_result_t kryon_storage_get_item(const char* key, char* out_value, size_t out_size);
kryon_storage_result_t kryon_storage_remove_item(const char* key);
kryon_storage_result_t kryon_storage_clear(void);
bool kryon_storage_has_key(const char* key);
size_t kryon_storage_count(void);
kryon_storage_result_t kryon_storage_keys(char*** out_keys, size_t* out_count);
void kryon_storage_free_keys(char** keys, size_t count);
bool kryon_storage_set_auto_save(bool enable);
kryon_storage_result_t kryon_storage_save(void);
kryon_storage_result_t kryon_storage_get_path(char* path, size_t size);
const char* kryon_storage_get_backend_name(void);

// Collection API
kryon_storage_result_t kryon_storage_save_collection(const char* name, const char* json_data);
kryon_storage_result_t kryon_storage_load_collection(const char* name, char** out_json, size_t* out_size);
kryon_storage_result_t kryon_storage_remove_collection(const char* name);
kryon_storage_result_t kryon_storage_collection_exists(const char* name, bool* out_exists);
kryon_storage_result_t kryon_storage_get_collection_path(const char* name, char* out_path, size_t out_size);
]]

-- Storage module
local Storage = {
    _initialized = false,
    _buffer_size = 4096,
    _buffer = ffi.new("char[?]", 4096),
    _lib = lib
}

-- ============================================================================
-- JSON Encode/Decode Functions
-- ============================================================================

-- Convert Lua value to cJSON
local function lua_to_cjson(value, depth)
    depth = depth or 0
    if depth > 100 then
        error("JSON encode: nesting too deep")
    end

    local value_type = type(value)

    if value_type == "table" then
        -- Check if it's an array
        local is_array = true
        local max_index = 0
        for k, v in pairs(value) do
            if type(k) ~= "number" then
                is_array = false
                break
            end
            if k > max_index then
                max_index = k
            end
        end

        if is_array and #value > 0 then
            local cjson_array = ffi.C.cJSON_CreateArray()
            for i = 1, #value do
                local item = lua_to_cjson(value[i], depth + 1)
                ffi.C.cJSON_AddItemToArray(cjson_array, item)
            end
            return cjson_array
        else
            local cjson_object = ffi.C.cJSON_CreateObject()
            for k, v in pairs(value) do
                local cjson_value = lua_to_cjson(v, depth + 1)
                ffi.C.cJSON_AddItemToObject(cjson_object, tostring(k), cjson_value)
            end
            return cjson_object
        end

    elseif value_type == "string" then
        return ffi.C.cJSON_CreateString(tostring(value))

    elseif value_type == "number" then
        return ffi.C.cJSON_CreateNumber(value)

    elseif value_type == "boolean" then
        return value and ffi.C.cJSON_CreateTrue() or ffi.C.cJSON_CreateFalse()

    elseif value_type == "nil" then
        return ffi.C.cJSON_CreateNull()

    else
        error("JSON encode: unsupported type " .. value_type)
    end
end

-- Convert cJSON to Lua value
local function cjson_to_lua(cjson, depth)
    depth = depth or 0
    if depth > 100 then
        error("JSON decode: nesting too deep")
    end

    if cjson == nil then
        return nil
    end

    if ffi.C.cJSON_IsObject(cjson) ~= 0 then
        local result = {}
        local child = cjson.child
        while child ~= nil do
            local key = ffi.string(child.string)
            local value = cjson_to_lua(child, depth + 1)
            result[key] = value
            child = child.next
        end
        return result
    end

    if ffi.C.cJSON_IsArray(cjson) ~= 0 then
        local size = ffi.C.cJSON_GetArraySize(cjson)
        local result = {}
        for i = 0, size - 1 do
            local item = ffi.C.cJSON_GetArrayItem(cjson, i)
            result[i + 1] = cjson_to_lua(item, depth + 1)
        end
        return result
    end

    if ffi.C.cJSON_IsString(cjson) ~= 0 then
        local val = ffi.C.cJSON_GetStringValue(cjson)
        if val ~= nil then
            return ffi.string(val)
        end
        return ""
    end

    if ffi.C.cJSON_IsNumber(cjson) ~= 0 then
        local num = ffi.C.cJSON_GetNumberValue(cjson)
        return tonumber(num)
    end

    if ffi.C.cJSON_IsTrue(cjson) ~= 0 then
        return true
    end

    if ffi.C.cJSON_IsFalse(cjson) ~= 0 then
        return false
    end

    if ffi.C.cJSON_IsNull(cjson) ~= 0 then
        return nil
    end

    return nil
end

-- Public JSON encode function
-- pretty: if true, output formatted JSON with indentation
function Storage.encode(value, pretty)
    local cjson = lua_to_cjson(value)
    if cjson == nil then
        return nil
    end
    local str
    if pretty then
        str = ffi.C.cJSON_Print(cjson)
    else
        str = ffi.C.cJSON_PrintUnformatted(cjson)
    end
    ffi.C.cJSON_Delete(cjson)
    if str ~= nil then
        local result = ffi.string(str)
        ffi.C.cJSON_free(str)
        return result
    end
    return nil
end

-- Public JSON decode function
function Storage.decode(str)
    if str == nil or str == "" then
        return nil
    end
    local cjson = ffi.C.cJSON_Parse(str)
    if cjson == nil then
        return nil, "JSON parse error"
    end

    -- Handle object case specially
    if ffi.C.cJSON_IsObject(cjson) ~= 0 then
        local result = {}
        local child = cjson.child
        while child ~= nil do
            local key = ffi.string(child.string)
            local value = cjson_to_lua(child)
            result[key] = value
            child = child.next
        end
        ffi.C.cJSON_Delete(cjson)
        return result
    end

    local result = cjson_to_lua(cjson)
    ffi.C.cJSON_Delete(cjson)
    return result
end

-- ============================================================================
-- Storage API Functions
-- ============================================================================

-- Initialize storage for an app
function Storage.init(appName)
    -- Make appName optional - auto-detect if not provided
    -- This allows the Kryon plugin system to call init() without params
    if not appName or appName == "" then
        -- Try environment variable first
        appName = os.getenv("KRYON_APP_NAME")

        -- If still not set, extract from calling file
        if not appName or appName == "" then
            local info = debug.getinfo(2, "S")
            if info and info.source then
                -- Extract filename without extension
                appName = info.source:match("([^/]+)%.lua$") or "app"
            else
                appName = "app"
            end
        end

        print(string.format("ℹ️  Storage.init: auto-detected appName='%s'", appName))
    end

    local result = Storage._lib.kryon_storage_init(appName)
    if result == 0 then -- KRYON_STORAGE_OK
        Storage._initialized = true
        return true
    else
        error("Storage.init: failed with error code " .. result)
    end
end

-- Set a key-value pair
function Storage.setItem(key, value)
    if not Storage._initialized then
        error("Storage not initialized. Call Storage.init(appName) first.")
    end
    if not key or key == "" then
        error("Storage.setItem: key is required")
    end
    if value == nil then
        value = ""
    end

    -- Auto-encode tables to JSON
    local value_to_store
    if type(value) == "table" then
        value_to_store = Storage.encode(value)
        if value_to_store == nil then
            error("Storage.setItem: failed to encode table to JSON")
        end
    else
        value_to_store = tostring(value)
    end

    local result = Storage._lib.kryon_storage_set_item(tostring(key), value_to_store)
    return result == 0
end

-- Get a value by key
function Storage.getItem(key, default)
    if not Storage._initialized then
        error("Storage not initialized")
    end
    if not key then
        error("Storage.getItem: key is required")
    end

    local result = Storage._lib.kryon_storage_get_item(tostring(key), Storage._buffer, Storage._buffer_size)
    if result == 0 then
        return ffi.string(Storage._buffer)
    else
        return default or nil
    end
end

-- Remove a key
function Storage.removeItem(key)
    if not Storage._initialized then
        error("Storage not initialized")
    end
    if not key then
        error("Storage.removeItem: key is required")
    end

    return Storage._lib.kryon_storage_remove_item(tostring(key)) == 0
end

-- Clear all storage
function Storage.clear()
    if not Storage._initialized then
        error("Storage not initialized")
    end

    return Storage._lib.kryon_storage_clear() == 0
end

-- Check if key exists
function Storage.hasKey(key)
    if not Storage._initialized then
        return false
    end
    if not key then
        return false
    end

    return Storage._lib.kryon_storage_has_key(tostring(key)) ~= 0
end

-- Get count of items
function Storage.count()
    if not Storage._initialized then
        return 0
    end

    return tonumber(Storage._lib.kryon_storage_count())
end

-- Get all keys
function Storage.keys()
    if not Storage._initialized then
        error("Storage not initialized")
    end

    local c_keys_arr = ffi.new("char*[1]")
    local count_ptr = ffi.new("size_t[1]")

    local result = Storage._lib.kryon_storage_keys(c_keys_arr, count_ptr)
    if result ~= 0 then
        return {}
    end

    local count = tonumber(count_ptr[0])
    local keys = {}

    for i = 0, count - 1 do
        table.insert(keys, ffi.string(c_keys_arr[0][i]))
    end

    Storage._lib.kryon_storage_free_keys(c_keys_arr[0], count)

    return keys
end

-- Enable/disable auto-save
function Storage.setAutoSave(enable)
    if not Storage._initialized then
        error("Storage not initialized")
    end

    local previous = Storage._lib.kryon_storage_set_auto_save(enable)
    return previous ~= 0
end

-- Manually save to disk
function Storage.save()
    if not Storage._initialized then
        error("Storage not initialized")
    end

    return Storage._lib.kryon_storage_save() == 0
end

-- Get storage file path
function Storage.getPath()
    if not Storage._initialized then
        error("Storage not initialized")
    end

    local path_buf = ffi.new("char[512]")
    local result = Storage._lib.kryon_storage_get_path(path_buf, 512)
    if result == 0 then
        return ffi.string(path_buf)
    end
    return nil
end

-- Get backend name
function Storage.getBackendName()
    local name = Storage._lib.kryon_storage_get_backend_name()
    return ffi.string(name)
end

-- ============================================================================
-- Reactive Integration
-- ============================================================================

-- Create a persisted reactive state
function Storage.persisted(key, initialValue, options)
    options = options or {}

    -- Try to load Reactive module
    local Reactive = require("kryon.reactive")

    -- Create reactive state with initial value
    local state = Reactive.state(initialValue)

    -- Try to load from storage
    local stored = Storage.getItem(key)
    if stored and stored ~= "" then
        local success, data = pcall(Storage.decode, stored)
        if success and data then
            state:set(data)
        end
    end

    -- Auto-save on change (unless explicitly disabled)
    if options.autoSave ~= false then
        state:observe(function(newValue)
            local encoded = Storage.encode(newValue)
            Storage.setItem(key, encoded)
        end)
    end

    -- Add persist() method for manual saving
    state.persist = function()
        local encoded = Storage.encode(state:get())
        return Storage.setItem(key, encoded)
    end

    -- Add reload() method for manual loading
    state.reload = function()
        local stored = Storage.getItem(key)
        if stored and stored ~= "" then
            local success, data = pcall(Storage.decode, stored)
            if success and data then
                state:set(data)
                return true
            end
        end
        return false
    end

    return state
end

-- ============================================================================
-- Collections API
-- ============================================================================

Storage.Collections = {}

-- Save a collection to disk
-- Usage: Storage.Collections.save("habits", habitsList)
function Storage.Collections.save(name, data)
    if not Storage._initialized then
        error("Storage not initialized. Call Storage.init(appName) first.")
    end
    if not name or name == "" then
        error("Collection name is required")
    end
    if not data then
        error("Collection data is required")
    end

    -- Wrap data in collection object: {"habits": [...]}
    local collection_obj = {[name] = data}

    -- Encode to JSON (pretty-printed for readability)
    local json_str = Storage.encode(collection_obj, true)
    if not json_str then
        error("Failed to encode collection to JSON")
    end

    -- Save to disk
    local result = Storage._lib.kryon_storage_save_collection(name, json_str)
    if result ~= 0 then
        error(string.format("Failed to save collection '%s' (error code: %d)", name, result))
    end

    return true
end

-- Load a collection from disk
-- Usage: local habits = Storage.Collections.load("habits", defaultHabits)
function Storage.Collections.load(name, default)
    if not Storage._initialized then
        error("Storage not initialized")
    end
    if not name or name == "" then
        error("Collection name is required")
    end

    -- Allocate pointers for C function
    local json_ptr = ffi.new("char*[1]")
    local size_ptr = ffi.new("size_t[1]")

    local result = Storage._lib.kryon_storage_load_collection(name, json_ptr, size_ptr)

    if result == 1 then  -- KRYON_STORAGE_ERROR_NOT_FOUND
        return default
    elseif result ~= 0 then
        error(string.format("Failed to load collection '%s' (error code: %d)", name, result))
    end

    -- Decode JSON
    local json_str = ffi.string(json_ptr[0], size_ptr[0])
    ffi.C.free(json_ptr[0])  -- Free C-allocated memory

    local collection_obj = Storage.decode(json_str)
    if not collection_obj then
        return default
    end

    -- Extract data from wrapper: {"habits": [...]} -> [...]
    return collection_obj[name] or default
end

-- Remove a collection file
function Storage.Collections.remove(name)
    if not Storage._initialized then
        error("Storage not initialized")
    end
    if not name or name == "" then
        error("Collection name is required")
    end

    local result = Storage._lib.kryon_storage_remove_collection(name)
    return result == 0
end

-- Check if collection exists
function Storage.Collections.exists(name)
    if not Storage._initialized then
        return false
    end
    if not name or name == "" then
        return false
    end

    local exists_ptr = ffi.new("bool[1]")
    local result = Storage._lib.kryon_storage_collection_exists(name, exists_ptr)

    if result ~= 0 then
        return false
    end

    return exists_ptr[0]
end

-- Get collection file path
function Storage.Collections.getPath(name)
    if not Storage._initialized then
        error("Storage not initialized")
    end
    if not name or name == "" then
        error("Collection name is required")
    end

    local path_buf = ffi.new("char[512]")
    local result = Storage._lib.kryon_storage_get_collection_path(name, path_buf, 512)

    if result == 0 then
        return ffi.string(path_buf)
    end

    return nil
end

return Storage
