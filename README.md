# Kryon Flowchart Plugin

Flowchart and node diagram support for Kryon with Mermaid syntax translation.

## Features

- Native flowchart components (Flowchart, FlowchartNode, FlowchartEdge)
- Mermaid syntax parser for runtime diagram generation
- Multi-backend support (Terminal, Desktop/SDL3, Web/HTML)
- Graph layout algorithm (layering, positioning, edge routing)
- Subgraph support for organizing complex diagrams

## Installation

```bash
cd kryon-plugin-flowchart
make
make install
```

This will install the plugin to `~/.local/lib/kryon/plugins/`.

## Usage

### Native Flowchart Syntax

```kry
@plugin flowchart

Flowchart {
    direction = "TB"
    width = 800
    height = 600

    nodes = [
        { id = "start", text = "Start", shape = "rounded" },
        { id = "process", text = "Process", shape = "box" },
        { id = "end", text = "End", shape = "rounded" }
    ]

    edges = [
        { from = "start", to = "process" },
        { from = "process", to = "end" }
    ]
}
```

### Mermaid Syntax

```kry
@plugin flowchart

Flowchart {
    mermaid = """
        flowchart TB
            A[Start] --> B{Decision}
            B -->|Yes| C[Process]
            B -->|No| D[End]
    """
}
```

### Markdown Integration

````markdown
# My Document

## Architecture Diagram

```mermaid
flowchart LR
    A[Client] --> B[Server]
    B --> C[Database]
```
````

## API Reference

### Flowchart Component

- `direction`: Flow direction ("TB", "LR", "BT", "RL")
- `nodes`: Array of node objects with id, text, shape
- `edges`: Array of edge objects with from, to, label
- `mermaid`: Mermaid syntax string (alternative to nodes/edges)

### Node Shapes

- `box`: Rectangle
- `rounded`: Rounded rectangle
- `circle`: Circle
- `diamond`: Diamond (for decisions)
- `hexagon`: Hexagon

### Edge Types

- `solid`: Solid line (default)
- `dashed`: Dashed line
- `dotted`: Dotted line

## Migration from Core Flowchart

If you're migrating from the deprecated core flowchart implementation:

1. Add `@plugin flowchart` annotation to your .kry files
2. No other changes needed - syntax is identical
3. Old .kir files will auto-load the plugin

## Building from Source

Requirements:
- GCC or compatible C compiler
- Kryon IR library (from main Kryon installation)

```bash
make clean
make
make install
```

## Development

### Project Structure

```
kryon-plugin-flowchart/
├── include/           # Public headers
├── src/              # Source files
│   └── renderers/    # Backend-specific rendering
├── bindings/         # Language bindings
├── examples/         # Example flowcharts
└── plugin.toml       # Plugin metadata
```

### Adding New Backends

To add support for a new backend:

1. Create `src/renderers/renderer_<backend>.c`
2. Implement `render_flowchart_<backend>()` function
3. Register in `plugin_init.c`

## License

Same as Kryon core (check main repository for details).

## Contributing

Contributions welcome! Please submit PRs to the kryon-plugin-flowchart repository.
