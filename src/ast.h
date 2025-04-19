#ifndef LON_AST_H_
#define LON_AST_H_

typedef enum LonTypeKind {
  TP_VOID = 1,
  TP_NUMBER,
  TP_CHAR,
  TP_REFERENCE,
  TP_POINTER
} LonTypeKind;

typedef enum LonTypeFlags {
  TPF_NONE  = 0,
  TPF_CONST = 1 << 0
} LonTypeFlags;

typedef struct LonType {
  LonTypeKind kind;
  LonTypeFlags flags;
  union {
    struct {
      char width; // to find length in bytes, do 1 << width
      char sign; // 0 - unsigned, 1 - signed
    } number;
    struct LonType* child; // pointer, reference
  };
} LonType;

void LonType_Destroy(LonType* tp);

typedef enum LonLiteralType {
  LIT_INT = 1,
  LIT_FLOAT,
  LIT_STRING
} LonLiteralType;

typedef struct LonLiteral {
  LonLiteralType tp;
  union {
    long long intVal;
    double fltVal;
    char* strVal;
  };
} LonLiteral;

void LonLiteral_Destroy(LonLiteral* lit);

typedef enum LonExpressionType {
  EXPR_CALL = 1,
  EXPR_LITERAL
} LonExpressionType;

typedef struct LonExpression {
  LonExpressionType tp;
  union {
    struct {
      char* name;
      struct LonExpression* args;
    } call;
    LonLiteral* literal;
  };

  // used in call args
  struct LonExpression* next;
} LonExpression;

void LonExpression_Destroy(LonExpression* expr);

typedef enum LobStatementType {
  ST_EXPR = 1,
  ST_BLOCK,
  ST_RETURN
} LonStatementType;

typedef struct LonStatement {
  LonStatementType tp;
  union {
    struct LonStatement* block;
    LonExpression* expr;
  };
  struct LonStatement* next;
} LonStatement;

void LonStatement_Destroy(LonStatement* st);

typedef enum LonRootStatementType {
  RST_FUNCTION = 1,
  RST_GLOBAL_VAR
} LonRootStatementType;

typedef struct LonRootStatement {
  LonRootStatementType tp;
  union {
    struct {
      char* name;
      LonType* returnType;
      LonStatement* body;
    } func;
  };
  struct LonRootStatement* next;
} LonRootStatement;

void LonRootStatement_Destroy(LonRootStatement* rst);

#endif // LON_AST_H_
