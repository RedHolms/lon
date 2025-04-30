#include "generator.h"

#include <string.h>
#include <ctype.h>
#include "utils.h"

#define outf(...) fprintf(gen->outFile,__VA_ARGS__)

// ebx - string pointer
// ecx - string length
static const char* __builtin_print =
  "__builtin_print: ; builtin\n"
  "  push -11\n"
  "  call [GetStdHandle]\n"
  "  push 0\n"
  "  push 0\n"
  "  push ecx\n"
  "  push ebx\n"
  "  push eax\n"
  "  call [WriteConsoleA]\n"
  "  ret\n";

enum {
  DEST_NONE,
  DEST_REG_A,
  DEST_REG_B,
  DEST_REG_C
};

// replace all non-ascii chars
static OwnedStr normalize_lib_name(const char* libName) {
  char* normName = _strupr(strclone(libName));
  for (char* it = normName; *it; ++it) {
    if (!isalnum(*it) && *it != '_')
      *it = '_';
  }
  return normName;
}

static void use_external_proc(LonGenerator* gen, const char* libNameRaw, const char* procName) {
  // make lib name uppercase
  char* libName = _strupr(strclone(libNameRaw));

  // search for LonImportLibrary
  LonImportLibrary* lib = gen->imports.head;
  while (lib) {
    if (strcmp(libName, lib->libName) == 0) {
      free(libName);
      break;
    }

    lib = lib->next;
  }

  if (!lib) {
    // create new
    lib = AllocStruct(LonImportLibrary);
    lib->normName = normalize_lib_name(libName);
    lib->libName = libName;
    LinkedList_Append(gen->imports.head, gen->imports.tail, lib);
  }

  // search if proc already imported
  LonImportEntry* entry = lib->head;
  while (entry) {
    if (strcmp(procName, entry->procName) == 0) {
      // ignore
      return;
    }

    entry = entry->next;
  }

  // append
  entry = AllocStruct(LonImportEntry);
  entry->procName = strclone(procName);

  LinkedList_Append(lib->head, lib->tail, entry);
}

static void use_data(LonGenerator* gen, const char* name, const void* data, int length) {
  LonBinaryData* item = AllocStruct(LonBinaryData);
  item->name = strclone(name);
  item->data = (char*)malloc(length);
  memcpy(item->data, data, length);
  item->length = length;
  LinkedList_Append(gen->data.head, gen->data.tail, item);
}

static void gen_expr(LonGenerator* gen, LonExpression* expr, int dest);

static void gen_call(LonGenerator* gen, const char* name, LonExpression* args, int dest) {
  if (strcmp(name, "print") == 0) {
    // FIXME no checks
    gen_expr(gen, args, DEST_REG_B);
    outf("  mov ecx, %d\n", (int)strlen(args->literal->strVal));
    outf("  call __builtin_print\n");
    switch (dest) {
      case DEST_REG_B:
        outf("  mov ebx, eax\n"); break;
      case DEST_REG_C:
        outf("  mov ecx, eax\n"); break;
    }
    return;
  }

  printf("Unknown function %s\n", name);
}

static void gen_lit(LonGenerator* gen, LonLiteral* lit, int dest) {
  if (lit->tp == LIT_INT && lit->intVal == 0) {
    switch (dest) {
      case DEST_REG_A: outf("  xor eax, eax\n"); break;
      case DEST_REG_B: outf("  xor ebx, ebx\n"); break;
      case DEST_REG_C: outf("  xor ecx, ecx\n"); break;
    }
    return;
  }

  switch (dest) {
    case DEST_REG_A: outf("  mov eax, "); break;
    case DEST_REG_B: outf("  mov ebx, "); break;
    case DEST_REG_C: outf("  mov ecx, "); break;
  }

  switch (lit->tp) {
    case LIT_INT:
      outf("%lld\n", lit->intVal); break;
    case LIT_STRING: {
      int id = gen->stringsCount++;
      char buffer[32];
      sprintf(buffer, "str%d", id);
      use_data(gen, buffer, lit->strVal, strlen(lit->strVal) + 1);
      outf("%s\n", buffer);
    } break;
    case LIT_FLOAT:
      break;
  }
}

static void gen_expr(LonGenerator* gen, LonExpression* expr, int dest) {
  switch (expr->tp) {
    case EXPR_CALL:
      gen_call(gen, expr->call.name, expr->call.args, dest);
      break;
    case EXPR_LITERAL:
      gen_lit(gen, expr->literal, dest);
      break;
  }
}

static void gen_body(LonGenerator* gen, LonStatement* body, LonType* retType) {
  while (body) {
    switch (body->tp) {
      case ST_RETURN:
        gen_expr(gen, body->expr, DEST_REG_A);
        outf("  ret\n");
        break;
      case ST_EXPR:
        gen_expr(gen, body->expr, DEST_NONE);
        break;
      case ST_BLOCK:
        gen_body(gen, body->block, NULL);
        break;
    }
    body = body->next;
  }
}

void LonGenerator_Init(LonGenerator* gen, LonRootStatement* statements, FILE* outFile) {
  gen->outFile = outFile;
  gen->rst = statements;
  gen->imports.head = NULL;
  gen->imports.tail = NULL;
  gen->data.head = NULL;
  gen->data.tail = NULL;
  gen->stringsCount = 0;
  gen->error = NULL;
}

void LonGenerator_Destroy(LonGenerator* gen) {

}

void LonGenerator_Generate(LonGenerator* gen) {
  use_external_proc(gen, "KERNEL32.DLL", "ExitProcess");
  use_external_proc(gen, "KERNEL32.DLL", "GetStdHandle");
  use_external_proc(gen, "KERNEL32.DLL", "WriteConsoleA");

  outf(";\n");
  outf("; lon generated assembly\n");
  outf(";\n");
  outf("format PE console\n");
  outf("entry __entry\n");
  outf("section '.text' code readable executable\n");

  outf("%s", __builtin_print);

  LonRootStatement* rst = gen->rst;
  while (rst && !gen->error) {
    outf("%s: ; function\n", rst->func.name);
    gen_body(gen, rst->func.body, rst->func.returnType);
    rst = rst->next;
  }

  if (gen->error)
    return;

  // entry point
  outf("__entry: ; ENTRY POINT\n");
  outf("  call main\n");
  outf("  mov ebx, eax\n");
  outf("__die:\n");
  outf("  push ebx\n");
  outf("  call [ExitProcess]\n");
  outf("  jmp __die\n");

  // data segment
  outf("section '.data' data readable writeable\n");

  LonBinaryData* item = gen->data.head;
  while (item) {
    outf("%s db ", item->name);

    int first = 1;
    for (int i = 0; i < item->length; ++i) {
      if (!first)
        outf(",");

      first = 0;
      outf("%u", ((unsigned)item->data[i]) & 0xFF);
    }

    outf("\n");

    item = item->next;
  }

  // imports
  outf("section '.idata' import data readable writeable\n");

  // header
  LonImportLibrary* lib = gen->imports.head;
  while (lib) {
    outf("dd 0,0,0,RVA %s_NAME,RVA %s_TABLE\n", lib->normName, lib->normName);
    lib = lib->next;
  }
  outf("dd 0,0,0,0,0\n");

  // tables
  lib = gen->imports.head;
  while (lib) {
    outf("%s_NAME db '%s', 0\n", lib->normName, lib->libName);

    // names
    LonImportEntry* entry = lib->head;
    while (entry) {
      outf("%s_ENTRY dw 0\n", entry->procName);
      outf(" db '%s', 0\n", entry->procName);
      entry = entry->next;
    }

    outf("%s_TABLE:\n", lib->normName);

    entry = lib->head;
    while (entry) {
      outf("%s dd RVA %s_ENTRY\n", entry->procName, entry->procName);
      entry = entry->next;
    }

    outf("dd 0\n");

    lib = lib->next;
  }
}