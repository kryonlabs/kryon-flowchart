/**
 * Kryon Syntax Plugin - Plugin Interface
 *
 * This file provides the plugin entry points for dynamic loading.
 * The syntax plugin provides syntax highlighting for code blocks
 * that works across all Kryon renderers (SDL3, Terminal, Web).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/kryon_syntax.h"

// IR Component types (minimal definition)
typedef enum {
    IR_COMPONENT_CODE_BLOCK = 41
} IRComponentType;

// IR Component structure (minimal definition)
typedef struct IRComponent IRComponent;
struct IRComponent {
    uint32_t id;
    IRComponentType type;
    void* custom_data;
};

// Code block data structure
typedef struct {
    char* language;
    char* code;
    size_t length;
    bool show_line_numbers;
    uint32_t start_line;
} IRCodeBlockData;

// Forward declarations - plugin registration functions (provided by host)
typedef struct {
    char name[64];
    char version[32];
    char description[256];
    char kryon_version_min[32];
    unsigned int command_id_start;
    unsigned int command_id_end;
    const char** required_capabilities;
    unsigned int capability_count;
} IRPluginMetadata;

extern int ir_plugin_register(const IRPluginMetadata* metadata);
extern bool ir_plugin_register_web_renderer(IRComponentType type, char* (*renderer)(IRComponent*, const char*));

/**
 * HTML escape helper function
 */
static void escape_html(const char* text, size_t text_len, char* buffer, size_t buffer_size) {
    if (!text || !buffer || buffer_size == 0) return;

    size_t pos = 0;
    for (size_t i = 0; i < text_len && pos < buffer_size - 1; i++) {
        char c = text[i];
        switch (c) {
            case '&':
                if (pos < buffer_size - 6) { strcpy(buffer + pos, "&amp;"); pos += 5; }
                break;
            case '<':
                if (pos < buffer_size - 5) { strcpy(buffer + pos, "&lt;"); pos += 4; }
                break;
            case '>':
                if (pos < buffer_size - 5) { strcpy(buffer + pos, "&gt;"); pos += 4; }
                break;
            case '"':
                if (pos < buffer_size - 7) { strcpy(buffer + pos, "&quot;"); pos += 6; }
                break;
            case '\'':
                if (pos < buffer_size - 7) { strcpy(buffer + pos, "&#x27;"); pos += 6; }
                break;
            default:
                buffer[pos++] = c;
                break;
        }
    }
    buffer[pos] = '\0';
}

/**
 * Web renderer for code blocks with syntax highlighting.
 * This function is called by the HTML generator when rendering CODE_BLOCK components.
 */
static char* syntax_web_renderer(IRComponent* component, const char* theme) {
    if (!component || component->type != IR_COMPONENT_CODE_BLOCK) {
        return NULL;
    }

    IRCodeBlockData* data = (IRCodeBlockData*)component->custom_data;
    if (!data || !data->code) {
        return NULL;
    }

    const char* language = data->language;
    const char* code = data->code;
    size_t code_len = data->length ? data->length : strlen(code);

    // Check if we support this language
    if (!language || !syntax_supports_language(language)) {
        // Return plain HTML for unsupported languages
        size_t html_size = code_len * 6 + 256;  // Extra space for HTML tags + escaping
        char* html = malloc(html_size);
        if (!html) return NULL;

        char* escaped = malloc(code_len * 6 + 1);
        if (!escaped) {
            free(html);
            return NULL;
        }

        escape_html(code, code_len, escaped, code_len * 6 + 1);
        snprintf(html, html_size, "<code class=\"language-%s\">%s</code>",
                 language ? language : "plain", escaped);
        free(escaped);
        return html;
    }

    // Tokenize the code
    uint32_t token_count = 0;
    SyntaxToken* tokens = syntax_tokenize(code, code_len, language, &token_count);

    if (!tokens || token_count == 0) {
        // Tokenization failed, return plain HTML
        syntax_free_tokens(tokens);
        size_t html_size = code_len * 6 + 256;
        char* html = malloc(html_size);
        if (!html) return NULL;

        char* escaped = malloc(code_len * 6 + 1);
        if (!escaped) {
            free(html);
            return NULL;
        }

        escape_html(code, code_len, escaped, code_len * 6 + 1);
        snprintf(html, html_size, "<code class=\"language-%s\">%s</code>", language, escaped);
        free(escaped);
        return html;
    }

    // Generate HTML with syntax highlighting
    // Estimate size: code + spans + classes
    size_t html_size = code_len * 8 + token_count * 100 + 256;
    char* html = malloc(html_size);
    if (!html) {
        syntax_free_tokens(tokens);
        return NULL;
    }

    // Start with opening code tag
    size_t pos = 0;
    pos += snprintf(html + pos, html_size - pos, "<code class=\"language-%s\">", language);

    // Render tokens with spans
    uint32_t code_pos = 0;
    for (uint32_t i = 0; i < token_count; i++) {
        SyntaxToken* tok = &tokens[i];

        // Output any text between tokens (whitespace, newlines)
        if (tok->start > code_pos) {
            size_t gap_len = tok->start - code_pos;
            char* gap_escaped = malloc(gap_len * 6 + 1);
            if (gap_escaped) {
                escape_html(code + code_pos, gap_len, gap_escaped, gap_len * 6 + 1);
                pos += snprintf(html + pos, html_size - pos, "%s", gap_escaped);
                free(gap_escaped);
            }
        }

        // Get CSS class for token type
        const char* token_class = syntax_token_class(tok->type);

        // Output token with span wrapper
        char* tok_escaped = malloc(tok->length * 6 + 1);
        if (tok_escaped) {
            escape_html(code + tok->start, tok->length, tok_escaped, tok->length * 6 + 1);
            if (token_class && token_class[0]) {
                pos += snprintf(html + pos, html_size - pos,
                               "<span class=\"%s\">%s</span>", token_class, tok_escaped);
            } else {
                pos += snprintf(html + pos, html_size - pos, "%s", tok_escaped);
            }
            free(tok_escaped);
        }

        code_pos = tok->start + tok->length;
    }

    // Output any remaining text after last token
    if (code_pos < code_len) {
        size_t rem_len = code_len - code_pos;
        char* rem_escaped = malloc(rem_len * 6 + 1);
        if (rem_escaped) {
            escape_html(code + code_pos, rem_len, rem_escaped, rem_len * 6 + 1);
            pos += snprintf(html + pos, html_size - pos, "%s", rem_escaped);
            free(rem_escaped);
        }
    }

    // Close code tag
    pos += snprintf(html + pos, html_size - pos, "</code>");

    syntax_free_tokens(tokens);
    return html;
}

/**
 * Plugin initialization function.
 * Called by ir_plugin_load() after dlopen().
 *
 * Returns 0 on success, non-zero on failure.
 */
int kryon_syntax_plugin_init(void) {
    // NOTE: Plugin registration is handled by the loading system (ir_plugin_load_with_metadata)
    // when loaded via discovery. Only self-register if loaded directly without discovery info.
    // Check if already registered to avoid duplicate registration.

    printf("[kryon][syntax] Syntax highlighting plugin initialized (v1.0.0)\n");
    printf("[kryon][syntax] Supported languages: ");

    // List supported languages
    unsigned int lang_count;
    const char** languages = syntax_list_languages(&lang_count);
    for (unsigned int i = 0; i < lang_count; i++) {
        printf("%s%s", languages[i], (i < lang_count - 1) ? ", " : "\n");
    }

    // Register web renderer for code blocks
    if (!ir_plugin_register_web_renderer(IR_COMPONENT_CODE_BLOCK, syntax_web_renderer)) {
        fprintf(stderr, "[kryon][syntax] Failed to register web renderer\n");
        return -1;
    }
    printf("[kryon][syntax] Registered web renderer for CODE_BLOCK components\n");

    return 0;
}

/**
 * Plugin shutdown function.
 * Called by ir_plugin_unload() before dlclose().
 */
void kryon_syntax_plugin_shutdown(void) {
    printf("[kryon][syntax] Syntax highlighting plugin shutdown\n");
}
