#include "parser.hpp"

using lon::Expression;
using lon::Statement;
using lon::Parser;
using lon::TypeID;
using lon::Type;

Parser::Parser(LexerResult const& lexerResult)
  : m_inputFileName(lexerResult.inputFileName),
    m_textTokens(lexerResult.tokens)
{
  m_tk = m_textTokens.begin();
}

Parser::~Parser() = default;

static Type makeVoid() {
  Type type;
  type.id = lon::TID_VOID;
  return std::move(type);
}

Type Parser::parseTypeName() {
  if (end())
    throw ParserError("Unexpected EOF. Expected type name", -1, -1);

  auto startTk = m_tk;
  TokenID id = m_tk->id;
  next();

  int sign = 0;

  if (id == TK_CONST) {
    auto child = parseTypeName();
    if (child.flags & TPF_CONST)
      throw ParserError("Multiple const modifiers", startTk);

    child.flags |= TPF_CONST;
    return child;
  }

  if (id == TK_UNSIGNED)
    sign = 1;
  else if (id == TK_SIGNED)
    sign = -1;

  if (sign != 0) {
    if (end())
      throw ParserError("Unexpected EOF. Expected numeric type name", -1, -1);

    id = m_tk->id;

    if (id < TK_BYTE || id > TK_LONG)
      throw ParserError("Expected numeric type name, but got non-numeric type name", m_tk);

    next();
  }

  int width = 0;
  switch (id) {
    case TK_VOID:
      return makeVoid();

    case TK_BYTE:
      if (sign == 0) sign = 1;
      width = 0;
      goto NUMBER_TYPE;
    case TK_SHORT:
      if (sign == 0) sign = 1;
      width = 1;
      goto NUMBER_TYPE;
    case TK_INTEGER:
      if (sign == 0) sign = -1;
      width = 2;
      goto NUMBER_TYPE;
    case TK_LONG:
      if (sign == 0) sign = -1;
      width = 3;

    NUMBER_TYPE: {
      Type type;
      type.id = TID_NUMBER;
      auto& number = type.data.emplace<Type::Number>();
      number.width = width;
      number.isSigned = sign == -1;
      return type;
    }
  }

  // TODO pointers

  throw ParserError("Invalid type name", startTk);
}

Expression Parser::parseExpression() {
  if (end())
    throw ParserError("Unexpected EOF. Expected expression", -1, -1);

  Literal literal;
  Expression expr;

  switch (m_tk->id) {
    default:
      throw ParserError("Unexpected token. Expected valid expression", m_tk);

    case TK_NUMBER_INT:
      literal.data.emplace<uint64_t>(m_tk->intValue);
      goto LITERAL;
    case TK_STRING:
      literal.data.emplace<std::string>(m_tk->strValue);
      // fallthrough
    LITERAL:
      expr.data.emplace<Literal>(std::move(literal));
      next();
      return expr;

    case TK_ID: {
      // FIXME only call for now

      std::string const& name = m_tk->strValue;
      next();

      assertToken('(');
      next();

      auto& call = expr.data.emplace<Expression::Call>();
      call.funcName = name;

      auto& args = call.args;

      bool hadComma = true;
      while (true) {
        if (end())
          throw ParserError("Unexpected EOF in functions arguments list", -1, -1);

        if (m_tk->id == ')')
          break;

        if (!hadComma)
          throw ParserError("Unexpected token. Expected closing parenthesis", -1, -1);

        hadComma = false;

        auto arg = parseExpression();
        args.emplace_back(std::move(arg));

        if (!end() && m_tk->id == ',') {
          hadComma = true;
          next();
        }
      }

      next();
      return expr;
    }
  }
}

std::list<Statement> Parser::parseBlock() {
  std::list<Statement> block;

  while (!end() && m_tk->id != '}') {
    Statement st;

    switch (m_tk->id) {
      default:
        throw ParserError("Unexpected token. Expected valid statement", m_tk);

      case TK_RETURN: {
        next();
        auto expr = parseExpression();
        st.data.emplace<Statement::Return>(Statement::Return { std::move(expr) });
        block.emplace_back(std::move(st));
      } break;

      case TK_ID:
        st.data.emplace<Expression>(parseExpression());
        block.emplace_back(std::move(st));
        break;
    }

    assertToken(';');
    next();
  }

  assertToken('}');
  next();

  return block;
}

void Parser::parse() {
  while (!end()) {
    switch (m_tk->id) {
      default:
        throw ParserError("Unexpected token", m_tk);

      case TK_FUNCTION: {
        next();
        assertToken(TK_ID);

        FunctionDefinition func;
        func.funcName = m_tk->strValue;

        next();
        assertToken('(');

        // TODO args

        next();
        assertToken(')');

        next();
        if (end())
          throw ParserError("Unexpected EOF. Expected return arrow or function body", -1, -1);

        if (m_tk->id == TK_RET_ARROW) {
          next();
          func.returnType = parseTypeName();

          if (end())
            throw ParserError("Unexpected EOF. Expected function body", -1, -1);
        } else {
          func.returnType = makeVoid();
        }

        assertToken('{');
        next();

        func.body = parseBlock();

        m_ast.functions.emplace_back(std::move(func));
      } break;
    }
  }
}

void Parser::debugPrint() {
  printf("Parser result:\n");

  printf("  Functions:\n");
  for (auto const& func : m_ast.functions) {
    printf("    Function %s -> ", func.funcName.c_str());
    printType(&func.returnType);
    printf("\n      Body:\n");
    printBlock(func.body, 8);
    printf("\n");
  }
}

void Parser::next() {
  if (end()) return;
  ++m_tk;
}

bool Parser::end() {
  return m_tk == m_textTokens.end();
}

void Parser::assertToken(TokenID id) {
  if (end()) {
    std::string info = "Unexpected EOF. Expected " + TokenIDToString(id);
    throw ParserError(info.c_str(), -1, -1);
  }

  if (m_tk->id != id) {
    std::string info = "Unexpected token. Expected " + TokenIDToString(id) + ". Got " + TokenIDToString(m_tk->id);
    throw ParserError(info.c_str(), m_tk);
  }
}

void Parser::printType(Type const* tp) {
  if (tp->flags & TPF_CONST)
    printf("const ");

  switch (tp->id) {
    case TID_VOID:
      printf("void"); return;
    case TID_NUMBER: {
      auto& ntp = std::get<Type::Number>(tp->data);
      printf("number<");
      if (ntp.isSigned)
        printf("signed, ");
      else
        printf("unsigned, ");
      printf("%d bits>", (1 << ntp.width) << 3);
    } return;
  }
}

void Parser::printLiteral(Literal const* lit) {
  switch (lit->getType()) {
    case LiteralType::INT:
      printf("int<%lld>", std::get<uint64_t>(lit->data));
      break;
    case LiteralType::STRING:
      printf("string<%s>", std::get<std::string>(lit->data).c_str());
      break;
    case LiteralType::FLOAT:
      printf("float<%f>", std::get<double>(lit->data));
      break;
  }
}

void Parser::printExpr(Expression const* expr, int indent) {
  switch (expr->getType()) {
    case ExpressionType::CALL: {
      auto& call = std::get<Expression::Call>(expr->data);
      printf("call %s (", call.funcName.c_str());

      bool first = true;
      for (auto const& arg : call.args) {
        printExpr(&arg, indent);

        if (!first)
          printf(", ");
        first = false;
      }
      printf(")");
    } break;
    case ExpressionType::LITERAL: {
      auto& lit = std::get<Literal>(expr->data);
      printf("literal ");
      printLiteral(&lit);
    } break;
  }
}

void Parser::printBlock(std::list<Statement> const& statements, int indent) {
  printf("%*c{\n", indent, ' ');
  indent += 2;
  for (auto const& st : statements) {
    switch (st.getType()) {
      case StatementType::EXPR: {
        auto& expr = std::get<Expression>(st.data);
        printf("%*c", indent, ' ');
        printExpr(&expr, indent + 2);
        printf(";\n");
      } break;
      case StatementType::RETURN: {
        auto& ret = std::get<Statement::Return>(st.data);
        printf("%*creturn ", indent, ' ');
        printExpr(&ret.value, indent + 2);
        printf(";\n");
      } break;
      case StatementType::BLOCK:
        break;
    }
  }
  indent -= 2;
  printf("%*c}", indent, ' ');
}
