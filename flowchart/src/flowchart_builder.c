#include "flowchart_builder.h"
#include "ir_builder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Default layout parameters
#define DEFAULT_NODE_SPACING 20.0f
#define DEFAULT_RANK_SPACING 40.0f
#define DEFAULT_SUBGRAPH_PADDING 40.0f

// ============================================================================
// State Management
// ============================================================================

IRFlowchartState* ir_flowchart_create_state(void) {
    IRFlowchartState* state = (IRFlowchartState*)calloc(1, sizeof(IRFlowchartState));
    if (!state) return NULL;

    state->direction = IR_FLOWCHART_DIR_TB;
    state->nodes = NULL;
    state->node_count = 0;
    state->node_capacity = 0;
    state->edges = NULL;
    state->edge_count = 0;
    state->edge_capacity = 0;
    state->subgraphs = NULL;
    state->subgraph_count = 0;
    state->subgraph_capacity = 0;
    state->layout_computed = false;
    state->node_spacing = DEFAULT_NODE_SPACING;
    state->rank_spacing = DEFAULT_RANK_SPACING;
    state->subgraph_padding = DEFAULT_SUBGRAPH_PADDING;

    return state;
}

void ir_flowchart_destroy_state(IRFlowchartState* state) {
    if (!state) return;
    free(state->nodes);
    free(state->edges);
    free(state->subgraphs);
    free(state);
}

IRFlowchartState* ir_get_flowchart_state(IRComponent* c) {
    if (!c || c->type != IR_COMPONENT_FLOWCHART) return NULL;
    return (IRFlowchartState*)c->custom_data;
}

// ============================================================================
// Node Data Management
// ============================================================================

IRFlowchartNodeData* ir_flowchart_node_data_create(const char* node_id, IRFlowchartShape shape, const char* label) {
    IRFlowchartNodeData* data = (IRFlowchartNodeData*)calloc(1, sizeof(IRFlowchartNodeData));
    if (!data) return NULL;

    data->node_id = node_id ? strdup(node_id) : NULL;
    data->shape = shape;
    data->label = label ? strdup(label) : NULL;
    data->fill_color = 0xFFFFFFFF;
    data->stroke_color = 0x000000FF;
    data->stroke_width = 1.0f;

    return data;
}

void ir_flowchart_node_data_destroy(IRFlowchartNodeData* data) {
    if (!data) return;
    free(data->node_id);
    free(data->label);
    free(data->subgraph_id);
    free(data);
}

IRFlowchartNodeData* ir_get_flowchart_node_data(IRComponent* c) {
    if (!c || c->type != IR_COMPONENT_FLOWCHART_NODE) return NULL;
    return (IRFlowchartNodeData*)c->custom_data;
}

// ============================================================================
// Edge Data Management
// ============================================================================

IRFlowchartEdgeData* ir_flowchart_edge_data_create(const char* from_id, const char* to_id) {
    IRFlowchartEdgeData* data = (IRFlowchartEdgeData*)calloc(1, sizeof(IRFlowchartEdgeData));
    if (!data) return NULL;

    data->from_id = from_id ? strdup(from_id) : NULL;
    data->to_id = to_id ? strdup(to_id) : NULL;
    data->type = IR_FLOWCHART_EDGE_ARROW;
    data->start_marker = IR_FLOWCHART_MARKER_NONE;
    data->end_marker = IR_FLOWCHART_MARKER_ARROW;

    return data;
}

void ir_flowchart_edge_data_destroy(IRFlowchartEdgeData* data) {
    if (!data) return;
    free(data->from_id);
    free(data->to_id);
    free(data->label);
    free(data->path_points);
    free(data);
}

IRFlowchartEdgeData* ir_get_flowchart_edge_data(IRComponent* c) {
    if (!c || c->type != IR_COMPONENT_FLOWCHART_EDGE) return NULL;
    return (IRFlowchartEdgeData*)c->custom_data;
}

void ir_flowchart_edge_set_label(IRFlowchartEdgeData* data, const char* label) {
    if (!data) return;
    free(data->label);
    data->label = label ? strdup(label) : NULL;
}

void ir_flowchart_edge_set_markers(IRFlowchartEdgeData* data, IRFlowchartMarker start, IRFlowchartMarker end) {
    if (!data) return;
    data->start_marker = start;
    data->end_marker = end;
}

// ============================================================================
// Subgraph Data Management
// ============================================================================

IRFlowchartSubgraphData* ir_flowchart_subgraph_data_create(const char* subgraph_id, const char* title) {
    IRFlowchartSubgraphData* data = (IRFlowchartSubgraphData*)calloc(1, sizeof(IRFlowchartSubgraphData));
    if (!data) return NULL;

    data->subgraph_id = subgraph_id ? strdup(subgraph_id) : NULL;
    data->title = title ? strdup(title) : NULL;
    data->direction = IR_FLOWCHART_DIR_TB;
    data->background_color = 0xF0F0F0FF;
    data->border_color = 0x000000FF;

    return data;
}

void ir_flowchart_subgraph_data_destroy(IRFlowchartSubgraphData* data) {
    if (!data) return;
    free(data->subgraph_id);
    free(data->title);
    free(data->parent_subgraph_id);
    free(data);
}

IRFlowchartSubgraphData* ir_get_flowchart_subgraph_data(IRComponent* c) {
    if (!c || c->type != IR_COMPONENT_FLOWCHART_SUBGRAPH) return NULL;
    return (IRFlowchartSubgraphData*)c->custom_data;
}

// ============================================================================
// Node Styling
// ============================================================================

void ir_flowchart_node_set_fill_color(IRFlowchartNodeData* data, uint32_t color) {
    if (!data) return;
    data->fill_color = color;
}

void ir_flowchart_node_set_stroke_color(IRFlowchartNodeData* data, uint32_t color) {
    if (!data) return;
    data->stroke_color = color;
}

void ir_flowchart_node_set_stroke_width(IRFlowchartNodeData* data, float width) {
    if (!data) return;
    data->stroke_width = width;
}

// ============================================================================
// Component Creation
// ============================================================================

IRComponent* ir_flowchart(IRFlowchartDirection direction) {
    IRComponent* comp = ir_create_component(IR_COMPONENT_FLOWCHART);
    if (!comp) return NULL;

    IRFlowchartState* state = ir_flowchart_create_state();
    if (!state) {
        ir_destroy_component(comp);
        return NULL;
    }

    state->direction = direction;
    comp->custom_data = (char*)state;
    return comp;
}

IRComponent* ir_flowchart_node(const char* node_id, IRFlowchartShape shape, const char* label) {
    IRComponent* comp = ir_create_component(IR_COMPONENT_FLOWCHART_NODE);
    if (!comp) return NULL;

    IRFlowchartNodeData* data = ir_flowchart_node_data_create(node_id, shape, label);
    if (!data) {
        ir_destroy_component(comp);
        return NULL;
    }

    comp->custom_data = (char*)data;
    return comp;
}

IRComponent* ir_flowchart_edge(const char* from_id, const char* to_id, IRFlowchartEdgeType type) {
    IRComponent* comp = ir_create_component(IR_COMPONENT_FLOWCHART_EDGE);
    if (!comp) return NULL;

    IRFlowchartEdgeData* data = ir_flowchart_edge_data_create(from_id, to_id);
    if (!data) {
        ir_destroy_component(comp);
        return NULL;
    }

    data->type = type;
    comp->custom_data = (char*)data;
    return comp;
}

IRComponent* ir_flowchart_subgraph(const char* subgraph_id, const char* title) {
    IRComponent* comp = ir_create_component(IR_COMPONENT_FLOWCHART_SUBGRAPH);
    if (!comp) return NULL;

    IRFlowchartSubgraphData* data = ir_flowchart_subgraph_data_create(subgraph_id, title);
    if (!data) {
        ir_destroy_component(comp);
        return NULL;
    }

    comp->custom_data = (char*)data;
    return comp;
}

IRComponent* ir_flowchart_label(const char* text) {
    IRComponent* comp = ir_create_component(IR_COMPONENT_FLOWCHART_LABEL);
    if (!comp) return NULL;

    if (text) {
        ir_add_child(comp, ir_text(text));
    }

    return comp;
}

// ============================================================================
// Registration Functions
// ============================================================================

void ir_flowchart_register_node(IRComponent* flowchart, IRComponent* node) {
    if (!flowchart || !node) return;

    IRFlowchartState* state = ir_get_flowchart_state(flowchart);
    IRFlowchartNodeData* node_data = ir_get_flowchart_node_data(node);
    if (!state || !node_data) return;

    // Expand array if needed
    if (state->node_count >= state->node_capacity) {
        uint32_t new_capacity = state->node_capacity == 0 ? 8 : state->node_capacity * 2;
        IRFlowchartNodeData** new_nodes = (IRFlowchartNodeData**)realloc(state->nodes, new_capacity * sizeof(IRFlowchartNodeData*));
        if (!new_nodes) return;
        state->nodes = new_nodes;
        state->node_capacity = new_capacity;
    }

    state->nodes[state->node_count++] = node_data;
}

void ir_flowchart_register_edge(IRComponent* flowchart, IRComponent* edge) {
    if (!flowchart || !edge) return;

    IRFlowchartState* state = ir_get_flowchart_state(flowchart);
    IRFlowchartEdgeData* edge_data = ir_get_flowchart_edge_data(edge);
    if (!state || !edge_data) return;

    // Expand array if needed
    if (state->edge_count >= state->edge_capacity) {
        uint32_t new_capacity = state->edge_capacity == 0 ? 8 : state->edge_capacity * 2;
        IRFlowchartEdgeData** new_edges = (IRFlowchartEdgeData**)realloc(state->edges, new_capacity * sizeof(IRFlowchartEdgeData*));
        if (!new_edges) return;
        state->edges = new_edges;
        state->edge_capacity = new_capacity;
    }

    state->edges[state->edge_count++] = edge_data;
}

void ir_flowchart_register_subgraph(IRComponent* flowchart, IRComponent* subgraph) {
    if (!flowchart || !subgraph) return;

    IRFlowchartState* state = ir_get_flowchart_state(flowchart);
    IRFlowchartSubgraphData* subgraph_data = ir_get_flowchart_subgraph_data(subgraph);
    if (!state || !subgraph_data) return;

    // Expand array if needed
    if (state->subgraph_count >= state->subgraph_capacity) {
        uint32_t new_capacity = state->subgraph_capacity == 0 ? 8 : state->subgraph_capacity * 2;
        IRFlowchartSubgraphData** new_subgraphs = (IRFlowchartSubgraphData**)realloc(state->subgraphs, new_capacity * sizeof(IRFlowchartSubgraphData*));
        if (!new_subgraphs) return;
        state->subgraphs = new_subgraphs;
        state->subgraph_capacity = new_capacity;
    }

    state->subgraphs[state->subgraph_count++] = subgraph_data;
}

// ============================================================================
// String Conversion Functions
// ============================================================================

static int ir_str_ieq(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

IRFlowchartDirection ir_flowchart_parse_direction(const char* str) {
    if (!str) return IR_FLOWCHART_DIR_TB;
    if (ir_str_ieq(str, "TB") || ir_str_ieq(str, "TD")) return IR_FLOWCHART_DIR_TB;
    if (ir_str_ieq(str, "LR")) return IR_FLOWCHART_DIR_LR;
    if (ir_str_ieq(str, "BT")) return IR_FLOWCHART_DIR_BT;
    if (ir_str_ieq(str, "RL")) return IR_FLOWCHART_DIR_RL;
    return IR_FLOWCHART_DIR_TB;
}

const char* ir_flowchart_direction_to_string(IRFlowchartDirection dir) {
    switch (dir) {
        case IR_FLOWCHART_DIR_TB: return "TB";
        case IR_FLOWCHART_DIR_LR: return "LR";
        case IR_FLOWCHART_DIR_BT: return "BT";
        case IR_FLOWCHART_DIR_RL: return "RL";
        default: return "TB";
    }
}

IRFlowchartShape ir_flowchart_parse_shape(const char* str) {
    if (!str) return IR_FLOWCHART_SHAPE_RECTANGLE;
    if (ir_str_ieq(str, "rectangle")) return IR_FLOWCHART_SHAPE_RECTANGLE;
    if (ir_str_ieq(str, "rounded")) return IR_FLOWCHART_SHAPE_ROUNDED;
    if (ir_str_ieq(str, "stadium")) return IR_FLOWCHART_SHAPE_STADIUM;
    if (ir_str_ieq(str, "diamond")) return IR_FLOWCHART_SHAPE_DIAMOND;
    if (ir_str_ieq(str, "circle")) return IR_FLOWCHART_SHAPE_CIRCLE;
    if (ir_str_ieq(str, "hexagon")) return IR_FLOWCHART_SHAPE_HEXAGON;
    if (ir_str_ieq(str, "parallelogram")) return IR_FLOWCHART_SHAPE_PARALLELOGRAM;
    if (ir_str_ieq(str, "cylinder")) return IR_FLOWCHART_SHAPE_CYLINDER;
    if (ir_str_ieq(str, "subroutine")) return IR_FLOWCHART_SHAPE_SUBROUTINE;
    if (ir_str_ieq(str, "asymmetric")) return IR_FLOWCHART_SHAPE_ASYMMETRIC;
    if (ir_str_ieq(str, "trapezoid")) return IR_FLOWCHART_SHAPE_TRAPEZOID;
    return IR_FLOWCHART_SHAPE_RECTANGLE;
}

const char* ir_flowchart_shape_to_string(IRFlowchartShape shape) {
    switch (shape) {
        case IR_FLOWCHART_SHAPE_RECTANGLE: return "rectangle";
        case IR_FLOWCHART_SHAPE_ROUNDED: return "rounded";
        case IR_FLOWCHART_SHAPE_STADIUM: return "stadium";
        case IR_FLOWCHART_SHAPE_DIAMOND: return "diamond";
        case IR_FLOWCHART_SHAPE_CIRCLE: return "circle";
        case IR_FLOWCHART_SHAPE_HEXAGON: return "hexagon";
        case IR_FLOWCHART_SHAPE_PARALLELOGRAM: return "parallelogram";
        case IR_FLOWCHART_SHAPE_CYLINDER: return "cylinder";
        case IR_FLOWCHART_SHAPE_SUBROUTINE: return "subroutine";
        case IR_FLOWCHART_SHAPE_ASYMMETRIC: return "asymmetric";
        case IR_FLOWCHART_SHAPE_TRAPEZOID: return "trapezoid";
        default: return "rectangle";
    }
}

IRFlowchartEdgeType ir_flowchart_parse_edge_type(const char* str) {
    if (!str) return IR_FLOWCHART_EDGE_ARROW;
    if (ir_str_ieq(str, "arrow")) return IR_FLOWCHART_EDGE_ARROW;
    if (ir_str_ieq(str, "open")) return IR_FLOWCHART_EDGE_OPEN;
    if (ir_str_ieq(str, "bidirectional")) return IR_FLOWCHART_EDGE_BIDIRECTIONAL;
    if (ir_str_ieq(str, "dotted")) return IR_FLOWCHART_EDGE_DOTTED;
    if (ir_str_ieq(str, "thick")) return IR_FLOWCHART_EDGE_THICK;
    return IR_FLOWCHART_EDGE_ARROW;
}

const char* ir_flowchart_edge_type_to_string(IRFlowchartEdgeType type) {
    switch (type) {
        case IR_FLOWCHART_EDGE_ARROW: return "arrow";
        case IR_FLOWCHART_EDGE_OPEN: return "open";
        case IR_FLOWCHART_EDGE_BIDIRECTIONAL: return "bidirectional";
        case IR_FLOWCHART_EDGE_DOTTED: return "dotted";
        case IR_FLOWCHART_EDGE_THICK: return "thick";
        default: return "arrow";
    }
}

IRFlowchartMarker ir_flowchart_parse_marker(const char* str) {
    if (!str) return IR_FLOWCHART_MARKER_NONE;
    if (ir_str_ieq(str, "none")) return IR_FLOWCHART_MARKER_NONE;
    if (ir_str_ieq(str, "arrow")) return IR_FLOWCHART_MARKER_ARROW;
    if (ir_str_ieq(str, "circle")) return IR_FLOWCHART_MARKER_CIRCLE;
    if (ir_str_ieq(str, "cross")) return IR_FLOWCHART_MARKER_CROSS;
    return IR_FLOWCHART_MARKER_NONE;
}

const char* ir_flowchart_marker_to_string(IRFlowchartMarker marker) {
    switch (marker) {
        case IR_FLOWCHART_MARKER_NONE: return "none";
        case IR_FLOWCHART_MARKER_ARROW: return "arrow";
        case IR_FLOWCHART_MARKER_CIRCLE: return "circle";
        case IR_FLOWCHART_MARKER_CROSS: return "cross";
        default: return "none";
    }
}

// ============================================================================
// Lookup Functions
// ============================================================================

IRFlowchartNodeData* ir_flowchart_find_node(IRFlowchartState* state, const char* node_id) {
    if (!state || !node_id) return NULL;

    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (node && node->node_id && strcmp(node->node_id, node_id) == 0) {
            return node;
        }
    }

    return NULL;
}

// ============================================================================
// Finalization
// ============================================================================

void ir_flowchart_finalize(IRComponent* flowchart) {
    if (!flowchart || flowchart->type != IR_COMPONENT_FLOWCHART) return;

    IRFlowchartState* state = ir_get_flowchart_state(flowchart);
    if (!state) return;

    // Register all children nodes, edges, and subgraphs
    for (uint32_t i = 0; i < flowchart->child_count; i++) {
        IRComponent* child = flowchart->children[i];
        if (!child) continue;

        if (child->type == IR_COMPONENT_FLOWCHART_NODE) {
            IRFlowchartNodeData* node_data = ir_get_flowchart_node_data(child);
            if (node_data) {
                ir_flowchart_register_node(flowchart, child);
            }
        } else if (child->type == IR_COMPONENT_FLOWCHART_EDGE) {
            IRFlowchartEdgeData* edge_data = ir_get_flowchart_edge_data(child);
            if (edge_data) {
                ir_flowchart_register_edge(flowchart, child);
            }
        } else if (child->type == IR_COMPONENT_FLOWCHART_SUBGRAPH) {
            IRFlowchartSubgraphData* subgraph_data = ir_get_flowchart_subgraph_data(child);
            if (subgraph_data) {
                ir_flowchart_register_subgraph(flowchart, child);
            }
        }
    }

    // Mark layout as not computed
    state->layout_computed = false;
}
