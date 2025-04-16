#include "parse.h"

#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "trie.h"
#include "str.h"

static ast_root_statement* rst_head = NULL;
static ast_root_statement* rst_tail = NULL;
static token* curtk = NULL;

static int have_error = 0;

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

#define ll_push(head, tail, elem) elem->next = NULL; if (tail) tail->next = elem; else head = elem

#define tk_next() curtk = curtk ? curtk->next : NULL

int tk_validate(int tp) {
  if (have_error) return 0;
  return !(have_error = !curtk || curtk->tp != tp);
}

#define tk_validate_next(tp) (tk_next(), tk_validate(tp))

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
      // already const
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
    if (!tk_validate(TK_NAME))
      return NULL;

    str = curtk->str;
    tk_next();
    v = trie_get(&builtin_types, str);

    if (v < BTP_BYTE || v > BTP_LONG) {
      // used with not a number type
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

    NUMBER_TYPE:
      ast_value_type* vtp = create_vtp(VTK_NUMBER);
      vtp->v.number.width = width;
      vtp->v.number.sign = sign == -1 ? 1 : 0;
      return vtp;

    case BTP_CHAR:
      return create_vtp(VTK_CHAR);
  }

  // TODO pointers
  have_error = 1;
  return NULL;
}

ast_literal* literal(void) {

}

ast_expression* expression(void) {
  ast_expression* expr = create_expr(EXPR_LITERAL);

  switch (curtk->tp) {
    case TK_NUMBER:
      expr->v.literal->tp = LIT_INT;
      expr->v.literal->v.intVal = 0;
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
  while (curtk) {
    switch (curtk->tp) {
      case TK_RETURN: {
        tk_next();

        ast_statement* stmt = create_stmt(ST_RETURN);
        ast_expression* expr = expression();
        stmt->v.expr = expr;

        if (!tk_validate(';')) {
          free_stmt(stmt);
          return NULL;
        }

        tk_next();
        ll_push(st_head, st_tail, stmt);
      } break;
      case TK_NAME: {
        const char* name = curtk->str;

        // FIXME only call available for now
        if (!tk_validate_next('('))
          return NULL;

        ast_statement* stmt = create_stmt(ST_EXPR);
        ast_expression* expr = create_expr(EXPR_CALL);
        stmt->v.expr = expr;

        // FIXME only literals

        tk_next();
        expr->v.call.args = expression();

        if (!tk_validate_next(')')) {
          free_stmt(stmt);
          return NULL;
        }

        if (!tk_validate(';')) {
          free_stmt(stmt);
          return NULL;
        }

        tk_next();
        ll_push(st_head, st_tail, stmt);
      } break;
    }

    if (have_error) return NULL;
  }
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
      case TK_FUNCTION:
        if (!tk_validate_next(TK_NAME))
          return;

        ast_root_statement* stmt = create_rst(RST_FUNCTION);
        stmt->v.func.name = strclone(curtk->str);

        if (!tk_validate_next('(')) {
          free_rst(stmt);
          return;
        }

        // TODO args

        if (!tk_validate_next(')')) {
          free_rst(stmt);
          return;
        }

        tk_next();
        if (!curtk) {
          have_error = 1;
          free_rst(stmt);
          return;
        }

        if (curtk->tp == TK_RET_ARROW) {
          if (!tk_validate_next(TK_NAME)) {
            free_rst(stmt);
            return;
          }

          stmt->v.func.ret = value_type();
        }
        else {
          stmt->v.func.ret = create_vtp(VTK_VOID);
        }

        if (curtk->tp == ';') {
          // todo pre-definition
        }
        else if (curtk->tp == '{') {
          tk_next();
          stmt->v.func.body = block();
        }

        ll_push(rst_head, rst_tail, stmt);

        break;
    }

    if (have_error) return;
  }
}
