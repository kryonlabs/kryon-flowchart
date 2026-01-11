/*
 * Kryon Syntax - Kry DSL Lexer
 *
 * Tokenizes Kryon's .kry declarative UI syntax.
 * Example:
 *   App {
 *     windowTitle = "Hello"
 *     Container {
 *       Text { text = "World" }
 *     }
 *   }
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

/* Kry component names (types) */
static const char* kry_components[] = {
    "App", "Container", "Column", "Row", "Center", "Text", "Button",
    "Input", "Checkbox", "Dropdown", "TabGroup", "TabBar", "Tab",
    "TabContent", "TabPanel", "Link", "Markdown", "Image", "Spacer", "Divider",
    "Grid", "Stack", "Scroll", "Form", "Label", "Select", "Option",
    "Canvas", "Flowchart", "Code", "Pre", "Paragraph",
    /* Table components */
    "Table", "TableHead", "TableBody", "TableFoot", "TableRow",
    "Tr", "Th", "Td", "TableCell", "TableHeaderCell",
    /* Heading components */
    "H1", "H2", "H3", "H4", "H5", "H6", "Heading",
    /* List components */
    "List", "ListItem", "Ul", "Ol", "Li",
    /* Other semantic components */
    "Blockquote", "HorizontalRule", "Strong", "Em", "Span",
    NULL
};

/* Kry property names */
static const char* kry_properties[] = {
    /* Window properties */
    "windowTitle", "windowWidth", "windowHeight",
    /* Dimension properties */
    "width", "height", "minWidth", "minHeight", "maxWidth", "maxHeight",
    /* Color properties */
    "backgroundColor", "background", "color", "textColor", "borderColor",
    "activeBackground", "activeTextColor", "hoverBackground", "hoverColor",
    /* Typography properties */
    "text", "fontSize", "fontFamily", "fontWeight", "fontStyle",
    "textAlign", "textDecoration", "lineHeight", "letterSpacing",
    /* Spacing properties */
    "padding", "paddingTop", "paddingRight", "paddingBottom", "paddingLeft",
    "margin", "marginTop", "marginRight", "marginBottom", "marginLeft",
    /* Border properties */
    "border", "borderRadius", "borderWidth", "borderStyle",
    /* Layout properties */
    "contentAlignment", "alignment", "alignItems", "justifyContent",
    "spacing", "gap", "direction", "wrap", "flex", "grow", "shrink", "basis",
    /* Position properties */
    "position", "top", "left", "right", "bottom", "posX", "posY",
    "zIndex", "opacity", "visible", "display",
    /* Form properties */
    "enabled", "disabled", "readonly", "placeholder", "value", "title",
    "checked", "selected", "selectedIndex", "options", "label", "name", "type",
    /* Link/media properties */
    "href", "target", "src", "alt", "rel",
    /* Event handlers */
    "onClick", "onChange", "onSubmit", "onFocus", "onBlur",
    "onHover", "onPress", "onLoad", "onError",
    /* Misc */
    "style", "class", "id", "overflow", "cursor", "transform",
    NULL
};

/* Kry constants */
static const char* kry_constants[] = {
    /* Boolean/null */
    "true", "false", "null", "none", "auto", "inherit",
    /* Alignment */
    "center", "left", "right", "top", "bottom",
    "start", "end", "stretch", "baseline",
    "flex-start", "flex-end", "space-between", "space-around", "space-evenly",
    /* Direction/orientation */
    "horizontal", "vertical", "row", "column", "wrap", "nowrap",
    "row-reverse", "column-reverse", "wrap-reverse",
    /* Font styles */
    "bold", "normal", "italic", "underline", "line-through",
    "lighter", "bolder",
    /* Position */
    "relative", "absolute", "fixed", "sticky", "static",
    /* Display */
    "block", "inline", "inline-block", "flex", "grid", "hidden",
    /* Overflow */
    "visible", "scroll", "clip",
    /* Cursor */
    "pointer", "default", "text", "move", "not-allowed", "grab",
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
        if (strlen(*p) == len && strncmp(*p, word, len) == 0) {
            return true;
        }
    }
    return false;
}

SyntaxToken* lexer_kry(const char* code, size_t length, uint32_t* count) {
    TokenBuffer buf;
    if (!buffer_init(&buf)) return NULL;

    size_t pos = 0;

    while (pos < length) {
        char c = code[pos];

        /* Skip whitespace */
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
                if (code[pos] == '\\' && pos + 1 < length) pos++;
                pos++;
            }
            if (pos < length) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Number */
        if (isdigit((unsigned char)c) || (c == '.' && pos + 1 < length && isdigit((unsigned char)code[pos + 1]))) {
            size_t start = pos;

            /* Hex */
            if (c == '0' && pos + 1 < length && tolower((unsigned char)code[pos + 1]) == 'x') {
                pos += 2;
                while (pos < length && isxdigit((unsigned char)code[pos])) pos++;
                buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
                continue;
            }

            /* Decimal or float */
            while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            if (pos < length && code[pos] == '.') {
                pos++;
                while (pos < length && isdigit((unsigned char)code[pos])) pos++;
            }
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_NUMBER);
            continue;
        }

        /* Color literal (#RGB, #RRGGBB, #RRGGBBAA) */
        if (c == '#') {
            size_t start = pos;
            pos++;
            while (pos < length && isxdigit((unsigned char)code[pos])) pos++;
            buffer_push(&buf, start, pos - start, SYNTAX_TOKEN_STRING);
            continue;
        }

        /* Identifier */
        if (is_ident_start(c)) {
            size_t start = pos;
            while (pos < length && is_ident_char(code[pos])) pos++;
            size_t len = pos - start;

            SyntaxTokenType type = SYNTAX_TOKEN_VARIABLE;

            if (in_list(code + start, len, kry_components)) {
                type = SYNTAX_TOKEN_TYPE;  /* Component names */
            } else if (in_list(code + start, len, kry_properties)) {
                type = SYNTAX_TOKEN_PROPERTY;
            } else if (in_list(code + start, len, kry_constants)) {
                type = SYNTAX_TOKEN_CONSTANT;
            }

            buffer_push(&buf, start, len, type);
            continue;
        }

        /* Operators */
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/') {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_OPERATOR);
            pos++;
            continue;
        }

        /* Punctuation */
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == ',' || c == ';') {
            buffer_push(&buf, pos, 1, SYNTAX_TOKEN_PUNCTUATION);
            pos++;
            continue;
        }

        /* Unknown - skip */
        pos++;
    }

    *count = buf.count;
    return buf.tokens;
}
