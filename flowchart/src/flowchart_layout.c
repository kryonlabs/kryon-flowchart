// FLOWCHART LAYOUT
// ============================================================================

#include "flowchart_types.h"
#include "flowchart_builder.h"
#include "ir_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// External functions from ir_core (text width estimation and layout dispatch)
extern float ir_get_text_width_estimate(const char* text, float font_size);
extern void ir_layout_dispatch(IRComponent* c, IRLayoutConstraints constraints, float parent_x, float parent_y);

// Default layout parameters
#define FLOWCHART_NODE_MIN_WIDTH 40.0f
#define FLOWCHART_NODE_MIN_HEIGHT 24.0f
#define FLOWCHART_NODE_PADDING 15.0f
#define FLOWCHART_NODE_SPACING 20.0f
#define FLOWCHART_RANK_SPACING 40.0f
#define FLOWCHART_SUBGRAPH_PADDING 40.0f
#define FLOWCHART_SUBGRAPH_TITLE_HEIGHT 30.0f

// Compute bounding boxes for subgraphs based on their contained nodes
static void compute_subgraph_bounds(IRFlowchartState* fc_state) {
    if (!fc_state) return;

    // Iterate over all subgraphs
    for (uint32_t i = 0; i < fc_state->subgraph_count; i++) {
        IRFlowchartSubgraphData* sg_data = fc_state->subgraphs[i];
        if (!sg_data || !sg_data->subgraph_id) continue;

        // Find all nodes that belong to this subgraph
        float min_x = INFINITY, min_y = INFINITY;
        float max_x = -INFINITY, max_y = -INFINITY;
        bool has_nodes = false;

        for (uint32_t j = 0; j < fc_state->node_count; j++) {
            IRFlowchartNodeData* node_data = fc_state->nodes[j];

            // Check if this node belongs to the current subgraph
            if (node_data && node_data->subgraph_id &&
                strcmp(node_data->subgraph_id, sg_data->subgraph_id) == 0) {

                has_nodes = true;

                // Update bounding box
                float node_left = node_data->x;
                float node_top = node_data->y;
                float node_right = node_data->x + node_data->width;
                float node_bottom = node_data->y + node_data->height;

                if (node_left < min_x) min_x = node_left;
                if (node_top < min_y) min_y = node_top;
                if (node_right > max_x) max_x = node_right;
                if (node_bottom > max_y) max_y = node_bottom;
            }
        }

        if (has_nodes) {
            // Add padding around nodes for subgraph box
            sg_data->x = min_x - FLOWCHART_SUBGRAPH_PADDING;
            sg_data->y = min_y - FLOWCHART_SUBGRAPH_PADDING - FLOWCHART_SUBGRAPH_TITLE_HEIGHT;
            sg_data->width = (max_x - min_x) + (FLOWCHART_SUBGRAPH_PADDING * 2);
            sg_data->height = (max_y - min_y) + (FLOWCHART_SUBGRAPH_PADDING * 2) + FLOWCHART_SUBGRAPH_TITLE_HEIGHT;

            #ifdef KRYON_TRACE_LAYOUT
            fprintf(stderr, "  📦 Subgraph '%s' bounds: x=%.1f y=%.1f w=%.1f h=%.1f\n",
                   sg_data->subgraph_id, sg_data->x, sg_data->y,
                   sg_data->width, sg_data->height);
            #endif
        }
    }
}

// Helper: Transform coordinates recursively for nested subgraphs
static void transform_subgraph_coordinates(IRComponent* subgraph, float offset_x, float offset_y, IRFlowchartState* state) {
    if (!subgraph || !state) return;

    IRFlowchartSubgraphData* sg_data = ir_get_flowchart_subgraph_data(subgraph);
    if (!sg_data || !sg_data->subgraph_id) return;

    // Transform all nodes in this subgraph
    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (node && node->subgraph_id && strcmp(node->subgraph_id, sg_data->subgraph_id) == 0) {
            node->x += offset_x;
            node->y += offset_y;
        }
    }

    // Transform edges (if they belong to this subgraph)
    for (uint32_t i = 0; i < state->edge_count; i++) {
        IRFlowchartEdgeData* edge = state->edges[i];
        if (edge && edge->path_points) {
            // Check if edge connects nodes in this subgraph
            bool from_in_subgraph = false;
            bool to_in_subgraph = false;

            for (uint32_t j = 0; j < state->node_count; j++) {
                IRFlowchartNodeData* node = state->nodes[j];
                if (node && node->subgraph_id && strcmp(node->subgraph_id, sg_data->subgraph_id) == 0) {
                    if (edge->from_id && node->node_id && strcmp(node->node_id, edge->from_id) == 0) {
                        from_in_subgraph = true;
                    }
                    if (edge->to_id && node->node_id && strcmp(node->node_id, edge->to_id) == 0) {
                        to_in_subgraph = true;
                    }
                }
            }

            // Transform edge if both endpoints are in this subgraph
            if (from_in_subgraph && to_in_subgraph) {
                for (uint32_t p = 0; p < edge->path_point_count; p++) {
                    edge->path_points[p * 2] += offset_x;
                    edge->path_points[p * 2 + 1] += offset_y;
                }
            }
        }
    }

    // Update subgraph bounds
    sg_data->x += offset_x;
    sg_data->y += offset_y;

    // Recursively transform nested subgraphs
    for (uint32_t i = 0; i < subgraph->child_count; i++) {
        IRComponent* child = subgraph->children[i];
        if (child && child->type == IR_COMPONENT_FLOWCHART_SUBGRAPH) {
            transform_subgraph_coordinates(child, offset_x, offset_y, state);
        }
    }
}

// Helper: Compute node sizes based on labels and shapes
static void compute_flowchart_node_sizes(IRFlowchartState* state, float font_size) {
    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (!node) continue;

        float label_width = 50.0f;
        float label_height = font_size * 1.2f;

        if (node->label && node->label[0] != '\0') {
            if (g_ir_font_metrics && g_ir_font_metrics->get_text_width) {
                size_t len = strlen(node->label);
                label_width = g_ir_font_metrics->get_text_width(node->label, (uint32_t)len, font_size, NULL);
                label_height = g_ir_font_metrics->get_font_height(font_size, NULL);
            } else {
                // No font metrics - use estimate
                label_width = strlen(node->label) * font_size * 0.6f;
            }
        }

        // Add padding around text
        float h_padding = 32.0f;  // 16px on each side
        float v_padding = 20.0f;  // 10px on each side

        // Extra padding for non-rectangular shapes (text area is smaller)
        if (node->shape == IR_FLOWCHART_SHAPE_DIAMOND) {
            h_padding *= 2.0f;  // Diamond needs ~2x because text area is diagonal
            v_padding *= 2.0f;
        } else if (node->shape == IR_FLOWCHART_SHAPE_CIRCLE ||
                   node->shape == IR_FLOWCHART_SHAPE_HEXAGON) {
            h_padding *= 1.5f;
            v_padding *= 1.5f;
        }

        node->width = fmaxf(FLOWCHART_NODE_MIN_WIDTH, label_width + h_padding);
        node->height = fmaxf(FLOWCHART_NODE_MIN_HEIGHT, label_height + v_padding);

        // Make circles and diamonds square
        if (node->shape == IR_FLOWCHART_SHAPE_CIRCLE ||
            node->shape == IR_FLOWCHART_SHAPE_DIAMOND) {
            float size = fmaxf(node->width, node->height);
            node->width = size;
            node->height = size;
        }
    }
}

// Helper: Layout nodes belonging to a specific subgraph (or top-level if subgraph_id is NULL)
// Returns the computed size of the subgraph in local coordinates
static void layout_subgraph_nodes(IRFlowchartState* state, const char* subgraph_id,
                                   IRFlowchartDirection direction, float node_spacing,
                                   float rank_spacing, float* out_width, float* out_height) {
    if (!state) return;

    // Filter nodes that belong to this subgraph
    uint32_t* subgraph_node_indices = (uint32_t*)malloc(state->node_count * sizeof(uint32_t));
    uint32_t subgraph_node_count = 0;

    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (!node) continue;

        // Check if node belongs to this subgraph
        bool belongs = false;
        if (subgraph_id == NULL) {
            // Top-level: nodes with no subgraph_id
            belongs = (node->subgraph_id == NULL);
        } else {
            // Specific subgraph: nodes with matching subgraph_id
            belongs = (node->subgraph_id && strcmp(node->subgraph_id, subgraph_id) == 0);
        }

        if (belongs) {
            subgraph_node_indices[subgraph_node_count++] = i;
        }
    }

    if (subgraph_node_count == 0) {
        free(subgraph_node_indices);
        if (out_width) *out_width = 0;
        if (out_height) *out_height = 0;
        return;
    }

    // TODO: Implement layering and positioning for subgraph nodes
    // For now, just free and return
    free(subgraph_node_indices);
    if (out_width) *out_width = 0;
    if (out_height) *out_height = 0;
}

// Simple layered layout for flowcharts
// Uses topological sort to assign layers, then positions nodes within layers
void ir_layout_compute_flowchart(IRComponent* flowchart, float available_width, float available_height) {
    if (!flowchart || flowchart->type != IR_COMPONENT_FLOWCHART) return;

    IRFlowchartState* state = ir_get_flowchart_state(flowchart);
    if (!state) return;

    // Check if layout is already computed and dimensions match
    if (state->layout_computed &&
        state->computed_width == available_width &&
        state->computed_height == available_height) {
        return;
    }

    #ifdef KRYON_TRACE_LAYOUT
    fprintf(stderr, "🔀 FLOWCHART_LAYOUT: %u nodes, %u edges, dir=%s\n",
            state->node_count, state->edge_count,
            ir_flowchart_direction_to_string(state->direction));
    #endif

    if (state->node_count == 0) {
        state->layout_computed = true;
        state->computed_width = available_width;
        state->computed_height = available_height;
        return;
    }

    // Use layout parameters from state or defaults
    float node_spacing = state->node_spacing > 0 ? state->node_spacing : FLOWCHART_NODE_SPACING;
    float rank_spacing = state->rank_spacing > 0 ? state->rank_spacing : FLOWCHART_RANK_SPACING;

    // Phase 1: Compute node sizes based on labels
    // Use flowchart's font size if specified, otherwise default to 14
    float font_size = (flowchart->style && flowchart->style->font.size > 0)
                      ? flowchart->style->font.size : 14.0f;
    compute_flowchart_node_sizes(state, font_size);

    // Check if any subgraphs have different directions than parent
    bool has_directional_subgraphs = false;
    for (uint32_t i = 0; i < state->subgraph_count; i++) {
        IRFlowchartSubgraphData* sg = state->subgraphs[i];
        if (sg && sg->direction != state->direction) {
            has_directional_subgraphs = true;
            #ifdef KRYON_TRACE_LAYOUT
            fprintf(stderr, "  📐 Subgraph '%s' has direction %s (parent: %s)\n",
                    sg->subgraph_id ? sg->subgraph_id : "?",
                    ir_flowchart_direction_to_string(sg->direction),
                    ir_flowchart_direction_to_string(state->direction));
            #endif
            break;
        }
    }

    #ifdef KRYON_TRACE_LAYOUT
    if (has_directional_subgraphs) {
        fprintf(stderr, "  🔀 Detected subgraphs with independent directions\n");
    }
    #endif

    // Phase 2: Assign nodes to layers using longest-path algorithm
    // This properly handles cycles by only considering forward edges

    // Create layer assignment (-1 = unassigned)
    int* node_layer = calloc(state->node_count, sizeof(int));
    bool* processed = calloc(state->node_count, sizeof(bool));
    if (!node_layer || !processed) {
        free(node_layer);
        free(processed);
        return;
    }

    // Initialize all layers to -1
    for (uint32_t i = 0; i < state->node_count; i++) {
        node_layer[i] = -1;
    }

    // Find nodes with no incoming edges (start nodes)
    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (!node || !node->node_id) continue;

        bool has_incoming = false;
        for (uint32_t e = 0; e < state->edge_count; e++) {
            IRFlowchartEdgeData* edge = state->edges[e];
            if (edge && edge->to_id && strcmp(edge->to_id, node->node_id) == 0) {
                has_incoming = true;
                break;
            }
        }

        if (!has_incoming) {
            node_layer[i] = 0;
            processed[i] = true;
        }
    }

    // If no start nodes found, pick the first node
    bool any_assigned = false;
    for (uint32_t i = 0; i < state->node_count; i++) {
        if (node_layer[i] >= 0) {
            any_assigned = true;
            break;
        }
    }
    if (!any_assigned && state->node_count > 0) {
        node_layer[0] = 0;
        processed[0] = true;
    }

    // Iterate until all nodes are assigned
    // Each node's layer = 1 + max(layer of predecessors that are already assigned)
    int max_iterations = state->node_count * 2;  // Prevent infinite loops
    for (int iter = 0; iter < max_iterations; iter++) {
        bool made_progress = false;

        for (uint32_t i = 0; i < state->node_count; i++) {
            if (processed[i]) continue;

            IRFlowchartNodeData* node = state->nodes[i];
            if (!node || !node->node_id) continue;

            // Find max layer of all predecessors that have been assigned
            int max_pred_layer = -1;
            bool has_assigned_pred = false;

            for (uint32_t e = 0; e < state->edge_count; e++) {
                IRFlowchartEdgeData* edge = state->edges[e];
                if (!edge || !edge->to_id || strcmp(edge->to_id, node->node_id) != 0) continue;

                // Find the source node
                for (uint32_t j = 0; j < state->node_count; j++) {
                    if (state->nodes[j] && state->nodes[j]->node_id &&
                        edge->from_id && strcmp(state->nodes[j]->node_id, edge->from_id) == 0) {
                        if (node_layer[j] >= 0) {
                            has_assigned_pred = true;
                            if (node_layer[j] > max_pred_layer) {
                                max_pred_layer = node_layer[j];
                            }
                        }
                        break;
                    }
                }
            }

            // If we have at least one assigned predecessor, assign this node
            if (has_assigned_pred) {
                node_layer[i] = max_pred_layer + 1;
                processed[i] = true;
                made_progress = true;
            }
        }

        // If no progress and still have unassigned nodes, pick one
        if (!made_progress) {
            for (uint32_t i = 0; i < state->node_count; i++) {
                if (!processed[i]) {
                    node_layer[i] = 0;  // Assign to layer 0
                    processed[i] = true;
                    made_progress = true;
                    break;
                }
            }
        }

        // Check if all nodes are assigned
        bool all_done = true;
        for (uint32_t i = 0; i < state->node_count; i++) {
            if (!processed[i]) {
                all_done = false;
                break;
            }
        }
        if (all_done) break;
    }

    // Find max layer
    int max_layer = 0;
    for (uint32_t i = 0; i < state->node_count; i++) {
        if (node_layer[i] > max_layer) {
            max_layer = node_layer[i];
        }
    }

    // Phase 3: Count nodes per layer and find max
    int* nodes_per_layer = calloc(max_layer + 1, sizeof(int));
    if (!nodes_per_layer) {
        free(node_layer);
        free(processed);
        return;
    }

    for (uint32_t i = 0; i < state->node_count; i++) {
        nodes_per_layer[node_layer[i]]++;
    }

    int max_nodes_in_layer = 0;
    for (int l = 0; l <= max_layer; l++) {
        if (nodes_per_layer[l] > max_nodes_in_layer) {
            max_nodes_in_layer = nodes_per_layer[l];
        }
    }

    // Phase 4: Position nodes
    // Track position within each layer
    int* layer_position = calloc(max_layer + 1, sizeof(int));
    if (!layer_position) {
        free(node_layer);
        free(processed);
        free(nodes_per_layer);
        return;
    }

    // Find max node dimensions for spacing
    float max_node_width = FLOWCHART_NODE_MIN_WIDTH;
    float max_node_height = FLOWCHART_NODE_MIN_HEIGHT;
    for (uint32_t i = 0; i < state->node_count; i++) {
        if (state->nodes[i]) {
            max_node_width = fmaxf(max_node_width, state->nodes[i]->width);
            max_node_height = fmaxf(max_node_height, state->nodes[i]->height);
        }
    }

    // Assign positions based on direction
    bool horizontal = (state->direction == IR_FLOWCHART_DIR_LR ||
                       state->direction == IR_FLOWCHART_DIR_RL);
    bool reversed = (state->direction == IR_FLOWCHART_DIR_BT ||
                     state->direction == IR_FLOWCHART_DIR_RL);

    float total_primary_size = 0;
    float total_secondary_size = 0;

    for (uint32_t i = 0; i < state->node_count; i++) {
        IRFlowchartNodeData* node = state->nodes[i];
        if (!node) continue;

        int layer = node_layer[i];

        // For nodes in subgraphs with different directions, track position separately
        bool node_in_directional_subgraph = false;
        if (has_directional_subgraphs && node->subgraph_id) {
            for (uint32_t sg_idx = 0; sg_idx < state->subgraph_count; sg_idx++) {
                IRFlowchartSubgraphData* sg = state->subgraphs[sg_idx];
                if (sg && sg->subgraph_id && strcmp(node->subgraph_id, sg->subgraph_id) == 0) {
                    if (sg->direction != state->direction) {
                        node_in_directional_subgraph = true;
                        break;
                    }
                }
            }
        }

        int pos;
        if (node_in_directional_subgraph) {
            // For directional subgraphs, calculate position within subgraph context
            pos = 0;  // Count nodes in same (layer, subgraph) before this one
            for (uint32_t j = 0; j < i; j++) {
                if (node_layer[j] == layer) {
                    IRFlowchartNodeData* other = state->nodes[j];
                    if (other && other->subgraph_id && node->subgraph_id &&
                        strcmp(other->subgraph_id, node->subgraph_id) == 0) {
                        pos++;
                    }
                }
            }
        } else {
            // Global position for top-level nodes
            pos = layer_position[layer]++;
        }

        // Calculate center position within layer
        // Count only nodes in the same subgraph (or top-level) for proper centering
        int nodes_in_this_layer = 0;
        for (uint32_t j = 0; j < state->node_count; j++) {
            if (node_layer[j] == layer) {
                IRFlowchartNodeData* other = state->nodes[j];
                if (!other) continue;

                // Check if both nodes are in the same subgraph
                bool same_subgraph = false;
                if (node->subgraph_id == NULL && other->subgraph_id == NULL) {
                    same_subgraph = true;  // Both top-level
                } else if (node->subgraph_id && other->subgraph_id &&
                           strcmp(node->subgraph_id, other->subgraph_id) == 0) {
                    same_subgraph = true;  // Both in same subgraph
                }

                if (same_subgraph) {
                    nodes_in_this_layer++;
                }
            }
        }

        #ifdef KRYON_TRACE_LAYOUT
        if (has_directional_subgraphs && node->subgraph_id) {
            fprintf(stderr, "    [DEBUG] Node '%s' L%d P%d: nodes_in_this_layer=%d (subgraph: %s)\n",
                    node->node_id ? node->node_id : "?", layer, pos,
                    nodes_in_this_layer, node->subgraph_id);
        }
        #endif

        // Check if node belongs to a subgraph with different direction
        IRFlowchartDirection node_direction = state->direction;
        bool node_horizontal = horizontal;
        bool node_reversed = reversed;

        if (has_directional_subgraphs && node->subgraph_id) {
            for (uint32_t sg_idx = 0; sg_idx < state->subgraph_count; sg_idx++) {
                IRFlowchartSubgraphData* sg = state->subgraphs[sg_idx];
                if (sg && sg->subgraph_id && strcmp(node->subgraph_id, sg->subgraph_id) == 0) {
                    if (sg->direction != state->direction) {  // Use subgraph's direction if different from parent
                        node_direction = sg->direction;
                        node_horizontal = (node_direction == IR_FLOWCHART_DIR_LR ||
                                          node_direction == IR_FLOWCHART_DIR_RL);
                        node_reversed = (node_direction == IR_FLOWCHART_DIR_BT ||
                                        node_direction == IR_FLOWCHART_DIR_RL);
                        #ifdef KRYON_TRACE_LAYOUT
                        fprintf(stderr, "    → Node '%s' in subgraph '%s' using direction: %s\n",
                                node->node_id ? node->node_id : "?", sg->subgraph_id,
                                ir_flowchart_direction_to_string(node_direction));
                        #endif
                    }
                    break;
                }
            }
        }

        // Calculate centering offset for nodes within this layer
        float layer_start;
        if (node_horizontal) {
            // LR/RL: nodes in layer arranged vertically (use height for spacing)
            layer_start = (max_nodes_in_layer - nodes_in_this_layer) * (max_node_height + node_spacing) / 2.0f;
        } else {
            // TB/BT: nodes in layer arranged horizontally (use width for spacing)
            layer_start = (max_nodes_in_layer - nodes_in_this_layer) * (max_node_width + node_spacing) / 2.0f;
        }

        float primary_coord, secondary_coord;

        if (node_horizontal) {
            // LR/RL: layers are columns, positions are rows
            primary_coord = layer * (max_node_width + rank_spacing);
            secondary_coord = layer_start + pos * (max_node_height + node_spacing);

            if (node_reversed) {
                primary_coord = (max_layer - layer) * (max_node_width + rank_spacing);
            }

            node->x = primary_coord + (max_node_width - node->width) / 2.0f;
            node->y = secondary_coord + (max_node_height - node->height) / 2.0f;
        } else {
            // TB/BT: layers are rows, positions are columns
            primary_coord = layer * (max_node_height + rank_spacing);
            secondary_coord = layer_start + pos * (max_node_width + node_spacing);

            if (node_reversed) {
                primary_coord = (max_layer - layer) * (max_node_height + rank_spacing);
            }

            node->x = secondary_coord + (max_node_width - node->width) / 2.0f;
            node->y = primary_coord + (max_node_height - node->height) / 2.0f;
        }

        // Track total size
        total_primary_size = fmaxf(total_primary_size,
            horizontal ? (node->x + node->width) : (node->y + node->height));
        total_secondary_size = fmaxf(total_secondary_size,
            horizontal ? (node->y + node->height) : (node->x + node->width));

        #ifdef KRYON_TRACE_LAYOUT
        fprintf(stderr, "  Node '%s' L%d P%d: (%.1f, %.1f) %.1fx%.1f\n",
                node->node_id ? node->node_id : "?", layer, pos,
                node->x, node->y, node->width, node->height);
        #endif
    }

    // Phase 5: Route edges (simple straight-line for now)
    for (uint32_t i = 0; i < state->edge_count; i++) {
        IRFlowchartEdgeData* edge = state->edges[i];
        if (!edge) continue;

        // Find source and target nodes
        IRFlowchartNodeData* from_node = NULL;
        IRFlowchartNodeData* to_node = NULL;

        for (uint32_t j = 0; j < state->node_count; j++) {
            if (state->nodes[j] && state->nodes[j]->node_id) {
                if (edge->from_id && strcmp(state->nodes[j]->node_id, edge->from_id) == 0) {
                    from_node = state->nodes[j];
                }
                if (edge->to_id && strcmp(state->nodes[j]->node_id, edge->to_id) == 0) {
                    to_node = state->nodes[j];
                }
            }
        }

        if (from_node && to_node) {
            // Free existing path points
            free(edge->path_points);

            // Create simple 2-point path (straight line from center to center)
            edge->path_point_count = 2;
            edge->path_points = malloc(4 * sizeof(float));  // 2 points x 2 coords
            if (edge->path_points) {
                // From center
                edge->path_points[0] = from_node->x + from_node->width / 2;
                edge->path_points[1] = from_node->y + from_node->height / 2;
                // To center
                edge->path_points[2] = to_node->x + to_node->width / 2;
                edge->path_points[3] = to_node->y + to_node->height / 2;

                #ifdef KRYON_TRACE_LAYOUT
                fprintf(stderr, "  Edge '%s'->'%s': (%.1f,%.1f) -> (%.1f,%.1f)\n",
                        edge->from_id ? edge->from_id : "?",
                        edge->to_id ? edge->to_id : "?",
                        edge->path_points[0], edge->path_points[1],
                        edge->path_points[2], edge->path_points[3]);
                #endif
            }
        }
    }

    // Cleanup
    free(node_layer);
    free(processed);
    free(nodes_per_layer);
    free(layer_position);

    // Calculate natural size
    float natural_width = horizontal ? total_primary_size : total_secondary_size;
    float natural_height = horizontal ? total_secondary_size : total_primary_size;

    // Add padding
    float padding = 20.0f;
    natural_width += padding * 2;
    natural_height += padding * 2;

    // Store natural dimensions (before scaling) for intrinsic sizing
    state->natural_width = natural_width;
    state->natural_height = natural_height;

    // Scale to fit within available space if needed
    float scale_x = 1.0f;
    float scale_y = 1.0f;

    if (natural_width > available_width && available_width > 0) {
        scale_x = (available_width - padding * 2) / (natural_width - padding * 2);
    }
    if (natural_height > available_height && available_height > 0) {
        scale_y = (available_height - padding * 2) / (natural_height - padding * 2);
    }

    // Use uniform scaling to preserve aspect ratio
    float scale = fminf(scale_x, scale_y);

    // Limit minimum scale to keep nodes readable (at least 0.6)
    // Below this, text becomes unreadable
    float min_scale = 0.6f;
    if (scale < min_scale) {
        scale = min_scale;
    }

    // Apply scaling to node POSITIONS only (not dimensions!)
    // Node dimensions must stay the same size as the text they contain
    // Only positions scale to fit within available space
    if (scale < 1.0f) {
        for (uint32_t i = 0; i < state->node_count; i++) {
            IRFlowchartNodeData* node = state->nodes[i];
            if (!node) continue;

            // Scale position only, NOT width/height
            // Text rendering uses node dimensions directly
            node->x = padding + (node->x) * scale;
            node->y = padding + (node->y) * scale;
            // DO NOT scale: node->width *= scale;
            // DO NOT scale: node->height *= scale;
        }

        // Also scale edge path points
        for (uint32_t i = 0; i < state->edge_count; i++) {
            IRFlowchartEdgeData* edge = state->edges[i];
            if (!edge || !edge->path_points) continue;

            for (uint32_t p = 0; p < edge->path_point_count; p++) {
                edge->path_points[p * 2] = padding + edge->path_points[p * 2] * scale;
                edge->path_points[p * 2 + 1] = padding + edge->path_points[p * 2 + 1] * scale;
            }
        }

        // Update computed size
        natural_width = (natural_width - padding * 2) * scale + padding * 2;
        natural_height = (natural_height - padding * 2) * scale + padding * 2;
    } else {
        // Just add padding offset
        for (uint32_t i = 0; i < state->node_count; i++) {
            IRFlowchartNodeData* node = state->nodes[i];
            if (!node) continue;
            node->x += padding;
            node->y += padding;
        }

        for (uint32_t i = 0; i < state->edge_count; i++) {
            IRFlowchartEdgeData* edge = state->edges[i];
            if (!edge || !edge->path_points) continue;

            for (uint32_t p = 0; p < edge->path_point_count; p++) {
                edge->path_points[p * 2] += padding;
                edge->path_points[p * 2 + 1] += padding;
            }
        }
    }

    // Compute subgraph bounding boxes after node positions are finalized
    compute_subgraph_bounds(state);

    // NOTE: Do NOT overwrite flowchart->rendered_bounds here!
    // The parent container (Column/Row) sets the flowchart's bounds based on
    // its width/height style properties. The flowchart layout just positions
    // nodes within those bounds.

    // Mark layout as computed
    state->layout_computed = true;
    state->computed_width = available_width;
    state->computed_height = available_height;

    // Use natural dimensions (already computed in Phase 4)
    // These include layer spacing, not just node bounding box
    // NOTE: SVG generator will add its own padding, so we remove the padding here
    // to avoid double-padding
    if (state->node_count > 0) {
        const float PADDING = 20.0f;
        state->content_width = state->natural_width - (PADDING * 2);
        state->content_height = state->natural_height - (PADDING * 2);
        state->content_offset_x = 0.0f;
        state->content_offset_y = 0.0f;
    } else {
        // Empty flowchart - use minimum dimensions
        state->content_width = 100.0f;
        state->content_height = 100.0f;
        state->content_offset_x = 0.0f;
        state->content_offset_y = 0.0f;
    }

    #ifdef KRYON_TRACE_LAYOUT
    fprintf(stderr, "🔀 FLOWCHART_LAYOUT done: size=%.1fx%.1f\n",
            flowchart->rendered_bounds.width, flowchart->rendered_bounds.height);
    #endif
}

// ============================================================================
// Two-Pass Layout System - Clay-Inspired Architecture
// ============================================================================

// ============================================================================
// Phase 1: Bottom-Up Intrinsic Sizing (Content-Based)
// ============================================================================

/**
 * Compute intrinsic size for Text components
 * Uses backend text measurement callback if available, otherwise fallback heuristic
 */

// ============================================================================
// Single-Pass Recursive Layout System (NEW)
// ============================================================================

/**
 * Single-pass recursive layout algorithm
 *
 * Computes final dimensions and positions in a single bottom-up post-order traversal.
 * Each component fully resolves its layout before returning to parent, eliminating
 * timing issues where parents read stale child dimensions.
 *
 * Algorithm:
 * 1. Resolve own width/height (or mark AUTO)
 * 2. For each child:
 *    - Recursively call ir_layout_single_pass (child computes FINAL dimensions)
 *    - Position child using fresh child.computed.width/height
 *    - Advance position for next child
 * 3. If AUTO dimensions: finalize based on positioned children
 * 4. Convert to absolute coordinates
 * 5. Return (parent can now use our final dimensions)
 */
void ir_layout_single_pass(IRComponent* c, IRLayoutConstraints constraints,
                           float parent_x, float parent_y) {
    if (!c) return;

    // Ensure layout state exists
    if (!c->layout_state) {
        c->layout_state = (IRLayoutState*)calloc(1, sizeof(IRLayoutState));
    }

    // Try to dispatch to component-specific trait first
    ir_layout_dispatch(c, constraints, parent_x, parent_y);

    // If dispatch handled it (trait registered), we're done
    // Check if computed dimensions are set (trait completed layout)
    if (c->layout_state->computed.width > 0 || c->layout_state->computed.height > 0) {
        return;
    }

    // Otherwise, fall back to generic container layout
    // This is a simple implementation - will be enhanced in Phase 4 (Axis-Parameterized Flexbox)

    if (getenv("KRYON_DEBUG_SINGLE_PASS")) {
        fprintf(stderr, "[SinglePass] Component type %d: using fallback generic layout\n", c->type);
    }

    // Resolve own dimensions
    float own_width = constraints.max_width;
    float own_height = constraints.max_height;

    // Apply explicit dimensions if set
    if (c->style) {
        if (c->style->width.type == IR_DIMENSION_PX) {
            own_width = c->style->width.value;
        }
        if (c->style->height.type == IR_DIMENSION_PX) {
            own_height = c->style->height.value;
        }
    }

    // Track whether dimensions are AUTO (need to compute from children)
    bool width_auto = (c->style && c->style->width.type == IR_DIMENSION_AUTO);
    bool height_auto = (c->style && c->style->height.type == IR_DIMENSION_AUTO);

    // Layout children recursively
    float child_y = 0;
    float max_child_width = 0;
    float total_child_height = 0;

    for (uint32_t i = 0; i < c->child_count; i++) {
        IRComponent* child = c->children[i];
        if (!child) continue;

        // Create constraints for child
        IRLayoutConstraints child_constraints = {
            .max_width = own_width,
            .max_height = own_height - child_y,
            .min_width = 0,
            .min_height = 0
        };

        // RECURSIVELY layout child (child computes FINAL dimensions)
        ir_layout_single_pass(child, child_constraints, parent_x, parent_y + child_y);

        // Track child dimensions for AUTO dimension calculation
        if (child->layout_state) {
            // Track for AUTO dimension calculation
            max_child_width = fmaxf(max_child_width, child->layout_state->computed.width);
            total_child_height += child->layout_state->computed.height;

            // Advance Y position for next child (Column layout)
            child_y += child->layout_state->computed.height;
        }
    }

    // Finalize AUTO dimensions
    if (width_auto) {
        own_width = max_child_width;
    }
    if (height_auto) {
        own_height = total_child_height;
    }

    // Set final computed dimensions
    c->layout_state->computed.width = own_width;
    c->layout_state->computed.height = own_height;
    c->layout_state->computed.x = parent_x;
    c->layout_state->computed.y = parent_y;

    // Mark layout as valid
    c->layout_state->layout_valid = true;
    c->layout_state->computed.valid = true;
}

// ============================================================================
// Public API
// ============================================================================

/**
 * Main entry point for layout computation
 * Single-pass recursive layout system
 */
void ir_layout_compute_tree(IRComponent* root, float viewport_width, float viewport_height) {
    if (!root) return;

    if (getenv("KRYON_DEBUG_LAYOUT_MODE")) {
        fprintf(stderr, "[Layout] Using SINGLE-PASS layout system\n");
    }

    IRLayoutConstraints root_constraints = {
        .max_width = viewport_width,
        .max_height = viewport_height,
        .min_width = 0,
        .min_height = 0
    };
    ir_layout_single_pass(root, root_constraints, 0, 0);
}

/**
 * Get computed layout bounds for a component
 * Returns NULL if layout hasn't been computed
 */
IRComputedLayout* ir_layout_get_bounds(IRComponent* component) {
    if (!component || !component->layout_state || !component->layout_state->layout_valid) {
        return NULL;
    }
    return &component->layout_state->computed;
}
