#include "generator.hpp"

#include <stdarg.h>
#include <algorithm>

using lon::Generator;

enum {
  DEST_NONE,
  DEST_REG_A,
  DEST_REG_B,
  DEST_REG_C,

  DEST_RETURN = DEST_REG_A
};

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

Generator::Generator() {
  m_rootStatements = nullptr;
}

Generator::~Generator() = default;

void Generator::generate(const std::list<std::shared_ptr<RootStatement>>& rootStatements, FILE* outFile) {
  m_rootStatements = &rootStatements;
  m_outFile = outFile;

  m_data.clear();
  m_imports.clear();
  m_stringsCount = 0;

  importProc("KERNEL32.DLL", "ExitProcess");
  importProc("KERNEL32.DLL", "GetStdHandle");
  importProc("KERNEL32.DLL", "WriteConsoleA");

  out(";\n");
  out("; lon generated assembly\n");
  out(";\n");
  out("format PE console\n");
  out("entry __entry\n");
  out("section '.text' code readable executable\n");

  out("%s", __builtin_print);

  for (auto const& rst : rootStatements)
    genFunction((FunctionRootStatement*)rst.get());

  // entry point
  out("__entry: ; ENTRY POINT\n");
  out("  call main\n");
  out("  mov ebx, eax\n");
  out("__die:\n");
  out("  push ebx\n");
  out("  call [ExitProcess]\n");
  out("  jmp __die\n");

  // data segment
  out("section '.data' data readable writeable\n");

  for (auto const& item : m_data) {
    out("%s db ", item.name.c_str());

    int first = 1;
    for (int i = 0; i < item.data.size(); ++i) {
      if (!first)
        out(",");

      first = 0;
      out("%u", ((unsigned)item.data[i]) & 0xFF);
    }

    out("\n");
  }

  // imports
  out("section '.idata' import data readable writeable\n");

  // header
  for (auto const& lib : m_imports)
    out("dd 0,0,0,RVA %s_NAME,RVA %s_TABLE\n", lib.normName.c_str(), lib.normName.c_str());
  out("dd 0,0,0,0,0\n");

  // tables
  for (auto const& lib : m_imports) {
    out("%s_NAME db '%s', 0\n", lib.normName.c_str(), lib.libName.c_str());

    // names
    for (auto const& procName : lib.procedures) {
      out("%s_ENTRY dw 0\n", procName.c_str());
      out(" db '%s', 0\n", procName.c_str());
    }

    out("%s_TABLE:\n", lib.normName.c_str());

    for (auto const& procName : lib.procedures)
      out("%s dd RVA %s_ENTRY\n", procName.c_str(), procName.c_str());

    out("dd 0\n");
  }
}

void Generator::genCall(const char* name, std::list<std::shared_ptr<Expression>> const& args, int dest) {
  if (strcmp(name, "print") == 0) {
    // FIXME no checks
    auto arg = (LiteralExpression*)args.begin()->get();
    genExpression(arg, DEST_REG_B);
    out("  mov ecx, %d\n", ((StringLiteral*)arg->value.get())->value.length());
    out("  call __builtin_print\n");
    switch (dest) {
      case DEST_REG_B:
        out("  mov ebx, eax\n"); break;
      case DEST_REG_C:
        out("  mov ecx, eax\n"); break;
    }
    return;
  }

  printf("Unknown function %s\n", name);
}

void Generator::genLiteral(Literal* lit, int dest) {
  if (
    lit->type == LiteralType::INT &&
    ((IntLiteral*)lit)->value == 0
  ) {
    switch (dest) {
      case DEST_REG_A: out("  xor eax, eax\n"); break;
      case DEST_REG_B: out("  xor ebx, ebx\n"); break;
      case DEST_REG_C: out("  xor ecx, ecx\n"); break;
    }
    return;
  }

  switch (dest) {
    case DEST_REG_A: out("  mov eax, "); break;
    case DEST_REG_B: out("  mov ebx, "); break;
    case DEST_REG_C: out("  mov ecx, "); break;
  }

  switch (lit->type) {
    case LiteralType::INT:
      out("%lld\n", ((IntLiteral*)lit)->value); break;
    case LiteralType::STRING: {
      auto strLit = (StringLiteral*)lit;
      int id = m_stringsCount++;
      char buffer[32];
      sprintf(buffer, "str%d", id);
      useData(buffer, strLit->value.data(), strLit->value.length());
      out("%s\n", buffer);
    } break;
    case LiteralType::FLOAT:
      break;
  }
}

void Generator::genExpression(Expression* expr, int dest) {
  switch (expr->type) {
    case ExpressionType::CALL: {
      auto callExpr = (CallExpression*)expr;
      genCall(callExpr->funcName.c_str(), callExpr->args, dest);
    } break;
    case ExpressionType::LITERAL: {
      genLiteral(((LiteralExpression*)expr)->value.get(), dest);
    } break;
  }
}

void Generator::genFunction(FunctionRootStatement* func) {
  out("%s: ; func\n", func->funcName.c_str());

  for (auto& st : func->body) {
    switch (st->type) {
      case StatementType::RETURN:
        genExpression(((ReturnStatement*)st.get())->value.get(), DEST_RETURN);
        out("  ret\n");
        break;
      case StatementType::EXPR:
        genExpression(((ExpressionStatement*)st.get())->expr.get(), DEST_NONE);
        break;
      case StatementType::BLOCK:
        //todo
        break;
    }
  }
}

void Generator::importProc(const char* libName, const char* procName) {
  std::string name = libName;
  std::transform(name.begin(), name.end(), name.begin(), toupper);

  auto it = std::find_if(
    m_imports.begin(), m_imports.end(),
    [&name](ImportLibrary& i) { return i.libName == name; }
  );

  if (it == m_imports.end()) {
    std::string normName = name;
    for (auto& chr : normName) {
      if (!isalnum(chr))
        chr = '_';
    }

    m_imports.push_back({ libName, normName, std::list<std::string>{} });
    it = --m_imports.end();
  }

  // find if procedure already imported
  auto jt = std::find(it->procedures.begin(), it->procedures.end(), procName);
  if (jt == it->procedures.end())
    it->procedures.emplace_back(procName);
}

void Generator::useData(const char* name, const void* data, int length) {
  m_data.push_back({ name, std::vector<uint8_t>((uint8_t*)data, ((uint8_t*)data) + length) });
}

void Generator::out(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(m_outFile, fmt, args);
}
