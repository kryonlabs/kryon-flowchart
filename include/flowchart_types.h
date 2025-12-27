#ifndef FLOWCHART_TYPES_H
#define FLOWCHART_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Flowchart component type IDs (matching core enum values for compatibility)
// These values must match what the core uses for deserialization
#define IR_COMPONENT_FLOWCHART 47
#define IR_COMPONENT_FLOWCHART_NODE 48
#define IR_COMPONENT_FLOWCHART_EDGE 49
#define IR_COMPONENT_FLOWCHART_SUBGRAPH 50
#define IR_COMPONENT_FLOWCHART_LABEL 51

// Flowchart direction (layout direction)
typedef enum {
    IR_FLOWCHART_DIR_TB,    // Top to Bottom (default)
    IR_FLOWCHART_DIR_LR,    // Left to Right
    IR_FLOWCHART_DIR_BT,    // Bottom to Top
    IR_FLOWCHART_DIR_RL     // Right to Left
} IRFlowchartDirection;

// Flowchart node shapes
typedef enum {
    IR_FLOWCHART_SHAPE_RECTANGLE,      // [text] - default
    IR_FLOWCHART_SHAPE_ROUNDED,        // (text) - rounded corners
    IR_FLOWCHART_SHAPE_STADIUM,        // ([text]) - pill shape
    IR_FLOWCHART_SHAPE_DIAMOND,        // {text} - decision
    IR_FLOWCHART_SHAPE_CIRCLE,         // ((text)) - circular
    IR_FLOWCHART_SHAPE_HEXAGON,        // {{text}} - hexagonal
    IR_FLOWCHART_SHAPE_PARALLELOGRAM,  // [/text/] - input/output
    IR_FLOWCHART_SHAPE_CYLINDER,       // [(text)] - database
    IR_FLOWCHART_SHAPE_SUBROUTINE,     // [[text]] - subroutine/predefined
    IR_FLOWCHART_SHAPE_ASYMMETRIC,     // >text] - flag shape
    IR_FLOWCHART_SHAPE_TRAPEZOID       // [/text\] - manual operation
} IRFlowchartShape;

// Flowchart edge types (line styles)
typedef enum {
    IR_FLOWCHART_EDGE_ARROW,           // --> solid arrow
    IR_FLOWCHART_EDGE_OPEN,            // --- solid line (no arrow)
    IR_FLOWCHART_EDGE_BIDIRECTIONAL,   // <--> arrows both ends
    IR_FLOWCHART_EDGE_DOTTED,          // -.-> dotted arrow
    IR_FLOWCHART_EDGE_THICK            // ==> thick arrow
} IRFlowchartEdgeType;

// Edge marker types (arrow heads)
typedef enum {
    IR_FLOWCHART_MARKER_NONE,          // No marker
    IR_FLOWCHART_MARKER_ARROW,         // Standard arrow (>)
    IR_FLOWCHART_MARKER_CIRCLE,        // Circle marker (o)
    IR_FLOWCHART_MARKER_CROSS          // Cross marker (x)
} IRFlowchartMarker;

// Flowchart node data (stored in custom_data)
typedef struct IRFlowchartNodeData {
    char* node_id;                     // Node ID for edge references (e.g., "A", "start")
    IRFlowchartShape shape;            // Visual shape
    char* label;                       // Display text

    // Computed layout (filled during layout phase)
    float x, y;                        // Position
    float width, height;               // Dimensions

    // Styling
    uint32_t fill_color;               // Background color (RGBA)
    uint32_t stroke_color;             // Border color (RGBA)
    float stroke_width;                // Border width

    // For subgraph containment
    char* subgraph_id;                 // ID of containing subgraph (NULL if none)
} IRFlowchartNodeData;

// Flowchart edge data (stored in custom_data)
typedef struct IRFlowchartEdgeData {
    char* from_id;                     // Source node ID
    char* to_id;                       // Target node ID
    char* label;                       // Optional edge label text

    // Edge styling
    IRFlowchartEdgeType type;          // Line style
    IRFlowchartMarker start_marker;    // Marker at start
    IRFlowchartMarker end_marker;      // Marker at end

    // Computed path (filled during layout phase)
    float* path_points;                // Array of x,y coordinates [x0,y0,x1,y1,...]
    uint32_t path_point_count;         // Number of coordinate pairs

    // Label position (computed)
    float label_x, label_y;            // Position for edge label
} IRFlowchartEdgeData;

// Flowchart subgraph data (for grouped nodes)
typedef struct IRFlowchartSubgraphData {
    char* subgraph_id;                 // Subgraph ID
    char* title;                       // Display title
    IRFlowchartDirection direction;    // Local direction override (LR, TB, etc.)

    // Computed bounds
    float x, y, width, height;

    // Layout cache (for nested layout)
    float local_width;                 // Width in local coordinates
    float local_height;                // Height in local coordinates
    bool layout_computed;              // Flag to track if layout done
    char* parent_subgraph_id;          // Parent subgraph ID (NULL if top-level)

    // Styling
    uint32_t background_color;         // Background color (RGBA)
    uint32_t border_color;             // Border color (RGBA)
} IRFlowchartSubgraphData;

// Flowchart state (stored in Flowchart component's custom_data)
typedef struct IRFlowchartState {
    IRFlowchartDirection direction;    // Layout direction (TB, LR, BT, RL)

    // Node registry (for edge resolution)
    IRFlowchartNodeData** nodes;       // Array of node data pointers
    uint32_t node_count;
    uint32_t node_capacity;

    // Edge registry
    IRFlowchartEdgeData** edges;       // Array of edge data pointers
    uint32_t edge_count;
    uint32_t edge_capacity;

    // Subgraph registry
    IRFlowchartSubgraphData** subgraphs;
    uint32_t subgraph_count;
    uint32_t subgraph_capacity;

    // Layout cache
    bool layout_computed;
    float computed_width;
    float computed_height;
    float natural_width;               // Natural width before scaling
    float natural_height;              // Natural height before scaling

    // Content bounds (for responsive SVG)
    float content_width;               // Actual width of flowchart content
    float content_height;              // Actual height of flowchart content
    float content_offset_x;            // Left offset (for centering)
    float content_offset_y;            // Top offset (for centering)

    // Layout parameters
    float node_spacing;                // Space between nodes
    float rank_spacing;                // Space between layers/ranks
    float subgraph_padding;            // Padding inside subgraphs
} IRFlowchartState;

#endif // FLOWCHART_TYPES_H
