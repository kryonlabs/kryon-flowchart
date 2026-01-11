/**
 * Kryon Storage Plugin - TypeScript Bindings
 *
 * Type-safe localStorage-compatible API for Kryon TypeScript applications.
 */

// Import JavaScript implementation
import storage from '../javascript/storage.js';

// Re-export for TypeScript consumption
export default storage;

/**
 * Storage options
 */
export interface StorageOptions {
  autoSave?: boolean;
}

/**
 * Reactive state interface
 */
export interface PersistedState<T> {
  get(): T;
  set(value: T): void;
  subscribe(listener: (value: T) => void): () => void;
  persist(): boolean;
  reload(): boolean;
}

/**
 * Type-safe storage API
 */
export class TypedStorage {
  /**
   * Initialize storage
   */
  static init(appName: string): boolean {
    return storage.init(appName);
  }

  /**
   * Set a key-value pair
   */
  static setItem(key: string, value: string): boolean {
    return storage.setItem(key, value);
  }

  /**
   * Get a value by key
   */
  static getItem(key: string, defaultValue?: string): string | null {
    return storage.getItem(key, defaultValue);
  }

  /**
   * Remove a key
   */
  static removeItem(key: string): boolean {
    return storage.removeItem(key);
  }

  /**
   * Clear all storage
   */
  static clear(): boolean {
    return storage.clear();
  }

  /**
   * Check if key exists
   */
  static hasKey(key: string): boolean {
    return storage.hasKey(key);
  }

  /**
   * Get all keys
   */
  static keys(): string[] {
    return storage.keys();
  }

  /**
   * Get count of items
   */
  static count(): number {
    return storage.count();
  }

  /**
   * Set JSON value (auto-serializes)
   */
  static setJSON<T>(key: string, value: T): boolean {
    return storage.setJSON(key, value);
  }

  /**
   * Get JSON value (auto-deserializes)
   */
  static getJSON<T>(key: string, defaultValue?: T): T | null {
    return storage.getJSON(key, defaultValue);
  }

  /**
   * Create a reactive persisted state
   */
  static persisted<T>(
    key: string,
    initialValue: T,
    options?: StorageOptions
  ): PersistedState<T> {
    return storage.persisted(key, initialValue, options) as PersistedState<T>;
  }
}

/**
 * React hook for persisted state
 * Use this with React/Preact applications
 */
export function usePersistedState<T>(
  key: string,
  initialValue: T,
  options?: StorageOptions
): [T, (value: T) => void] {
  // This would integrate with React's useState/useEffect
  // For now, providing type signature for future implementation
  const state = storage.persisted(key, initialValue, options);

  // In React:
  // const [value, setValue] = useState(state.get());
  // useEffect(() => state.subscribe(setValue), []);

  return [state.get(), state.set.bind(state)];
}

/**
 * Preact hook for persisted state
 */
export function usePersistedSignal<T>(
  key: string,
  initialValue: T,
  options?: StorageOptions
): PersistedState<T> {
  return storage.persisted(key, initialValue, options) as PersistedState<T>;
}

// Type exports
export type { StorageOptions, PersistedState };
