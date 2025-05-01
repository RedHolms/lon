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
    std::list<std::shared_ptr<RootStatement>> const* m_rootStatements;

    std::list<BinaryData> m_data;
    std::list<ImportLibrary> m_imports;
    int m_stringsCount;

  public:
    Generator();
    ~Generator();

  public:
    void generate(
      std::list<std::shared_ptr<RootStatement>> const& rootStatements,
      FILE* outFile
    );

  private:
    void genCall(const char* name, std::list<std::shared_ptr<Expression>> const& args, int dest);
    void genLiteral(Literal* lit, int dest);
    void genExpression(Expression* expr, int dest);
    void genFunction(FunctionRootStatement* func);

    void importProc(const char* libName, const char* procName);
    void useData(const char* name, const void* data, int length);
    void out(const char* fmt, ...);
  };

}
