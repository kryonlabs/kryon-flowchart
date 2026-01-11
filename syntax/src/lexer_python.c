/*
 * Kryon Syntax - Python Lexer
 */

#include "../include/kryon_syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 256

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
    if (length == 0) return true;
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

static const char* python_keywords[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
    "while", "with", "yield",
    NULL
};

static const char* python_builtins[] = {
    "print", "len", "range", "str", "int", "float", "list", "dict", "set",
    "tuple", "bool", "type", "isinstance", "open", "input", "map", "filter",
    "zip", "enumerate", "sorted", "reversed", "sum", "min", "max", "abs",
    "all", "any", "hasattr", "getattr", "setattr", "delattr", "callable",
    "super", "staticmethod", "classmethod", "property",
    NULL
};

static bool is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static bool in_list(const char* word, size_t len, const char** list) {
    for (const char** p = list; *p; p++) {
        if (strlen(*p) == len && strncmp(*p, word, len) == 0) return true;
    }
    return false;
}

SyntaxToken* lexer_python(const char* code, size_t length, uint32_t* count) {
    TokenBuffer buf;
    if (!buffer_init(&buf)) return NULL;

    size_t pos = 0;

    while (pos < length) {
        char c = code[pos];

        if (isspace((unsigned char)c)) {
            pos++;
            continue;
        }

        /* Comment */
        if (c == '#') {
            size_t start = pos;
            while (pos < length && code[pos] != '\n') pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_COMMENT);
            continue;
        }

        /* Triple-quoted string */
        if ((c == '"' || c == '\'') && pos + 2 < length &&
            code[pos + 1] == c && code[pos + 2] == c) {
            size_t start = pos;
            char quote = c;
            pos += 3;
            while (pos + 2 < length) {
                if (code[pos] == quote && code[pos + 1] == quote && code[pos + 2] == quote) {
                    pos += 3;
                    break;
                }
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* String */
        if (c == '"' || c == '\'') {
            size_t start = pos;
            char quote = c;
            pos++;
            while (pos < length && code[pos] != quote && code[pos] != '\n') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length && code[pos] == quote) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* f-string prefix */
        if ((c == 'f' || c == 'r' || c == 'b' || c == 'F' || c == 'R' || c == 'B') &&
            pos + 1 < length && (code[pos + 1] == '"' || code[pos + 1] == '\'')) {
            size_t start = pos;
            pos++;
            char quote = code[pos];
            pos++;
            while (pos < length && code[pos] != quote && code[pos] != '\n') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length && code[pos] == quote) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Number */
        if (isdigit((unsigned char)c)) {
            size_t start = pos;
            if (c == '0' && pos + 1 < length) {
                char next = tolower((unsigned char)code[pos + 1]);
                if (next == 'x') {
                    pos += 2;
                    while (pos < length && isxdigit((unsigned char)code[pos])) pos++;
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
            while (pos < length && (isdigit((unsigned char)code[pos]) || code[pos] == '_')) pos++;
            if (pos < length && code[pos] == '.') {
                pos++;
                while (pos < length && (isdigit((unsigned char)code[pos]) || code[pos] == '_')) pos++;
            }
            if (pos < length && (code[pos] == 'e' || code[pos] == 'E')) {
                pos++;
                if (pos < length && (code[pos] == '+' || code[pos] == '-')) pos++;
                while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            }
            if (pos < length && (code[pos] == 'j' || code[pos] == 'J')) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
            continue;
        }

        /* Decorator */
        if (c == '@') {
            size_t start = pos;
            pos++;
            while (pos < length && is_ident_char(code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_ATTRIBUTE);
            continue;
        }

        /* Identifier */
        if (is_ident_start(c)) {
            size_t start = pos;
            while (pos < length && is_ident_char(code[pos])) pos++;
            size_t len = pos - start;

            SyntaxTokenType type = SYNTAX_TOKEN_VARIABLE;
            if (in_list(code + start, len, python_keywords)) {
                type = SYNTAX_TOKEN_KEYWORD;
            } else if (in_list(code + start, len, python_builtins)) {
                type = SYNTAX_TOKEN_FUNCTION;
            } else if (pos < length && code[pos] == '(') {
                type = SYNTAX_TOKEN_FUNCTION;
            }

            buffer_push(&buf, start, len, type);
            continue;
        }

        /* Operators */
        if (strchr("+-*/%=<>!&|^~@:.", c)) {
            size_t start = pos;
            while (pos < length && strchr("+-*/%=<>!&|^~@:.", code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_OPERATOR);
            continue;
        }

        /* Punctuation */
        if (strchr("(){}[],;", c)) {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_PUNCTUATION);
            pos++;
            continue;
        }

        pos++;
    }

    *count = buf.count;
    return buf.tokens;
}
