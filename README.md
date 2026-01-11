# Kryon Plugins

Official plugin repository for [Kryon](https://github.com/kryon-lang/kryon) - a modular UI framework and intermediate representation.

## Plugins

| Plugin | Description | Languages |
|--------|-------------|-----------|
| [flowchart](./flowchart) | Flowchart and node diagram support with Mermaid syntax | C |
| [syntax](./syntax) | Syntax highlighting for code display | C |
| [canvas](./canvas) | Love2D-inspired immediate mode drawing | Nim, Lua |
| [storage](./storage) | localStorage-like persistent storage | Nim, Lua, JS, TS |

## Quick Start

### Development Environment

```bash
# Using Nix (recommended)
nix-shell

# Or install dependencies manually:
# - gcc
# - make
# - SDL3, sdl3-ttf (for canvas)
# - nim (for canvas bindings)
```

### Building All Plugins

```bash
make -C flowchart
make -C syntax
make -C canvas
make -C storage
```

### Installing

Each plugin installs to `~/.local/lib/kryon/plugins/`:

```bash
cd flowchart && make install
cd syntax && make install
cd canvas && make install
cd storage && make install
```

## Plugin Details

### Flowchart

Flowchart and node diagram support with Mermaid syntax translation.

- Native flowchart components
- Mermaid syntax parser
- Multi-backend support (Terminal, SDL3, HTML)
- Graph layout algorithm

```kry
@plugin flowchart

Flowchart {
    mermaid = """
        flowchart TB
            A[Start] --> B{Decision}
            B -->|Yes| C[Process]
    """
}
```

### Syntax

Syntax highlighting library for code display in Kryon applications.

- Supports: C-like, Python, Bash, JSON, Kryon
- Themable output
- Terminal and HTML renderers

### Canvas

Love2D-inspired immediate mode canvas drawing plugin.

- Vector graphics: shapes, lines, circles, polygons
- Multi-backend: SDL3 and terminal rendering
- Zero hardcoding: generic component renderer registration

```nim
Canvas:
  width = 800
  height = 600
  backgroundColor = "black"
```

### Storage

localStorage-like persistent storage for Kryon applications.

- Simple key-value string storage
- Auto-save on every change
- JSON format storage files
- Cross-platform (Linux, macOS, Windows, WASM)

```lua
Storage.init("myapp")
Storage.setItem("username", "alice")
local name = Storage.getItem("username", "guest")
```

## Project Structure

```
kryon-plugins/
├── flowchart/       # Flowchart rendering
├── syntax/          # Syntax highlighting
├── canvas/          # Canvas drawing
├── storage/         # Persistent storage
├── .gitignore       # Unified gitignore
├── shell.nix        # Nix development shell
└── README.md        # This file
```

## Requirements

- **All**: gcc, make
- **Canvas**: SDL3, sdl3-ttf, nim (for bindings)
- **Kryon IR**: From main Kryon installation

## License

Each plugin has its own license - see individual plugin directories.

0BSD for canvas and storage (public domain equivalent).

## Contributing

Contributions welcome! Please submit PRs to individual plugin subdirectories.
