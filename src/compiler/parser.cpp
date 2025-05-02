#include "parser.hpp"

using lon::Expression;
using lon::Statement;
using lon::Parser;
using lon::TypeID;
using lon::Type;

Parser::Parser(LexerResult const& lexerResult)
  : m_lexerResult(lexerResult)
{
  m_tk = m_lexerResult.tokens.begin();
}

Parser::~Parser() = default;

static std::shared_ptr<Type> makeVoid() {
  auto type = std::make_shared<Type>();
  type->id = lon::TID_VOID;
  return std::move(type);
}

std::shared_ptr<Type> Parser::parseTypeName() {
  if (end())
    throw ParserError("Unexpected EOF. Expected type name", -1, -1);

  auto startTk = m_tk;
  TokenID id = m_tk->id;
  next();

  int sign = 0;

  if (id == TK_CONST) {
    auto child = parseTypeName();
    if (child->flags & TPF_CONST)
      throw ParserError("Multiple const modifiers", startTk);

    child->flags |= TPF_CONST;
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
      auto type = std::make_shared<NumericType>();
      type->id = TID_CHAR;
      type->width = width;
      type->isSigned = sign == -1;
      return type;
    }
  }

  // TODO pointers

  throw ParserError("Invalid type name", startTk);
}

std::shared_ptr<Expression> Parser::parseExpression() {
  if (end())
    throw ParserError("Unexpected EOF. Expected expression", -1, -1);

  switch (m_tk->id) {
    default:
      throw ParserError("Unexpected token. Expected valid expression", m_tk);

    case TK_NUMBER_INT: {
      auto expr = std::make_shared<LiteralExpression>();
      expr->type = ExpressionType::LITERAL;
      auto lit = std::make_shared<IntLiteral>();
      lit->type = LiteralType::INT;
      lit->value = 0; // FIXME
      expr->value = std::move(lit);
      next();
      return expr;
    }
    case TK_STRING: {
      auto expr = std::make_shared<LiteralExpression>();
      expr->type = ExpressionType::LITERAL;
      auto lit = std::make_shared<StringLiteral>();
      lit->type = LiteralType::STRING;
      lit->value = m_tk->strValue;
      expr->value = std::move(lit);
      next();
      return expr;
    }
  }
}

std::list<std::shared_ptr<Statement>> Parser::parseBlock() {
  std::list<std::shared_ptr<Statement>> block;

  while (!end() && m_tk->id != '}') {
    switch (m_tk->id) {
      default:
        throw ParserError("Unexpected token. Expected valid statement", m_tk);

      case TK_RETURN: {
        next();
        auto expr = parseExpression();
        auto st = std::make_shared<ReturnStatement>();
        st->type = StatementType::RETURN;
        st->value = std::move(expr);
        block.emplace_back(std::move(st));
      } break;

      case TK_ID: {
        // FIXME only call for now

        std::string const& name = m_tk->strValue;
        next();

        assertToken('(');
        next();

        auto st = std::make_shared<ExpressionStatement>();
        st->type = StatementType::EXPR;

        auto expr = std::make_shared<CallExpression>();
        expr->type = ExpressionType::CALL;
        expr->funcName = name;

        auto& args = expr->args;

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

        st->expr = std::move(expr);
        block.emplace_back(std::move(st));

        next();
      } break;
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

      case TK_FUNCTION:
        next();
        assertToken(TK_ID);

        auto rootSt = std::make_shared<FunctionRootStatement>();
        rootSt->type = RootStatementType::FUNCTION;
        rootSt->funcName = m_tk->strValue;

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
          rootSt->returnType = parseTypeName();

          if (end())
            throw ParserError("Unexpected EOF. Expected function body", -1, -1);
        }
        else {
          rootSt->returnType = makeVoid();
        }

        assertToken('{');
        next();

        rootSt->body = parseBlock();

        m_rootStatements.emplace_back(std::move(rootSt));

        break;
    }
  }
}

void Parser::debugPrint() {
  printf("Parser result:\n");

  for (auto const& rst : m_rootStatements) {
    printf("  Root statement:\n");
    switch (rst->type) {
      case RootStatementType::FUNCTION: {
        auto fst = (FunctionRootStatement*)rst.get();
        printf("    Function %s -> ", fst->funcName.c_str());
        printType(fst->returnType.get());
        printf("\n      Body:\n");
        printBlock(fst->body, 8);
        printf("\n");
      } break;
    }
  }
}

void Parser::next() {
  if (end()) return;
  ++m_tk;
}

bool Parser::end() {
  return m_tk == m_lexerResult.tokens.end();
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

void Parser::printType(Type* tp) {
  if (tp->flags & TPF_CONST)
    printf("const ");

  switch (tp->id) {
    case TID_VOID:
      printf("void"); return;
    case TID_NUMBER: {
      auto ntp = (NumericType*)tp;
      printf("number<");
      if (ntp->isSigned)
        printf("signed, ");
      else
        printf("unsigned, ");
      printf("%d bits>", (1 << ntp->width) << 3);
    } return;
  }
}

void Parser::printLiteral(Literal* lit) {
  switch (lit->type) {
    case LiteralType::INT:
      printf("int<%lld>", lit->asInt()->value);
      break;
    case LiteralType::STRING:
      printf("string<%s>", lit->asString()->value.c_str());
      break;
    case LiteralType::FLOAT:
      printf("float<%f>", lit->asFloat()->value);
      break;
  }
}

void Parser::printExpr(Expression* expr, int indent) {
  switch (expr->type) {
    case ExpressionType::CALL: {
      printf("call %s (", expr->asCall()->funcName.c_str());

      bool first = true;
      for (auto const& arg : expr->asCall()->args) {
        printExpr(arg.get(), indent);

        if (!first)
          printf(", ");
        first = false;
      }
      printf(")");
    } break;
    case ExpressionType::LITERAL:
      printf("literal ");
      printLiteral(expr->asLiteral()->value.get());
      break;
  }
}

void Parser::printBlock(std::list<std::shared_ptr<Statement>> const& statements, int indent) {
  printf("%*c{\n", indent, ' ');
  indent += 2;
  for (auto const& st : statements) {
    switch (st->type) {
      case StatementType::EXPR:
        printf("%*c", indent, ' ');
        printExpr(((ExpressionStatement*)st.get())->expr.get(), indent + 2);
        printf(";\n");
        break;
      case StatementType::RETURN:
        printf("%*creturn ", indent, ' ');
        printExpr(((ReturnStatement*)st.get())->value.get(), indent + 2);
        printf(";\n");
        break;
      case StatementType::BLOCK:
        break;
    }
  }
  indent -= 2;
  printf("%*c}", indent, ' ');
}

std::list<std::shared_ptr<lon::RootStatement>> lon::Parser::getResult() {
  return m_rootStatements;
}
