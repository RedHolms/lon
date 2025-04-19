#include "ast.h"

#include <stdlib.h>

void LonType_Destroy(LonType* tp) {
  if (!tp) return;
  switch (tp->kind) {
    case TP_REFERENCE:
    case TP_POINTER:
      if (tp->child) {
        LonType_Destroy(tp->child);
        free(tp->child);
      }
      break;
    default:
      break;
  }
}

void LonLiteral_Destroy(LonLiteral* lit) {
  if (!lit) return;
  if (lit->tp == LIT_STRING && lit->strVal)
    free(lit->strVal);
}

void LonExpression_Destroy(LonExpression* expr) {
  if (!expr) return;
  switch (expr->tp) {
    case EXPR_CALL:
      if (expr->call.name)
        free(expr->call.name);

      LonExpression* arg = expr->call.args;
      while (arg) {
        LonExpression* next = arg->next;
        LonExpression_Destroy(arg);
        free(arg);
        arg = next;
      }
      break;
    case EXPR_LITERAL:
      LonLiteral_Destroy(expr->literal);
      free(expr->literal);
      break;
  }
}

void LonStatement_Destroy(LonStatement* st) {
  if (!st) return;
  switch (st->tp) {
    case ST_EXPR:
    case ST_RETURN:
      LonExpression_Destroy(st->expr);
      free(st->expr);
      break;
    case ST_BLOCK:
      LonStatement_Destroy(st->block);
      free(st->block);
      break;
  }
}

void LonRootStatement_Destroy(LonRootStatement* rst) {
  if (!rst) return;
  switch (rst->tp) {
    case RST_FUNCTION:
      if (rst->func.name)
        free(rst->func.name);
      if (rst->func.returnType) {
        LonType_Destroy(rst->func.returnType);
        free(rst->func.returnType);
      }
      LonStatement* st = rst->func.body;
      while (st) {
        LonStatement* next = st->next;
        LonStatement_Destroy(st);
        free(st);
        st = next;
      }
      break;
    default:
      break;
  }
}
