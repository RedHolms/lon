#include "lexer.hpp"

#include <map>

using lon::TokenID;
using lon::Token;
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
    case TK_NUMBER: return "number";

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

Lexer::Lexer() {
  m_row = m_column = 0;
  m_pt = nullptr;
}

Lexer::~Lexer() = default;

void Lexer::feed(const char* input) {
  m_row = m_column = 1;
  m_pt = input;

  char cur;
  bool noNext = false;

  for (; *m_pt; !noNext ? next() : (void)(noNext = false)) {
    cur = *m_pt;

    if (cur == '\n') {
      m_column = 0; // will be 1
      ++m_row;
      continue;
    }

    if (isspace(cur))
      continue;

    // begin of a token
    int row = m_row, column = m_column;

    switch (cur) {
      case '/':
        if (m_pt[1] == '/') {
          // single line comment
          while (m_pt[1] != '\n')
            next();

          continue;
        }
        else if (m_pt[1] == '*') {
          // multi line comment
          while (*m_pt != '*' || m_pt[1] != '/')
            next();

          next();
          continue;
        }

        goto SINGLE_CHAR_TOKEN;
      case '-':
        if (m_pt[1] == '>') {
          next();
          token(TK_RET_ARROW, nullptr, row, column);
          break;
        }
        // fallthrough
      case '(':
      case ')':
      case '{':
      case '}':
      case ';':
      SINGLE_CHAR_TOKEN:
        token((TokenID)cur, nullptr, row, column);
        break;
      default:
        // number
        if (isdigit(cur)) {
          const char* begin = m_pt;

          int length = 0;
          while (isdigit(*m_pt)) ++length, next();

          token(TK_NUMBER, std::string(begin, length).c_str(), row, column);

          noNext = true;
          break;
        }

        // string
        if (cur == '"') {
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

          token(TK_STRING, buffer.c_str(), row, column);
          break;
        }

        // identifier
        if (isalpha(cur) || cur == '_') {
          const char* begin = m_pt;

          int length = 0;
          while (isalnum(*m_pt) || *m_pt == '_')
            ++length, next();

          noNext = true;

          std::string name = std::string(begin, length);

          auto it = KEYWORDS.find(name);
          if (it != KEYWORDS.end()) {
            token(it->second, nullptr, row, column);
            break;
          }

          token(TK_ID, name.c_str(), row, column);
          break;
        }

        printf("%c\n", cur);
        throw LexerError("Unknown token", row, column);
    }
  }
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
        case TK_ID:        printf("identifier"); break;
        case TK_STRING:    printf("string"); break;
        case TK_NUMBER:    printf("number"); break;
        case TK_RET_ARROW: printf("->"); break;
        case TK_FUNCTION:  printf("keyword <function>"); break;
        case TK_RETURN:    printf("keyword <return>"); break;
        case TK_CONST:     printf("keyword <const>"); break;
        case TK_SIGNED:    printf("keyword <signed>"); break;
        case TK_UNSIGNED:  printf("keyword <unsigned>"); break;
        case TK_VOID:      printf("keyword <void>"); break;
        case TK_BYTE:      printf("keyword <byte>"); break;
        case TK_SHORT:     printf("keyword <short>"); break;
        case TK_INTEGER:   printf("keyword <integer>"); break;
        case TK_LONG:      printf("keyword <long>"); break;
        case TK_CHAR:      printf("keyword <char>"); break;
        case TK_BOOLEAN:   printf("keyword <boolean>"); break;
        default:           printf("unknown<%d>", tk.id); break;
      }
    }

    if (!tk.value.empty())
      printf(" \"%s\"", tk.value.c_str());

    printf(" at %d:%d\n", tk.row, tk.column);
  }
}

void Lexer::token(TokenID id, const char* value, int row, int column) {
  m_tokens.push_back(Token { id, row, column, std::string(value ? value : "") });
}

inline void Lexer::next() {
  ++m_pt;
  ++m_column;
}
