# Kryon Storage Plugin - DSL Integration
#
# Provides reactive integration with the Kryon DSL for automatic persistence.

import storage
import json
import options

# Import reactive system from Kryon
when compileOption("path", "/mnt/storage/Projects/kryon/bindings/nim"):
  import ../../kryon/bindings/nim/reactive_system
else:
  # Fallback import paths
  when fileExists("../../kryon/bindings/nim/reactive_system.nim"):
    import ../../kryon/bindings/nim/reactive_system
  else:
    {.error: "Cannot find reactive_system.nim - ensure Kryon is in expected location".}

type
  PersistedVar*[T] = ref object
    key*: string
    reactiveVar*: ReactiveVar[T]
    autoSave*: bool

# Forward declaration for persist
proc persist*[T](reactiveVar: ReactiveVar[T], key: string, autoSave = true): ReactiveVar[T]

# Helper to serialize/deserialize JSON
proc toJsonString*[T](value: T): string =
  ## Convert any value to JSON string
  when T is string:
    # String values are already strings, but we need to JSON-encode them
    %* value
  elif T is bool or T is SomeInteger or T is SomeFloat:
    $(%* value)
  else:
    # Complex types: serialize to JSON
    $(% value)

proc fromJsonString*[T](jsonStr: string, default: T): T =
  ## Parse JSON string to value
  try:
    when T is string:
      parseJson(jsonStr).getStr()
    elif T is bool:
      parseJson(jsonStr).getBool()
    elif T is SomeInteger:
      T(parseJson(jsonStr).getInt())
    elif T is SomeFloat:
      T(parseJson(jsonStr).getFloat())
    else:
      # Complex types: deserialize from JSON
      parseJson(jsonStr).to(T)
  except:
    default

proc persist*[T](reactiveVar: ReactiveVar[T], key: string, autoSave = true): ReactiveVar[T] =
  ## Make a reactive variable persistent
  ##
  ## Loads initial value from storage if available, then auto-saves on changes.
  ##
  ## Example:
  ##   var count = persist(ReactiveVar(0), "counter")
  ##   count.value += 1  # Automatically saved

  # Load initial value from storage if it exists
  if hasKey(key):
    let stored = getItem(key)
    if stored.len > 0:
      try:
        let parsedValue = fromJsonString[T](stored, reactiveVar.value)
        reactiveVar.value = parsedValue
      except:
        # Failed to parse, use current value
        discard

  # Register auto-save if enabled
  if autoSave:
    reactiveVar.onChange(proc() =
      # Save to storage when value changes
      let jsonVal = toJsonString(reactiveVar.value)
      discard setItem(key, jsonVal)
    )

  return reactiveVar

# Template for declarative persistence
template persistent*(name: untyped, initialValue: typed): untyped =
  ## Create a persistent reactive variable
  ##
  ## Example:
  ##   var habits {.persistent.} = @[Habit(...)]
  ##   # or with explicit key:
  ##   persistent(habits, @[Habit(...)])
  var name {.global.} = persist(ReactiveVar(initialValue), astToStr(name), autoSave = true)

# Batch operations for efficiency
proc savePersisted*[T](pvar: ReactiveVar[T], key: string): bool =
  ## Manually save a persisted variable
  let jsonVal = toJsonString(pvar.value)
  setItem(key, jsonVal)

proc loadPersisted*[T](pvar: var ReactiveVar[T], key: string): bool =
  ## Manually load a persisted variable
  if hasKey(key):
    let stored = getItem(key)
    if stored.len > 0:
      try:
        pvar.value = fromJsonString[T](stored, pvar.value)
        return true
      except:
        return false
  return false

# Helper for JSON storage of complex types
proc setItemJson*[T](key: string, value: T): bool =
  ## Set a key-value pair with JSON serialization
  let jsonVal = toJsonString(value)
  setItem(key, jsonVal)

proc getItemJson*[T](key: string, default: T): T =
  ## Get a value with JSON deserialization
  if hasKey(key):
    let stored = getItem(key)
    if stored.len > 0:
      return fromJsonString[T](stored, default)
  return default
