# Kryon Syntax Highlighting Library
# Build: make
# Install as plugin: make install-plugin (recommended)
# Install as library: make install-lib (legacy)

CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -I./include
LDFLAGS = -shared

# Kryon IR path (for ir_plugin.h when building plugin.c)
KRYON_IR_PATH ?= ../kryon/ir

# Installation paths
PREFIX ?= $(HOME)/.local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include
PLUGIN_DIR = $(HOME)/.kryon/plugins/syntax

# Source files
SRCS = src/lexer.c \
       src/lexer_clike.c \
       src/lexer_python.c \
       src/lexer_bash.c \
       src/lexer_json.c \
       src/lexer_kry.c \
       src/theme.c \
       src/plugin.c

OBJS = $(SRCS:.c=.o)
TARGET = libkryon_syntax.so

.PHONY: all clean install install-plugin install-lib uninstall uninstall-plugin

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Plugin.c needs ir_plugin.h from kryon
src/plugin.o: src/plugin.c include/kryon_syntax.h
	$(CC) $(CFLAGS) -I$(KRYON_IR_PATH) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

# Install as a Kryon plugin (recommended)
install-plugin: $(TARGET)
	@mkdir -p $(PLUGIN_DIR)
	cp $(TARGET) $(PLUGIN_DIR)/
	cp plugin.toml $(PLUGIN_DIR)/
	@echo "Installed syntax plugin to $(PLUGIN_DIR)"
	@echo "  - $(TARGET)"
	@echo "  - plugin.toml"

# Install as a shared library (legacy, for backwards compatibility)
install-lib: $(TARGET)
	@mkdir -p $(LIBDIR)
	@mkdir -p $(INCLUDEDIR)
	cp $(TARGET) $(LIBDIR)/
	cp include/kryon_syntax.h $(INCLUDEDIR)/
	@echo "Installed $(TARGET) to $(LIBDIR)"
	@echo "Installed kryon_syntax.h to $(INCLUDEDIR)"

# Default install is now plugin mode
install: install-plugin

uninstall-plugin:
	rm -rf $(PLUGIN_DIR)

uninstall:
	rm -f $(LIBDIR)/$(TARGET)
	rm -f $(INCLUDEDIR)/kryon_syntax.h
	rm -rf $(PLUGIN_DIR)

# Dependencies
src/lexer.o: src/lexer.c include/kryon_syntax.h
src/lexer_clike.o: src/lexer_clike.c include/kryon_syntax.h
src/lexer_python.o: src/lexer_python.c include/kryon_syntax.h
src/lexer_bash.o: src/lexer_bash.c include/kryon_syntax.h
src/lexer_json.o: src/lexer_json.c include/kryon_syntax.h
src/lexer_kry.o: src/lexer_kry.c include/kryon_syntax.h
src/theme.o: src/theme.c include/kryon_syntax.h
