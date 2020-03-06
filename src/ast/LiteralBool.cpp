#include "LiteralBool.h"
#include "RandNumGen.h"

LiteralBool::LiteralBool(bool value) : matrix(new Matrix(value)) {

}

LiteralBool::LiteralBool(Matrix<bool> *inputMatrix) : matrix(inputMatrix) {}

json LiteralBool::toJson() const {
  json j;
  j["type"] = getNodeType();
  j["value"] = (matrix->isScalar() ? json(matrix->getScalarValue()) : matrix->toJson());
  return j;
}

void LiteralBool::accept(Visitor &v) {
  v.visit(*this);
}

bool LiteralBool::getValue() const {
  return matrix->getScalarValue();
}

Matrix<bool> *LiteralBool::getMatrix() {
  return matrix;
}

std::string LiteralBool::getTextValue() const {
  return getValue() ? "true" : "false";
}

std::string LiteralBool::getNodeType() const {
  return "LiteralBool";
}

LiteralBool::~LiteralBool() = default;

void LiteralBool::print(std::ostream &str) const {
  str << this->getTextValue();
}

bool LiteralBool::operator==(const LiteralBool &rhs) const {
  return *matrix==*rhs.matrix;
}

bool LiteralBool::operator!=(const LiteralBool &rhs) const {
  return !(rhs==*this);
}

void LiteralBool::addLiteralValue(std::string identifier,
                                  std::unordered_map<std::string, AbstractLiteral *> &paramsMap) {
  paramsMap.emplace(identifier, this);
}

void LiteralBool::setValue(bool newValue) {
  delete matrix;
  this->matrix = new Matrix(newValue);
}

void LiteralBool::setRandomValue(RandLiteralGen &rlg) {
  setValue(rlg.getRandomBool());
}

std::string LiteralBool::toString(bool printChildren) const {
  return AbstractNode::generateOutputString(printChildren, {this->getTextValue()});
}

bool LiteralBool::supportsCircuitMode() {
  return true;
}

bool LiteralBool::supportsDatatype(Datatype &datatype) {
  return datatype.getType()==Types::BOOL;
}

LiteralBool *LiteralBool::clone(bool keepOriginalUniqueNodeId) {
  auto clonedNode = new LiteralBool(this->getValue());
  if (keepOriginalUniqueNodeId) clonedNode->setUniqueNodeId(this->getUniqueNodeId());
  if (this->isReversed) clonedNode->swapChildrenParents();
  return clonedNode;
}

bool LiteralBool::isEqual(AbstractExpr *other) {
  auto otherLiteralBool = dynamic_cast<LiteralBool *>(other);
  return otherLiteralBool!=nullptr && this->getValue()==otherLiteralBool->getValue();
}

bool LiteralBool::isNull() {
  return !this->getValue();
}

