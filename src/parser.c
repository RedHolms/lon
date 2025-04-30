#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.hpp"
#include "trie.h"
#include "str.h"
#include "utils.h"

// Built-in types
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

// push to any linked list
#define ll_push(head, tail, elem) do { elem->next = NULL; if (tail) tail->next = elem; else head = tail = elem; } while(0)

static inline LonToken* next_tok(LonParser* parser) {
  return parser->tk = parser->tk ? parser->tk->next : NULL;
}

static inline int check_tok(LonParser* parser, int id) {
  return parser->tk && parser->tk->id == id;
}

static inline int assert_tok(LonParser* parser, int id, const char* errorInfo) {
  if (!check_tok(parser, id)) {
    parser->error = errorInfo;
    return 0;
  }

  return 1;
}

static LonType* create_void_type() {
  LonType* tp = AllocStruct(LonType);
  tp->kind = TP_VOID;
  return tp;
}

static LonType* parse_typename(LonParser* parser) {
  char* str = parser->tk->string;
  next_tok(parser);

  long v = trie_get(&parser->builtinTypes, str);
  int sign = 0;

  if (v == BTP_CONST) {
    LonType* tp = parse_typename(parser);
    if (tp->flags & TPF_CONST) {
      parser->error = "Multiple const modifiers";
      LonType_Destroy(tp);
      free(tp);
      return NULL;
    }

    tp->flags |= TPF_CONST;
    return tp;
  }

  if (v == BTP_UNSIGNED)
    sign = 1;
  else if (v == BTP_SIGNED)
    sign = -1;

  if (sign != 0) {
    if (!assert_tok(parser, TK_ID, "Expected numeric type name"))
      return NULL;

    str = parser->tk->string;
    next_tok(parser);
    v = trie_get(&parser->builtinTypes, str);

    if (v < BTP_BYTE || v > BTP_LONG) {
      parser->error = "Expected numeric type name, but got non-numeric type name";
      return NULL;
    }
  }

  int width = 0;
  switch (v) {
    case BTP_VOID:
      return create_void_type();

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
      LonType* tp = AllocStruct(LonType);
      tp->kind = TP_NUMBER;
      tp->number.width = width;
      tp->number.sign = sign == -1 ? 1 : 0;
      return tp;
    }

    case BTP_CHAR:
      LonType* tp = AllocStruct(LonType);
      tp->kind = TP_CHAR;
      return tp;
  }

  // TODO pointers

  parser->error = "Expected type name";
  return NULL;
}

static LonExpression* parse_expression(LonParser* parser) {
  LonExpression* expr = AllocStruct(LonExpression);

  switch (parser->tk->id) {
    case TK_NUMBER:
      expr->tp = EXPR_LITERAL;
      expr->literal = AllocStruct(LonLiteral);
      expr->literal->tp = LIT_INT;
      expr->literal->intVal = 0; // FIXME TODO
      break;
    case TK_STRING:
      expr->tp = EXPR_LITERAL;
      expr->literal = AllocStruct(LonLiteral);
      expr->literal->tp = LIT_STRING;
      expr->literal->strVal = strclone(parser->tk->string);
      break;
    default:
      parser->error = "Expected valid expression";
      free(expr);
      return NULL;
  }

  next_tok(parser);
  return expr;
}

static LonStatement* parse_block(LonParser* parser) {
  LonStatement* head = NULL;
  LonStatement* tail = NULL;

  while (parser->tk && parser->tk->id != '}') {
    LonStatement* st = AllocStruct(LonStatement);
    switch (parser->tk->id) {
      case TK_RETURN:
        next_tok(parser);
        st->tp = ST_RETURN;
        st->expr = parse_expression(parser);
        break;
      case TK_ID: {
        const char* name = parser->tk->string;
        next_tok(parser);

        // FIXME only call available for now
        if (!assert_tok(parser, '(', "Only call available for now")) {
          free(st);
          return head;
        }

        st->tp = ST_EXPR;
        st->expr = AllocStruct(LonExpression);
        st->expr->tp = EXPR_CALL;
        st->expr->call.name = strclone(name);
        st->expr->call.args = NULL;
        LonExpression* argsTail = NULL;

        next_tok(parser);

        int had_comma = 1;
        while (1) {
          if (!parser->tk) {
            parser->error = "Unexpected EOF in function arguments list";
            LonStatement_Destroy(st);
            free(st);
            return head;
          }

          if (parser->tk->id == ')')
            break;

          if (!had_comma) {
            parser->error = "Unexpected arguments list closing parenthesis";
            LonStatement_Destroy(st);
            free(st);
            return head;
          }

          had_comma = 0;

          LonExpression* arg = parse_expression(parser);
          if (!arg) {
            LonStatement_Destroy(st);
            free(st);
            return head;
          }

          LinkedList_Append(st->expr->call.args, argsTail, arg);

          if (check_tok(parser, ',')) {
            had_comma = 1;
            next_tok(parser);
          }
        }

        next_tok(parser);
      } break;
      default:
        parser->error = "Expected valid statement";
        free(st);
        return head;
    }

    if (!assert_tok(parser, ';', "Expected semicolon")) {
      LonStatement_Destroy(st);
      free(st);
      return head;
    }

    next_tok(parser);
    LinkedList_Append(head, tail, st);
  }

  if (!assert_tok(parser, '}', "Unexpected EOF before block is closed"))
    return head;

  next_tok(parser);
  return head;
}

void LonParser_Init(LonParser* parser, LonLexer* lex) {
  memset(&parser->builtinTypes, 0, sizeof(parser->builtinTypes));
  trie_insert(&parser->builtinTypes, "void", BTP_VOID);
  trie_insert(&parser->builtinTypes, "byte", BTP_BYTE);
  trie_insert(&parser->builtinTypes, "short", BTP_SHORT);
  trie_insert(&parser->builtinTypes, "integer", BTP_INTEGER);
  trie_insert(&parser->builtinTypes, "long", BTP_LONG);
  trie_insert(&parser->builtinTypes, "char", BTP_CHAR);
  trie_insert(&parser->builtinTypes, "unsigned", BTP_UNSIGNED);
  trie_insert(&parser->builtinTypes, "signed", BTP_SIGNED);
  trie_insert(&parser->builtinTypes, "const", BTP_CONST);
  parser->lexer = lex;
  parser->tk = parser->lexer->tokens;
  parser->rootStatements = parser->tail = NULL;
  parser->error = NULL;
}

void LonParser_Destroy(LonParser* parser) {}

void LonParser_Parse(LonParser* parser) {
  while (parser->tk && !parser->error) {
    switch (parser->tk->id) {
      default:
        parser->error = "Unexpected token";
        return;

      case TK_FUNCTION:
        next_tok(parser);

        if (!assert_tok(parser, TK_ID, "Expected function name"))
          return;

        LonRootStatement* rst = AllocStruct(LonRootStatement);
        rst->tp = RST_FUNCTION;
        rst->func.name = strclone(parser->tk->string);

        next_tok(parser);
        if (!assert_tok(parser, '(', "Expected open parenthesis for arguments"))
          goto FUNC_FAIL;

        // TODO args

        next_tok(parser);
        if (!assert_tok(parser, ')', "Expected close parenthesis for arguments"))
          goto FUNC_FAIL;

        next_tok(parser);
        if (!parser->tk) {
          parser->error = "Unexpected EOF. Expected return arrow or function body";
          goto FUNC_FAIL;
        }

        if (parser->tk->id == TK_RET_ARROW) {
          next_tok(parser);
          rst->func.returnType = parse_typename(parser);
          if (parser->error)
            goto FUNC_FAIL;
        }
        else {
          rst->func.returnType = create_void_type();
        }

        if (!parser->tk) {
          parser->error = "Unexpected EOF. Expected return arrow or function body";
          goto FUNC_FAIL;
        }

        if (parser->tk->id == ';') {
          // TODO pre-definition
        }
        else if (parser->tk->id == '{') {
          next_tok(parser);
          rst->func.body = parse_block(parser);
          if (parser->error)
            goto FUNC_FAIL;
        }

        LinkedList_Append(parser->rootStatements, parser->tail, rst);
        break;
      
      FUNC_FAIL:
        LonRootStatement_Destroy(rst);
        if (rst)
          free(rst);
        return;
    }
  }
}

static void print_type(LonType* tp, FILE* outFile) {
  if (tp->flags & TPF_CONST)
    fprintf(outFile, "const ");
  
  switch (tp->kind) {
    case TP_VOID:
      fprintf(outFile, "void"); return;
    case TP_NUMBER:
      fprintf(outFile, "number<");
      if (tp->number.sign)
        fprintf(outFile, "signed, ");
      else
        fprintf(outFile, "unsigned, ");
      fprintf(outFile, "%d bits>", (1 << tp->number.width) << 3);
      return;
    case TP_CHAR:
    case TP_REFERENCE:
    case TP_POINTER:
      break;
  }
}

static void print_lit(LonLiteral* lit, FILE* outFile) {
  switch (lit->tp) {
    case LIT_INT:
      fprintf(outFile, "int<%lld>", lit->intVal);
      break;
    case LIT_STRING:
      fprintf(outFile, "string<%s>", lit->strVal);
      break;
    case LIT_FLOAT:
      fprintf(outFile, "float<%f>", lit->fltVal);
      break;
  }
}

static void print_expr(LonExpression* expr, FILE* outFile, int indent) {
  switch (expr->tp) {
    case EXPR_CALL: {
      fprintf(outFile, "call %s (", expr->call.name);
      LonExpression* arg = expr->call.args;
      while (arg) {
        print_expr(arg, outFile, indent);
        if ((arg = arg->next))
          fprintf(outFile, ", ");
      }
      fprintf(outFile, ")");
    } break;
    case EXPR_LITERAL:
      fprintf(outFile, "literal ");
      print_lit(expr->literal, outFile);
      break;
  }
}

static void print_block(LonStatement* st, FILE* outFile, int indent) {
  fprintf(outFile, "%*c{\n", indent, ' ');
  indent += 2;
  while (st) {
    switch (st->tp) {
      case ST_EXPR:
        fprintf(outFile, "%*c", indent, ' ');
        print_expr(st->expr, outFile, indent + 2);
        fprintf(outFile, ";\n");
        break;
      case ST_RETURN:
        fprintf(outFile, "%*creturn ", indent, ' ');
        print_expr(st->expr, outFile, indent + 2);
        fprintf(outFile, ";\n");
        break;
      case ST_BLOCK:
        break;
    }
    st = st->next;
  }
  indent -= 2;
  fprintf(outFile, "%*c}", indent, ' ');
}

void LonParser_Print(LonParser* parser, FILE* outFile) {
  if (parser->error) {
    if (parser->tk)
      fprintf(outFile, "Parser error! At %d:%d. %s.\n", parser->tk->row, parser->tk->column, parser->error);
    else
      fprintf(outFile, "Parser error! %s.\n", parser->error);
    return;
  }

  fprintf(outFile, "Parser result:\n");

  LonRootStatement* rst = parser->rootStatements;
  while (rst) {
    fprintf(outFile, "  Root statement:\n");
    switch (rst->tp) {
      case RST_FUNCTION:
        fprintf(outFile, "    Function %s -> ", rst->func.name);
        print_type(rst->func.returnType, outFile);
        fprintf(outFile, "\n      Body:\n");
        print_block(rst->func.body, outFile, 8);
        fprintf(outFile, "\n");
        break;
      case RST_GLOBAL_VAR:
        break;
    }
    rst = rst->next;
  }
}
