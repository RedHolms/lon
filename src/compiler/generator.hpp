#pragma once

#include <stdio.h>
#include <vector>
#include "ast/ast.hpp"

namespace lon {

  struct ImportLibrary {
    std::string libName;
    std::string normName;
    std::list<std::string> procedures;
  };

  struct BinaryData {
    std::string name;
    std::vector<uint8_t> data;
  };

  class Generator {
  private:
    FILE* m_outFile;

    std::list<BinaryData> m_data;
    std::list<ImportLibrary> m_imports;
    int m_stringsCount;

  public:
    Generator();
    ~Generator();

  public:
    void generate(
      AbstractSourceTree const& ast,
      FILE* outFile
    );

  private:
    void genCall(const char* name, std::list<Expression> const& args, int dest);
    void genLiteral(Literal const* lit, int dest);
    void genExpression(Expression const* expr, int dest);
    void genFunction(FunctionDefinition const* func);

    void importProc(const char* libName, const char* procName);
    void useData(const char* name, const void* data, int length);
    void out(const char* fmt, ...);
  };

}
