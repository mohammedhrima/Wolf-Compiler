#pragma once
#include "header.h"

#define SPLIT "=================================================\n"
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define CYAN "\033[0;36m"
#define BOLD "\e[1m"
#define BLUE "\x1b[34m"
#define RESET "\033[0m"
#define LINE __LINE__
#define FUNC __func__
#define FILE __FILE__

#define TOKENIZE 1
#define TAB 3

#if TOKENIZE
#define AST 1
#endif

#if AST
#define IR 1
#else
#define IR 0
#endif

#define WITH_COMMENTS 1

#if IR
#define BUILTINS 1
#ifndef OPTIMIZE
#define OPTIMIZE 0
#endif

#define ASM 1
#else
#define ASM 0
#endif

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef TESTING
#define TESTING false
#endif


#define TREE 0

#define DEBUG_INC_PTR 0
#define DEBUG_NEW_TOKEN 0

#define allocate(len, size) allocate_func(LINE, len, size)
#define check(cond, fmt, ...) check_error(FILE, FUNC, LINE, cond, fmt, ##__VA_ARGS__)
#define to_string(type) to_string_(FILE, LINE, type)
#define todo(cond, fmt, ...) check_error(FILE, FUNC, LINE, cond, fmt, ##__VA_ARGS__); exit(1);
#define stop(cond, fmt, ...) check_error(FILE, FUNC, LINE, cond, fmt, ##__VA_ARGS__); exit(1);

#if DEBUG_NEW_TOKEN
#define new_token(input, s, e, type, space) new_token_(FILE, LINE, input, s, e, type, space)
#endif

#if DEBUG_INC_PTR
#define inc_ptr(value) inc_ptr_(FILE, LINE, value)
#endif

#define DATA_TYPES INT, BOOL, CHARS, CHAR, FLOAT, VOID, LONG, PTR, SHORT

#define AST_NODE(name, child_func, ...) \
Node *name() { \
    Node *left = child_func(); \
    Token *token; \
    while ((token = find(__VA_ARGS__, 0))) { \
        Node *node = new_node(token); \
        node->left = left; \
        node->right = child_func(); \
        left = node; \
    } \
    return left; \
}
