#pragma once

// HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include "macros.h"

// STRUCTS
typedef enum
{
   TMP = 1, CHILDREN, DEFAULT,
   // TODO: don't assign from reference if it does not have reference
   REF_ID, REF_HOLD_ID, REF_VAL, REF_HOLD_REF, REF_REF, ID_ID, ID_REF, ID_VAL,
   ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
   EQUAL, NOT_EQUAL, LESS_EQUAL, MORE_EQUAL, LESS, MORE,
   ADD, SUB, MUL, DIV, MOD,
   AND, OR, NOT,
   LPAR, RPAR, LBRA, RBRA, COMA, DOT, DOTS,
   RETURN,
   IF, ELIF, ELSE,
   WHILE, CONTINUE, BREAK,
   FDEC, FCALL,
   VOID, INT, CHARS, CHAR, BOOL, FLOAT, PTR, LONG,
   STRUCT_DEF, STRUCT_CALL, ID, REF,
   JNE, JE, JMP, BLOC, END_BLOC,
   PUSH, POP,
   END
} Type;

typedef struct Token
{
   Type type;
   Type retType; // return type
   Type assign_type;
   char *name;
   int ptr; // pointer
   // bool declare; // is variable declaration
   int space; // indentation
   bool remove;
   int reg;
   char *creg;
   int index;
   bool is_cond;
   bool is_ref;
   bool has_ref;
   bool is_data_type;
   // int rsp;
   // bool isarg;
   bool isattr;
   int offset;

   struct
   {
      // integer
      struct
      {
         long value;
         int power;
         struct Int *next;
      } Int;
      // long
      struct
      {
         long long value;
         int power;
         struct Int *next;
      } Long;
      // float
      struct
      {
         float value;
      } Float;
      // boolean
      struct
      {
         bool value;
         char c;
      } Bool;
      // chars
      struct
      {
         char *value;
      } Chars;
      // char
      struct
      {
         char value;
      } Char;
      struct
      {
         int id;
         struct Token **attrs;
         int pos;
         int len;
      } Struct;
   };
} Token;

typedef struct Node
{
   struct Node *left;
   struct Node *right;
   Token *token;

   struct Node **children;
   int cpos; // children pos
   int csize; // children size

   // bloc Infos
   struct {
      struct Node **functions;
      int fpos;
      int fsize;

      Token **structs;
      int spos;
      int ssize;

      Token **vars;
      int vpos;
      int vsize;
   };
} Node;


typedef struct
{
   Token *token;
   Token *left;
   Token *right;
} Inst;



// GLOBAL
extern bool found_error;
extern bool did_pasm;
extern char *input;
extern Token **tokens;
extern Node *head;
extern Node *global;
extern int exe_pos;
extern Inst **OrgInsts;
extern Inst **insts;

extern Node **Gscoop;
extern Node *scoop;
extern int scoopSize;
extern int scoopPos;

extern int ptr;
extern struct _IO_FILE *asm_fd;
extern int str_index;
extern int bloc_index;
extern char *eregs[];
extern char *rregs[];

// ----------------------------------------------------------------------------
// Parsing
// ----------------------------------------------------------------------------

#if DEBUG_NEW_TOKEN
Token* new_token_(char *filename, int line, char *input, int s, int e, Type type, int space);
#else
Token* new_token(char *input, int s, int e, Type type, int space);
#endif
void add_token(Token *token);
void tokenize();
Node *expr();
Node *assign();
Node *logic();
Node *equality();
Node *comparison();
Node *add_sub();
Node *mul_div();
Node *dot();
Node *sign();
Node *brackets();
Node *prime();
Node *new_node(Token *token);
bool includes(Type to_find, ...);
Token *find(Type type, ...);
void generate_ast();
Node *new_function(Node *node);
Node *get_function(char *name);
Token *get_variable(char *name);
Token *new_variable(Token *token);
void free_node(Node *node);
Token *copy_token(Token *token);
Node *copy_node(Node *node);
Token *new_struct(Token *token);
Token *get_struct(char *name);
Token *get_struct_by_id(int id);
Token *is_struct(Token *token);
void add_attribute(Token *obj, Token *attr);
Node* add_child(Node *node, Node *child);
void add_variable(Node *bloc, Token *token);
void set_struct_size(Token *token);

// ----------------------------------------------------------------------------
// Code Generation
// ----------------------------------------------------------------------------

void generate(char *name);
Inst *new_inst(Token *token);
void enter_scoop(Node *node);
void exit_scoop();
void copy_insts();
bool compatible(Token *left, Token *right);
void initialize();
void asm_space(int space);
void finalize();
void pasm(char *fmt, ...);
Token* generate_ir(Node *node);
int calculate_padding(int offset, int alignment);
void generate_asm(char *name);
void to_default(Token *token, Type type);

// ----------------------------------------------------------------------------
// Utilities
// ----------------------------------------------------------------------------

void open_file(char *filename);
const char *to_string_(const char *filename, const int line, Type type);
void setName(Token *token, char *name);
void setReg(Token *token, char *creg);
bool within_space(int space);
bool check_error(const char *filename, const char *funcname, int line, bool cond, char *fmt, ...);
void free_memory();
void *allocate_func(int line, int len, int size);
void create_builtin(char *name, Type *params, Type retType);
char *strjoin(char *str0, char *str1, char *str2);
int sizeofToken(Token *token);
int alignofToken(Token *token);
void add_builtins();
Type getRetType(Node *node);
bool optimize_ir();
void config();

// ----------------------------------------------------------------------------
// Logs
// ----------------------------------------------------------------------------

int debug(char *conv, ...);
int pnode(Node *node, char *side, int space);
int ptoken(Token *token);
void print_ast();
void print_ir();
int print_value(Token *token);

