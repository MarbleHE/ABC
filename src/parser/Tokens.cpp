#include <string_view>
#include <stack>
#include <string>

#include "ast_opt/parser/Tokens.h"
#include "ast_opt/parser/Lookup.h"
#include "ast_opt/parser/Helpers.h"
#include "ast_opt/parser/PushBackStream.h"

namespace stork {
namespace {
const Lookup<std::string_view, reservedTokens> operator_token_map{
    {"++", reservedTokens::inc},
    {"--", reservedTokens::dec},

    {"+", reservedTokens::add},
    {"-", reservedTokens::sub},
    {"..", reservedTokens::concat},
    {"*", reservedTokens::mul},
    {"/", reservedTokens::div},
    {"\\", reservedTokens::idiv},
    {"%", reservedTokens::mod},
    {"+++", reservedTokens::fhe_add},
    {"---", reservedTokens::fhe_sub},
    {"***", reservedTokens::fhe_mul},

    {"~", reservedTokens::bitwise_not},
    {"&", reservedTokens::bitwise_and},
    {"|", reservedTokens::bitwise_or},
    {"^", reservedTokens::bitwise_xor},
    {"<<", reservedTokens::shiftl},
    {">>", reservedTokens::shiftr},

    {"=", reservedTokens::assign},

    {"+=", reservedTokens::add_assign},
    {"-=", reservedTokens::sub_assign},
    {"..=", reservedTokens::concat_assign},
    {"*=", reservedTokens::mul_assign},
    {"/=", reservedTokens::div_assign},
    {"\\=", reservedTokens::idiv_assign},
    {"%=", reservedTokens::mod_assign},

    {"&=", reservedTokens::and_assign},
    {"|=", reservedTokens::or_assign},
    {"^=", reservedTokens::xor_assign},
    {"<<=", reservedTokens::shiftl_assign},
    {">>=", reservedTokens::shiftr_assign},

    {"!", reservedTokens::logical_not},
    {"&&", reservedTokens::logical_and},
    {"||", reservedTokens::logical_or},

    {"==", reservedTokens::eq},
    {"!=", reservedTokens::ne},
    {"<", reservedTokens::lt},
    {">", reservedTokens::gt},
    {"<=", reservedTokens::le},
    {">=", reservedTokens::ge},

    {"?", reservedTokens::question},
    {":", reservedTokens::colon},

    {",", reservedTokens::comma},

    {";", reservedTokens::semicolon},

    {"(", reservedTokens::open_round},
    {")", reservedTokens::close_round},

    {"{", reservedTokens::open_curly},
    {"}", reservedTokens::close_curly},

    {"[", reservedTokens::open_square},
    {"]", reservedTokens::close_square},
};

const Lookup<std::string_view, reservedTokens> keyword_token_map{
    {"sizeof", reservedTokens::kw_sizeof},
    {"tostring", reservedTokens::kw_tostring},

    {"if", reservedTokens::kw_if},
    {"else", reservedTokens::kw_else},
    {"elif", reservedTokens::kw_elif},

    {"switch", reservedTokens::kw_switch},
    {"case", reservedTokens::kw_case},
    {"default", reservedTokens::kw_default},

    {"for", reservedTokens::kw_for},
    {"while", reservedTokens::kw_while},
    {"do", reservedTokens::kw_do},

    {"break", reservedTokens::kw_break},
    {"continue", reservedTokens::kw_continue},
    {"return", reservedTokens::kw_return},

    {"function", reservedTokens::kw_function},

    {"bool", reservedTokens::kw_bool},
    {"char", reservedTokens::kw_char},
    {"int", reservedTokens::kw_int},
    {"float", reservedTokens::kw_float},
    {"double", reservedTokens::kw_double},
    {"string", reservedTokens::kw_string},
    {"void", reservedTokens::kw_void},

    {"secret", reservedTokens::kw_secret},

    {"public", reservedTokens::kw_public},

    {"rotate", reservedTokens::kw_rotate},

    {"true", reservedTokens::kw_true},
    {"false", reservedTokens::kw_false}
};

const Lookup<reservedTokens, std::string_view> token_string_map = ([]() {
  std::vector<std::pair<reservedTokens, std::string_view>> container;
  container.reserve(operator_token_map.size() + keyword_token_map.size());
  for (const auto &p : operator_token_map) {
    container.emplace_back(p.second, p.first);
  }
  for (const auto &p : keyword_token_map) {
    container.emplace_back(p.second, p.first);
  }
  return Lookup<reservedTokens, std::string_view>(std::move(container));
})();
}

std::optional<reservedTokens> getKeyword(std::string_view word) {
  auto it = keyword_token_map.find(word);
  return it==keyword_token_map.end() ? std::nullopt : std::make_optional(it->second);
}

namespace {
class maximal_munch_comparator {
 private:
  size_t _idx;
 public:
  explicit maximal_munch_comparator(size_t idx) :
      _idx(idx) {
  }

  bool operator()(char l, char r) const {
    return l < r;
  }

  bool operator()(std::pair<std::string_view, reservedTokens> l, char r) const {
    return l.first.size() <= _idx || l.first[_idx] < r;
  }

  bool operator()(char l, std::pair<std::string_view, reservedTokens> r) const {
    return r.first.size() > _idx && l < r.first[_idx];
  }

  bool operator()(std::pair<std::string_view, reservedTokens> l, std::pair<std::string_view, reservedTokens> r) const {
    return r.first.size() > _idx && (l.first.size() < _idx || l.first[_idx] < r.first[_idx]);
  }
};
}

std::optional<reservedTokens> getOperator(PushBackStream &stream) {
  auto candidates = std::make_pair(operator_token_map.begin(), operator_token_map.end());

  std::optional<reservedTokens> ret;
  size_t match_size = 0;

  std::stack<char> chars;

  for (size_t idx = 0; candidates.first!=candidates.second; ++idx) {
    chars.push(stream());

    candidates =
        std::equal_range(candidates.first, candidates.second, char(chars.top()), maximal_munch_comparator(idx));

    if (candidates.first!=candidates.second && candidates.first->first.size()==idx + 1) {
      match_size = idx + 1;
      ret = candidates.first->second;
    }
  }

  while (chars.size() > match_size) {
    stream.pushBack(chars.top());
    chars.pop();
  }

  return ret;
}

token::token(token_value value, size_t line_number, size_t char_index) :
    _value(std::move(value)),
    _line_number(line_number),
    _char_index(char_index) {
}

bool token::isReservedToken() const {
  return std::holds_alternative<reservedTokens>(_value);
}

bool token::isIdentifier() const {
  return std::holds_alternative<identifier>(_value);
}

bool token::isDouble() const {
  return std::holds_alternative<double>(_value);
}

bool token::isInteger() const {
  return std::holds_alternative<int>(_value);
}

bool token::isBool() const {
  return std::holds_alternative<bool>(_value);
}

bool token::isString() const {
  return std::holds_alternative<std::string>(_value);
}

bool token::isEof() const {
  return std::holds_alternative<eof>(_value);
}

bool token::isChar() const {
  return std::holds_alternative<char>(_value);
}

bool token::isFloat() const {
  return std::holds_alternative<float>(_value);
}

reservedTokens token::get_reserved_token() const {
  return std::get<reservedTokens>(_value);
}

const identifier &token::getIdentifier() const {
  return std::get<identifier>(_value);
}

double token::getDouble() const {
  return std::get<double>(_value);
}

int token::getInteger() const {
  return std::get<int>(_value);
}

const std::string &token::getString() const {
  return std::get<std::string>(_value);
}

const token_value &token::getValue() const {
  return _value;
}

size_t token::getLineNumber() const {
  return _line_number;
}

size_t token::getCharIndex() const {
  return _char_index;
}

bool token::hasValue(const token_value &value) const {
  return _value==value;
}
bool token::getBool() const {
  return std::get<bool>(_value);
}

char token::getChar() const {
  return std::get<char>(_value);
}

float token::getFloat() const {
  return std::get<float>(_value);
}

bool operator==(const identifier &id1, const identifier &id2) {
  return id1.name==id2.name;
}

bool operator!=(const identifier &id1, const identifier &id2) {
  return id1.name!=id2.name;
}

bool operator==(const eof &, const eof &) {
  return true;
}

bool operator!=(const eof &, const eof &) {
  return false;
}

std::string to_string(const reservedTokens t) {
  return std::string(token_string_map.find(t)->second);
}

std::string to_string(const token_value &t) {
  //reservedTokens, identifier, double, std::string, eof, int, bool, char, float
  return std::visit(overloaded{
      [](reservedTokens rt) {
        return to_string(rt);
      },
      [](const identifier &id) {
        return id.name;
      },
      [](double d) {
        return std::to_string(d);
      },
      [](const std::string &str) {
        return str;
      },
      [](eof) {
        return std::string("<EOF>");
      },
      [](int i) {
        return std::to_string(i);
      },
      [](bool b) {
        return std::to_string(b);
      },
      [](char c) {
        return std::to_string(c);
      },
      [](float f) {
        return std::to_string(f);
      }
  }, t);
}
}