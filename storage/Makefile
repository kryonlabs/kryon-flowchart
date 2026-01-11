# Kryon Storage Plugin Makefile

KRYON_ROOT ?= $(HOME)/Projects/kryon

CC := gcc
CFLAGS := -Wall -Wextra -O2 -fPIC -I$(KRYON_ROOT)/ir -I$(KRYON_ROOT)/core/include -Iinclude -Ideps/cJSON
LDFLAGS := -shared

# Source files
SRCS := src/storage.c src/storage_backends.c src/storage_json.c src/storage_collections.c
OBJS := $(SRCS:src/%.c=build/%.o)

# cJSON
CJSON_SRC := deps/cJSON/cJSON.c
CJSON_OBJ := build/cJSON.o

# Library outputs
LIB_STATIC := build/libkryon_storage.a
LIB_SHARED := build/libkryon_storage.so

.PHONY: all clean test install

all: $(LIB_STATIC) $(LIB_SHARED)
	@echo "✓ Storage plugin built successfully"
	@echo "  Static:  $(LIB_STATIC)"
	@echo "  Shared:  $(LIB_SHARED)"

# Create build directory
build:
	@mkdir -p build

# Compile object files
build/%.o: src/%.c | build
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile cJSON to object file (for static linking)
$(CJSON_OBJ): $(CJSON_SRC) | build
	@echo "Compiling cJSON..."
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "✓ cJSON compiled: $@"

# Build static library (includes cJSON)
$(LIB_STATIC): $(OBJS) $(CJSON_OBJ) | build
	@echo "Creating static library..."
	@ar rcs $@ $^ $(CJSON_OBJ)
	@echo "✓ Static library built: $@"

# Build shared library (statically links cJSON - no runtime dependency)
$(LIB_SHARED): $(OBJS) $(CJSON_OBJ)
	@echo "Creating shared library..."
	@$(CC) $(LDFLAGS) -o $@ $(OBJS) $(CJSON_OBJ)
	@echo "✓ Shared library built: $@"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build

# Run tests
test: all
	@echo "Running tests..."
	@if [ -f tests/test_storage.c ]; then \
		$(CC) $(CFLAGS) tests/test_storage.c -Lbuild -lkryon_storage -lpthread -o build/test_storage && \
		LD_LIBRARY_PATH=build:$$LD_LIBRARY_PATH ./build/test_storage; \
	else \
		echo "No tests found"; \
	fi

# Install libraries to user-local directory
install: all
	@echo "Installing kryon-storage plugin..."
	@mkdir -p $(DESTDIR)$(HOME)/.local/lib
	@mkdir -p $(DESTDIR)$(HOME)/.local/share/kryon/bindings/lua
	@cp $(LIB_STATIC) $(DESTDIR)$(HOME)/.local/lib/
	@cp $(LIB_SHARED) $(DESTDIR)$(HOME)/.local/lib/
	@cp bindings/lua/storage.lua $(DESTDIR)$(HOME)/.local/share/kryon/bindings/lua/
	@echo "✓ Installed to $(HOME)/.local/lib/"
	@echo "  Static:  $(LIB_STATIC)"
	@echo "  Shared:  $(LIB_SHARED)"
	@echo "  Lua binding: storage.lua"
