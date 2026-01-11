/*
 * Kryon Syntax - Color Themes
 *
 * Built-in color themes for syntax highlighting.
 * Colors are designed to match popular editor themes.
 */

#include "../include/kryon_syntax.h"
#include <string.h>

/* GitHub Dark theme colors */
static const SyntaxTheme theme_dark = {
    .name = "dark",
    .colors = {
        [SYNTAX_TOKEN_PLAIN]       = {201, 209, 217, 255},  /* #c9d1d9 */
        [SYNTAX_TOKEN_KEYWORD]     = {255, 123, 114, 255},  /* #ff7b72 */
        [SYNTAX_TOKEN_STRING]      = {165, 214, 255, 255},  /* #a5d6ff */
        [SYNTAX_TOKEN_NUMBER]      = {121, 192, 255, 255},  /* #79c0ff */
        [SYNTAX_TOKEN_COMMENT]     = {139, 148, 158, 255},  /* #8b949e */
        [SYNTAX_TOKEN_OPERATOR]    = {201, 209, 217, 255},  /* #c9d1d9 */
        [SYNTAX_TOKEN_PUNCTUATION] = {201, 209, 217, 255},  /* #c9d1d9 */
        [SYNTAX_TOKEN_FUNCTION]    = {210, 168, 255, 255},  /* #d2a8ff */
        [SYNTAX_TOKEN_TYPE]        = {126, 231, 135, 255},  /* #7ee787 */
        [SYNTAX_TOKEN_VARIABLE]    = {201, 209, 217, 255},  /* #c9d1d9 */
        [SYNTAX_TOKEN_CONSTANT]    = {121, 192, 255, 255},  /* #79c0ff */
        [SYNTAX_TOKEN_ATTRIBUTE]   = {210, 168, 255, 255},  /* #d2a8ff */
        [SYNTAX_TOKEN_TAG]         = {126, 231, 135, 255},  /* #7ee787 */
        [SYNTAX_TOKEN_PROPERTY]    = {121, 192, 255, 255},  /* #79c0ff */
    }
};

/* GitHub Light theme colors */
static const SyntaxTheme theme_light = {
    .name = "light",
    .colors = {
        [SYNTAX_TOKEN_PLAIN]       = {36, 41, 47, 255},     /* #24292f */
        [SYNTAX_TOKEN_KEYWORD]     = {207, 34, 46, 255},    /* #cf222e */
        [SYNTAX_TOKEN_STRING]      = {10, 48, 105, 255},    /* #0a3069 */
        [SYNTAX_TOKEN_NUMBER]      = {5, 80, 174, 255},     /* #0550ae */
        [SYNTAX_TOKEN_COMMENT]     = {110, 119, 129, 255},  /* #6e7781 */
        [SYNTAX_TOKEN_OPERATOR]    = {36, 41, 47, 255},     /* #24292f */
        [SYNTAX_TOKEN_PUNCTUATION] = {36, 41, 47, 255},     /* #24292f */
        [SYNTAX_TOKEN_FUNCTION]    = {130, 80, 223, 255},   /* #8250df */
        [SYNTAX_TOKEN_TYPE]        = {17, 99, 41, 255},     /* #116329 */
        [SYNTAX_TOKEN_VARIABLE]    = {36, 41, 47, 255},     /* #24292f */
        [SYNTAX_TOKEN_CONSTANT]    = {5, 80, 174, 255},     /* #0550ae */
        [SYNTAX_TOKEN_ATTRIBUTE]   = {130, 80, 223, 255},   /* #8250df */
        [SYNTAX_TOKEN_TAG]         = {17, 99, 41, 255},     /* #116329 */
        [SYNTAX_TOKEN_PROPERTY]    = {5, 80, 174, 255},     /* #0550ae */
    }
};

/* Available themes */
static const SyntaxTheme* themes[] = {
    &theme_dark,
    &theme_light,
    NULL
};

static const char* theme_names[] = {
    "dark",
    "light",
    NULL
};

const SyntaxTheme* syntax_get_theme(const char* theme_name) {
    if (!theme_name) return &theme_dark;  /* Default to dark */

    for (const SyntaxTheme** t = themes; *t; t++) {
        if (strcmp((*t)->name, theme_name) == 0) {
            return *t;
        }
    }
    return &theme_dark;  /* Fallback to dark */
}

SyntaxColor syntax_theme_color(const char* theme_name, SyntaxTokenType type) {
    const SyntaxTheme* theme = syntax_get_theme(theme_name);
    if (type < 0 || type >= SYNTAX_TOKEN_COUNT) {
        return theme->colors[SYNTAX_TOKEN_PLAIN];
    }
    return theme->colors[type];
}

const char** syntax_list_themes(uint32_t* count) {
    if (count) *count = 2;  /* dark, light */
    return theme_names;
}
