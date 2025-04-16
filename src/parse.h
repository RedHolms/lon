#ifndef LON_PARSE_H_
#define LON_PARSE_H_

enum {
  VTK_VOID,
  VTK_NUMBER,
  VTK_CHAR,
  VTK_POINTER
};

enum {
  VTFL_NONE = 0,
  VTFL_CONST = 1 << 0
};

typedef struct ast_value_type {
  int kind;
  int flags;
  union {
    struct {
      char width; // to find byte length, do 1 << width
      char sign; // 0 - unsigned, 1 - signed
    } number;
    struct {
      struct ast_value_type* vt;
    } pointer;
  } v;
} ast_value_type;

enum {
  LIT_INT,
  LIT_FLOAT,
  LIT_STRING
};

typedef struct ast_literal {
  int tp;
  union {
    long long intVal;
    double fltVal;
    char* strVal;
  } v;
} ast_literal;

enum {
  EXPR_CALL,
  EXPR_LITERAL
};

typedef struct ast_expression {
  int tp;
  union {
    struct {
      char* name;
      struct ast_expression* args;
    } call;
    ast_literal* literal;
  } v;
  // not always used
  struct ast_expression* next;
} ast_expression;

enum {
  ST_EXPR,
  ST_BLOCK,
  ST_RETURN
};

typedef struct ast_statement {
  int tp;
  union {
    struct ast_statement* block;
    ast_expression* expr;
  } v;
  struct ast_statement* next;
} ast_statement;

enum {
  RST_FUNCTION,
  RST_GLOBAL_VAR
};

typedef struct ast_root_statement {
  int tp;
  union {
    struct {
      char* name;
      ast_value_type* ret;
      ast_statement* body;
    } func;
  } v;
  struct ast_root_statement* next;
} ast_root_statement;

void ast_init(void);
void ast_parse(void);
void ast_print(void);

#endif // LON_PARSE_H_
