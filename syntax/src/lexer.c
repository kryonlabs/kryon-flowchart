/*
 * Kryon Syntax - Lexer Framework
 *
 * Main entry point for tokenization. Dispatches to language-specific lexers.
 */

#include "../include/kryon_syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Forward declarations for language lexers */
SyntaxToken* lexer_clike(const char* code, size_t length, const char* lang, uint32_t* count);
SyntaxToken* lexer_python(const char* code, size_t length, uint32_t* count);
SyntaxToken* lexer_bash(const char* code, size_t length, uint32_t* count);
SyntaxToken* lexer_json(const char* code, size_t length, uint32_t* count);
SyntaxToken* lexer_kry(const char* code, size_t length, uint32_t* count);

/* Language to lexer mapping */
typedef struct {
    const char* name;
    const char* aliases[8];
    SyntaxToken* (*lexer)(const char* code, size_t length, const char* lang, uint32_t* count);
    bool is_clike;  /* Uses the C-like lexer with language param */
} LanguageEntry;

static const LanguageEntry languages[] = {
    /* C-like languages (share same lexer with different keywords) */
    {"c", {"c", "h", NULL}, lexer_clike, true},
    {"cpp", {"cpp", "c++", "cxx", "cc", "hpp", NULL}, lexer_clike, true},
    {"javascript", {"javascript", "js", "jsx", "mjs", NULL}, lexer_clike, true},
    {"typescript", {"typescript", "ts", "tsx", NULL}, lexer_clike, true},
    {"java", {"java", NULL}, lexer_clike, true},
    {"rust", {"rust", "rs", NULL}, lexer_clike, true},
    {"go", {"go", "golang", NULL}, lexer_clike, true},
    {"csharp", {"csharp", "cs", "c#", NULL}, lexer_clike, true},
    {"swift", {"swift", NULL}, lexer_clike, true},
    {"kotlin", {"kotlin", "kt", NULL}, lexer_clike, true},
    {"scala", {"scala", NULL}, lexer_clike, true},
    {"dart", {"dart", NULL}, lexer_clike, true},

    /* Kryon DSL */
    {"kry", {"kry", "kryon", NULL}, NULL, false},

    /* Other languages with dedicated lexers */
    {"python", {"python", "py", NULL}, NULL, false},
    {"bash", {"bash", "sh", "shell", "zsh", NULL}, NULL, false},
    {"json", {"json", NULL}, NULL, false},

    {NULL, {NULL}, NULL, false}
};

/* CSS class names for token types (highlight.js compatible) */
static const char* token_classes[] = {
    "",                    /* PLAIN */
    "hljs-keyword",        /* KEYWORD */
    "hljs-string",         /* STRING */
    "hljs-number",         /* NUMBER */
    "hljs-comment",        /* COMMENT */
    "hljs-operator",       /* OPERATOR (custom) */
    "hljs-punctuation",    /* PUNCTUATION (custom) */
    "hljs-title function_", /* FUNCTION */
    "hljs-type",           /* TYPE */
    "hljs-variable",       /* VARIABLE */
    "hljs-literal",        /* CONSTANT */
    "hljs-meta",           /* ATTRIBUTE */
    "hljs-tag",            /* TAG */
    "hljs-attr",           /* PROPERTY */
};

/* Find language entry by name or alias */
static const LanguageEntry* find_language(const char* lang) {
    if (!lang) return NULL;

    /* Convert to lowercase for comparison */
    char lower[32];
    size_t len = strlen(lang);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower((unsigned char)lang[i]);
    }
    lower[len] = '\0';

    for (const LanguageEntry* e = languages; e->name; e++) {
        if (strcmp(e->name, lower) == 0) return e;
        for (int i = 0; e->aliases[i]; i++) {
            if (strcmp(e->aliases[i], lower) == 0) return e;
        }
    }
    return NULL;
}

/* Main tokenization entry point */
SyntaxToken* syntax_tokenize(const char* code, size_t length,
                             const char* language, uint32_t* token_count) {
    if (!code || !token_count) return NULL;
    *token_count = 0;

    /* Auto-detect length if 0 */
    if (length == 0) {
        length = strlen(code);
    }

    const LanguageEntry* entry = find_language(language);
    if (!entry) return NULL;

    /* Dispatch to appropriate lexer */
    if (entry->is_clike) {
        return lexer_clike(code, length, entry->name, token_count);
    }

    /* Language-specific lexers */
    if (strcmp(entry->name, "python") == 0) {
        return lexer_python(code, length, token_count);
    }
    if (strcmp(entry->name, "bash") == 0) {
        return lexer_bash(code, length, token_count);
    }
    if (strcmp(entry->name, "json") == 0) {
        return lexer_json(code, length, token_count);
    }
    if (strcmp(entry->name, "kry") == 0) {
        return lexer_kry(code, length, token_count);
    }

    return NULL;
}

void syntax_free_tokens(SyntaxToken* tokens) {
    free(tokens);
}

bool syntax_supports_language(const char* language) {
    return find_language(language) != NULL;
}

const char** syntax_list_languages(uint32_t* count) {
    static const char* names[32];
    static bool initialized = false;
    static uint32_t num_languages = 0;

    if (!initialized) {
        for (const LanguageEntry* e = languages; e->name && num_languages < 31; e++) {
            names[num_languages++] = e->name;
        }
        names[num_languages] = NULL;
        initialized = true;
    }

    if (count) *count = num_languages;
    return names;
}

const char* syntax_token_class(SyntaxTokenType type) {
    if (type < 0 || type >= SYNTAX_TOKEN_COUNT) return "";
    return token_classes[type];
}
