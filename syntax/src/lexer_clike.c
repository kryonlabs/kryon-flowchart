/*
 * Kryon Syntax - C-like Languages Lexer
 *
 * Tokenizes C, C++, JavaScript, TypeScript, Java, Rust, Go, etc.
 * Uses a state machine approach with language-specific keyword tables.
 */

#include "../include/kryon_syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Initial token capacity */
#define INITIAL_CAPACITY 256

/* Token buffer for building result */
typedef struct {
    SyntaxToken* tokens;
    uint32_t count;
    uint32_t capacity;
} TokenBuffer;

static bool buffer_init(TokenBuffer* buf) {
    buf->tokens = malloc(INITIAL_CAPACITY * sizeof(SyntaxToken));
    if (!buf->tokens) return false;
    buf->count = 0;
    buf->capacity = INITIAL_CAPACITY;
    return true;
}

static bool buffer_push(TokenBuffer* buf, uint32_t start, uint32_t length, SyntaxTokenType type) {
    if (length == 0) return true;  /* Skip empty tokens */

    if (buf->count >= buf->capacity) {
        uint32_t new_cap = buf->capacity * 2;
        SyntaxToken* new_tokens = realloc(buf->tokens, new_cap * sizeof(SyntaxToken));
        if (!new_tokens) return false;
        buf->tokens = new_tokens;
        buf->capacity = new_cap;
    }

    buf->tokens[buf->count].start = start;
    buf->tokens[buf->count].length = length;
    buf->tokens[buf->count].type = type;
    buf->count++;
    return true;
}

/* Keyword tables for different languages */

static const char* keywords_c[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short",
    "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    NULL
};

static const char* keywords_cpp[] = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
    "compl", "concept", "const", "consteval", "constexpr", "constinit",
    "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype",
    "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
    "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
    "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
    "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
    "protected", "public", "register", "reinterpret_cast", "requires",
    "return", "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "thread_local",
    "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while",
    "xor", "xor_eq",
    NULL
};

static const char* keywords_js[] = {
    "async", "await", "break", "case", "catch", "class", "const", "continue",
    "debugger", "default", "delete", "do", "else", "export", "extends",
    "finally", "for", "function", "if", "import", "in", "instanceof", "let",
    "new", "of", "return", "static", "super", "switch", "this", "throw",
    "try", "typeof", "var", "void", "while", "with", "yield",
    NULL
};

static const char* keywords_ts[] = {
    "abstract", "any", "as", "asserts", "async", "await", "bigint", "boolean",
    "break", "case", "catch", "class", "const", "constructor", "continue",
    "debugger", "declare", "default", "delete", "do", "else", "enum", "export",
    "extends", "false", "finally", "for", "from", "function", "get", "global",
    "if", "implements", "import", "in", "infer", "instanceof", "interface",
    "is", "keyof", "let", "module", "namespace", "never", "new", "null",
    "number", "object", "of", "override", "package", "private", "protected",
    "public", "readonly", "require", "return", "set", "static", "string",
    "super", "switch", "symbol", "this", "throw", "true", "try", "type",
    "typeof", "undefined", "unique", "unknown", "var", "void", "while",
    "with", "yield",
    NULL
};

static const char* keywords_java[] = {
    "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char",
    "class", "const", "continue", "default", "do", "double", "else", "enum",
    "extends", "final", "finally", "float", "for", "goto", "if", "implements",
    "import", "instanceof", "int", "interface", "long", "native", "new",
    "package", "private", "protected", "public", "return", "short", "static",
    "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
    "transient", "try", "void", "volatile", "while",
    NULL
};

static const char* keywords_rust[] = {
    "as", "async", "await", "break", "const", "continue", "crate", "dyn",
    "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
    "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
    "self", "Self", "static", "struct", "super", "trait", "true", "type",
    "unsafe", "use", "where", "while",
    NULL
};

static const char* keywords_go[] = {
    "break", "case", "chan", "const", "continue", "default", "defer", "else",
    "fallthrough", "for", "func", "go", "goto", "if", "import", "interface",
    "map", "package", "range", "return", "select", "struct", "switch", "type",
    "var",
    NULL
};

static const char* types_common[] = {
    "int", "float", "double", "char", "bool", "void", "string", "byte",
    "short", "long", "unsigned", "signed", "size_t", "uint8_t", "uint16_t",
    "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
    "boolean", "String", "Object", "Array", "Map", "Set", "Promise",
    "number", "any", "unknown", "never",
    NULL
};

static const char* constants_common[] = {
    "true", "false", "null", "nil", "nullptr", "undefined", "NaN", "Infinity",
    "None", "True", "False",
    NULL
};

/* Get keyword list for language */
static const char** get_keywords(const char* lang) {
    if (strcmp(lang, "c") == 0) return keywords_c;
    if (strcmp(lang, "cpp") == 0) return keywords_cpp;
    if (strcmp(lang, "javascript") == 0) return keywords_js;
    if (strcmp(lang, "typescript") == 0) return keywords_ts;
    if (strcmp(lang, "java") == 0) return keywords_java;
    if (strcmp(lang, "rust") == 0) return keywords_rust;
    if (strcmp(lang, "go") == 0) return keywords_go;
    /* Default to JS keywords for unknown C-like */
    return keywords_js;
}

/* Check if word is in list */
static bool in_list(const char* word, size_t len, const char** list) {
    for (const char** p = list; *p; p++) {
        if (strlen(*p) == len && strncmp(*p, word, len) == 0) {
            return true;
        }
    }
    return false;
}

/* Character classification helpers */
static bool is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_' || c == '$';
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '$';
}

static bool is_digit(char c) {
    return isdigit((unsigned char)c);
}

static bool is_hex_digit(char c) {
    return isxdigit((unsigned char)c);
}

static bool is_operator_char(char c) {
    return strchr("+-*/%=<>!&|^~?:", c) != NULL;
}

static bool is_punctuation(char c) {
    return strchr("(){}[];,.", c) != NULL;
}

/* Main C-like lexer */
SyntaxToken* lexer_clike(const char* code, size_t length, const char* lang, uint32_t* count) {
    TokenBuffer buf;
    if (!buffer_init(&buf)) return NULL;

    const char** keywords = get_keywords(lang);
    size_t pos = 0;

    while (pos < length) {
        char c = code[pos];

        /* Skip whitespace (don't create tokens for it) */
        if (isspace((unsigned char)c)) {
            pos++;
            continue;
        }

        /* Line comment */
        if (c == '/' && pos + 1 < length && code[pos + 1] == '/') {
            size_t start = pos;
            pos += 2;
            while (pos < length && code[pos] != '\n') pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_COMMENT);
            continue;
        }

        /* Block comment */
        if (c == '/' && pos + 1 < length && code[pos + 1] == '*') {
            size_t start = pos;
            pos += 2;
            while (pos + 1 < length && !(code[pos] == '*' && code[pos + 1] == '/')) pos++;
            if (pos + 1 < length) pos += 2;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_COMMENT);
            continue;
        }

        /* String (double quotes) */
        if (c == '"') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '"') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;  /* Skip escape */
                pos++;
            }
            if (pos < length) pos++;  /* Skip closing quote */
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* String (single quotes) */
        if (c == '\'') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '\'') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Template literal (backtick) - JS/TS */
        if (c == '`') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '`') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Number */
        if (is_digit(c) || (c == '.' && pos + 1 < length && is_digit(code[pos + 1]))) {
            size_t start = pos;

            /* Hex/binary/octal prefix */
            if (c == '0' && pos + 1 < length) {
                char next = tolower((unsigned char)code[pos + 1]);
                if (next == 'x') {
                    pos += 2;
                    while (pos < length && is_hex_digit(code[pos])) pos++;
                    buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
                    continue;
                }
                if (next == 'b') {
                    pos += 2;
                    while (pos < length && (code[pos] == '0' || code[pos] == '1')) pos++;
                    buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
                    continue;
                }
                if (next == 'o') {
                    pos += 2;
                    while (pos < length && code[pos] >= '0' && code[pos] <= '7') pos++;
                    buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
                    continue;
                }
            }

            /* Regular number (int or float) */
            while (pos < length && is_digit(code[pos])) pos++;
            if (pos < length && code[pos] == '.') {
                pos++;
                while (pos < length && is_digit(code[pos])) pos++;
            }
            /* Exponent */
            if (pos < length && (code[pos] == 'e' || code[pos] == 'E')) {
                pos++;
                if (pos < length && (code[pos] == '+' || code[pos] == '-')) pos++;
                while (pos < length && is_digit(code[pos])) pos++;
            }
            /* Suffix (f, L, u, etc.) */
            while (pos < length && isalpha((unsigned char)code[pos])) pos++;

            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
            continue;
        }

        /* Identifier or keyword */
        if (is_ident_start(c)) {
            size_t start = pos;
            while (pos < length && is_ident_char(code[pos])) pos++;
            size_t len = pos - start;

            /* Classify the identifier */
            SyntaxTokenType type = SYNTAX_TOKEN_VARIABLE;

            if (in_list(code + start, len, keywords)) {
                type = SYNTAX_TOKEN_KEYWORD;
            } else if (in_list(code + start, len, constants_common)) {
                type = SYNTAX_TOKEN_CONSTANT;
            } else if (in_list(code + start, len, types_common)) {
                type = SYNTAX_TOKEN_TYPE;
            } else if (pos < length && code[pos] == '(') {
                /* Followed by ( -> function call */
                type = SYNTAX_TOKEN_FUNCTION;
            }

            buffer_push(&buf, start, len, type);
            continue;
        }

        /* Attribute/decorator (@something) */
        if (c == '@') {
            size_t start = pos;
            pos++;
            while (pos < length && is_ident_char(code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_ATTRIBUTE);
            continue;
        }

        /* Preprocessor directive (#include, etc.) */
        if (c == '#') {
            size_t start = pos;
            pos++;
            while (pos < length && is_ident_char(code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_ATTRIBUTE);
            continue;
        }

        /* Operators (multi-char) */
        if (is_operator_char(c)) {
            size_t start = pos;
            /* Consume operator characters */
            while (pos < length && is_operator_char(code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_OPERATOR);
            continue;
        }

        /* Punctuation */
        if (is_punctuation(c)) {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_PUNCTUATION);
            pos++;
            continue;
        }

        /* Unknown character - skip */
        pos++;
    }

    *count = buf.count;
    return buf.tokens;
}
