/*
 * Kryon Syntax Highlighting Library
 *
 * A lightweight C library for tokenizing source code for syntax highlighting.
 * Designed to integrate with Kryon's IR system but can be used standalone.
 */

#ifndef KRYON_SYNTAX_H
#define KRYON_SYNTAX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Token types for syntax highlighting */
typedef enum {
    SYNTAX_TOKEN_PLAIN = 0,      /* Default text */
    SYNTAX_TOKEN_KEYWORD,        /* if, else, for, while, return, etc. */
    SYNTAX_TOKEN_STRING,         /* "string" or 'string' */
    SYNTAX_TOKEN_NUMBER,         /* 123, 3.14, 0xFF */
    SYNTAX_TOKEN_COMMENT,        /* Comments (line or block) */
    SYNTAX_TOKEN_OPERATOR,       /* +, -, *, /, =, ==, etc. */
    SYNTAX_TOKEN_PUNCTUATION,    /* (), {}, [], ;, , */
    SYNTAX_TOKEN_FUNCTION,       /* Function names */
    SYNTAX_TOKEN_TYPE,           /* Type names (int, string, etc.) */
    SYNTAX_TOKEN_VARIABLE,       /* Variable names */
    SYNTAX_TOKEN_CONSTANT,       /* true, false, null, nil */
    SYNTAX_TOKEN_ATTRIBUTE,      /* @decorator, #pragma */
    SYNTAX_TOKEN_TAG,            /* HTML/XML tags */
    SYNTAX_TOKEN_PROPERTY,       /* Object properties */
    SYNTAX_TOKEN_COUNT           /* Number of token types */
} SyntaxTokenType;

/* Token span within source code */
typedef struct {
    uint32_t start;              /* Start offset in code */
    uint32_t length;             /* Token length in bytes */
    SyntaxTokenType type;        /* Token type */
} SyntaxToken;

/* RGBA color */
typedef struct {
    uint8_t r, g, b, a;
} SyntaxColor;

/* Theme structure */
typedef struct {
    const char* name;
    SyntaxColor colors[SYNTAX_TOKEN_COUNT];
} SyntaxTheme;

/*
 * Tokenize source code based on language.
 *
 * @param code        Source code to tokenize
 * @param length      Length of source code in bytes
 * @param language    Language identifier (e.g., "javascript", "python")
 * @param token_count Output: number of tokens generated
 * @return            Allocated array of tokens (caller must free with syntax_free_tokens)
 *                    Returns NULL if language is not supported or on error
 */
SyntaxToken* syntax_tokenize(const char* code, size_t length,
                             const char* language, uint32_t* token_count);

/*
 * Free a token array returned by syntax_tokenize.
 */
void syntax_free_tokens(SyntaxToken* tokens);

/*
 * Check if a language is supported for tokenization.
 */
bool syntax_supports_language(const char* language);

/*
 * Get list of supported languages.
 *
 * @param count Output: number of languages
 * @return      Array of language name strings (do not free)
 */
const char** syntax_list_languages(uint32_t* count);

/*
 * Get color for a token type from a theme.
 *
 * @param theme_name  Theme name ("dark" or "light")
 * @param type        Token type
 * @return            RGBA color
 */
SyntaxColor syntax_theme_color(const char* theme_name, SyntaxTokenType type);

/*
 * Get a theme by name.
 *
 * @param theme_name  Theme name ("dark" or "light")
 * @return            Theme struct pointer (do not free), defaults to "dark" if not found
 */
const SyntaxTheme* syntax_get_theme(const char* theme_name);

/*
 * Get list of available themes.
 *
 * @param count Output: number of themes
 * @return      Array of theme name strings (do not free)
 */
const char** syntax_list_themes(uint32_t* count);

/*
 * Get CSS class name for a token type (for HTML output).
 * Returns names compatible with highlight.js themes.
 */
const char* syntax_token_class(SyntaxTokenType type);

#ifdef __cplusplus
}
#endif

#endif /* KRYON_SYNTAX_H */
