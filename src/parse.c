#include "parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "trie.h"
#include "str.h"

static ast_root_statement* rst_head = NULL;
static ast_root_statement* rst_tail = NULL;
static token* curtk = NULL;

static int have_error = 0;

// check for fail and return if failed
#define check_fail(...) if (have_error) return __VA_ARGS__

enum {
  BTP_VOID,
  BTP_BYTE,
  BTP_SHORT,
  BTP_INTEGER,
  BTP_LONG,
  BTP_CHAR,

  // not a types, but let's store them here
  BTP_UNSIGNED,
  BTP_SIGNED,
  BTP_CONST
};

static trie builtin_types = {0};

// push to any linked list
#define ll_push(head, tail, elem) do { elem->next = NULL; if (tail) tail->next = elem; else head = tail = elem; } while(0)

static inline token* tk_next(void) {
  return curtk = curtk ? curtk->next : NULL;
}

static inline int tk_check(int tp) {
  return !(have_error = !curtk || curtk->tp != tp);
}

static void print_current_token() {
  if (!curtk) {
    puts("No token");
    return;
  }

  if (curtk->tp <= 255) {
    printf("Token: char (%c)\n", curtk->tp);
  }
  else {
    switch (curtk->tp) {
      case TK_NAME: printf("Token: TK_NAME\n"); break;
      case TK_STRING: printf("Token: TK_STRING\n"); break;
      case TK_NUMBER: printf("Token: TK_NUMBER\n"); break;
      case TK_RET_ARROW: printf("Token: TK_RET_ARROW\n"); break;
      case TK_FUNCTION: printf("Token: TK_FUNCTION\n"); break;
      case TK_RETURN: printf("Token: TK_RETURN\n"); break;
    }
  }
}

ast_root_statement* create_rst(int tp) {
  ast_root_statement* rstmt = (ast_root_statement*)malloc(sizeof(ast_root_statement));
  memset(rstmt, 0, sizeof(ast_root_statement));
  rstmt->tp = tp;
  return rstmt;
}

void free_rst(ast_root_statement* stmt) {
  switch (stmt->tp) {
    case RST_FUNCTION:
      if (stmt->v.func.name)
        free(stmt->v.func.name);
      break;
  }

  free(stmt);
}

ast_value_type* create_vtp(int kind) {
  ast_value_type* vtp = (ast_value_type*)malloc(sizeof(ast_value_type));
  memset(vtp, 0, sizeof(ast_value_type));
  vtp->kind = kind;
  return vtp;
}

void free_vtp(ast_value_type* vtp) {
  free(vtp);
}

ast_expression* create_expr(int tp) {
  ast_expression* expr = (ast_expression*)malloc(sizeof(ast_expression));
  memset(expr, 0, sizeof(ast_expression));
  expr->tp = tp;
  return expr;
}

void free_expr(ast_expression* expr) {
  switch (expr->tp) {
    case EXPR_CALL:
      if (expr->v.call.name)
        free(expr->v.call.name);
      if (expr->v.call.args)
        free(expr->v.call.args);
      break;
  }

  if (expr->next)
    free(expr->next);

  free(expr);
}

ast_statement* create_stmt(int tp) {
  ast_statement* stmt = (ast_statement*)malloc(sizeof(ast_statement));
  memset(stmt, 0, sizeof(ast_statement));
  stmt->tp = tp;
  return stmt;
}

void free_stmt(ast_statement* stmt) {
  switch (stmt->tp) {
    case ST_EXPR:
    case ST_RETURN:
      if (stmt->v.expr)
        free_expr(stmt->v.expr);
      break;
    case ST_BLOCK:
      if (stmt->v.block)
        free_stmt(stmt->v.block);
      break;
  }

  free(stmt);
}

ast_value_type* value_type(void) {
  char* str = curtk->str;
  tk_next();

  long v = trie_get(&builtin_types, str);
  int sign = 0;

  if (v == BTP_CONST) {
    ast_value_type* vtp = value_type();
    if (vtp->flags & VTFL_CONST) {
      puts("Multiple consts");
      have_error = 1;
      free_vtp(vtp);
      return NULL;
    }

    vtp->flags |= VTFL_CONST;
    return vtp;
  }

  if (v == BTP_UNSIGNED)
    sign = 1;
  else if (v == BTP_SIGNED)
    sign = -1;

  if (sign != 0) {
    if (!tk_check(TK_NAME)) {
      puts("Got unsigned/signed without a type name");
      have_error = 1;
      return NULL;
    }

    str = curtk->str;
    tk_next();
    v = trie_get(&builtin_types, str);

    if (v < BTP_BYTE || v > BTP_LONG) {
      puts("Got unsigned/signed with not a number type");
      have_error = 1;
      return NULL;
    }
  }

  int width = 0;
  switch (v) {
    case BTP_VOID:
      return create_vtp(VTK_VOID);

    case BTP_BYTE:
      if (sign == 0) sign = 1;
      width = 0;
      goto NUMBER_TYPE;
    case BTP_SHORT:
      if (sign == 0) sign = 1;
      width = 1;
      goto NUMBER_TYPE;
    case BTP_INTEGER:
      if (sign == 0) sign = -1;
      width = 2;
      goto NUMBER_TYPE;
    case BTP_LONG:
      if (sign == 0) sign = -1;
      width = 3;

    NUMBER_TYPE: {
      ast_value_type* vtp = create_vtp(VTK_NUMBER);
      vtp->v.number.width = width;
      vtp->v.number.sign = sign == -1 ? 1 : 0;
      return vtp;
    }

    case BTP_CHAR:
      return create_vtp(VTK_CHAR);
  }

  // TODO pointers

  puts("Unknown type");
  have_error = 1;
  return NULL;
}

ast_expression* expression(void) {
  // FIXME only literals
  ast_expression* expr = create_expr(EXPR_LITERAL);
  expr->v.literal = (ast_literal*)malloc(sizeof(ast_literal));

  switch (curtk->tp) {
    case TK_NUMBER:
      expr->v.literal->tp = LIT_INT;
      expr->v.literal->v.intVal = 0; // FIXME TODO
      break;
    case TK_STRING:
      expr->v.literal->tp = LIT_STRING;
      expr->v.literal->v.strVal = strclone(curtk->str);
      break;
  }

  tk_next();
  return expr;
}

ast_statement* block(void) {
  ast_statement* st_head = NULL;
  ast_statement* st_tail = NULL;
  while (curtk && curtk->tp != '}') {
    switch (curtk->tp) {
      case TK_RETURN: {
        tk_next();

        ast_statement* stmt = create_stmt(ST_RETURN);
        ast_expression* expr = expression();
        stmt->v.expr = expr;

        if (!tk_check(';')) {
          puts("No semicolon after return");
          have_error = 1;
          free_stmt(stmt);
          return NULL;
        }

        tk_next();
        ll_push(st_head, st_tail, stmt);
      } break;
      case TK_NAME: {
        const char* name = curtk->str;
        tk_next();

        // FIXME only call available for now
        if (!tk_check('(')) {
          puts("Only call available for now");
          have_error = 1;
          return NULL;
        }

        ast_statement* stmt = create_stmt(ST_EXPR);
        ast_expression* expr = create_expr(EXPR_CALL);
        stmt->v.expr = expr;

        // FIXME only 1 arg

        tk_next();
        expr->v.call.name = strclone(name);
        expr->v.call.args = expression();

        if (!tk_check(')')) {
          puts("No closing parenthesis in call");
          have_error = 1;
          free_stmt(stmt);
          return NULL;
        }

        tk_next();

        if (!tk_check(';')) {
          puts("No semicolon after call");
          have_error = 1;
          free_stmt(stmt);
          return NULL;
        }

        tk_next();
        ll_push(st_head, st_tail, stmt);
      } break;
      default:
        puts("Unknown token in block");
        print_current_token();
        have_error = 1;
        return NULL;
    }

    if (have_error) return NULL;
  }

  if (!tk_check('}')) {
    puts("No end of the block");
    have_error = 1;
    //TODO free list
    return NULL;
  }

  tk_next();
  return st_head;
}

void ast_init(void) {
  trie_insert(&builtin_types, "void", BTP_VOID);
  trie_insert(&builtin_types, "byte", BTP_BYTE);
  trie_insert(&builtin_types, "short", BTP_SHORT);
  trie_insert(&builtin_types, "integer", BTP_INTEGER);
  trie_insert(&builtin_types, "long", BTP_LONG);
  trie_insert(&builtin_types, "char", BTP_CHAR);
  trie_insert(&builtin_types, "unsigned", BTP_UNSIGNED);
  trie_insert(&builtin_types, "signed", BTP_SIGNED);
  trie_insert(&builtin_types, "const", BTP_CONST);
}

void ast_parse(void) {
  curtk = lex_result();

  while (curtk) {
    switch (curtk->tp) {
      default:
        puts("Unknown token");
        have_error = 1;
        return;

      case TK_FUNCTION:
        tk_next();

        if (!tk_check(TK_NAME)) {
          puts("No function name");
          have_error = 1;
          return;
        }

        ast_root_statement* stmt = create_rst(RST_FUNCTION);
        stmt->v.func.name = strclone(curtk->str);
        
        tk_next();
        if (!tk_check('(')) {
          puts("No arguments open parenthesis");
          goto funcfail;
        }

        // TODO args

        tk_next();
        if (!tk_check(')')) {
          puts("No arguments close parenthesis");
          goto funcfail;
        }

        tk_next();
        if (!curtk) {
          puts("No block or ret arrow");
          goto funcfail;
        }

        if (curtk->tp == TK_RET_ARROW) {
          tk_next();
          if (!tk_check(TK_NAME)) {
            puts("No type after ret arrow");
            goto funcfail;
          }

          stmt->v.func.ret = value_type();
        }
        else {
          stmt->v.func.ret = create_vtp(VTK_VOID);
        }

        if (!curtk) {
          puts("No function body or semicolon");
          goto funcfail;
        }

        if (curtk->tp == ';') {
          // TODO pre-definition
        }
        else if (curtk->tp == '{') {
          tk_next();
          stmt->v.func.body = block();
          if (have_error) goto funcfail;
        }

        ll_push(rst_head, rst_tail, stmt);

        break;
      
      funcfail:
        have_error = 1;
        if (stmt)
          free_rst(stmt);
        return;
    }

    if (have_error) return;
  }
}

static void print_vt(ast_value_type* vt) {
  if (vt->flags & VTFL_CONST)
    printf("const ");
  
  switch (vt->kind) {
    case VTK_VOID:
      printf("void"); return;
    case VTK_NUMBER:
      printf("number<");
      if (vt->v.number.sign)
        printf("signed, ");
      else
        printf("unsigned, ");
      printf("%d bits>", (1 << vt->v.number.width) << 3);
      return;
  }
}

static void print_lit(ast_literal* lit) {
  switch (lit->tp) {
    case LIT_INT:
      printf("int<%lld>", lit->v.intVal);
      break;
    case LIT_STRING:
      printf("string<%s>", lit->v.strVal);
      break;
  }
}

static void print_expr(ast_expression* expr) {
  switch (expr->tp) {
    case EXPR_CALL: {
      printf("call %s (", expr->v.call.name);
      ast_expression* arg = expr->v.call.args;
      while (arg) {
        print_expr(arg);
        if ((arg = arg->next))
          printf(", ");
      }
      printf(")");
    } break;
    case EXPR_LITERAL:
      printf("literal ");
      print_lit(expr->v.literal);
      break;
  }
}

static void print_block(ast_statement* st) {
  puts("{");
  while (st) {
    switch (st->tp) {
      case ST_EXPR:
        printf("  ");
        print_expr(st->v.expr);
        printf(";\n");
        break;
      case ST_RETURN:
        printf("  return ");
        print_expr(st->v.expr);
        printf(";\n");
        break;
    }
    st = st->next;
  }
  puts("}");
}

void ast_print(void) {
  if (have_error) {
    puts("Have an error");
    return;
  }

  ast_root_statement* rst = rst_head;
  while (rst) {
    printf("Root statement:\n");
    switch (rst->tp) {
      case RST_FUNCTION:
        printf(" Function %s -> ", rst->v.func.name);
        print_vt(rst->v.func.ret);
        printf("\n Body:\n");
        print_block(rst->v.func.body);
        break;
    }
    rst = rst->next;
  }
}
