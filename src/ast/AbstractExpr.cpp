#include <iostream>
#include <Variable.h>
#include <AbstractExpr.h>

#include "LiteralInt.h"
#include "LiteralBool.h"
#include "LiteralString.h"
#include "LiteralFloat.h"

std::string AbstractExpr::toString() const {
  return this->toJson().dump();
}

json AbstractExpr::toJson() const {
  return json({"type", "AbstractExpr"});
}

void AbstractExpr::accept(IVisitor &v) {
  std::cout << "This shouldn't be executed!" << std::endl;
}

std::ostream &operator<<(std::ostream &outs, const AbstractExpr &obj) {
  return outs << obj.toString();
}

LiteralInt* AbstractExpr::createParam(int i) {
  return new LiteralInt(i);
}

LiteralBool* AbstractExpr::createParam(bool b) {
  return new LiteralBool(b);
}

LiteralString* AbstractExpr::createParam(const char* str) {
  return new LiteralString(str);
}

AbstractExpr* AbstractExpr::createParam(AbstractExpr* abstractExpr) {
  return abstractExpr;
}

BinaryExpr* AbstractExpr::contains(BinaryExpr* bexpTemplate, AbstractExpr* excludedSubtree) {
  return nullptr;
}

bool AbstractExpr::contains(Variable* var) {
  return false;
}

bool AbstractExpr::isEqual(AbstractExpr* other) {
  return false;
}

Literal* AbstractExpr::evaluate(Ast &ast) {
  return nullptr;
}

LiteralFloat* AbstractExpr::createParam(float f) {
  return new LiteralFloat(f);
}

int AbstractExpr::countByTemplate(AbstractExpr* abstractExpr) {
  return 0;
}

std::vector<std::string> AbstractExpr::getVariableIdentifiers() {
  return std::vector<std::string>();
}

void to_json(json &j, const AbstractExpr &absExpr) {
  j = absExpr.toJson();
}

