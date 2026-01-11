# Kryon Storage Plugin

localStorage-like persistent storage for Kryon applications.

## Features

- **localStorage-like API**: Simple key-value string storage
- **Cross-platform**: Works on Linux, macOS, Windows, and Web (WASM)
- **Auto-save**: Data persists automatically on every change
- **JSON format**: Human-readable storage files for easy debugging
- **Multi-language**: Nim and Lua bindings via FFI

## Installation

```bash
cd kryon-plugin-storage
make
```

## Usage

### Lua

```lua
local Storage = require("kryon.storage")

-- Initialize with app name
Storage.init("myapp")

-- Set values
Storage.setItem("username", "alice")
Storage.setItem("theme", "dark")

-- Get values
local name = Storage.getItem("username", "guest")
local theme = Storage.getItem("theme", "light")

-- Check if key exists
if Storage.hasKey("settings") then
  -- ...
end

-- Get all keys
local keys = Storage.keys()

-- Remove a key
Storage.removeItem("old_key")

-- Clear all storage
Storage.clear()

-- Get number of items
local count = Storage.count()
```

### Nim

```nim
import storage

# Initialize
discard storage.init("myapp")

# Set values
discard storage.setItem("username", "alice")
discard storage.setItem("theme", "dark")

# Get values
let name = storage.getItem("username", "guest")
let theme = storage.getItem("theme", "light")

# Check if key exists
if storage.hasKey("settings"):
  # ...

# Get count
let count = storage.count()

# Remove
discard storage.removeItem("old_key")

# Clear all
discard storage.clear()
```

### C API

```c
#include "kryon_storage.h"

// Initialize
kryon_storage_init("myapp");

// Set values
kryon_storage_set_item("username", "alice");

// Get value
char value[256];
if (kryon_storage_get_item("username", value, sizeof(value)) == KRYON_STORAGE_OK) {
    printf("Username: %s\n", value);
}

// Cleanup
kryon_storage_shutdown();
```

## Storage Location

The storage file is automatically placed in the appropriate location for each platform:

| Platform   | Path                                    |
|------------|-----------------------------------------|
| Linux      | `~/.local/share/{app_name}/storage.json` |
| macOS      | `~/Library/Application Support/{app_name}/storage.json` |
| Windows    | `%APPDATA%/{app_name}/storage.json`      |
| Web        | `localStorage` (browser)                |

## API Reference

### Lifecycle

- `init(app_name)` - Initialize storage for an app
- `shutdown()` - Save and cleanup (automatic on exit)

### Key-Value Operations

- `setItem(key, value)` - Store a string value
- `getItem(key, default)` - Get a value, returns default if not found
- `removeItem(key)` - Delete a key
- `clear()` - Delete all keys
- `hasKey(key)` - Check if key exists

### Query Operations

- `count()` - Get number of stored items
- `keys()` - Get array of all keys

### Persistence

- `save()` - Manually save to disk (only needed if auto-save disabled)
- `setAutoSave(enable)` - Enable/disable auto-save
- `getPath()` - Get the storage file path

## License

0BSD - Public domain equivalent
