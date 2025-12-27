#include "flowchart_types.h"
#include "flowchart_parser.h"
#include "flowchart_layout.h"
#include "flowchart_renderer_terminal.h"
#include "ir_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Plugin metadata
const char* PLUGIN_NAME = "kryon-flowchart";
const char* PLUGIN_VERSION = "1.0.0";
const char* PLUGIN_DESCRIPTION = "Flowchart and node diagram support with Mermaid syntax";

// Forward declarations for renderer functions
extern void render_flowchart_sdl3(IRComponent* flowchart, void* sdl_renderer);
extern char* render_flowchart_svg(IRComponent* flowchart);

/**
 * Plugin initialization function
 *
 * This function is called when the plugin is loaded by Kryon.
 * It registers component renderers for each backend.
 *
 * Note: The plugin registration API is still being designed.
 * For now, this serves as a placeholder showing the intended structure.
 */
void kryon_plugin_flowchart_init(const char* backend) {
    fprintf(stderr, "[%s] Initializing plugin v%s\n", PLUGIN_NAME, PLUGIN_VERSION);
    fprintf(stderr, "[%s] Backend: %s\n", PLUGIN_NAME, backend ? backend : "unknown");

    // TODO: Register renderers based on backend
    // This will use the plugin API once it's fully implemented:
    //
    // if (strcmp(backend, "terminal") == 0) {
    //     ir_plugin_register_component_renderer(
    //         IR_COMPONENT_FLOWCHART,
    //         (IRPluginComponentRenderer)render_flowchart_terminal
    //     );
    // } else if (strcmp(backend, "sdl3") == 0) {
    //     ir_plugin_register_component_renderer(
    //         IR_COMPONENT_FLOWCHART,
    //         (IRPluginComponentRenderer)render_flowchart_sdl3
    //     );
    // } else if (strcmp(backend, "web") == 0) {
    //     ir_plugin_register_component_renderer(
    //         IR_COMPONENT_FLOWCHART,
    //         (IRPluginComponentRenderer)render_flowchart_svg
    //     );
    // }

    // Register Mermaid parser function
    // ir_plugin_register_function("flowchart.parseMermaid", ir_flowchart_parse);

    fprintf(stderr, "[%s] Plugin initialized successfully\n", PLUGIN_NAME);
}

/**
 * Plugin cleanup function
 *
 * Called when the plugin is unloaded.
 */
void kryon_plugin_flowchart_cleanup(void) {
    fprintf(stderr, "[%s] Cleaning up plugin\n", PLUGIN_NAME);
    // TODO: Unregister renderers and free resources
}

/**
 * Get plugin metadata
 */
const char* kryon_plugin_flowchart_get_name(void) {
    return PLUGIN_NAME;
}

const char* kryon_plugin_flowchart_get_version(void) {
    return PLUGIN_VERSION;
}

const char* kryon_plugin_flowchart_get_description(void) {
    return PLUGIN_DESCRIPTION;
}
