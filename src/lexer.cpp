#include "lexer.hpp"

#include <map>

using lon::TokenID;
using lon::Token;
using lon::Lexer;

static std::map<std::string, TokenID> KEYWORDS = {
  { "function", lon::TK_FUNCTION },
  { "return",   lon::TK_RETURN }
};

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
