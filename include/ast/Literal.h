#ifndef AST_OPTIMIZER_LITERAL_H
#define AST_OPTIMIZER_LITERAL_H

#include <variant>
#include <ostream>
#include <map>
#include <string>
#include <vector>
#include "../ast/AbstractExpr.h"

class RandLiteralGen;

class Literal : public AbstractExpr {
 protected:
  ~Literal();
  virtual void print(std::ostream &str) const = 0;

 public:
  friend std::ostream &operator<<(std::ostream &os, const Literal &literal);

  bool operator==(const Literal &rhs) const;

  bool operator!=(const Literal &rhs) const;

  virtual void addLiteralValue(std::string identifier, std::map<std::string, Literal*> &paramsMap) = 0;

  virtual void setRandomValue(RandLiteralGen &rlg) = 0;
};

inline std::ostream &operator<<(std::ostream &os, const std::vector<Literal*> &v) {
  os << "[";
  for (auto lit : v) { os << " " << *lit; }
  os << " ]" << std::endl;
  return os;
}

#endif //AST_OPTIMIZER_LITERAL_H
