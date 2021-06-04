#ifndef tokens_hpp
#define tokens_hpp

#include <optional>
#include <string_view>
#include <ostream>
#include <variant>

namespace stork {
enum struct reservedTokens {
  inc,
  dec,

  add,
  sub,
  concat,
  mul,
  div,
  idiv,
  mod,
  fhe_add,
  fhe_sub,
  fhe_mul,

  bitwise_not,
  bitwise_and,
  bitwise_or,
  bitwise_xor,
  shiftl,
  shiftr,

  assign,

  add_assign,
  sub_assign,
  concat_assign,
  mul_assign,
  div_assign,
  idiv_assign,
  mod_assign,

  and_assign,
  or_assign,
  xor_assign,
  shiftl_assign,
  shiftr_assign,

  logical_not,
  logical_and,
  logical_or,

  eq,
  ne,
  lt,
  gt,
  le,
  ge,

  question,
  colon,

  comma,

  semicolon,

  open_round,
  close_round,

  open_curly,
  close_curly,

  open_square,
  close_square,

  kw_sizeof,
  kw_tostring,

  kw_if,
  kw_else,
  kw_elif,

  kw_switch,
  kw_case,
  kw_default,

  kw_for,
  kw_while,
  kw_do,

  kw_break,
  kw_continue,
  kw_return,

  kw_function,

  kw_bool,
  kw_char,
  kw_int,
  kw_float,
  kw_double,
  kw_string,
  kw_void,

  kw_secret,

  kw_public,

  kw_rotate,

  kw_true,
  kw_false
};

class PushBackStream;

std::ostream &operator<<(std::ostream &os, reservedTokens t);

std::optional<reservedTokens> getKeyword(std::string_view word);

std::optional<reservedTokens> getOperator(PushBackStream &stream);

struct identifier {
  std::string name;
};

bool operator==(const identifier &id1, const identifier &id2);
bool operator!=(const identifier &id1, const identifier &id2);

struct eof {
};

bool operator==(const eof &, const eof &);
bool operator!=(const eof &, const eof &);

typedef std::variant<reservedTokens, identifier, double, std::string, eof, int, bool, char, float> token_value;

class token {
 private:
  token_value _value;
  size_t _line_number;
  size_t _char_index;
 public:
  token(token_value value, size_t line_number, size_t char_index);

  [[nodiscard]] bool isReservedToken() const;
  [[nodiscard]] bool isIdentifier() const;
  [[nodiscard]] bool isDouble() const;
  [[nodiscard]] bool isString() const;
  [[nodiscard]] bool isEof() const;
  [[nodiscard]] bool isInteger() const;
  [[nodiscard]] bool isBool() const;
  [[nodiscard]] bool isChar() const;
  [[nodiscard]] bool isFloat() const;

  [[nodiscard]] reservedTokens get_reserved_token() const;
  [[nodiscard]] const identifier &getIdentifier() const;
  [[nodiscard]] double getDouble() const;
  [[nodiscard]] const std::string &getString() const;
  [[nodiscard]] int getInteger() const;
  [[nodiscard]] bool getBool() const;
  [[nodiscard]] char getChar() const;
  [[nodiscard]] float getFloat() const;


  [[nodiscard]] size_t getLineNumber() const;
  [[nodiscard]] size_t getCharIndex() const;

  [[nodiscard]] bool hasValue(const token_value &value) const;
  [[nodiscard]] const token_value &getValue() const;
};

std::string to_string(const stork::reservedTokens t);
std::string to_string(const stork::token_value &t);
}

#endif /* tokens_hpp */
