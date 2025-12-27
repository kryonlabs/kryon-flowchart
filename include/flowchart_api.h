#ifndef FLOWCHART_API_H
#define FLOWCHART_API_H

#include "flowchart_types.h"
#include "ir_core.h"

/**
 * Kryon Flowchart Plugin Public API
 *
 * This header defines the public API for the kryon-flowchart plugin.
 */

// Plugin initialization and metadata
void kryon_plugin_flowchart_init(const char* backend);
void kryon_plugin_flowchart_cleanup(void);
const char* kryon_plugin_flowchart_get_name(void);
const char* kryon_plugin_flowchart_get_version(void);
const char* kryon_plugin_flowchart_get_description(void);

// Mermaid parser API
IRComponent* ir_flowchart_parse(const char* source, size_t length);
char* ir_flowchart_to_kir(const char* source, size_t length);
bool ir_flowchart_is_mermaid(const char* source, size_t length);

// Layout API
void ir_layout_compute_flowchart(IRComponent* flowchart, float available_width, float available_height);

// Renderer API (backend-specific)
bool render_flowchart_terminal(IRComponent* flowchart, const void* caps);
void render_flowchart_sdl3(IRComponent* flowchart, void* sdl_renderer);
char* render_flowchart_svg(IRComponent* flowchart);

#endif // FLOWCHART_API_H
