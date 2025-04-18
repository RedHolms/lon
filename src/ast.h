#ifndef LON_AST_H_
#define LON_AST_H_

typedef enum LonTypeKind {
  TP_VOID,
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

typedef enum LonLiteralType {
  LIT_INT,
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

typedef enum LonExpressionType {
  EX_CALL,
  EX_LITERAL
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

typedef enum LobStatementType {
  ST_EXPR,
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

typedef enum LonRootStatementType {
  RST_FUNCTION,
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

#endif // LON_AST_H_
