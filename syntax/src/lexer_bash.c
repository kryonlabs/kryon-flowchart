/*
 * Kryon Syntax - Bash/Shell Lexer
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

static const char* bash_keywords[] = {
    "if", "then", "else", "elif", "fi", "case", "esac", "for", "select",
    "while", "until", "do", "done", "in", "function", "time", "coproc",
    "return", "exit", "break", "continue", "declare", "typeset", "local",
    "export", "readonly", "unset", "shift", "source", "alias", "unalias",
    "set", "shopt", "trap", "eval", "exec",
    NULL
};

static const char* bash_builtins[] = {
    "echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs",
    "let", "test", "true", "false", "command", "builtin", "enable",
    "help", "logout", "mapfile", "readarray", "type", "ulimit", "umask",
    "wait", "kill", "jobs", "fg", "bg", "disown", "suspend", "hash",
    "getopts", "bind", "complete", "compgen", "compopt",
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

SyntaxToken* lexer_bash(const char* code, size_t length, uint32_t* count) {
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

        /* Double-quoted string (allows variable interpolation) */
        if (c == '"') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '"') {
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Single-quoted string (literal) */
        if (c == '\'') {
            size_t start = pos;
            pos++;
            while (pos < length && code[pos] != '\'') pos++;
            if (pos < length) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* $(...) or ${...} or $VAR */
        if (c == '$') {
            size_t start = pos;
            pos++;
            if (pos < length) {
                if (code[pos] == '(' || code[pos] == '{') {
                    char close = (code[pos] == '(') ? ')' : '}';
                    pos++;
                    int depth = 1;
                    while (pos < length && depth > 0) {
                        if (code[pos] == code[start + 1]) depth++;
                        else if (code[pos] == close) depth--;
                        pos++;
                    }
                } else if (is_ident_start(code[pos]) || isdigit((unsigned char)code[pos])) {
                    while (pos < length && is_ident_char(code[pos])) pos++;
                }
            }
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_VARIABLE);
            continue;
        }

        /* Number */
        if (isdigit((unsigned char)c)) {
            size_t start = pos;
            while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
            continue;
        }

        /* Identifier or keyword */
        if (is_ident_start(c)) {
            size_t start = pos;
            while (pos < length && (is_ident_char(code[pos]) || code[pos] == '-')) pos++;
            size_t len = pos - start;

            SyntaxTokenType type = SYNTAX_TOKEN_PLAIN;
            if (in_list(code + start, len, bash_keywords)) {
                type = SYNTAX_TOKEN_KEYWORD;
            } else if (in_list(code + start, len, bash_builtins)) {
                type = SYNTAX_TOKEN_FUNCTION;
            }

            buffer_push(&buf, start, len, type);
            continue;
        }

        /* Operators and special chars */
        if (strchr("|&;<>()[]!=-+", c)) {
            size_t start = pos;
            /* Multi-char operators */
            if ((c == '|' && pos + 1 < length && code[pos + 1] == '|') ||
                (c == '&' && pos + 1 < length && code[pos + 1] == '&') ||
                (c == '<' && pos + 1 < length && code[pos + 1] == '<') ||
                (c == '>' && pos + 1 < length && code[pos + 1] == '>')) {
                pos += 2;
            } else {
                pos++;
            }
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_OPERATOR);
            continue;
        }

        /* Punctuation */
        if (strchr("{},;", c)) {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_PUNCTUATION);
            pos++;
            continue;
        }

        pos++;
    }

    *count = buf.count;
    return buf.tokens;
}
