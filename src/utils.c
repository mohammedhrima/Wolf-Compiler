#include "./include/header.h"

const char *to_string(Type type) {
   const char *type_strings[] = {
      [ASSIGN] = "ASSIGN", [ADD_ASSIGN] = "ADD ASSIGN", [SUB_ASSIGN] = "SUB ASSIGN",
      [MUL_ASSIGN] = "MUL ASSIGN", [DIV_ASSIGN] = "DIV ASSIGN", [EQUAL] = "EQUAL",
      [NOT_EQUAL] = "NOT EQUAL", [LESS_EQUAL] = "LESS THAN OR EQUAL",
      [MORE_EQUAL] = "MORE THAN OR EQUAL", [LESS] = "LESS THAN", [MORE] = "MORE THAN",
      [ADD] = "ADD", [SUB] = "SUB", [MUL] = "MUL", [DIV] = "DIV", [MOD] = "MOD",
      [AND] = "AND", [OR]  = "OR", [RPAR] = "R_PAR", [LPAR] = "L_PAR", [COMA] = "COMMA",
      [DOTS] = "DOTS", [DOT] = "DOT", [RETURN] = "RETURN", [IF] = "IF", [ELIF] = "ELIF",
      [ELSE] = "ELSE", [WHILE] = "HILE", [FDEC] = "F_DEC", [FCALL] = "F_CALL", [INT] = "INT",
      [VOID] = "VOID", [CHARS] = "CHARS", [CHAR] = "CHAR", [BOOL] = "BOOL", [FLOAT] = "FLOAT",
      [STRUCT] = "STRUCT", [ID] = "ID", [END_BLOC] = "END_BLOC", [BLOC] = "BLOC", [JNE] = "JNE",
      [JE] = "JE", [JMP] = "JMP", [END] = "END"
   };
   if
   (
      type >= 1 &&
      type < sizeof(type_strings) / sizeof(type_strings[0]) &&
      type_strings[type] != NULL
   )
      return type_strings[type];
   check(1, "Unknown type [%d]\n", type);
   return NULL;
}

void *allocate_func(size_t line, size_t len, size_t size)
{
   void *ptr = calloc(len, size);
   check(!ptr, "allocate did failed in line %zu\n", line);
   return ptr;
}

void open_file()
{
   struct _IO_FILE *file = fopen(obj.filename, "r");
   fseek(file, 0, SEEK_END);
   size_t size = ftell(file);
   fseek(file, 0, SEEK_SET);
   char *input = allocate((size + 1), sizeof(char));
   if (input) fread(input, size, sizeof(char), file);
   fclose(file);
   obj.input = input;
}

void free_token(Token *token)
{
   if (token->name) free(token->name);
   if (token->creg) free(token->creg);
   if (token->Chars.value) free(token->Chars.value);
   free(token);
}

void free_memory()
{
   free(obj.input);
   for(int i = 0; obj.tokens && obj.tokens[i]; i++) free_token(obj.tokens[i]);
}

void check_error(const char *filename, const char *funcname, int line, bool cond, char *fmt, ...)
{
   if (!cond) return;
   obj.is_error = true;
   va_list ap;
   va_start(ap, fmt);
   fprintf(stderr, "%sError:%s:%s:%d %s", RED, filename, funcname, line, RESET);
   vfprintf(stderr, fmt, ap);
   fprintf(stderr, "\n");
   va_end(ap);
}

int ptoken(Token *token)
{
   int res = 0;
   if (!DEBUG) return res;

   res += debug("token: space [%d] [%t] ", token->space, token->type);
   switch (token->type)
   {
   case VOID: case CHARS: case CHAR: case INT: case BOOL: case FLOAT:
   {
      if (token->name) res += debug("name [%s] ", token->name);
      if (token->declare) res += debug("[declare] ");
      if (!token->name && !token->declare)
      {
         switch (token->type)
         {
         case INT: res += debug("value [%lld] ", token->Int.value); break;
         case CHARS: res += debug("value [%s] ", token->Chars.value); break;
         case CHAR: res += debug("value [%c] ", token->Char.value); break;
         case BOOL: res += debug("value [%d] ", token->Bool.value); break;
         case FLOAT: res += debug("value [%f] ", token->Float.value); break;
         default: break;
         }
      }
      break;
   }
   case FCALL: case FDEC: case ID: res += debug("name [%s] ", token->name); break;
   default: break;
   }
   if (token->remove) res += debug(" [remove]");
   return res;
}

int pnode(Node *node)
{
   int res = 0;
   if (!node) return res;
   res += debug("node: %t", node->token);
   res += debug("LEFT : %n", node->left);
   res += debug("RIGHT: %n", node->right);
   return res;
}

int debug(char *conv, ...)
{
   size_t i = 0;
   int res = 0;

   va_list args;
   va_start(args, conv);
   while (conv[i])
   {
      if (conv[i] == '%')
      {
         i++;
         if (strncmp(conv + i, "zu", 2) == 0)
         {
            res += fprintf(stdout, "%zu", va_arg(args, size_t));
            i++;
         }
         if (strncmp(conv + i, "lld", 2) == 0)
         {
            res += fprintf(stdout, "%lld", va_arg(args, long long));
            i += 2;
         }
         else
         {
            switch (conv[i])
            {
            case 'c': res += fprintf(stdout, "%c", va_arg(args, int)); break;
            case 's': res += fprintf(stdout, "%s", va_arg(args, char *)); break;
            case 'p': res += fprintf(stdout, "%p", (void*)(va_arg(args, void *))); break;
            case 'x': res += fprintf(stdout, "%x", (unsigned int)va_arg(args, void *)); break;
            case 'X': res += fprintf(stdout, "%X", (unsigned int)va_arg(args, void *)); break;
            case 'd': res += fprintf(stdout, "%d", (int)va_arg(args, int)); break;
            case 'f': res += fprintf(stdout, "%f", va_arg(args, double)); break;
            case '%': res += fprintf(stdout, "%%"); break;
            case 't': res += fprintf(stdout, "%s", to_string((Type)va_arg(args, Type))); break;
            case 'k':
            {
               Token *token = (Token *)va_arg(args, Token *);
               if (token) res += ptoken(token);
               else res += fprintf(stdout, "(null)");
               break;
            }
            case 'n':
            {
               Node *node = (Node*)va_arg(args, Node*);
               if(node) res += pnode(node);
               else res += fprintf(stdout, "(null)");
               break;
            }
            default:
               check(true, "invalid precedent [%c]", conv[i]);
               break;
            }
         }
      }
      else res += fprintf(stdout, "%c", conv[i]);
      i++;
   }
   return res;
}

