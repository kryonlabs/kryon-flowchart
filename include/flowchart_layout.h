#ifndef FLOWCHART_LAYOUT_H
#define FLOWCHART_LAYOUT_H

#include "flowchart_types.h"
#include "ir_core.h"

/**
 * Compute flowchart layout using graph layering algorithm
 *
 * This function performs hierarchical graph layout:
 * 1. Computes node sizes based on labels
 * 2. Assigns nodes to layers (ranks)
 * 3. Orders nodes within each layer
 * 4. Positions nodes to minimize edge crossings
 * 5. Routes edges between nodes
 * 6. Computes subgraph bounds
 *
 * @param flowchart Flowchart component
 * @param available_width Available width for layout
 * @param available_height Available height for layout
 */
void ir_layout_compute_flowchart(IRComponent* flowchart, float available_width, float available_height);

#endif // FLOWCHART_LAYOUT_H
