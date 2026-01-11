PLUGIN_NAME = kryon_flowchart
CC = gcc
CFLAGS = -Wall -Wextra -fPIC -I../kryon/ir -I./include
LDFLAGS = -shared -L../kryon/build -lkryon_ir

# Source files
SOURCES = src/plugin_init.c \
          src/flowchart_builder.c \
          src/flowchart_parser.c \
          src/flowchart_layout.c \
          src/renderers/renderer_terminal.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Output library
TARGET = lib$(PLUGIN_NAME).so

# Installation directory
INSTALL_DIR = ~/.local/lib/kryon/plugins

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/

uninstall:
	rm -f $(INSTALL_DIR)/$(TARGET)
