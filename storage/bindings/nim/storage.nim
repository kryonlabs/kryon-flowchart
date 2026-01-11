# Kryon Storage Plugin - Nim Bindings
#
# localStorage-like persistent storage for Kryon Nim applications.

{.pragma: dynlib: "libkryon_storage".}

type
  StorageResult* = enum
    StorageOK = 0
    StorageErrorNotFound
    StorageErrorIO
    StorageErrorParse
    StorageErrorNoMemory
    StorageErrorNotInitialized
    StorageErrorInvalidArg

# Lifecycle
proc storage_init*(app_name: cstring): StorageResult {.importc: "kryon_storage_init".}
proc storage_shutdown*() {.importc: "kryon_storage_shutdown".}

# Key-Value Operations
proc storage_set_item*(key: cstring, value: cstring): StorageResult {.importc: "kryon_storage_set_item".}
proc storage_get_item*(key: cstring, out_value: ptr char, out_size: csize_t): StorageResult {.importc: "kryon_storage_get_item".}
proc storage_remove_item*(key: cstring): StorageResult {.importc: "kryon_storage_remove_item".}
proc storage_clear*(): StorageResult {.importc: "kryon_storage_clear".}

# Query Operations
proc storage_has_key*(key: cstring): bool {.importc: "kryon_storage_has_key".}
proc storage_count*(): csize_t {.importc: "kryon_storage_count".}

# Persistence
proc storage_set_auto_save*(enable: bool): bool {.importc: "kryon_storage_set_auto_save".}
proc storage_save*(): StorageResult {.importc: "kryon_storage_save".}
proc storage_get_backend_name*(): cstring {.importc: "kryon_storage_get_backend_name".}

# Nim-friendly wrapper module
type StorageError* = object of CatchableError
  code*: StorageResult

proc checkError*(res: StorageResult) =
  if res != StorageOK:
    raise StorageError(msg: "Storage error: " & $res, code: res)

# High-level API
proc initStorage*(appName: string): bool {.raises: [].} =
  ## Initialize storage for an app
  try:
    storage_init(appName) == StorageOK
  except:
    false

proc setItem*(key, value: string): bool {.raises: [].} =
  ## Set a key-value pair
  try:
    storage_set_item(key, value) == StorageOK
  except:
    false

proc getItem*(key: string, default = ""): string {.raises: [].} =
  ## Get a value by key, returns default if not found
  var buf: array[4096, char]
  let res = storage_get_item(key, buf.addr, 4096)
  if res == StorageOK:
    result = $buf.addr
  else:
    result = default

proc removeItem*(key: string): bool {.raises: [].} =
  ## Remove a key
  try:
    storage_remove_item(key) == StorageOK
  except:
    false

proc clear*(): bool {.raises: [].} =
  ## Clear all storage
  try:
    storage_clear() == StorageOK
  except:
    false

proc hasKey*(key: string): bool {.raises: [].} =
  ## Check if a key exists
  try:
    storage_has_key(key)
  except:
    false

proc count*(): int {.raises: [].} =
  ## Get count of items
  try:
    int(storage_count())
  except:
    0

proc setAutoSave*(enable: bool): bool {.raises: [].} =
  ## Enable/disable auto-save
  try:
    storage_set_auto_save(enable)
  except:
    false

proc save*(): bool {.raises: [].} =
  ## Manually save to disk
  try:
    storage_save() == StorageOK
  except:
    false

proc getBackendName*(): string {.raises: [].} =
  ## Get the active backend name
  try:
    $storage_get_backend_name()
  except:
    "unknown"
