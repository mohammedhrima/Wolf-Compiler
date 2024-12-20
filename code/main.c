// HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

// MACROS
#define DEBUG 1
#define EXIT_STATUS 0
#define SPLIT "=================================================\n"
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define CYAN "\033[0;36m"
#define RESET "\033[0m"
#define LINE __LINE__
#define FUNC __func__
#define FILE __FILE__

#define TOKENIZE 1

#if TOKENIZE
#define AST 1
#endif

#if AST
#define IR 1
#else
#define IR 0
#endif

// curent max value 4
#define WITH_COMMENTS 1

#if IR
#define BUILTINS 0
#ifndef OPTIMIZE
#define OPTIMIZE 1
#endif
#define ASM 1
#endif

#ifndef DEBUG
#define DEBUG 1
#endif


// STRUCTS
typedef enum
{
  START = 11,
  ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN,
  EQUAL, NOT_EQUAL, LESS_EQUAL, MORE_EQUAL, LESS, MORE,
  ADD, SUB, MUL, DIV, MOD,
  AND, OR,
  RPAR, LPAR, COMA, DOT, DOTS,
  RETURN,
  IF, ELIF, ELSE, WHILE,
  FDEC, FCALL,
  VOID, INT, CHARS, CHAR, BOOL, FLOAT,
  STRUCT, ID, BLOC, END_BLOC,
  JNE, JE, JMP,
  END
} Type;

// PROTOTYPES
void check_error(const char *filename, const char *funcname, int line, bool cond, char *fmt, ...);
#define check(cond, fmt, ...) check_error(__FILE__, __func__, __LINE__, cond, fmt, ##__VA_ARGS__)
bool includes(Type *types, Type type);

char *to_string(Type type)
{
  switch (type)
  {
  case ASSIGN: return "ASSIGN";
  case ADD_ASSIGN: return "ADD ASSIGN";
  case SUB_ASSIGN: return "SUB ASSIGN";
  case MUL_ASSIGN: return "MUL ASSIGN";
  case DIV_ASSIGN: return "DIV ASSIGN";

  case EQUAL: return "EQUAL";
  case NOT_EQUAL: return "NOT EQUAL";
  case LESS_EQUAL: return "LESS THAN OR EQUAL";
  case MORE_EQUAL: return "MORE THAN OR EQUAL";
  case LESS: return "LESS THAN";
  case MORE: return "MORE THAN";

  case ADD: return "ADD";
  case SUB: return "SUB";
  case MUL: return "MUL";
  case DIV: return "DIV";
  case MOD: return "MOD";

  case AND: return "AND";
  case OR:  return "OR";

  case RPAR: return "R_PAR";
  case LPAR: return "L_PAR";
  case COMA: return "COMMA";
  case DOTS: return "DOTS";
  case DOT: return "DOT";

  case RETURN: return "RETURN";
  case IF: return "IF";
  case ELIF: return "ELIF";
  case ELSE: return "ELSE";
  case WHILE: return "WHILE";

  case FDEC: return "F_DEC";
  case FCALL: return "F_CALL";

  case INT: return "INT";
  case VOID: return "VOID";
  case CHARS: return "CHARS";
  case CHAR: return "CHAR";
  case BOOL: return "BOOL";
  case FLOAT: return "FLOAT";

  case STRUCT: return "STRUCT";
  case ID: return "ID";
  case END_BLOC: return "END_BLOC";
  case BLOC: return "BLOC";
  case JNE: return "JNE";
  case JE: return "JE";
  case JMP: return "JMP";
  case END: return "END";

  default: check(1, "Unknown type [%d]\n", type);
  }
  return NULL;
}

typedef struct
{
  Type type;
  Type retType;
  char *name;
  size_t ptr;
  bool declare;
  size_t space;
  bool remove;
  size_t reg;
  char *creg;
  size_t index;

  struct
  {
    // integer
    struct
    {
      long long value;
      int power;
      struct Int *next;
    } Int;
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
  };
} Token;

typedef struct Node
{
  struct Node *left;
  struct Node *right;
  Token *token;
} Node;

typedef struct
{
  char *value;
  Type type;
} Specials;

Specials *specials = (Specials[]){
{".", DOT}, {":", DOTS}, {"+=", ADD_ASSIGN}, {"-=", SUB_ASSIGN}, 
{"*=", MUL_ASSIGN}, {"/=", DIV_ASSIGN}, {"!=", NOT_EQUAL},
{"==", EQUAL}, {"is", EQUAL}, {"<=", LESS_EQUAL}, {">=", MORE_EQUAL},
{"<", LESS}, {">", MORE}, {"=", ASSIGN}, {"+", ADD}, {"-", SUB}, 
{"*", MUL}, {"/", DIV}, {"%", MOD}, {"(", LPAR}, {")", RPAR},
{",", COMA}, {"if", IF}, {"elif", ELIF}, {"else", ELSE}, 
{"while", WHILE}, {"func", FDEC}, {"return", RETURN}, 
{"and", AND}, {"&&", AND}, {"or", OR}, {"||", OR}, 
{0, (Type)0}};

Specials *dataTypes = (Specials[]){
{"int", INT}, {"bool", BOOL}, {"func", FDEC}, {"chars", CHARS},
{0, (Type)0},
};

// UTILS
void debug(char *fmt, ...)
{
#if DEBUG
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
#endif
}

bool found_error;
void check_error(const char *filename, const char *funcname, int line, bool cond, char *fmt, ...)
{
  if (!cond)
    return;
  found_error = true;
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "%sError:%s:%s:%d %s", RED, filename, funcname, line, RESET);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(1);
}



void ptoken(Token *token)
{
  debug("token: space [%.2d] [%s] ", token->space, to_string(token->type));
  switch (token->type)
  {
  case VOID: case CHARS: case CHAR: case INT: case BOOL: case FLOAT:
  {
    if (token->name) debug("name [%s] ", token->name);
    if (token->declare) debug("[declare] ");
    if (!token->name && !token->declare)
    {
      if (token->type == INT) debug("value [%lld] ", token->Int.value);
      else if (token->type == CHARS) debug("value [%s] ", token->Chars.value);
      else if (token->type == CHAR) debug("value [%c] ", token->Char.value);
      else if (token->type == BOOL) debug("value [%d] ", token->Bool.value);
      else if (token->type == FLOAT) debug("value [%f] ", token->Float.value);
      // else if(token->type == CHARS)
      //     debug(" value [%s]", token->Chars.value);
      // else if(token->type == BOOL)
      //     debug(" value [%d]", token->Bool.value);
    }
    break;
  }
  case FCALL: case FDEC: case ID:
    debug("name [%s] ", token->name);
    break;
  default:
  {
    // for (int i = 0; specials[i].value; i++)
    // {
    //     if (specials[i].type == token->type)
    //     {
    //         debug("[%s]", specials[i].value);
    //         break;
    //     }
    // }
    break;
  }
  }
  token->remove ? debug(" [remove]\n") : debug("\n");
}

void *allocate_func(size_t line, size_t len, size_t size)
{
  void *ptr = calloc(len, size);
  check(!ptr, "allocate did failed in line %zu\n", line);
  return ptr;
}

#define allocate(len, size) allocate_func(LINE, len, size)

char *open_file(char *filename)
{
  struct _IO_FILE *file = fopen(filename, "r");
  check(!filename, "failed to open file %s\n", filename);
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *input = allocate((size + 1), sizeof(char));
  if (input) fread(input, size, sizeof(char), file);
  fclose(file);
  return input;
}

// TOKENIZE
Token **tokens;
void add_token(Token *token)
{
  static size_t pos;
  static size_t len;
  if (len == 0)
  {
    len = 10;
    tokens = allocate(len, sizeof(Token *));
  }
  else if (pos + 1 == len)
  {
    Token **tmp = allocate(len * 2, sizeof(Token *));
    memcpy(tmp, tokens, len * sizeof(Token *));
    free(tokens);
    tokens = tmp;
    len *= 2;
  }
  tokens[pos++] = token;
}

Token *new_token(char *input, size_t s, size_t e, Type type, size_t space)
{
  Token *new = allocate(1, sizeof(Token));
  new->type = type;
  new->space = space;
  switch (type)
  {
  case INT:
  {
    while (s < e) new->Int.value = new->Int.value * 10 + input[s++] - '0';
    break;
  }
  case BLOC: case ID: case JMP: case JE: case JNE:
  {
    if(e > s)
    {
      new->name = allocate(e - s + 1, sizeof(char));
      strncpy(new->name, input + s, e - s);
      if (strcmp(new->name, "True") == 0)
      {
        free(new->name);
        new->name = NULL;
        new->type = BOOL;
        new->Bool.value = true;
      }
      else if (strcmp(new->name, "False") == 0)
      {
        free(new->name);
        new->name = NULL;
        new->type = BOOL;
        new->Bool.value = false;
      }
    }
    break;
  }
  case CHARS:
  {
    if(e > s)
    {
      new->Chars.value = allocate(e - s + 1, sizeof(char));
      strncpy(new->Chars.value, input + s, e - s);
    }
    break;
  }
  case CHAR:
  {
    new->Char.value = input[s];
    break;
  }
  default:
    break;
  }
  add_token(new);
  return new;
}

Token *copy_token(Token *token)
{
  if (token == NULL) return NULL;
  Token *new = allocate(1, sizeof(Token));
  memcpy(new, token, sizeof(Token));
  // TODO: check all values that can be copied example: name ...
  if (token->name) new->name = strdup(token->name);
  if (token->Chars.value) new->Chars.value = strdup(token->Chars.value);
  add_token(new);
  return new;
}

void tokenize(char *input)
{
  if (!TOKENIZE)
    return;
  debug(GREEN "=========== TOKENIZE ===========\n" RESET);
  // size_t len = 10;
  // size_t pos = 0;
  size_t i = 0;
  size_t space = 0;
  bool inc_space = true;
  while (input[i] && !found_error)
  {
    size_t s = i;
    if (isspace(input[i]))
    {
      if (input[i] == '\n')
      {
        inc_space = true;
        space = 0;
      }
      else if (inc_space)
        space++;
      i++;
      continue;
    }
    // TODO: handle new lines inside comment
    else if (strncmp(input + i, "/*", 2) == 0) 
    {
      i += 2;
      while (input[i] && input[i + 1] && strncmp(input + i, "*/", 2)) i++;
      check(input[i + 1] && strncmp(input + i, "*/", 2), "expected '*/'\n");
      i += 2;
      continue;
    }
    else if (strncmp(input + i, "//", 2) == 0)
    {
      while (input[i] && input[i] != '\n') i++;
      continue;
    }
    inc_space = false;
    bool found = false;
    for (int j = 0; specials[j].value; j++)
    {
      if (strncmp(specials[j].value, input + i, strlen(specials[j].value)) == 0)
      {
        new_token(NULL, 0, 0, specials[j].type, space);
        found = true;
        i += strlen(specials[j].value);
        if(strcmp(specials[j].value, ":") == 0) space++;
        break;
      }
    }
    for (int j = 0; !found && dataTypes[j].value; j++)
    {
      if (strncmp(dataTypes[j].value, input + i, strlen(dataTypes[j].value)) == 0)
      {
        Token *token = new_token(NULL, 0, 0, dataTypes[j].type, space);
        token->declare = true;
        free(token->name);
        token->name = NULL;
        found = true;
        i += strlen(dataTypes[j].value);
        break;
      }
    }
    if (found)
      continue;
    if (input[i] && strchr("\"\'", input[i]))
    {
      i++;
      while(input[i] && input[i] != '"') i++;
      check(input[i] != '"', "Expected '\"'");
      i++;
      new_token(input, s, i, CHARS, space);
      continue;
    }
    if (isalpha(input[i]))
    {
      while (isalnum(input[i]))
        i++;
      new_token(input, s, i, ID, space);
      continue;
    }
    if (isdigit(input[i]))
    {
      while (isdigit(input[i]))
        i++;
      new_token(input, s, i, INT, space);
      continue;
    }
    check(input[i], "Syntax error <%c>\n", input[i]);
  }
  new_token(input, 0, 0, END, space);
  for (size_t i = 0; tokens[i]; i++)
    ptoken(tokens[i]);
}

void free_token(Token *token)
{
  if (token->name) free(token->name);
  if (token->Chars.value) free(token->Chars.value);
  free(token);
}

// AST
Node *expr();
Node *assign();
Node *logic();
Node *equality();
Node *comparison();
Node *add_sub();
Node *mul_div();
Node *dot();
Node *sign();
Node *prime();

size_t exe_pos;
Token *find(Type type, ...)
{
  va_list ap;
  va_start(ap, type);
  while (type)
  {
    if (type == tokens[exe_pos]->type)
      return tokens[exe_pos++];
    type = va_arg(ap, Type);
  }
  return NULL;
};

void pnode(Node *node, char *side, int space)
{
  if (node)
  {
    int i = 0;
    while (i < space)
      i += printf(" ");
    if (side)
      debug("%s", side);
    debug("node: ");
    if (node->token)
      ptoken(node->token);
    else
      debug("(NULL)\n");
    pnode(node->left, "L:", space + 2);
    pnode(node->right, "R:", space + 2);
  }
}

Node *new_node(Token *token)
{
  Node *new = allocate(1, sizeof(Node));
  new->token = token;
  // pnode(new, NULL, 0);
  return new;
}

Node *copy_node(Node *node)
{
  Node *new = allocate(1, sizeof(Node));
  new->token = copy_token(node->token);
  if (node->left)
    new->left = copy_node(node->left);
  if (node->right)
    new->right = copy_node(node->right);
  return new;
}

void free_node(Node *node)
{
  if (node)
  {
    free_node(node->left);
    free_node(node->right);
    free(node);
  }
}

Node *expr()
{
  return assign();
}

Node *assign()
{
  Node *left = logic();
  Token *token;
  while ((token = find(ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, 0)))
  {
    Node *node = new_node(token);
    Node *right = logic();
    if(token->type != ASSIGN)
    {
      switch(token->type)
      {
        case ADD_ASSIGN: token->type = ADD; break;
        case SUB_ASSIGN: token->type = SUB; break;
        case MUL_ASSIGN: token->type = MUL; break;
        case DIV_ASSIGN: token->type = DIV; break;
        default: break;
      }
      Node *tmp = new_node(new_token(NULL, 0, 0, token->type, node->token->space));
      node->token->type = ASSIGN;
      tmp->left = copy_node(left);
      tmp->left->token->declare = false;
      tmp->right = right;
      right = tmp;
    }
    node->left = left;
    node->right = right;
    left = node;
  }
  return left;
}

Node *logic()
{
  Node *left = equality();
  Token *token;
  while ((token = find(AND, OR, 0)))
  {
    Node *node = new_node(token);
    node->left = left;
    node->right = equality();
    left = node;
  }
  return left;
}

// TODO: handle NOT operator
Node *equality()
{
  Node *left = comparison();
  Token *token;
  while ((token = find(EQUAL, NOT_EQUAL, 0)))
  {
    Node *node = new_node(token);
    node->left = left;
    node->right = comparison();
    left = node;
  }
  return left;
}

Node *comparison()
{
  Node *left = add_sub();
  Token *token;
  while ((token = find(LESS, MORE, LESS_EQUAL, MORE_EQUAL, 0)))
  {
    Node *node = new_node(token);
    node->left = left;
    node->right = add_sub();
    left = node;
  }
  return left;
}

Node *add_sub()
{
  Node *left = mul_div();
  Token *token;
  while ((token = find(ADD, SUB, 0)))
  {
    Node *node = new_node(token);
    node->left = left;
    node->right = mul_div();
    left = node;
  }
  return left;
}

Node *mul_div()
{
  Node *left = dot();
  Token *token;
  while ((token = find(MUL, DIV, 0))) // TODO: handle modulo
  {
    Node *node = new_node(token);
    node->left = left;
    node->right = dot();
    left = node;
  }
  return left;
}

Node *dot()
{
  return sign();
}

Node *sign()
{
  return prime();
}

Node *prime()
{
  Token *token = NULL;
  Node *node = NULL;
  if ((token = find(ID, INT, BOOL, CHARS, 0)))
  {
    if (token->declare) // int num
    {
      debug("is declare %d\n", token->declare);
      Token *tmp = find(ID, 0);
      token->name = strdup(tmp->name);
      ptoken(token);
      check(!token, "Expected variable name after [%s] symbol\n", to_string(token->type));
    }
    else if (token->type == ID && token->name && find(LPAR, 0)) // id
    {
      node = new_node(token);
      if (strcmp(token->name, "main") == 0)
      {
        check(!find(RPAR, 0), "expected ) after main declaration", "");
        check(!find(DOTS, 0), "expected : after main() declaration", "");
        token->type = FDEC;
        size_t space = token->space;
        Node *curr = node;
        Node *last = node;
        while (tokens[exe_pos]->space > space && tokens[exe_pos]->type != END)
        {
          curr->right = new_node(NULL);
          curr = curr->right;
          curr->left = expr();
          last = curr->left;
        }
        if(last->token->type != RETURN)
        {
          curr->right = new_node(NULL);
          curr = curr->right;
          curr->left = new_node(new_token(NULL, 0, 0, RETURN, node->token->space + 4));
          curr->left->left = new_node(new_token(NULL, 0, 0, INT, node->token->space + 4));
        }
        return node;
      }
      else
      {
        token->type = FCALL;
        Node *tmp = node;
        while(!find(RPAR, END, 0))
        {
          tmp->left = expr();
          find(COMA, 0);
          // tmp->
          tmp->right = new_node(NULL);
          tmp = tmp->right;
        }
        return node;
      }
    }
    node = new_node(token);
  }
  else if((token = find(FDEC, 0)))
  {
    node = new_node(token);

    Node *func_name = prime();

    check(!func_name->token || !func_name->token->declare, "expected data type after func declaration");

    node->left = new_node(NULL);
    node->left->left = func_name;

    node->token->retType = func_name->token->type;
    node->token->name = func_name->token->name;
    func_name->token->name = NULL;

    check(!find(LPAR, 0), "expected ( after function declaration");
    check(!find(RPAR, 0), "expected ) after function declaration");
    check(!find(DOTS, 0), "Expected : after function declaration");

    Node *curr = node;
    while(tokens[exe_pos]->space > node->token->space)
    {
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = expr();
    }
    return node;
  }
  else if((token = find(IF, 0)))
  {
    node = new_node(token);
    node->left = new_node(NULL);

    Node *tmp = node;
    tmp = tmp->left;

    tmp->left = expr(); // if condition
    check(!find(DOTS, 0), "Expected : after if condition\n", "");
    tmp->right = new_node(NULL);
    tmp = tmp->right;

    // if bloc code
    while (tokens[exe_pos]->space > node->token->space) 
    {
      tmp->left = expr();
      tmp->right = new_node(NULL);
      tmp = tmp->right;
    }

    tmp = node;
    while (includes((Type[]){ELSE, ELIF, 0}, tokens[exe_pos]->type) && tokens[exe_pos]->space == node->token->space)
    {
      token = tokens[exe_pos++];
      tmp->right = new_node(NULL);
      tmp = tmp->right;
      tmp->left = new_node(token);
      if (token->type == ELIF)
      {
        Node *tmp0 = tmp->left;
        tmp0->left = expr();
        check(!find(DOTS, 0), "expected : after else");
        tmp0->right = new_node(NULL);
        tmp0 = tmp0->right;
        while (tokens[exe_pos]->space > token->space)
        {
          tmp0->left = expr();
          tmp0->right = new_node(NULL);
          tmp0 = tmp0->right;
        }
      }
      else if (token->type == ELSE)
      {
        check(!find(DOTS, 0), "expected dots");
        Node *tmp0 = tmp->left;
        tmp0->right = new_node(NULL);
        tmp0 = tmp0->right;
        while (tokens[exe_pos]->space > token->space)
        {
          tmp0->left = expr();
          tmp0->right = new_node(NULL);
          tmp0 = tmp0->right;
        }
        break;
      }
    }
  }
  else if ((token = find(WHILE, 0)))
  {
    node = new_node(token);
    node->left = expr();
    check(!find(DOTS, 0), "Expected : after while condition\n", "");
    Node *tmp = node;
    while (tokens[exe_pos]->type != END && tokens[exe_pos]->space > token->space)
    {
      tmp->right = new_node(NULL);
      tmp = tmp->right;
      tmp->left = expr();
    }
  }
#if 0
  else if ((token = find(FDEC, 0)))
  {
    node = new_node(token);
    node->left = new_node(NULL);
    node->left->left = expr();
    check(!node->left->left->token || !node->left->left->token->declare, "declaraing function\n", "");
    node->token->retType = node->left->left->token->type;
    node->token->name = node->left->left->token->name;
    node->left->left->token->name = NULL;
    check(!find(LPAR, 0), "Expected ( after function declaration\n", "");
    check(!find(RPAR, 0), "Expected ( after function declaration\n", "");
  }
#endif
  else if((token = find(RETURN, 0)))
  {
    // TODO: chec kif return type is compatible with function
    node = new_node(token);
    node->left = expr();
  }
  else if ((token = find(LPAR, 0)))
  {
    node = expr();
    check(!find(RPAR, 0), "Expected )\n", "");
  }
  else if (find(END, 0)) exe_pos--;
  else check(1, "Unexpected token has type %s\n", to_string(tokens[exe_pos]->type));
  return node;
}

Node *ast()
{
  if (!AST) return NULL;
  debug(GREEN "===========   AST    ===========\n" RESET);
  Node *head = new_node(NULL);
  Node *curr = head;
  curr->left = expr();
  while (curr->left)
  {
    curr->right = new_node(NULL);
    curr = curr->right;
    curr->left = expr();
  }
  debug(GREEN "=========== PRINT AST ==========\n" RESET);
  curr = head;
  while (curr)
  {
    pnode(curr->left, NULL, 0);
    curr = curr->right;
  }
  return head;
}

// IR

typedef struct
{
  char *name;

  Node **functions;
  size_t fpos;
  size_t fsize;

  Token **vars;
  size_t vpos;
  size_t vsize;

} Scoop;

Scoop *Gscoop;
Scoop *scoop;
size_t scoopSize;
size_t scoopPos;

void enter_scoop(char *name)
{
  debug(CYAN "Enter Scoop [%s]\n" RESET, name);
  if (Gscoop == NULL)
  {
    scoopSize = 10;
    Gscoop = allocate(scoopSize, sizeof(Scoop));
  }
  else if (scoopPos + 1 == scoopSize)
  {
    Scoop *tmp = allocate(scoopSize * 2, sizeof(Scoop));
    memcpy(tmp, Gscoop, scoopPos * sizeof(Scoop));
    scoopSize *= 2;
    free(Gscoop);
    Gscoop = tmp;
  }
  scoopPos++;
  Gscoop[scoopPos] = (Scoop){};
  Gscoop[scoopPos].name = name;
  scoop = &Gscoop[scoopPos];
}

void exit_scoop()
{
  debug(CYAN "Exit Scoop [%s]\n" RESET, scoop->name);
  free(scoop->functions);
  free(scoop->vars);
  scoop[scoopPos] = (Scoop){};
  scoop = &Gscoop[--scoopPos];
}

typedef struct
{
  Token *token;
  Token *left;
  Token *right;
} Inst;

Inst **OrgInsts;
Inst **insts;

void clone_insts()
{
  size_t pos = 0;
  size_t len = 100;
  free(insts);
  insts = allocate(len, sizeof(Inst *));

  for (size_t i = 0; OrgInsts[i]; i++)
  {
    if (!OrgInsts[i]->token->remove)
    {
      insts[pos++] = OrgInsts[i];
      if (pos + 1 == len)
      {
        Inst **tmp = allocate((len *= 2), sizeof(Inst *));
        memcpy(tmp, insts, pos * sizeof(Inst *));
        free(insts);
        insts = tmp;
      }
    }
  }
}

void add_inst(Inst *inst)
{
  static size_t pos;
  static size_t len;
  if (len == 0)
  {
    len = 10;
    OrgInsts = allocate(len, sizeof(Inst *));
  }
  else if (pos + 1 == len)
  {
    Inst **tmp = allocate(len * 2, sizeof(Inst *));
    memcpy(tmp, OrgInsts, len * sizeof(Inst *));
    free(OrgInsts);
    OrgInsts = tmp;
    len *= 2;
  }
  OrgInsts[pos++] = inst;
}

int sizeofToken(Token *token)
{
  switch (token->type)
  {
  case INT: return sizeof(int);
  case FLOAT: return sizeof(float);
  case CHARS: return sizeof(char *);
  case CHAR: return sizeof(char);
  case BOOL: return sizeof(bool);
  // case struct_: return token->size;
  default: check(1, "add this type [%s]\n", to_string(token->type));
  }
  return 0;
}

Node *new_function(Node *node)
{
  debug("new_func %s in %s scoop\n", node->token->name, scoop->name);
#if 0
    char *builtins[] = {"output", 0};
    for(int i = 0; builtins[i]; i++)
    {
        if(strcmp(node->token->name, builtins[i]) == 0)
        {
            error("%s is a built in function\n", node->token->name);
            exit(1);
        }
    }
#endif
  for (size_t i = 0; i < scoop->fpos; i++)
  {
    Node *func = scoop->functions[i];
    bool cond = strcmp(func->token->name, node->token->name) == 0;
    check(cond, "Redefinition of %s\n", node->token->name);
  }
  if (scoop->functions == NULL)
  {
    scoop->fsize = 10;
    scoop->functions = allocate(scoop->fsize, sizeof(Node *));
  }
  else if (scoop->fpos + 1 == scoop->fsize)
  {
    scoop->fsize *= 2;
    Node **tmp = allocate(scoop->fsize, sizeof(Node *));
    memcpy(tmp, scoop->functions, scoop->fpos * sizeof(Node *));
    free(scoop->functions);
    scoop->functions = tmp;
  }
  scoop->functions[scoop->fpos++] = node;
  return node;
}

Node *get_function(char *name)
{
  // TODO: remove output from here
  debug("get_func %s in %s scoop\n", name, scoop->name);

#if 0
  char *builtins[] = {"output", 0};
  for (int i = 0; builtins[i]; i++)
    if (strcmp(name, builtins[i]) == 0)
      return NULL;
  for (size_t i = 0; i < builtins_pos; i++)
  {
    if (strcmp(name, builtins_functions[i]->token->name) == 0)
      return builtins_functions[i];
  }
  // if(strcmp(name, "write") == 0)
  // {
  //     Node *func = new_node(new_token("write", 0, 5, 0, fdec_));
  //     Node *curr = func;
  //     curr->left = new_node(NULL);
  //     curr = curr->left;
  //     curr->right = new_node(NULL);
  //     curr = curr->right;
  //     curr->left = new_node(new_token(0, 0, 0, 0, int_));
  //     curr->right = new_node(NULL);
  //     curr = curr->right;
  //     curr->left = new_node(new_token(0, 0, 0, 0, chars_));
  //     curr->right = new_node(NULL);
  //     curr = curr->right;
  //     curr->left = new_node(new_token(0, 0, 0, 0, int_));
  //     return func;
  // }
#endif
  for (size_t j = scoopPos; j >= 0; j--)
  {
    Scoop *scoop = &Gscoop[j];
    for (size_t i = 0; i < scoop->fpos; i++)
    {
      Node *func = scoop->functions[i];
      if (strcmp(func->token->name, name) == 0)
        return func;
    }
  }
  check(1, "'%s' Not found\n", name);
  return NULL;
}

Token *new_variable(Token *token)
{
  debug(CYAN "variable [%s] in [%s] scoop\n" RESET, token->name, scoop->name);
  for (size_t i = 0; i < scoop->vpos; i++)
  {
    Token *curr = scoop->vars[i];
    if (strcmp(curr->name, token->name) == 0) check(1, "Redefinition of %s\n", token->name);
  }
  if (scoop->vars == NULL)
  {
    scoop->vsize = 10;
    scoop->vars = allocate(scoop->vsize, sizeof(Token *));
  }
  else if (scoop->vpos + 1 == scoop->vsize)
  {
    Token **tmp = allocate(scoop->vsize *= 2, sizeof(Token *));
    memcpy(tmp, scoop->vars, scoop->vpos * sizeof(Token *));
    free(scoop->vars);
    scoop->vars = tmp;
  }
  scoop->vars[scoop->vpos++] = token;
  return token;
}

Token *get_variable(char *name)
{
  debug(CYAN "get variable [%s] from [%s] scoop\n" RESET, name, scoop->name);
  for (size_t i = 0; i < scoop->vpos; i++)
  {
    Token *curr = scoop->vars[i];
    if (strcmp(curr->name, name) == 0) return curr;
  }
  check(1, "%s not found\n", name);
  return NULL;
}

size_t ptr;
Inst *new_inst(Token *token)
{
  static size_t reg;

  Inst *new = allocate(1, sizeof(Inst));
  new->token = token;
  if (token->name && token->declare)
  {
    new_variable(token);
    token->ptr = (ptr += sizeofToken(token));
    token->reg = ++reg;
  }
  else if (includes((Type[]){ADD, SUB, MUL, DIV, 0}, token->type))
  {
    token->reg = ++reg;
  }
  else if (includes((Type[]){EQUAL, NOT_EQUAL, LESS, MORE, LESS_EQUAL, MORE_EQUAL, 0}, token->type))
  {
    token->reg = ++reg;
  }
  else if (includes((Type[]){FCALL, 0}, token->type))
  {
    token->reg = ++reg;
  }
  debug("inst: ");
  ptoken(new->token);
  add_inst(new);
  return new;
}
// TODO: implement it
bool are_compatible(Token *left, Token *right)
{
  return true;
}

size_t bloc_index;
size_t str_index;
Token *generate_ir(Node *node)
{
  Inst *inst = NULL;
  switch (node->token->type)
  {
  case ID:
  {
    Token *token = get_variable(node->token->name);
    return token;
    break;
  }
  case INT: case BOOL: case CHARS:
  {
    inst = new_inst(node->token);
    break;
  }
  case ASSIGN: 
  case ADD: case SUB: case MUL: case DIV:
  case EQUAL: case NOT_EQUAL: 
  case LESS: case MORE: case LESS_EQUAL: case MORE_EQUAL:
  {
    Token *left = generate_ir(node->left);
    Token *right = generate_ir(node->right);
    check(!are_compatible(left, right), "incompatible type for %s op\n", to_string(node->token->type));
    inst = new_inst(node->token);
    inst->left = left;
    inst->right = right;
    switch (node->token->type)
    {
    case ASSIGN:
      node->token->retType = left->type;
      break;
    case ADD: case SUB: case MUL: case DIV:
    {
      // TODO: verify this
      // if(left->type != INT && left->type != FLOAT)
      // {
      //     char *c = to_string(node->token->type);
      //     char *l = to_string(left->type);
      //     char *r = to_string(right->type);
      //     check(1, "Invalid operation [%s] between [%s] and [%s]\n", c, l, r);
      // }
      node->token->retType = left->type;
      if (right->type == INT) node->token->creg = "eax";
      else if (right->type == FLOAT) node->token->creg = "xmm0";
      break;
    }
    case NOT_EQUAL: case EQUAL: case LESS: 
    case MORE: case LESS_EQUAL: case MORE_EQUAL:
    {
      node->token->retType = BOOL;
      node->token->index = ++bloc_index;
      // debug(RED "this is [%d]\n" RESET, node->token->reg);
      break;
    }
    default: break;
    }
    break;
  }

    // {
    // 	Token *left = generate_ir(node->left);
    // 	Token *right = generate_ir(node->right);
    // 	check(!are_compatible(left, right), "incompatible type for %s op\n", to_string(node->token->type));
    // 	inst = new_inst(node->token);
    // 	inst->left = left;
    // 	inst->right = right;
    // 	inst->token->retType = BOOL;
    // 	break;
    // }
  case FDEC:
  {
    new_function(node);
    enter_scoop(node->token->name);
    inst = new_inst(node->token);
    Node *curr = node->right;
    while (curr)
    {
      generate_ir(curr->left);
      curr = curr->right;
    }
    Token *new = new_token(NULL, 0, 0, END_BLOC, node->token->space);
    new->name = strdup(node->token->name);
    new_inst(new);
    exit_scoop();
    break;
  }
  case FCALL:
  {
    // Node *func = get_function(node->token->name);
    new_inst(node->token);
    return node->token;
    break;
  }
  case IF:
  {
    Node *curr = node->left;

    Inst *tmp = new_inst(copy_token(node->token));
    tmp->token->name = strdup("if");
    tmp->token->type =  BLOC;
    tmp->token->index = ++bloc_index;

    generate_ir(curr->left); // TODO: check if it's boolean
    node->token->type = JNE;
    node->token->name = strdup("endif");
    node->token->index = ++bloc_index;

    Token *lastInst = copy_token(node->token);
    new_inst(lastInst); // jne to endif
    curr = curr->right;
    
    // if code bloc
    while (curr->left) 
    {
      generate_ir(curr->left);
      curr = curr->right;
    }

    Inst *endInst = NULL;
    if (node->right)
    {
      endInst = new_inst(new_token("endif", 0, 5, JMP, node->token->space));
      endInst->token->index = node->token->index;
    }

    curr = node->right;
    while (curr)
    {
      if (curr->left->token->type == ELIF)
      {
        curr->left->token->index = ++bloc_index;
        curr->left->token->type = BLOC;
        curr->left->token->name = strdup("elif");

        {
          free(lastInst->name);
          lastInst->name = strdup("elif");
          lastInst->index = curr->left->token->index;
          lastInst = copy_token(lastInst);
        }

        new_inst(curr->left->token);
        Node *tmp = curr->left;
        generate_ir(tmp->left); // elif condition, TODO: check is boolean

        new_inst(lastInst);

        tmp = tmp->right;
        while (tmp->left)
        {
          generate_ir(tmp->left);
          tmp = tmp->right;
        }
      }
      else if (curr->left->token->type == ELSE)
      {
        curr->left->token->index = ++bloc_index;
        curr->left->token->type = BLOC;
        curr->left->token->name = strdup("else");
        new_inst(curr->left->token);

        {
          free(lastInst->name);
          lastInst->name = strdup("else");
          lastInst->index = curr->left->token->index;
          lastInst = copy_token(lastInst);
        }

        Node *tmp = curr->left;
        tmp = tmp->right;
        while (tmp->left)
        {
          generate_ir(tmp->left);
          tmp = tmp->right;
        }
        break;
      }

      free(lastInst->name);
      lastInst->name = strdup("endif");
      lastInst->index = node->token->index;

      if (curr->right) // to not add a jmp in the last statement to avoid
                       // jne endifX
                       // endifX:
      {
        endInst = new_inst(new_token("endif", 0, 5, JMP, node->token->space));
        endInst->token->index = node->token->index;
      }
      curr = curr->right;
    }

    Token *new = new_token("endif", 0, 5, BLOC, node->token->space);
    new->index = node->token->index;
    new_inst(new);
    // free_token(lastInst);
    return node->left->token;
    break;
  }
  case WHILE:
  {
    node->token->type = BLOC;
    node->token->name = strdup("while");
    node->token->index = ++bloc_index;
    inst = new_inst(node->token);

    generate_ir(node->left); // TODO: check if it's boolean
    Token *end = copy_token(node->token);
    end->type = JNE;
    if (end->name) free(end->name);
    end->name = strdup("endwhile");
    new_inst(end);

    Node *curr = node->right;
    while (curr) // while code bloc
    {
      generate_ir(curr->left);
      curr = curr->right;
    }

    Token *lastInst = copy_token(node->token);
    lastInst->type = JMP;
    free(lastInst->name);
    lastInst->name = strdup("while");
    new_inst(lastInst); // jmp back to while loop

    lastInst = copy_token(node->token);
    lastInst->type = BLOC;
    free(lastInst->name);
    lastInst->name = strdup("endwhile");
    new_inst(lastInst); // end while bloc
    break;
  }
  case RETURN:
  {
    Token *left = generate_ir(node->left);
    inst = new_inst(node->token);
    inst->left = left;
    break;
  }
  default: break;
  }
  return inst->token;
}

void print_ir()
{
  debug(GREEN "==========   PRINT IR  =========\n" RESET);
  int i = 0;
  clone_insts();
  for (i = 0; insts[i]; i++)
  {
    Token *curr = insts[i]->token;
    Token *left = insts[i]->left;
    Token *right = insts[i]->right;
    curr->reg ? debug("r%.2d:", curr->reg) : debug("rxx:");
    int k = 0;
    while (k < curr->space) k += printf(" ");
    switch (curr->type)
    {
    case ASSIGN:
    {
      // debug("%s\n", right->reg ? "has reg" : "no reg");
      curr->reg = left->reg;
      debug("[%-6s] ", to_string(curr->type));
      debug("r%.2d (%s) to ", left->reg, left->name);

      if (right->reg) debug("r%.2d (%s)", right->reg, right->name ? right->name : "");
      else
      {
        // if(right->type == ADD) debug("<r%.2d>\n", right->reg);
        switch (right->type)
        { // TODO: handle the other cases
        case INT: debug("%lld", right->Int.value); break;
        case BOOL: debug("%s", right->Bool.value ? "True" : "False"); break;
        case FLOAT: debug("%f", right->Float.value); break;
        case CHAR: debug("%c", right->Char.value); break;
        case CHARS: debug("%s", right->Chars.value); break;
        default:
        {
          check(1, "handle this case [%s]\n", to_string(right->type));
          break;
        }
        }
      }
      break;
    }
    case ADD: case SUB: case MUL: case DIV:
    case EQUAL: case NOT_EQUAL: case LESS: case MORE: case LESS_EQUAL: case MORE_EQUAL:
    {
      debug("[%-6s] ", to_string(curr->type));
      if (left->reg)
        debug("r%.2d", left->reg);
      else
      {
        switch (left->type)
        { // TODO: handle the other cases
        case INT: debug("%lld", left->Int.value); break;
        case BOOL: debug("%s", left->Bool.value ? "True" : "False"); break;
        case FLOAT: debug("%f", left->Float.value); break;
        case CHAR: debug("%c", left->Char.value); break;
        case CHARS: debug("%s", left->Chars.value); break;
        default: check(1, "handle this case [%s]\n", to_string(left->type));
        }
      }
      if (left->name) debug(" (%s)", left->name);
      debug(" to ");
      if (right->reg) debug("r%.2d", right->reg);
      else
      {
        switch (right->type)
        { // TODO: handle the other cases
        case INT: debug("%lld", right->Int.value); break;
        case BOOL: debug("%s", right->Bool.value ? "True" : "False"); break;
        case FLOAT: debug("%f", right->Float.value); break;
        case CHAR: debug("%c", right->Char.value); break;
        case CHARS: debug("%s", right->Chars.value); break;
        default: check(1, "handle this case [%s]\n", to_string(right->type)); break;
        }
      }
      if (right->name) debug(" (%s)", right->name);
      break;
    }
    case INT: case BOOL: case CHARS:
    {
      debug("[%-6s] ", to_string(curr->type));
      if (curr->declare) debug("declare [%s] PTR=[%zu]", curr->name, curr->ptr);
      else if (curr->name) debug("variable %s ", curr->name);
      else if (curr->type == INT) debug("value %lld ", curr->Int.value);
      else if (curr->type == BOOL) debug("value %s ", curr->Bool.value ? "True" : "False");
      // else if(curr->type == CHAR) debug("value %c ", curr->Char.value);
      // else if(curr->type == BOOL) debug("value %s ", curr->Bool.value ? "True" : "False");
      // else if(curr->type == FLOAT)
      // {
      //     curr->index = ++float_index;
      //     debug("value %f ", curr->Float.value);
      // }
      else if(curr->type == CHARS)
      {
          curr->index = ++str_index;
          debug("value %s in STR%zu ", curr->Chars.value, curr->index);
      }
      else check(1, "handle this case in generate ir\n", "");
      break;
    }
    case JMP: debug("jmp to [%s]", curr->name); break;
    case JNE: debug("jne to [%s]", curr->name); break;
    case FCALL: debug("call [%s]", curr->name); break;
    case BLOC: case FDEC: debug("[%s] bloc", curr->name); break;
    case END_BLOC: debug("[%s] endbloc", curr->name); break;
    case RETURN: debug("return"); break;
    default: debug(RED "handle [%s]"RESET, to_string(curr->type)); break;
    }
    // if(curr->remove) debug(" remove");
    debug("\n");
  }
  debug("total instructions [%d]\n", i);
}

bool includes(Type *types, Type type)
{
  for (int i = 0; types[i]; i++)
    if (types[i] == type)
      return true;
  return false;
}

/*
+ TODO: check optimization when doing it in sub scoops
+ example:
    int x = 10
    if x == 2: (this will caus problem now)
        x = 3
*/

bool optimize_ir(int op)
{
  bool optimize = false;
  if(op == 0)
  {
    debug(CYAN "OP[%d] calculate operations on constants\n" RESET, op);
    for (int i = 0; insts[i]; i++)
    {
      Token *token = insts[i]->token;
      Token *left = insts[i]->left;
      Token *right = insts[i]->right;
      if (includes((Type[]){ADD, SUB, MUL, DIV, 0}, token->type))
      {
        Type types[] = {INT, FLOAT, CHAR, 0};
        // TODO: check if left nad right are compatible
        if (includes(types, left->type) && includes(types, right->type) && !left->name && !right->name)
        {
          switch (left->type)
          {
          case INT:
            switch (token->type)
            {
            case ADD: token->Int.value = left->Int.value + right->Int.value; break;
            case SUB: token->Int.value = left->Int.value - right->Int.value; break;
            case MUL: token->Int.value = left->Int.value * right->Int.value; break;
            case DIV: token->Int.value = left->Int.value / right->Int.value; break;
            default: break;
            }
            break;
          case FLOAT:
            // left->index = 0;
            // right->index = 0;
            // token->index = ++float_index;
            // switch (token->type)
            // {
            // case ADD: token->Float.value = left->Float.value + right->Float.value; break;
            // case SUB: token->Float.value = left->Float.value - right->Float.value; break;
            // case MUL: token->Float.value = left->Float.value * right->Float.value; break;
            // case DIV: token->Float.value = left->Float.value / right->Float.value; break;
            // default: break;
            // }
            break;
          // case CHARS:
          //     switch(token->type)
          //     {
          //     case add_: token->Chars.value = strjoin(left, right); break;
          //     default:
          //         error("Invalid %s op in chars\n", to_string(token->type)); break;
          //     }
          default:
            check(1, "handle this case\n", "");
            break;
          }
          token->type = left->type;
          left->remove = true;
          right->remove = true;
          token->reg = 0;
          token->creg = NULL;
          clone_insts();
          i = 0;
          optimize = true;
        }
      }
    }
  }
  else if(op == 1)
  {
    // TODO: to be checked
    debug(CYAN "OP[%d] calculate operations on constants\n" RESET, op);
    int i = 1;
    while (insts[i])
    {
      Token *curr_token = insts[i]->token;
      Token *curr_left = insts[i]->left;
      Token *curr_right = insts[i]->right;

      Token *prev_token = insts[i - 1]->token;
      Token *prev_left = insts[i - 1]->left;
      Token *prev_right = insts[i - 1]->right;

      //  TODO: handle string also here X'D ma fiyach daba
      if(curr_token->type == ADD && prev_token->type == ADD)
      {
        if (curr_left == prev_token && !prev_right->name && !curr_right->name)
        {
          // prev_right->type = INT;
          curr_token->remove = true;
          prev_right->Int.value += curr_right->Int.value;
          i = 1;
          clone_insts();
          optimize = true;
          continue;
        }
        // else
        // if(curr_right == prev_token && !prev_left->name && !curr_left->name)
        // {
        //   // prev_r->type = INT;
        //   // curr_token->remove = true;
        //   prev_left->Int.value += curr_left->Int.value;
        //   i = 1;
        //   clone_insts();
        //   optimize = true;
        //   continue;
        // }
      }

      i++;
    }
  }
  else if(op == 2)
  {
    debug(CYAN "OP[%d] remove reassigned variables\n" RESET, op);
    for (int i = 0; insts[i]; i++)
    {
      if (insts[i]->token->declare)
      {
        int j = i + 1;
        while (insts[j] && insts[j]->token->space == insts[i]->token->space)
        {
          // ptoken(insts[j]->token);
          if (insts[j]->token->type == ASSIGN && insts[j]->left == insts[i]->token)
          {
            insts[i]->token->declare = false;
            insts[i]->token->remove = true;
            optimize = true;
            break;
          }
          if (
              (insts[j]->left && insts[j]->left->reg == insts[i]->token->reg) ||
              (insts[j]->right && insts[j]->right->reg == insts[i]->token->reg))
            break;
          j++;
        }
      }
      else if (insts[i]->token->type == ASSIGN)
      {
        int j = i + 1;
        while (insts[j] && insts[j]->token->space == insts[i]->token->space)
        {
          if (!insts[j]->left || !insts[j]->right || !insts[i]->token)
          {
            j++;
            continue;
          }
          if (
              insts[j]->token->type == ASSIGN &&
              insts[j]->left == insts[i]->left)
          {
            insts[i]->token->remove = true;
            optimize = true;
            break;
          }
          // if the variable is used some where
          else if (
              insts[j]->left->reg == insts[i]->token->reg ||
              insts[j]->right->reg == insts[i]->token->reg)
            break;
          j++;
        }
      }
    }
  }
  else if(op == 3)
  {
    debug(CYAN"OP[%d] remove followed return instructions\n"RESET, op);
    for (int i = 1; insts[i]; i++)
    {
      if (insts[i]->token->type == RETURN && insts[i - 1]->token->type == RETURN)
      {
        optimize = true;
        insts[i]->token->remove = true;
        clone_insts();
        i = 1;
      }
    }
  }
  else if(op == 4)
  {
    debug(CYAN "OP[%d] calculate followed operations\n" RESET, op);
    for(int i = 0; insts[i]; i++)
    {
      Token *curr = insts[i]->token;
      Token *left = insts[i]->left;   // a ptr
      Token *right = insts[i]->right; // value (not a ptr)
      // TODO: or declare
      if(curr->type == ASSIGN && !right->ptr)
      {
        for(int j = i + 1; insts[j]; j++)
        {
          // TODO: handle other mathematical operations
          if(insts[j]->token->type == ADD)
          {
            Token *math_left = insts[j]->left;
            Token *math_right = insts[j]->right;
            if(math_right->ptr == left->ptr)
            {
              insts[j]->right = copy_token(right);
              clone_insts();
              return true;
            }
            else if(math_left->ptr == left->ptr)
            {
              insts[j]->left = copy_token(right);
              clone_insts();
              return true;
            }
          }
        }
      }
    }
  }
  else if(op == 5)
  {
    // TODO: be carefull this one remove anything that don't have reg
    debug(CYAN "OP[%d] (remove unused instructions)\n"RESET, op);
    for (size_t i = 0; insts[i]; i++)
    {
      Token *curr = insts[i]->token;
      if (!curr->ptr && !curr->reg && includes((Type[]){INT, 0}, curr->type))
      {
        curr->remove = true;
        i = 0;
        clone_insts();
        return true;
      }
    }
  }
#define MAX_OPTIMIZATION 6
  return optimize;
}

void ir(Node *head)
{
  if (!IR)
    return;

  debug(GREEN "========== GENERATE IR =========\n" RESET);
  enter_scoop("");
  Node *curr = head;
  while (curr->left)
  {
    generate_ir(curr->left);
    curr = curr->right;
  }
  exit_scoop();

#if OPTIMIZE
  int i = 0;
  bool optimized = false;
  while (i < MAX_OPTIMIZATION)
  {
    print_ir();
    optimized = optimize_ir(i++) || optimized;
    if (i == MAX_OPTIMIZATION && optimized)
    {
      optimized = false;
      i = 0;
    }
  }
#endif
  print_ir();
}

// ASSEMBLY
struct _IO_FILE *asm_fd;
void pasm(char *fmt, ...)
{
  int i = 0;
  va_list args;
  va_start(args, fmt);
#if 1
#define isInstruction(inst)                        \
  do                                               \
  {                                                \
    if (strncmp(fmt + i, inst, strlen(inst)) == 0) \
    {                                              \
      i += strlen(inst);                           \
      fprintf(asm_fd, "\t%-8s", inst);             \
    }                                              \
  } while (0)
  isInstruction("movss ");
  isInstruction("mov ");
  isInstruction("sub ");
  isInstruction("lea ");
  isInstruction("cmp ");
  isInstruction("push ");
  isInstruction("call ");
  isInstruction("leave");
  isInstruction("ret");
#endif

#if WITH_COMMENTS
  isInstruction("//");
#else
  if (strncmp(fmt + i, "//", 2) == 0)
    return;
#endif

  while (fmt[i])
  {
    if (fmt[i] == '%')
    {
      i++;
      if (fmt[i] == 'i')
      {
        i++;
        fprintf(asm_fd, "\t%-8s", va_arg(args, char *));
      }
      else if (fmt[i] == 'r')
      {
        i++;
        Token *token = va_arg(args, Token *);
        if (token->creg)
        {
          // printf("%s:%dhas been used\n",FUNC, LINE);
          // exit(1);
          fprintf(asm_fd, "%s", token->creg);
        }
        else
        {
          Type type = token->retType ? token->retType : token->type;
          switch (type)
          {
          case CHARS: fputs("rax", asm_fd); break;
          case INT: fputs("eax", asm_fd); break;
          case BOOL: case CHAR: fputs("al", asm_fd); break;
          case FLOAT: fputs("xmm0", asm_fd); break;
          default: check(1, "Unknown type [%s]\n", to_string(token->type)); break;
          }
        }
      }
      else if (fmt[i] == 'a')
      {
        i++;
        Token *token = va_arg(args, Token *);
        switch (token->type)
        {
        case CHARS: fprintf(asm_fd, "QWORD PTR -%ld[rbp]", token->ptr); break;
        case INT: fprintf(asm_fd, "DWORD PTR -%ld[rbp]", token->ptr); break;
        case CHAR: fprintf(asm_fd, "BYTE PTR -%ld[rbp]", token->ptr); break;
        case BOOL: fprintf(asm_fd, "BYTE PTR -%ld[rbp]", token->ptr); break;
        case FLOAT: fprintf(asm_fd, "DWORD PTR -%ld[rbp]", token->ptr); break;
        default: check(1, "Unknown type [%s]\n", to_string(token->type)); break;
        }
      }
      else if (fmt[i] == 'v')
      {
        i++;
        Token *token = va_arg(args, Token *);
        switch (token->type)
        {
        case INT: fprintf(asm_fd, "%lld", token->Int.value); break;
        case BOOL: fprintf(asm_fd, "%d", token->Bool.value); break;
        case CHAR: fprintf(asm_fd, "%d", token->Char.value); break;
        default:
          check(1, "Unknown type [%s]\n", to_string(token->type));
          break;
        }
      }
      else
      {
        int handled = 0;
#define check_format(string, type)                     \
  do                                                   \
  {                                                    \
    if (strncmp(fmt + i, string, strlen(string)) == 0) \
    {                                                  \
      handled = 1;                                     \
      i += strlen(string);                             \
      fprintf(asm_fd, "%" string, va_arg(args, type)); \
    }                                                  \
  } while (0)
        check_format("d", int);
        check_format("ld", long);
        check_format("s", char *);
        check_format("zu", unsigned long);
        check_format("f", double);
        check(!handled, "handle this case [%s]\n", fmt + i);
      }
    }
    else
    {
      fputc(fmt[i], asm_fd);
      i++;
    }
  }
  va_end(args);
}

void initialize()
{
  pasm(".intel_syntax noprefix\n");
  pasm(".include \"./import/header.s\"\n\n");
  pasm(".text\n");
  pasm(".globl	main\n");
}

void finalize()
{
#if TOKENIZE
  for (int i = 0; tokens[i]; i++)
  {
    Token *curr = tokens[i];
    // test char variable before making any modification
    if (curr->type == CHARS && !curr->name && !curr->ptr && curr->index)
      pasm(".STR%zu: .string %s\n", curr->index, curr->Chars.value);
    if (curr->type == FLOAT && !curr->name && !curr->ptr && curr->index)
      pasm(".FLT%zu: .long %zu /* %f */\n", curr->index,
           *((uint32_t *)(&curr->Float.value)), curr->Float.value);
  }
  pasm(".section	.note.GNU-stack,\"\",@progbits\n\n");
#endif
}

void generate_asm()
{
  if(!ASM) return;
  debug(GREEN "======= GENERATE ASSEMBLY ======\n" RESET);
  debug(CYAN);
  // asm_fd = stdout;
  initialize();
  clone_insts();
  for (size_t i = 0; insts[i]; i++)
  {
    Token *curr = insts[i]->token;
    Token *left = insts[i]->left;
    Token *right = insts[i]->right;
    switch (curr->type)
    {
    case INT: case BOOL: case CHARS:
    {
      if (curr->declare)
      {
        pasm("//declare [%s]\n", curr->name);
        pasm("mov %a, 0\n", curr);
      }
      break;
    }
    case ASSIGN:
    {
      pasm("//assign [%s]\n", left->name);
      // if(strcmp(left->name, "i") == 0)
      // {
      //   debug(RED"found i\n");
      //   debug("has right: ");
      //   ptoken(right);
      //   if(right->ptr) debug("has ptr %zu\n", right->ptr);
      //   if(right->creg) debug("has creg %s\n", right->creg);
      //   exit(1);
      // }
      if (right->ptr || right->creg)
      {
        char *inst = left->type == FLOAT ? "movss " : "mov ";
        if (right->ptr && !right->creg) pasm("%i%r, %a\n", inst, left, right);
        pasm("%i%a, %r\n", inst, left, left);
      }
      else
      {
        switch (right->type)
        {
        case INT: case BOOL: case CHAR:
          pasm("mov %a, %v\n", left, right);
          break;
        case CHARS:
            pasm("lea %r, .STR%zu[rip]\n", right, right->index);
            pasm("mov %a, %r\n", left, left);
            break;
        // case float_:
        //     pasm("movss %r, DWORD PTR .FLT%zu[rip]\n", right, right->index);
        //     pasm("movss %a, %r\n", left, left);
        //     break;
        default:
          check(1, "handle this case (%s)\n", to_string(right->type));
          break;
        }
      }
      break;
    }
    case ADD: case SUB: case MUL: case DIV: // TODO: check all math_op operations
    {
      // TODO: use rax for long etc...
      // TODO: something wrong here, fix it
      // Type type = curr->type;
      char *inst = left->type == FLOAT ? "movss" : "mov";

      if (left->ptr)
        pasm("%i%r, %a\n", inst, curr, left);
      else if (left->creg && right->creg && strcmp(left->creg, right->creg))
        pasm("%i%r, %r\n", inst, curr, left);
      else if (!left->creg)
        pasm("%i%r, %v\n", inst, curr, left);

      switch (curr->type)
      {
      case ADD: inst = left->type == FLOAT ? "addss " : "add "; break;
      case SUB: inst = left->type == FLOAT ? "subss " : "sub "; break;
      case MUL: inst = left->type == FLOAT ? "imulss ": "imul "; break;
      case DIV: inst = left->type == FLOAT ? "divss " : "div "; break;
      default: break;
      }
      if (right->ptr)
        pasm("%i%r, %a\n", inst, curr, right);
      else if (right->creg)
        pasm("%i%r, %r\n", inst, curr, right);
      else if (!right->creg)
        pasm("%i%r, %v\n", inst, curr, right);
      curr->type = left->type;
      break;
    }
    case EQUAL: case NOT_EQUAL: case LESS: case MORE: case MORE_EQUAL:
    {
      char *inst = left->type == FLOAT ? "movss" : "mov";
      if (left->ptr) pasm("%i%r, %a\n", inst, left, left);
      else if (left->creg /*&& strcmp(left->creg, r->creg)*/)
        pasm("%i%r, %r\n", inst, left, left);
      else if (!left->creg) pasm("%i%r, %v\n", inst, left, left);

      char *reg = NULL;
      switch (left->type)
      {
        case INT: reg = "ebx"; break;
        case FLOAT: reg = "xmm1"; break;
        case CHAR: reg = "bl"; break;
        case BOOL: reg = "ebx"; break;
      default: check(1, "Unkown type [%s]\n", to_string(left->type)); break;
      }
      if (right->ptr) pasm("%i%s, %a\n", inst, reg, right);
      else if (right->creg) pasm("%i%s, %r\n", inst, reg, right);
      else if (!right->creg) pasm("%i%s, %v\n", inst, reg, right);

      inst = left->type == FLOAT ? "ucomiss" : "cmp";
      pasm("%i%r, %s\n", inst, left, reg);
      switch (curr->type)
      {
        case EQUAL: inst = "sete"; break;
        case NOT_EQUAL: inst = "setne"; break;
        case LESS: inst = "setl"; break;
        case LESS_EQUAL: inst = "setle"; break;
        case MORE: inst = "setg"; break;
        case MORE_EQUAL: inst = "setge"; break;
        default: check(1, "Unkown type [%s]\n", to_string(left->type)); break;
      }
      curr->retType = BOOL;
      curr->creg = "al";
      pasm("%i%r\n", inst, curr);
      break;
      break;
    }
    case FDEC:
    {
      pasm("%s:\n", curr->name);
      pasm("push rbp\n");
      pasm("mov rbp, rsp\n");
      pasm("sub rsp, %zu\n", (((ptr) + 15) / 16) * 16);
      break;
    }
    case JE:
    {
      pasm("%ial, 1\n", "cmp");
      pasm("%i.%s%zu\n", "je", curr->name, curr->index);
      break;
    }
    case JNE:
    {
      pasm("%ial, 1\n", "cmp");
      pasm("%i.%s%zu\n", "jne", curr->name, curr->index);
      break;
    }
    case JMP:
    {
      pasm("%i.%s%zu\n", "jmp", curr->name, curr->index);
      break;
    }
    case FCALL:
    {
      pasm("call %s\n", curr->name);
      break;
    }
    case BLOC:
    {
      pasm(".%s%zu:\n", curr->name, curr->index);
      break;
    }
    case END_BLOC:
    {
      pasm(".end%s:\n", curr->name);
      break;
    }
    case RETURN:
    {
      if(left->ptr) pasm("mov %r, %a\n", left, left);
      else if(left->reg)
      {
        debug("reg: %c\n", left->creg);
        exit(1);
      }
      else
      {
        switch (left->type)
        {
        case INT: pasm("mov %r, %ld\n", left, left->Int.value); break;
        case VOID: pasm("mov eax, 0\n"); break;
        default:
        {
          check(1, "handle this case [%s]\n", to_string(left->type));
          break;
        }
        }
      }
      pasm("leave\n");
      pasm("ret\n");
      break;
    }
    default: check(1, "handle this case (%s)\n", to_string(curr->type)); break;
    }
  }
  finalize();
  debug(RESET);
}
// MAIN
int main()
{

  char *input = open_file("file.w");
  char *outputFile = strdup("file.w");
  outputFile[strlen(outputFile) - 1] = 's';

  asm_fd = fopen(outputFile, "w+");
  check(!asm_fd, "openning %s\n", outputFile);
  free(outputFile);
  // debug("===========    IR    ===========\n");
  tokenize(input);
  Node *head = ast();
  ir(head);
  generate_asm();
  free_node(head);
  for (size_t i = 0; OrgInsts && OrgInsts[i]; i++)
    free(OrgInsts[i]);
  for (size_t i = 0; tokens && tokens[i]; i++)
    free_token(tokens[i]);
  free(tokens);
  free(input);
}
