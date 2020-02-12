#include <iostream>
#include <AbstractExpr.h>
#include "../../include/ast/LiteralFloat.h"
#include "IVisitor.h"
#include "../utilities/RandNumGen.h"

LiteralFloat::LiteralFloat(float value) : value(value) {}

json LiteralFloat::toJson() const {
  json j;
  j["type"] = getNodeName();
  j["value"] = this->value;
  return j;
}

float LiteralFloat::getValue() const {
  return value;
}

void LiteralFloat::accept(IVisitor &v) {
  v.visit(*this);
}

std::string LiteralFloat::getNodeName() const {
  return "LiteralFloat";
}

LiteralFloat::~LiteralFloat() = default;

Literal* LiteralFloat::evaluate(Ast &ast) {
  return this;
}

LiteralFloat LiteralFloat::operator+(LiteralFloat const &lint) {
  return LiteralFloat(this->getValue() + lint.getValue());
}

std::ostream &operator<<(std::ostream &os, const LiteralFloat &an_int) {
  os << an_int.getValue();
  return os;
}

void LiteralFloat::print(std::ostream &str) const {
  str << this->value;
}

bool LiteralFloat::operator==(const LiteralFloat &rhs) const {
  return value == rhs.value;
}

bool LiteralFloat::operator!=(const LiteralFloat &rhs) const {
  return !(rhs == *this);
}

void LiteralFloat::addLiteralValue(std::string identifier, std::map<std::string, Literal*> &paramsMap) {
  paramsMap.emplace(identifier, this);
}

void LiteralFloat::setValue(float newValue) {
  this->value = newValue;
}

void LiteralFloat::setRandomValue(RandLiteralGen &rlg) {
  setValue(rlg.getRandomFloat());
}

std::string LiteralFloat::toString() const {
  return std::to_string(this->getValue());
}

bool LiteralFloat::supportsCircuitMode() {
  return true;
}
