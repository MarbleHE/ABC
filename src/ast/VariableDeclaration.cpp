#include <utility>
#include "ast_opt/ast/VariableDeclaration.h"
#include "ast_opt/utilities/IVisitor.h"

VariableDeclaration::~VariableDeclaration() = default;

VariableDeclaration::VariableDeclaration(Datatype datatype,
                                         std::unique_ptr<Variable> target,
                                         std::unique_ptr<AbstractExpression> value)
    : target(std::move(target)), datatype(std::move(datatype)), value(std::move(value)) {}

VariableDeclaration::VariableDeclaration(const VariableDeclaration &other)
    : datatype(other.datatype),
      target(other.target ? other.target->clone(this) : nullptr),
      value(other.value ? other.value->clone(this) : nullptr) {}

VariableDeclaration::VariableDeclaration(VariableDeclaration &&other) noexcept
    : datatype(std::move(other.datatype)), target(std::move(other.target)), value(std::move(other.value)) {}

VariableDeclaration &VariableDeclaration::operator=(const VariableDeclaration &other) {
  datatype = other.datatype;
  target = other.target ? other.target->clone(this) : nullptr;
  value = other.value ? other.value->clone(this) : nullptr;
  return *this;
}

VariableDeclaration &VariableDeclaration::operator=(VariableDeclaration &&other) noexcept {
  datatype = other.datatype;
  target = std::move(other.target);
  value = std::move(other.value);
  return *this;
}

std::unique_ptr<VariableDeclaration> VariableDeclaration::clone(AbstractNode* parent_) const {
  return std::unique_ptr<VariableDeclaration>(clone_impl(parent_));
}

bool VariableDeclaration::hasTarget() const {
  return target!=nullptr;
}
bool VariableDeclaration::hasDatatype() const {
  return true; // Since datatype is simple object member
}
bool VariableDeclaration::hasValue() const {
  return value!=nullptr;
}
Variable &VariableDeclaration::getTarget() {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}
const Variable &VariableDeclaration::getTarget() const {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}
std::unique_ptr<AbstractExpression> VariableDeclaration::takeTarget() {
  return std::move(this->target);
}

Datatype &VariableDeclaration::getDatatype() {
  return datatype;
}

const Datatype &VariableDeclaration::getDatatype() const {
  return datatype;
}

AbstractExpression &VariableDeclaration::getValue() {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

const AbstractExpression &VariableDeclaration::getValue() const {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

std::unique_ptr<AbstractExpression> VariableDeclaration::takeValue() {
  return std::move(this->value);
}

void VariableDeclaration::setTarget(std::unique_ptr<Variable> newTarget) {
  target = std::move(newTarget);
}

void VariableDeclaration::setValue(std::unique_ptr<AbstractExpression> newValue) {
  value = std::move(newValue);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////
VariableDeclaration *VariableDeclaration::clone_impl(AbstractNode* parent_) const {
  auto p = new VariableDeclaration(*this);
  if(parent_) {p->setParent(*parent_);}
  return p;
}

void VariableDeclaration::accept(IVisitor &v) {
  v.visit(*this);
}
AbstractNode::iterator VariableDeclaration::begin() {
  return AbstractNode::iterator(std::make_unique<VariableDeclarationIteratorImpl<AbstractNode>>(*this, 0));
}

AbstractNode::const_iterator VariableDeclaration::begin() const {
  return AbstractNode::const_iterator(std::make_unique<VariableDeclarationIteratorImpl<const AbstractNode>>(*this, 0));
}

AbstractNode::iterator VariableDeclaration::end() {
  return AbstractNode::iterator(std::make_unique<VariableDeclarationIteratorImpl<AbstractNode>>(*this,
                                                                                                countChildren()));
}

AbstractNode::const_iterator VariableDeclaration::end() const {
  return AbstractNode::const_iterator(std::make_unique<VariableDeclarationIteratorImpl<const AbstractNode>>(*this,
                                                                                                            countChildren()));
}

size_t VariableDeclaration::countChildren() const {
  return hasValue() + hasTarget();
}

nlohmann::json VariableDeclaration::toJson() const {
  nlohmann::json j = {{"type", getNodeType()},
                      {"datatype", getDatatype().toString()},
                      {"target", getTarget().toJson()}};
  if (hasValue()) {
    j["value"] = getValue().toJson();
  }
  return j;
}

std::string VariableDeclaration::toString(bool printChildren) const {
  return AbstractNode::toStringHelper(printChildren, {datatype.toString()});
}

std::string VariableDeclaration::getNodeType() const {
  return "VariableDeclaration";
}