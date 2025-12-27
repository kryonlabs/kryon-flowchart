#ifndef FLOWCHART_BUILDER_H
#define FLOWCHART_BUILDER_H

#include "flowchart_types.h"
#include "ir_core.h"

// Builder API for flowchart components
// Implementations are in flowchart_builder.c

// State management
extern IRFlowchartState* ir_flowchart_create_state(void);
extern void ir_flowchart_destroy_state(IRFlowchartState* state);
extern IRFlowchartState* ir_get_flowchart_state(IRComponent* c);

// Node/Edge/Subgraph data management
extern IRFlowchartNodeData* ir_flowchart_node_data_create(const char* node_id, IRFlowchartShape shape, const char* label);
extern void ir_flowchart_node_data_destroy(IRFlowchartNodeData* data);
extern IRFlowchartNodeData* ir_get_flowchart_node_data(IRComponent* c);

extern IRFlowchartEdgeData* ir_flowchart_edge_data_create(const char* from_id, const char* to_id);
extern void ir_flowchart_edge_data_destroy(IRFlowchartEdgeData* data);
extern IRFlowchartEdgeData* ir_get_flowchart_edge_data(IRComponent* c);

extern IRFlowchartSubgraphData* ir_flowchart_subgraph_data_create(const char* subgraph_id, const char* title);
extern void ir_flowchart_subgraph_data_destroy(IRFlowchartSubgraphData* data);
extern IRFlowchartSubgraphData* ir_get_flowchart_subgraph_data(IRComponent* c);

// Component creation
extern IRComponent* ir_flowchart(IRFlowchartDirection direction);
extern IRComponent* ir_flowchart_node(const char* node_id, IRFlowchartShape shape, const char* label);
extern IRComponent* ir_flowchart_edge(const char* from_id, const char* to_id, IRFlowchartEdgeType type);
extern IRComponent* ir_flowchart_subgraph(const char* subgraph_id, const char* title);
extern IRComponent* ir_flowchart_label(const char* text);

// Edge/Node styling
extern void ir_flowchart_edge_set_label(IRFlowchartEdgeData* data, const char* label);
extern void ir_flowchart_edge_set_markers(IRFlowchartEdgeData* data, IRFlowchartMarker start, IRFlowchartMarker end);
extern void ir_flowchart_node_set_fill_color(IRFlowchartNodeData* data, uint32_t color);
extern void ir_flowchart_node_set_stroke_color(IRFlowchartNodeData* data, uint32_t color);
extern void ir_flowchart_node_set_stroke_width(IRFlowchartNodeData* data, float width);

// Registration
extern void ir_flowchart_register_node(IRComponent* flowchart, IRComponent* node);
extern void ir_flowchart_register_edge(IRComponent* flowchart, IRComponent* edge);
extern void ir_flowchart_register_subgraph(IRComponent* flowchart, IRComponent* subgraph);

// String helpers
extern IRFlowchartDirection ir_flowchart_parse_direction(const char* str);
extern const char* ir_flowchart_direction_to_string(IRFlowchartDirection dir);
extern IRFlowchartShape ir_flowchart_parse_shape(const char* str);
extern const char* ir_flowchart_shape_to_string(IRFlowchartShape shape);
extern IRFlowchartEdgeType ir_flowchart_parse_edge_type(const char* str);
extern const char* ir_flowchart_edge_type_to_string(IRFlowchartEdgeType type);
extern IRFlowchartMarker ir_flowchart_parse_marker(const char* str);
extern const char* ir_flowchart_marker_to_string(IRFlowchartMarker marker);

// Lookup
extern IRFlowchartNodeData* ir_flowchart_find_node(IRFlowchartState* state, const char* node_id);

// Finalization
extern void ir_flowchart_finalize(IRComponent* flowchart);

// Layout (from flowchart_layout.c in plugin)
void ir_layout_compute_flowchart(IRComponent* flowchart, float available_width, float available_height);

#endif // FLOWCHART_BUILDER_H
