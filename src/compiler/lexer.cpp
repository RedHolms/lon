#include "lexer.hpp"

#include <map>
#include "../utils.hpp"

using lon::TokenID;
using lon::Token;
using lon::LexerResult;
using lon::Lexer;

static std::map<std::string, TokenID> KEYWORDS = {
  {"function", lon::TK_FUNCTION},
  {"return",   lon::TK_RETURN},
  {"const",    lon::TK_CONST},
  {"signed",   lon::TK_SIGNED},
  {"unsigned", lon::TK_UNSIGNED},
  {"void",     lon::TK_VOID},
  {"byte",     lon::TK_BYTE},
  {"short",    lon::TK_SHORT},
  {"integer",  lon::TK_INTEGER},
  {"long",     lon::TK_LONG},
  {"char",     lon::TK_CHAR},
  {"boolean",  lon::TK_BOOLEAN},
};

std::string lon::TokenIDToString(TokenID id) {
  if (id <= 255)
    return std::string() += (char)id;

  switch (id) {
    case TK_ID: return "identifier";
    case TK_STRING: return "string";
    case TK_NUMBER_INT: return "integer number";
    case TK_NUMBER_FLOAT: return "float number";

    case TK_RET_ARROW: return "->";

    case TK_FUNCTION: return "keyword <function>";
    case TK_RETURN: return "keyword <return>";
    case TK_CONST: return "keyword <const>";
    case TK_SIGNED: return "keyword <signed>";
    case TK_UNSIGNED: return "keyword <unsigned>";
    case TK_VOID: return "keyword <void>";
    case TK_BYTE: return "keyword <byte>";
    case TK_SHORT: return "keyword <short>";
    case TK_INTEGER: return "keyword <integer>";
    case TK_LONG: return "keyword <long>";
    case TK_CHAR: return "keyword <char>";
    case TK_BOOLEAN: return "keyword <boolean>";
  }

  return std::string("unknown<") + std::to_string(id) + '>';
}

Lexer::Lexer(std::string_view inputFilePath, std::string_view replaceContent)
  : m_inputFileName(inputFilePath),
    m_content(replaceContent)
{
  m_row = m_column = 1;
  m_pt = nullptr;
}

Lexer::~Lexer() = default;

void Lexer::tokenize() {
  if (!m_tokens.empty())
    return;

  if (m_content.empty()) {
    std::ifstream file(m_inputFileName);
    file.exceptions(std::ifstream::failbit);
    m_content = slurpFile(file);
  }

  m_row = m_column = 1;
  m_pt = m_content.data();

  while (*m_pt) {
    if (*m_pt == '\r') {
      if (m_pt[1] != '\n')
        throw LexerError("Invalid line separator in file. Classic Mac OS style not supported", m_row, m_column);

      next();
    }

    if (*m_pt == '\n') {
      m_column = 1;
      ++m_row; ++m_pt;
      continue;
    }

    if (isspace(*m_pt)) {
      next();
      continue;
    }

    m_tkRow = m_row, m_tkColumn = m_column;

    switch (*m_pt) {
      case '/':
        if (m_pt[1] == '/') {
          // single line comment
          while (*m_pt != '\n')
            next();

          continue;
        }
        else if (m_pt[1] == '*') {
          // multi line comment
          while (*m_pt != '*' || m_pt[1] != '/') {
            if (*m_pt == '\n') {
              m_column = 1;
              ++m_row;
            }

            next();
          }

          next(2);
          continue;
        }

        goto SINGLE_CHAR_TOKEN;
      case '.':
      case '+':
      case '-':
        if (isdigit(m_pt[1])) {
          processNumber();
          continue;
        }

        if (*m_pt != '-')
          goto SINGLE_CHAR_TOKEN;

        if (m_pt[1] == '>') {
          token(TK_RET_ARROW);
          next(2);
          continue;
        }

        // fallthrough
      case '(':
      case ')':
      case '{':
      case '}':
      case ';':
      SINGLE_CHAR_TOKEN:
        token(*m_pt);
        next();
        continue;
      default:
        if (isdigit(*m_pt)) {
          processNumber();
          continue;
        }

        // string
        if (*m_pt == '"') {
          next();
          const char* begin = m_pt;

          std::string buffer;
          buffer.reserve(64);

          while (*m_pt != '"') {
            switch (*m_pt) {
              case '\r':
              case '\n':
                throw LexerError("Unexpected line break in string", m_row, m_column);
              case '\\':
                next();

                switch (*m_pt) {
                  case 'n': buffer.push_back('\n'); break;
                  case 't': buffer.push_back('\t'); break;
                  default:
                    throw LexerError("Unknown escape sequence", m_row, m_column);
                }

                break;
              default:
                buffer.push_back(*m_pt);
            }

            next();
          }

          token(TK_STRING, buffer);
          next();
          continue;
        }

        // identifier or keyword
        if (isalpha(*m_pt) || *m_pt == '_') {
          const char* begin = m_pt;

          int length = 0;
          while (isalnum(*m_pt) || *m_pt == '_')
            ++length, next();

          std::string name(begin, length);

          auto it = KEYWORDS.find(name);
          if (it != KEYWORDS.end()) {
            token(it->second);
            continue;
          }

          token(TK_ID, name);
          continue;
        }

        token(*m_pt);
        next();
    }
  }
}

// parse number at m_pt
void Lexer::processNumber() {
  bool isNegative = false;

  if (*m_pt == '+' || *m_pt == '-')
    isNegative = (*m_pt == '-'), next();

  // NaN (case-sensitive)
  if (
    m_pt[0] == 'N' &&
    m_pt[1] == 'a' &&
    m_pt[2] == 'N'
  ) {
    next(3);

    double nan = std::numeric_limits<double>::quiet_NaN();
    if (isNegative)
      nan = -nan;
    token(TK_NUMBER_FLOAT, nan);

    return;
  }

  char* dt = nullptr; // dot pointer
  int base = 10;

  if (*m_pt == '0') {
    switch (m_pt[1]) {
      case 'x':
      case 'X':
        base = 16;
        next(2);
        break;
      case 'b':
      case 'B':
        base = 2;
        next(2);
        break;
      default:
        while (*m_pt == '0')
          next();
        break;
    }
  }

  // hex and bin
  if (base != 10) {
    union { int64_t result; uint64_t uresult; };
    result = 0;
    int count = 0;

    if (base == 2) {
      while (*m_pt == '0' || *m_pt == '1') {
        if (count == 64)
          throw LexerError("Too big number", m_tkRow, m_tkColumn);

        ++count;
        uresult <<= 1;
        if (*m_pt == '1')
          uresult |= 1;

        next();
      }

      if (isalnum(*m_pt))
        throw new LexerError("Invalid digit for a binary number", m_row, m_column);
    }
    else if (base == 16) {
      while (
        isdigit(*m_pt) ||
        (*m_pt >= 'a' && *m_pt <= 'f') ||
        (*m_pt >= 'A' && *m_pt <= 'F')
      ) {
        if (count == 16)
          throw LexerError("Too big number", m_tkRow, m_tkColumn);

        ++count;
        uresult <<= 4;

        if (isdigit(*m_pt))
          uresult |= (*m_pt) - '0';
        else if (*m_pt >= 'a' && *m_pt <= 'f')
          uresult |= ((*m_pt) - 'a') + 10;
        else if (*m_pt >= 'A' && *m_pt <= 'F')
          uresult |= ((*m_pt) - 'A') + 10;

        next();
      }

      if (isalpha(*m_pt))
        throw new LexerError("Invalid digit for a hexadecimal number", m_row, m_column);
    }

    if (isNegative)
      result = -result;

    token(TK_NUMBER_INT, uresult);
    return;
  }

  // decimal
  union { int64_t result; uint64_t uresult; };
  result = 0;

  while (isdigit(*m_pt)) {
    result *= 10;
    result += (*m_pt) - '0';
    next();
  }

  if (isalpha(*m_pt))
    throw new LexerError("Invalid digit for a decimal number", m_row, m_column);

  if (isNegative)
    result = -result;

  token(TK_NUMBER_INT, uresult);

  // TODO floats
}

void Lexer::debugPrint() {
  printf("Lexer result:\n");

  for (Token& tk : m_tokens) {
    printf("  Token ");

    if (tk.id <= 255) {
      printf("%c", tk.id);
    }
    else {
      switch (tk.id) {
        case TK_ID:           printf("identifier"); break;
        case TK_STRING:       printf("string"); break;
        case TK_NUMBER_INT:   printf("integer number"); break;
        case TK_NUMBER_FLOAT: printf("float number"); break;
        case TK_RET_ARROW:    printf("->"); break;
        case TK_FUNCTION:     printf("keyword <function>"); break;
        case TK_RETURN:       printf("keyword <return>"); break;
        case TK_CONST:        printf("keyword <const>"); break;
        case TK_SIGNED:       printf("keyword <signed>"); break;
        case TK_UNSIGNED:     printf("keyword <unsigned>"); break;
        case TK_VOID:         printf("keyword <void>"); break;
        case TK_BYTE:         printf("keyword <byte>"); break;
        case TK_SHORT:        printf("keyword <short>"); break;
        case TK_INTEGER:      printf("keyword <integer>"); break;
        case TK_LONG:         printf("keyword <long>"); break;
        case TK_CHAR:         printf("keyword <char>"); break;
        case TK_BOOLEAN:      printf("keyword <boolean>"); break;
        default:              printf("unknown<%d>", tk.id); break;
      }
    }

    if (tk.id == TK_ID)
      printf(" %s", tk.strValue.c_str());
    else if (tk.id == TK_STRING)
      printf(" \"%s\"", tk.strValue.c_str());
    else if (tk.id == TK_NUMBER_INT)
      printf(" %llu", tk.intValue);
    else if (tk.id == TK_NUMBER_FLOAT)
      printf(" %lf", tk.fltValue);

    printf(" at %d:%d\n", tk.row, tk.column);
  }
}

void Lexer::token(TokenID id) {
  m_tokens.push_back(Token { id, m_tkRow, m_tkColumn, "", 0, 0.0 });
}

void Lexer::token(TokenID id, std::string_view value) {
  m_tokens.push_back(Token { id, m_tkRow, m_tkColumn, std::string(value), 0, 0.0 });
}

void Lexer::token(TokenID id, uint64_t value) {
  m_tokens.push_back(Token { id, m_tkRow, m_tkColumn, "", value, 0.0 });
}

void Lexer::token(TokenID id, double value) {
  m_tokens.push_back(Token { id, m_tkRow, m_tkColumn, "", 0, value });
}

inline void Lexer::next(int amount) {
  m_pt += amount;
  m_column += amount;
}

LexerResult Lexer::getResult() const {
  return {
    m_inputFileName,
    m_tokens
  };
}
