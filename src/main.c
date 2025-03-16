#include "./include/header.h"

// GLOBALS
bool found_error = false;
Inst **OrgInsts;
Inst **insts;
Token **tokens;
Node *head;
size_t exe_pos;
char *input;

// TOKENIZE
void tokenize()
{
   if (found_error) return;
   debug(GREEN BOLD"TOKENIZE:\n" RESET);
   Specials *specials = (Specials[]) {
      {".", DOT}, {":", DOTS}, {"+=", ADD_ASSIGN}, {"-=", SUB_ASSIGN},
      {"*=", MUL_ASSIGN}, {"/=", DIV_ASSIGN}, {"!=", NOT_EQUAL},
      {"==", EQUAL}, {"is", EQUAL}, {"<=", LESS_EQUAL}, {">=", MORE_EQUAL},
      {"<", LESS}, {">", MORE}, {"=", ASSIGN}, {"+", ADD}, {"-", SUB},
      {"*", MUL}, {"/", DIV}, {"%", MOD}, {"(", LPAR}, {")", RPAR},
      {",", COMA}, {"if", IF}, {"elif", ELIF}, {"else", ELSE},
      {"while", WHILE}, {"func", FDEC}, {"return", RETURN},
      {"and", AND}, {"&&", AND}, {"or", OR}, {"||", OR},
      {0, (Type)0}
   };

   size_t i = 0;
   size_t space = 0;
   bool inc_space = true;
   while (input[i])
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
         {
            if (input[i] == '\t') space += TAB;
            else space++;
         }
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
            if (strcmp(specials[j].value, ":") == 0) space++;
            break;
         }
      }
      if (found) continue;
      if (input[i] && input[i] == '\"')
      {
         i++;
         while (input[i] && input[i] != '\"') i++;
         check(input[i] != '\"', "Expected '\"'");
         i++;
         new_token(input, s, i, CHARS, space);
         continue;
      }
      if (input[i] && input[i] == '\'')
      {
         i++;
         if (input[i] && input[i] != '\'') i++;
         check(input[i] != '\'', "Expected '\''");
         i++;
         new_token(input, s + 1, i, CHAR, space);
         continue;
      }
      if (isalpha(input[i]))
      {
         while (isalnum(input[i])) i++;
         new_token(input, s, i, ID, space);
         continue;
      }
      if (isdigit(input[i]))
      {
         while (isdigit(input[i])) i++;
         new_token(input, s, i, INT, space);
         continue;
      }
      check(input[i], "Syntax error <%c>\n", input[i]);
   }
   new_token(input, 0, 0, END, -1);
}

// ABSTRACT SYNTAX TREE
Type getRetType(Node *node)
{
   if (includes((Type[]) {INT, 0}, node->token->type)) return node->token->type;
   if (includes((Type[]) {INT, 0}, node->token->retType)) return node->token->retType;
   Type left = 0, right = 0;
   if (node->left) left = getRetType(node->left);
   if (node->right) right = getRetType(node->right);
   if (left) return left;
   if (right) return right;
   return 0;
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
      node->left = left;
      node->right = logic();
      node->token->retType = getRetType(node);
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
      node->token->retType = getRetType(node);
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
      node->token->retType = getRetType(node);
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

Node *func_dec(Node *node)
{
   Node *fname = prime();
   check(!fname->token || !fname->token->declare, "expected data type after func declaration");
   node->token->retType = fname->token->type;
   node->token->name = fname->token->name;
   fname->token->name = NULL;
   free_node(fname);
   enter_scoop(node->token->name);
   debug("found FDEC with retType %s\n", to_string(node->token->retType));
   char *eregs[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d", NULL};
   char *rregs[] = {"rdi", "rsi", "rdx", "rcx", "r8d", "r9d", NULL};
   int i = 0;
   Token *arg = NULL;
   Token *token = node->token;

   node->left = new_node(NULL);
   Node *curr = node->left;

   check(!find(LPAR, 0), "expected ( after function declaration");
   while (!(arg = find(RPAR, END, 0)))
   {
      Node *assign = new_node(new_token(NULL, 0, 0, ASSIGN, token->space + 2 * TAB));
      assign->left = expr();
      check(!assign->left->token->declare, "Invalid argument for function declaration");
      assign->left->token->space = token->space + 2 * TAB;
      assign->right = new_node(new_token(NULL, 0, 0, assign->left->token->type, token->space + 2 * TAB));
      if (eregs[i])
      {
         // TODO: add other data type and math operations
         switch (assign->left->token->type)
         {
         case CHARS: setReg(assign->right->token, rregs[i]); break;
         case INT:   setReg(assign->right->token, eregs[i]); break;
         default: check(1, "set reg for %s", to_string(assign->left->token->type));
         };
         i++;
      }
      else
      {
         // TODO:
         check(1, "implemnt assigning function argument using PTR");
      }
      curr->left = assign;
      find(COMA, 0);
      curr->right = new_node(NULL);
      curr = curr->right;
   }
   check(arg->type != RPAR, "expected ) after function declaration");
   check(!find(DOTS, 0), "Expected : after function declaration");

   curr = node;
   while (within_space(node->token->space) && !found_error)
   {
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = expr();
   }
   exit_scoop();
   return node;
}

Node *func_call(Node *node)
{
   node->token->type = FCALL;
   Node *curr = node;
   Token *arg = NULL;
   Token *token = node->token;

   char *eregs[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d", NULL};
   char *rregs[] = {"rdi", "rsi", "rdx", "rcx", "r8d", "r9d", NULL};
   int i = 0;
   while (!(arg = find(RPAR, END, 0)))
   {
      Node *assign = new_node(new_token(NULL, 0, 0, ASSIGN, token->space));
      assign->right = expr();
      assign->right->token->space = token->space;
      assign->left = new_node(new_token(NULL, 0, 0, assign->right->token->type, token->space));
      if (eregs[i])
      {
         if (assign->left->token->type == ID)
         {
            Token *var = get_variable(assign->right->token->name);
            assign->left->token->type = var->type;
         }
         Type type = assign->left->token->type;
         switch (type)
         {
         case CHARS: setReg(assign->left->token, rregs[i]); break;
         case ADD: case SUB: case MUL: case DIV:
         case INT: setReg(assign->left->token, eregs[i]); break;
         default: check(1, "handle this case [%s]", to_string(type));
         }
         i++;
      }
      else
      {
         check(1, "implemnt PTR");
      }
      curr->left = assign;
      find(COMA, 0);
      curr->right = new_node(NULL);
      curr = curr->right;
   }
   check(arg->type != RPAR, "expected ) after function call");
   return node;
}

Node *func_main(Node *node)
{
   check(!find(RPAR, 0), "expected ) after main declaration");
   check(!find(DOTS, 0), "expected : after main() declaration");

   node->token->type = FDEC;
   node->token->retType = INT;
   Node *curr = node;
   Node *last = node;
   while (within_space(node->token->space) && !found_error)
   {
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = expr();
      last = curr->left;
   }
   if (last->token->type != RETURN)
   {
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = new_node(new_token(NULL, 0, 0, RETURN, node->token->space + TAB));
      curr->left->left = new_node(new_token(NULL, 0, 0, INT, node->token->space + TAB));
   }
   return node;
}

Node *if_node(Node *node)
{
   node->left = new_node(NULL);
   Node *curr = node->left; // if bloc
   
   curr->left = expr(); // condition
   curr->left->token->space = node->token->space + TAB;

   check(!find(DOTS, 0), "Expected : after if condition\n", "");
   curr->right = new_node(NULL);
   curr = curr->right;

   while(within_space(node->token->space))
   {
      curr->left = expr();
      curr->left->token->space = node->token->space + TAB;
      curr->right = new_node(NULL);
      curr = curr->right;
   }
   curr = node;
   while(includes((Type[]) {ELSE, ELIF, 0}, tokens[exe_pos]->type) && node->token->space == tokens[exe_pos]->space)
   {
      Token *token = find(ELSE, ELIF, 0);
      
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = new_node(token);
      if (token->type == ELIF)
      {
         Node *elif_node = curr->left;
         elif_node->left = expr();
         check(!find(DOTS, 0), "expected : after elif condition");
         elif_node->right = new_node(NULL);
         elif_node = elif_node->right;
         while (within_space(token->space))
         {
            elif_node->left = expr();
            elif_node->right = new_node(NULL);
            elif_node = elif_node->right;
         }
      }
      else if (token->type == ELSE)
      {
         check(!find(DOTS, 0), "expected dots");
         Node *else_node = curr->left;
         else_node->right = new_node(NULL);
         else_node = else_node->right;
         while (within_space(token->space))
         {
            else_node->left = expr();
            else_node->right = new_node(NULL);
            else_node = else_node->right;
         }
         break;
      }
   }
   return node;
}

Node *prime()
{
   Node *node = NULL;
   Token *token;
   if ((token = find(ID, INT, BOOL, CHARS, CHAR, 0)))
   {
      if (token->declare) // int variable_name
      {
         Token *tmp = find(ID, 0);
         check(!tmp, "Expected variable name after [%s] symbol\n", to_string(token->type));
         setName(token, tmp->name);
         return new_node(token);
      }
      else if (token->type == ID && token->name && find(LPAR, 0))
      {
         node = new_node(token);
         if (strcmp(token->name, "main") == 0) return func_main(node);
         return func_call(node);
      }
      return new_node(token);
   }
   else if ((token = find(FDEC, 0)))
   {
      node = new_node(token);
      return func_dec(node);
   }
   else if ((token = find(RETURN, 0)))
   {
      // TODO: check if return type is compatible with function
      node = new_node(token);
      node->left = expr();
      return node;
   }
   else if ((token = find(IF, 0)))
   {
      node = new_node(token);
      return if_node(node);
   }
   else check(1, "Unexpected token has type %s\n", to_string(tokens[exe_pos]->type));
   return NULL;
}

void generate_ast()
{
   if (found_error) return;
   debug(BLUE BOLD"AST:\n" RESET);
   head = new_node(NULL);
   Node *curr = head;
   curr->left = expr();
   while (tokens[exe_pos]->type != END && !found_error)
   {
      curr->right = new_node(NULL);
      curr = curr->right;
      curr->left = expr();
   }
   if (found_error) return;
   debug(BLUE BOLD"PRINT AST:\n" RESET);
   curr = head;
   while (curr && !found_error)
   {
      debug("%n\n", curr->left);
      curr = curr->right;
   }
}

// INTERMEDIATE REPRESENTATION
bool optimize_ir()
{
   static int op = 0;
   static bool did_optimize = false;
   bool did_something = false;
   switch (op)
   {
   case 0:
   {
      debug(CYAN "OP[%d] calculate operations on values\n" RESET, op);
      for (int i = 0; insts[i]; i++)
      {
         Token *token = insts[i]->token;
         Token *left = insts[i]->left;
         Token *right = insts[i]->right;

         Type types[] = {INT, FLOAT, CHAR, 0};
         Type ops[] = {ADD, SUB, MUL, DIV, 0};
         // TODO: check if left and right are compatible
         // test if left is function, and right is number ...
         if (
            includes(ops, token->type) && includes(types, left->type) &&
            compatible(left, right) && !left->name && !right->name)
         {
            did_something = true;
            did_optimize = true;
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
            default:
               check(1, "handle this case\n", "");
               break;
            }
            token->type = left->type;
            token->retType = 0;
            left->remove = true;
            right->remove = true;
            token->reg = 0;
            setReg(token, NULL);
            if (i > 0) i -= 2;
         }
      }
      if (did_something) print_ir();
      break;
   }
   case 1:
   {
      break;
   }
   case 2:
   {
      debug(CYAN "OP[%d] remove reassigned variables\n" RESET, op);
      for (int i = 0; insts[i]; i++)
      {
         Token *token = insts[i]->token;
         if (token->declare)
         {
            for (int j = i + 1; insts[j] && insts[j]->token->space == token->space; j++) {
               if (insts[j]->token->type == ASSIGN && insts[j]->left == token) {
                  token->declare = false;
                  token->remove = true;
                  did_optimize = true;
                  did_something = true;
                  break;
               }
               if ((insts[j]->left && insts[j]->left->reg == token->reg) ||
                     (insts[j]->right && insts[j]->right->reg == token->reg)) {
                  break;
               }
            }
         }
         else if (token->type == ASSIGN)
         {
            for (int j = i + 1; insts[j] && insts[j]->token->space == token->space; j++) {
               if (!insts[j]->left || !insts[j]->right)
                  continue;
               if (insts[j]->token->type == ASSIGN && insts[j]->left == insts[i]->left) {
                  token->remove = true;
                  did_optimize = true;
                  did_something = true;
                  break;
               }
               if (insts[j]->left->reg == token->reg || insts[j]->right->reg == token->reg)
                  break;
            }
         }
      }
      if (did_something) print_ir();
      break;
   }
   case 3:
   {
      break;
   }
   case 4:
   {
      // TODO: be carefull this one remove anything that don't have reg
      debug(CYAN "OP[%d] (remove unused instructions)\n"RESET, op);
      for (size_t i = 0; insts[i]; i++)
      {
         Token *curr = insts[i]->token;
         if (!curr->ptr && !curr->reg && includes((Type[]) {INT, 0}, curr->type))
         {
            curr->remove = true;
            did_something = true;
            did_optimize = true;
         }
      }
      if (did_something) print_ir();
      break;
   }
   default:
   {
      op = 0;
      if (!did_optimize) return false;
      did_optimize = false;
      break;
   }
   }
   op++;
   return true;
}

Token* generate_ir(Node *node)
{
   static size_t str_index;
   static size_t bloc_index;

   if (found_error) return NULL;
   Inst *inst = NULL;
   switch (node->token->type)
   {
   case ID:
   {
      Token *token = get_variable(node->token->name);
      return token;
   }
   case INT: case BOOL:
   {
      inst = new_inst(node->token);
      if(inst->token->name && inst->token->declare) new_variable(inst->token);
      break;
   }
   case CHARS:
   {
      inst = new_inst(node->token);
      if (!node->token->creg) node->token->index = ++str_index;
      break;
   }
   case ASSIGN: case ADD_ASSIGN:
   case ADD: case SUB: case MUL: case DIV:
   case EQUAL: case NOT_EQUAL:
   case LESS: case MORE: case LESS_EQUAL: case MORE_EQUAL:
   {

      Token *left = generate_ir(node->left);
      Token *right = generate_ir(node->right);
      check(!compatible(left, right), "invalid [%s] op between %s and %s \n",
            to_string(node->token->type), to_string(left->type), to_string(right->type));

      inst = new_inst(node->token);
      inst->left = left;
      inst->right = right;
      switch (node->token->type)
      {
      case ADD_ASSIGN:
      case ASSIGN:
         node->token->reg = left->reg;
         node->token->retType = getRetType(node);
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
         node->token->retType = getRetType(node);
         if (right->type == INT) setReg(node->token, "eax");
         else if (right->type == FLOAT) setReg(node->token, "xmm0");
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
   case IF:
   {
      Node *curr = node->left;
      // set if bloc
      Inst *tmp = new_inst(copy_token(node->token));
      setName(tmp->token, "if");
      tmp->token->type =  BLOC;
      tmp->token->index = ++bloc_index;

      // check condition
      generate_ir(curr->left); // TODO: check if it's boolean

      Node *next = copy_node(node);
      setName(next->token, "endif");
      next->token->type = JNE;
      next->token->index = ++bloc_index;
      next->token->space = node->token->space + TAB;
      new_inst(next->token);

      curr = curr->right;
      // if code bloc
      while (curr->left)
      {
         generate_ir(curr->left);
         curr = curr->right;
      }
#if 1
      Inst *inst = NULL;
      if (node->right) // add jmp to endif if there is elif or else blocs
      {
         inst = new_inst(new_token("endif", 0, 5, JMP, node->token->space));
         inst->token->index = node->token->index;
      }
      curr = node->right;
      while(curr)
      {
#if 0
         if (curr->left->token->type == ELIF)
         {
            curr->left->token->index = ++bloc_index;
            setName(curr->left->token, "elif");
            curr->left->token->type = BLOC;
            {
               setName(next->token, "elif");
               next->token->index = curr->left->token->index;
               next->token = copy_token(next->token);
            }
            new_inst(curr->left->token);
            Node *tmp = curr->left;
            generate_ir(tmp->left); // elif condition, TODO: check is boolean

            new_inst(next->token);

            tmp = tmp->right;
            while (tmp->left)
            {
               generate_ir(tmp->left);
               tmp = tmp->right;
            }
         }
         else 
#endif
         if (curr->left->token->type == ELSE)
         {
            curr->left->token->index = ++bloc_index;
            setName(curr->left->token, "else");
            curr->left->token->type = BLOC;
            new_inst(curr->left->token);

            {
               setName(next->token, "else");
               next->token->index = curr->left->token->index;
               next->token = copy_token(next->token);
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
         if (curr->right) // to avoid adding jmp in the last statement
         {
            inst = new_inst(new_token("endif", 0, 5, JMP, node->token->space));
            inst->token->index = node->token->index;
         }
         curr = curr->right;
      }
#endif
      Token *new = new_token("endif", 0, 5, BLOC, node->token->space);
      new->index = node->token->index;
      new_inst(new);
      free_node(next);
      return node->left->token;
      break;
   }
   case FDEC:
   {
      new_function(node);
      switch (node->token->retType)
      {
      case INT: setReg(node->token, "eax"); break;
      case CHARS: setReg(node->token, "rax"); break;
      default: check(1, "handle this case [%s]\n", to_string(node->token->retType)); break;
      }
      enter_scoop(node->token->name);

      debug("%n", node);
      size_t tmp_ptr = ptr;
      ptr = 0;
      inst = new_inst(node->token);

      // arguments
      Node *arg = node->left;
      // dest: left  (variable inside function)
      // src : right
      // debug(GREEN"print arguments\n"RESET);
      while (arg && arg->left)
      {
         debug("%n", arg->left);
         generate_ir(arg->left);
         arg = arg->right;
      }

      // code bloc
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
      node->token->ptr = ptr;
      ptr = tmp_ptr;
      break;
   }
   case FCALL:
   {
      if (strcmp(node->token->name, "output") == 0)
      {
         check(1, "handle this case"); // TODO
      }
      else
      {
         Node *func = get_function(node->token->name);
         if(!func) return NULL;
         node->token->retType = func->token->retType;
         setReg(node->token, func->token->creg);
         debug("%s: has the following arguments\n", node->token->name);
         Node *arg = node;
         while (arg->left)
         {
            pnode(arg->left, "", 10);
            generate_ir(arg->left);
            arg = arg->right;
         }
      }
      inst = new_inst(node->token);
      break;
   }
   case RETURN:
   {
      Token *left = generate_ir(node->left);
      inst = new_inst(node->token);
      inst->left = left;
      break;
   }
   default:
      check(1, "handle this case %s", to_string(node->token->type));
      return NULL;
      break;
   }
   return inst->token;
}

// ASSMBLY GENERATION
struct _IO_FILE *asm_fd;

void generate_asm(char *name)
{
   if (found_error) return;
   char *outfile = strdup(name);
   outfile[strlen(outfile) - 1] = 's';
   asm_fd = fopen(outfile, "w+");
   check(!asm_fd, "openning %s\n", outfile);
   if (found_error) return;
   free(outfile);
   initialize();
   clone_insts();
   for (size_t i = 0; insts[i]; i++)
   {
      Token *curr = insts[i]->token;
      Token *left = insts[i]->left;
      Token *right = insts[i]->right;
      skip_space(curr->space);
      switch (curr->type)
      {
      case INT: case BOOL: case CHARS:
      {
         if (curr->declare)
         {
            pasm("%i%a, 0 ;// declare [%s]", "mov", curr, curr->name); skip_space(curr->space);
         }
         break;
      }
      case ADD_ASSIGN:
      {
         char *inst = "add";
         pasm("%i%a, %v", inst, left, right);
         if (left->name) {pasm(" ;// add_assign [%s]", left->name); skip_space(curr->space);}
         break;
      }
      case ASSIGN:
      {
         char *inst = left->type == FLOAT ? "movss " : "mov ";
         if (right->ptr)
         {
            pasm("%i%r, %a", inst, left, right);
            // TODO: test this case before changing
            // chars str = "fffff" int a = strlen(str)
            if (!left->creg) pasm("%i%a, %r", inst, left, left);
         }
         else if (right->creg)
         {
            pasm("%i%a, %r", inst, left, right);
         }
         else
         {
            switch (right->type)
            {
            case INT: case BOOL: case CHAR:
               pasm("%i%a, %v", "mov", left, right);
               break;
            case CHARS:
               pasm("%i%r, .STR%zu[rip]", "lea", left, right->index);
               // I did this to diffenticiate function parameter from
               // variable declaration
               if (left->ptr) {skip_space(curr->space); pasm("%i%a, %r", "mov", left, right); }
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

         if (left->name) {pasm(" ;// assign [%s]", left->name); }
         else if (left->creg) {pasm(" ;// assign [%s]", left->creg);}
         break;
      }
      case ADD: case SUB: case MUL: case DIV: // TODO: check all math_op operations
      {
         // TODO: use rax for long etc...
         // TODO: something wrong here, fix it
         // Type type = curr->type;
         char *inst = left->type == FLOAT ? "movss" : "mov";
         char *inst2 = NULL;
         switch (curr->type)
         {
         case ADD: inst2 = left->type == FLOAT ? "addss " : "add "; break;
         case SUB: inst2 = left->type == FLOAT ? "subss " : "sub "; break;
         case MUL: inst2 = left->type == FLOAT ? "imulss " : "imul "; break;
         case DIV: inst2 = left->type == FLOAT ? "divss " : "div "; break;
         default: break;
         }
         // debug("left: "); ptoken(left);
         // debug("right: "); ptoken(right);
         // debug("left creg %s\n", left->creg);
         // debug("right creg %s\n", right->creg);
         // debug("left %s, right: %s\n", left->creg, right->creg);
         // if (left->ptr)
         //   pasm("%i%r, %a\n", inst, curr, left);
         // else if (left->creg && right->creg && strcmp(left->creg, right->creg)) // they should != eax
         //   pasm("%i%r, %r\n", inst, curr, left);
         // else if (!left->creg)
         // {
         //   // debug("helloooo\n");
         //   if(!right->creg) pasm("%i%r, %v\n", inst, curr, left);
         //   else if(strcmp(right->creg, "eax") == 0)
         //   {
         //     inst = "add";
         //     pasm("%i%r, %v\n",inst2, curr, left);
         //   }
         // }


         // if (right->ptr)
         //   pasm("%i%r, %a\n", inst2, curr, right);
         // else if (right->creg && (!left->creg || (left->creg && strcmp(left->creg, right->creg)))) // they should != eax
         //   pasm("%i%r, %r\n", inst2, curr, right);
         // else
         //   pasm("%i%r, %v\n", inst2, curr, right);
         // skip_space(curr->space);
         pasm("%i%r, ", inst, left);
         if (left->ptr) pasm("%a", left);
         else if (left->creg) pasm("%r", left) ;
         else pasm("%v", left);
         // pasm("\n [%zu]", curr->space);
         skip_space(curr->space);


         pasm("%i%r, ", inst2, right);
         if (right->ptr) pasm("%a", right);
         else if (right->creg) pasm("%r", right) ;
         else pasm("%v", right);

         // if (!left->creg && !left->ptr)
         // {
         //   if(!right->creg) pasm("%i%r, %v", inst, curr, left);
         //   else if(strcmp(right->creg, "eax") == 0)
         //   {
         //     inst = "add";
         //     pasm("%i%r, %v\n",inst2, curr, left);
         //   }
         // }
         // else
         // {
         //   if(right->ptr) pasm("%i%a, ", inst2, right);
         //   else if(right->creg) pasm("%i%r, ", inst2, right);
         //   else if (!right->creg) pasm("%i%v, ", inst2, right);
         // }
         // if(right->ptr)
         curr->type = left->type;
         break;
      }
      case EQUAL: case NOT_EQUAL: case LESS: case MORE: case MORE_EQUAL:
      {
         if (curr->isCond)
         {
            char *inst = NULL;
            switch (curr->type)
            {
            case EQUAL: inst = "jne"; break;
            case NOT_EQUAL: inst = "je"; break;
            case LESS: inst = "jge"; break;
            case LESS_EQUAL: inst = "jg"; break;
            case MORE: inst = "jle"; break;
            case MORE_EQUAL: inst = "jl"; break;
            default: check(1, "Unkown type [%s]\n", to_string(left->type)); break;
            }
            skip_space(curr->space); pasm("%i", "cmp");
            if (left->ptr) pasm("%a", left);
            else if (left->creg) pasm("%r", left);
            else if (!left->creg) pasm("%v", left);

            // skip_space(curr->space);
            if (right->ptr) pasm(", %a", right);
            else if (right->creg) pasm(", %r", right);
            else if (!right->creg) pasm(", %v", right);
            skip_space(curr->space); pasm("%i .%s%zu", inst, curr->name, curr->index);
         }
         else
         {
            char *inst = left->type == FLOAT ? "movss" : "mov";
            if (left->ptr) pasm("%i%r, %a", inst, left, left);
            else if (left->creg /*&& strcmp(left->creg, r->creg)*/)
               pasm("%i%r, %r", inst, left, left);
            else if (!left->creg) pasm("%i%r, %v", inst, left, left);

            char *reg = NULL;
            switch (left->type)
            {
            case INT: reg = "ebx"; break;
            case FLOAT: reg = "xmm1"; break;
            case CHAR: reg = "bl"; break;
            case BOOL: reg = "ebx"; break;
            default: check(1, "Unkown type [%s]\n", to_string(left->type)); break;
            }
            skip_space(curr->space);
            if (right->ptr) pasm("%i%s, %a", inst, reg, right);
            else if (right->creg) pasm("%i%s, %r", inst, reg, right);
            else if (!right->creg) pasm("%i%s, %v", inst, reg, right);

            inst = left->type == FLOAT ? "ucomiss" : "cmp";
            skip_space(curr->space);
            pasm("%i%r, %s", inst, left, reg);
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
            setReg(curr, "al");
            skip_space(curr->space);
            pasm("%i%r", inst, curr);
         }
         break;
      }
      case FDEC:
      {
         pasm("%s:", curr->name);
         skip_space(curr->space + TAB); pasm("%irbp", "push");
         skip_space(curr->space + TAB); pasm("%irbp, rsp", "mov");
         skip_space(curr->space + TAB); pasm("%irsp, %zu", "sub", (((curr->ptr) + 15) / 16) * 16);
         break;
      }
      case JE:
      {
         pasm("%ial, 1", "cmp");
         skip_space(curr->space); pasm("%i.%s%zu", "je", curr->name, curr->index);
         break;
      }
      case JNE:
      {
         pasm("%ial, 1", "cmp");
         skip_space(curr->space); pasm("%i.%s%zu", "jne", curr->name, curr->index);
         break;
      }
      case JMP:
      {
         pasm("%i.%s%zu", "jmp", curr->name, curr->index);
         break;
      }
      case FCALL:
      {
         pasm("%i%s", "call", curr->name);
         break;
      }
      case BLOC:
      {
         pasm(".%s%zu:", (curr->name ? curr->name : "(null)"), curr->index);
         break;
      }
      case END_BLOC:
      {
         pasm(".end%s:", curr->name);
         break;
      }
      case RETURN:
      {
         if (left->ptr) pasm("%i%r, %a", "mov", left, left);
         else if (left->creg)
         {
            // TODO: check the type
            if (strcmp(left->creg, "eax")) pasm("%i%r, %a", "mov", left, left);
         }
         else
         {
            switch (left->type)
            {
            case INT: pasm("%i%r, %ld", "mov", left, left->Int.value); break;
            case VOID: pasm("%ieax, 0", "mov"); break;
            default:
            {
               check(1, "handle this case [%s]\n", to_string(left->type));
               break;
            }
            }
         }
         skip_space(curr->space); pasm("%i", "leave");
         skip_space(curr->space); pasm("%i", "ret");
         break;
      }
      // case PUSH:
      // {
      //   /*
      //     left: source
      //     right: destination
      //   */
      //   if(right->name) pasm("mov %s, ", right->name);
      //   else if(right->ptr) check(1, "handle this case");
      //   else if(right->creg) pasm("mov %r, ", right);
      //   else check(1, "handle this case");

      //   if(left->ptr) pasm("%a\n", left);
      //   else if(left->creg) pasm("%r\n", left);
      //   else pasm("%v\n", left);
      //   break;
      // }
      // case POP:
      // {
      //   /*
      //     left: source
      //     right: destination
      //   */
      //   if(right->name) pasm("mov %s, ", right->name);
      //   else if(right->ptr) check(1, "handle this case");
      //   else if(right->creg) pasm("mov %r, ", right);
      //   else check(1, "handle this case");

      //   if(left->ptr) pasm("%a\n", left);
      //   else if(left->creg) pasm("%r\n", left);
      //   else pasm("%v\n", left);
      //   break;
      // }
      default: check(1, "handle this case (%s)\n", to_string(curr->type)); break;
      }
   }
   finalize();
}

void add_builtins()
{
   if(found_error) return;
   create_builtin("putnbr", (Type[]) {INT, 0}, INT);
   create_builtin("write", (Type[]) {INT, CHARS, INT, 0}, INT);
   // create_builtin("read", (Type[]){int_, chars_, int_, 0}, int_);
   // create_builtin("exit", (Type[]){int_, 0}, int_);
   // create_builtin("malloc", (Type[]){int_, 0}, ptr_);
   // create_builtin("calloc", (Type[]){int_, int_, 0}, ptr_);
   // create_builtin("strdup", (Type[]){chars_, 0}, chars_);
   create_builtin("strlen", (Type[]) {CHARS, 0}, INT);
   // create_builtin("free", (Type[]){ptr_, 0}, void_);
   // create_builtin("strcpy", (Type[]){chars_, chars_, 0}, chars_);
   // create_builtin("strncpy", (Type[]){chars_, chars_, int_, 0}, chars_);
   // // create_builtin("puts", (Type[]){chars_, 0}, int_);
   create_builtin("putstr", (Type[]) {CHARS, 0}, INT);
   create_builtin("putchar", (Type[]) {CHAR, 0}, INT);
   // create_builtin("putbool", (Type[]){bool_, 0}, int_);
   // create_builtin("putfloat", (Type[]){float_, 0}, int_);
}

void generate(char *name)
{
   if (found_error) return;
   debug(BLUE BOLD"GENERATE IR:\n" RESET);
   enter_scoop("");
   Node *curr = head;
   add_builtins();
   while (curr && !found_error)
   {
      generate_ir(curr->left);
      curr = curr->right;
   }
   if (found_error) return;
   debug(BLUE BOLD"PRINT IR:\n" RESET);
   print_ir();
   debug(BLUE BOLD"OPTIMIZE IR:\n" RESET);
   while (!found_error && optimize_ir()) ;
#if 1
   debug(BLUE BOLD"GENERATE ASM:\n" RESET);
   generate_asm(name);
#endif
}

int main(int argc, char **argv)
{
   check(argc < 2, "Invalid arguments");
   open_file(argv[1]);
   tokenize();
   generate_ast();
   generate(argv[1]);
   free_memory();
}