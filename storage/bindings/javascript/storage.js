/**
 * Kryon Storage Plugin - JavaScript Bindings
 *
 * localStorage-compatible API for Kryon JavaScript applications.
 * Works in both browser (via localStorage) and native (via C plugin).
 */

// Detect environment
const isBrowser = typeof window !== 'undefined' && typeof window.localStorage !== 'undefined';
const isNode = typeof process !== 'undefined' && process.versions && process.versions.node;

/**
 * Storage manager class
 */
class KryonStorage {
  constructor() {
    this._initialized = false;
    this._appName = null;
    this._backend = null;
  }

  /**
   * Initialize storage with app name
   * @param {string} appName - Application name for namespace
   * @returns {boolean} Success status
   */
  init(appName) {
    if (!appName || typeof appName !== 'string') {
      throw new Error('Storage.init: appName must be a non-empty string');
    }

    this._appName = appName;
    this._initialized = true;

    // Use appropriate backend
    if (isBrowser) {
      this._backend = new BrowserBackend(appName);
    } else if (isNode) {
      this._backend = new NodeBackend(appName);
    } else {
      throw new Error('Unsupported environment for KryonStorage');
    }

    return true;
  }

  /**
   * Set a key-value pair
   * @param {string} key - Storage key
   * @param {string} value - Storage value
   * @returns {boolean} Success status
   */
  setItem(key, value) {
    this._checkInitialized();
    return this._backend.setItem(key, String(value));
  }

  /**
   * Get a value by key
   * @param {string} key - Storage key
   * @param {string} [defaultValue] - Default value if key not found
   * @returns {string|null} Stored value or default
   */
  getItem(key, defaultValue = null) {
    this._checkInitialized();
    const value = this._backend.getItem(key);
    return value !== null ? value : defaultValue;
  }

  /**
   * Remove a key
   * @param {string} key - Storage key to remove
   * @returns {boolean} Success status
   */
  removeItem(key) {
    this._checkInitialized();
    return this._backend.removeItem(key);
  }

  /**
   * Clear all storage
   * @returns {boolean} Success status
   */
  clear() {
    this._checkInitialized();
    return this._backend.clear();
  }

  /**
   * Check if key exists
   * @param {string} key - Storage key
   * @returns {boolean} True if key exists
   */
  hasKey(key) {
    this._checkInitialized();
    return this._backend.hasKey(key);
  }

  /**
   * Get all keys
   * @returns {string[]} Array of keys
   */
  keys() {
    this._checkInitialized();
    return this._backend.keys();
  }

  /**
   * Get count of items
   * @returns {number} Number of stored items
   */
  count() {
    this._checkInitialized();
    return this._backend.count();
  }

  /**
   * Set JSON value (auto-serializes)
   * @param {string} key - Storage key
   * @param {*} value - Value to serialize and store
   * @returns {boolean} Success status
   */
  setJSON(key, value) {
    return this.setItem(key, JSON.stringify(value));
  }

  /**
   * Get JSON value (auto-deserializes)
   * @param {string} key - Storage key
   * @param {*} [defaultValue] - Default value if key not found or parse fails
   * @returns {*} Parsed value or default
   */
  getJSON(key, defaultValue = null) {
    const item = this.getItem(key);
    if (item === null) return defaultValue;

    try {
      return JSON.parse(item);
    } catch {
      return defaultValue;
    }
  }

  /**
   * Create a reactive persisted state
   * Requires Kryon reactive system to be loaded
   * @param {string} key - Storage key
   * @param {*} initialValue - Initial value
   * @param {Object} [options] - Options {autoSave: boolean}
   * @returns {Object} Reactive state object
   */
  persisted(key, initialValue, options = {}) {
    // Note: This requires the Kryon reactive system
    // For web, this would integrate with React/Preact hooks
    // For native, this would integrate with Kryon's reactive system

    const state = {
      _key: key,
      _value: initialValue,
      _listeners: [],
      _autoSave: options.autoSave !== false,

      get() {
        return this._value;
      },

      set(newValue) {
        this._value = newValue;
        this._listeners.forEach(fn => fn(newValue));

        if (this._autoSave) {
          storage.setJSON(this._key, newValue);
        }
      },

      subscribe(listener) {
        this._listeners.push(listener);
        return () => {
          const index = this._listeners.indexOf(listener);
          if (index > -1) this._listeners.splice(index, 1);
        };
      },

      persist() {
        return storage.setJSON(this._key, this._value);
      },

      reload() {
        const value = storage.getJSON(this._key);
        if (value !== null) {
          this._value = value;
          return true;
        }
        return false;
      }
    };

    // Load initial value from storage
    const stored = this.getJSON(key);
    if (stored !== null) {
      state._value = stored;
    }

    return state;
  }

  _checkInitialized() {
    if (!this._initialized) {
      throw new Error('Storage not initialized. Call storage.init(appName) first.');
    }
  }
}

/**
 * Browser localStorage backend
 */
class BrowserBackend {
  constructor(appName) {
    this.prefix = `${appName}.`;
  }

  setItem(key, value) {
    localStorage.setItem(this.prefix + key, value);
    return true;
  }

  getItem(key) {
    return localStorage.getItem(this.prefix + key);
  }

  removeItem(key) {
    localStorage.removeItem(this.prefix + key);
    return true;
  }

  clear() {
    const keysToRemove = [];
    for (let i = 0; i < localStorage.length; i++) {
      const key = localStorage.key(i);
      if (key && key.startsWith(this.prefix)) {
        keysToRemove.push(key);
      }
    }
    keysToRemove.forEach(key => localStorage.removeItem(key));
    return true;
  }

  hasKey(key) {
    return localStorage.getItem(this.prefix + key) !== null;
  }

  keys() {
    const result = [];
    for (let i = 0; i < localStorage.length; i++) {
      const key = localStorage.key(i);
      if (key && key.startsWith(this.prefix)) {
        result.push(key.substring(this.prefix.length));
      }
    }
    return result;
  }

  count() {
    return this.keys().length;
  }
}

/**
 * Node.js file-based backend
 */
class NodeBackend {
  constructor(appName) {
    const fs = require('fs');
    const path = require('path');
    const os = require('os');

    this.data = new Map();
    this.autoSave = true;

    const dataDir = path.join(os.homedir(), '.local', 'share', appName);
    this.filePath = path.join(dataDir, 'storage.json');

    // Create directory
    if (!fs.existsSync(dataDir)) {
      fs.mkdirSync(dataDir, { recursive: true });
    }

    // Load existing data
    if (fs.existsSync(this.filePath)) {
      try {
        const content = fs.readFileSync(this.filePath, 'utf8');
        const parsed = JSON.parse(content);
        this.data = new Map(Object.entries(parsed.items || {}));
      } catch (error) {
        console.warn('Failed to load storage:', error);
        this.data = new Map();
      }
    }
  }

  setItem(key, value) {
    this.data.set(key, value);
    if (this.autoSave) this._save();
    return true;
  }

  getItem(key) {
    return this.data.get(key) || null;
  }

  removeItem(key) {
    this.data.delete(key);
    if (this.autoSave) this._save();
    return true;
  }

  clear() {
    this.data.clear();
    if (this.autoSave) this._save();
    return true;
  }

  hasKey(key) {
    return this.data.has(key);
  }

  keys() {
    return Array.from(this.data.keys());
  }

  count() {
    return this.data.size;
  }

  _save() {
    const fs = require('fs');
    const content = JSON.stringify({
      _app: 'kryon',
      _version: 1,
      items: Object.fromEntries(this.data)
    }, null, 2);

    try {
      fs.writeFileSync(this.filePath, content, 'utf8');
    } catch (error) {
      console.error('Failed to save storage:', error);
    }
  }
}

// Singleton instance
const storage = new KryonStorage();

// Export
export default storage;
export { KryonStorage };
