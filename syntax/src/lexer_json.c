/*
 * Kryon Syntax - JSON Lexer
 */

#include "../include/kryon_syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 128

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

SyntaxToken* lexer_json(const char* code, size_t length, uint32_t* count) {
    TokenBuffer buf;
    if (!buffer_init(&buf)) return NULL;

    size_t pos = 0;

    while (pos < length) {
        char c = code[pos];

        if (isspace((unsigned char)c)) {
            pos++;
            continue;
        }

        /* String (key or value) */
        if (c == '"') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '"') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length) pos++;

            /* Check if this is a key (followed by :) */
            size_t check = pos;
            while (check < length && isspace((unsigned char)code[check])) check++;
            if (check < length && code[check] == ':') {
                buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_PROPERTY);
            } else {
                buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            }
            continue;
        }

        /* Number */
        if (isdigit((unsigned char)c) || c == '-') {
            size_t start = pos;
            if (c == '-') pos++;
            while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            if (pos < length && code[pos] == '.') {
                pos++;
                while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            }
            if (pos < length && (code[pos] == 'e' || code[pos] == 'E')) {
                pos++;
                if (pos < length && (code[pos] == '+' || code[pos] == '-')) pos++;
                while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            }
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
            continue;
        }

        /* Literals: true, false, null */
        if (strncmp(code + pos, "true", 4) == 0 &&
            (pos + 4 >= length || !isalnum((unsigned char)code[pos + 4]))) {
            buffer_push(&buf, pos, 4, SYNTAX_TOKEN_CONSTANT);
            pos += 4;
            continue;
        }
        if (strncmp(code + pos, "false", 5) == 0 &&
            (pos + 5 >= length || !isalnum((unsigned char)code[pos + 5]))) {
            buffer_push(&buf, pos, 5, SYNTAX_TOKEN_CONSTANT);
            pos += 5;
            continue;
        }
        if (strncmp(code + pos, "null", 4) == 0 &&
            (pos + 4 >= length || !isalnum((unsigned char)code[pos + 4]))) {
            buffer_push(&buf, pos, 4, SYNTAX_TOKEN_CONSTANT);
            pos += 4;
            continue;
        }

        /* Punctuation */
        if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == ':') {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_PUNCTUATION);
            pos++;
            continue;
        }

        pos++;
    }

    *count = buf.count;
    return buf.tokens;
}
