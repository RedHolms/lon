#include "gen.h"

#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "str.h"

static FILE* out;

// ebx - string pointer
// ecx - string length
static const char* __builtin_print = R"(
__builtin_print:
  push -11
  call [GetStdHandle]
  push 0
  push 0
  push ecx
  push ebx
  push eax
  call [WriteConsoleA]
  ret
)";

static const char* __entry = R"(
__entry:
  call main
  mov ebx, eax
__die:
  push ebx
  call [ExitProcess]
  jmp __die
)";

enum {
  DEST_NONE,
  DEST_REG_A,
  DEST_REG_B,
  DEST_REG_C
};

typedef struct const_string {
  int index;
  const char* str;
  struct const_string* next;
} const_string;

static int const_string_idx = 0;
static const_string* const_strings = NULL;

void genexpr(ast_expression* expr, int dest);

void gencall(const char* name, ast_expression* args, int dest) {
  if (strcmp(name, "print") == 0) {
    // FIXME no checks
    genexpr(args, DEST_REG_B);
    fprintf(out, "  mov ecx, %d\n", (int)strlen(args->v.literal->v.strVal));
    fprintf(out, "  call __builtin_print\n");
    switch (dest) {
      case DEST_REG_B:
        fprintf(out, "  mov ebx, eax\n"); break;
      case DEST_REG_C:
        fprintf(out, "  mov ecx, eax\n"); break;
    }
    return;
  }

  printf("Unknown function %s\n", name);
}

void genlit(ast_literal* lit, int dest) {
  if (lit->tp == LIT_INT && lit->v.intVal == 0) {
    switch (dest) {
      case DEST_REG_A: fprintf(out, "  xor eax, eax\n"); break;
      case DEST_REG_B: fprintf(out, "  xor ebx, ebx\n"); break;
      case DEST_REG_C: fprintf(out, "  xor ecx, ecx\n"); break;
    }
    return;
  }

  switch (dest) {
    case DEST_REG_A: fprintf(out, "  mov eax, "); break;
    case DEST_REG_B: fprintf(out, "  mov ebx, "); break;
    case DEST_REG_C: fprintf(out, "  mov ecx, "); break;
  }

  switch (lit->tp) {
    case LIT_INT:
      fprintf(out, "%lld\n", lit->v.intVal); break;
    case LIT_STRING: {
      fprintf(out, "str%d\n", const_string_idx);
      const_string* string = (const_string*)malloc(sizeof(const_string));
      string->index = const_string_idx;
      string->str = strclone(lit->v.strVal);
      string->next = const_strings;
      const_strings = string;
      ++const_string_idx;
    } break;
  }
}

void genexpr(ast_expression* expr, int dest) {
  switch (expr->tp) {
    case EXPR_CALL:
      gencall(expr->v.call.name, expr->v.call.args, dest);
      break;
    case EXPR_LITERAL:
      genlit(expr->v.literal, dest);
      break;
  }
}

void genbody(ast_statement* stmt, ast_value_type* ret) {
  while (stmt) {
    switch (stmt->tp) {
      case ST_RETURN:
        genexpr(stmt->v.expr, DEST_REG_A);
        fprintf(out, "  ret\n");
        break;
      case ST_EXPR:
        genexpr(stmt->v.expr, DEST_NONE);
        break;
    }
    stmt = stmt->next;
  }
}

void codegen(FILE* output) {
  out = output;

  // prologue
  fprintf(out, ";\n");
  fprintf(out, "; lon generated assembly\n");
  fprintf(out, ";\n");
  fprintf(out, "format PE console\n");
  fprintf(out, "entry __entry\n");
  fprintf(out, "section '.text' code readable executable\n");

  // builin functions
  fprintf(out, "%s\n", __builtin_print);

  // functions
  ast_root_statement* rst = ast_result();
  while (rst) {
    if (rst->tp != RST_FUNCTION)
      continue;

    fprintf(out, "%s:\n", rst->v.func.name);
    genbody(rst->v.func.body, rst->v.func.ret);

    rst = rst->next;
  }

  fprintf(out, "%s\n", __entry);

  // strings
  fprintf(out, "section '.data' data readable writeable\n");

  const_string* string = const_strings;
  while (string) {
    fprintf(out, "  str%d db \"%s\", 0\n", string->index, string->str);
    string = string->next;
  }

  // todo hard-coded
  fprintf(out, R"(
section '.idata' import data readable writeable
  dd 0,0,0,RVA KERNEL32DLL_name,RVA KERNEL32DLL_table
  dd 0,0,0,0,0

  KERNEL32DLL_table:
    ExitProcess dd RVA _ExitProcess
    GetStdHandle dd RVA _GetStdHandle
    WriteConsoleA dd RVA _WriteConsoleA
    dd 0

  KERNEL32DLL_name db 'KERNEL32.DLL',0

  _ExitProcess dw 0
    db 'ExitProcess',0
  _GetStdHandle dw 0
    db 'GetStdHandle',0
  _WriteConsoleA dw 0
    db 'WriteConsoleA',0

section '.reloc' fixups data readable discardable
)");
}
