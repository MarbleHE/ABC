#include "ast_opt/ast/Assignment.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/utilities/IVisitor.h"

Assignment::~Assignment() = default;

Assignment::Assignment(std::unique_ptr<AbstractTarget> target_,
                       std::unique_ptr<AbstractExpression> value_)
    : target(std::move(target_)), value(std::move(value_)) {
  target->setParent(*this);
  value->setParent(*this);
}

Assignment::Assignment(const Assignment &other)
    : target(other.target ? other.target->clone(this) : nullptr),
      value(other.value ? other.value->clone(this) : nullptr) {};

Assignment::Assignment(Assignment &&other)
noexcept: target(std::move(other.target)), value(std::move(other.value)) {};

Assignment &Assignment::operator=(const Assignment &other) {
  AbstractStatement::operator=(other);
  target = other.target ? other.target->clone(this) : nullptr;
  value = other.value ? other.value->clone(this) : nullptr;
  return *this;
}

Assignment &Assignment::operator=(Assignment &&other) noexcept {
  AbstractStatement::operator=(other);
  target = std::move(other.target);
  value = std::move(other.value);
  return *this;
}

std::unique_ptr<Assignment> Assignment::clone(AbstractNode *parent_) const {
  return std::unique_ptr<Assignment>(clone_impl(parent_));
}

bool Assignment::hasTarget() const {
  return target!=nullptr;
}

bool Assignment::hasValue() const {
  return value!=nullptr;
}

AbstractTarget &Assignment::getTarget() {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

const AbstractTarget &Assignment::getTarget() const {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

std::unique_ptr<AbstractExpression> Assignment::takeTarget() {
  return std::move(target);
}

AbstractExpression &Assignment::getValue() {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

const AbstractExpression &Assignment::getValue() const {
  if (hasValue()) {
    return *value;
  } else {
    throw std::runtime_error("Cannot get null value.");
  }
}

std::unique_ptr<AbstractExpression> Assignment::takeValue() {
  return std::move(value);
}

void Assignment::setTarget(std::unique_ptr<AbstractTarget> newTarget) {
  target = std::move(newTarget);
}

void Assignment::setValue(std::unique_ptr<AbstractExpression> newValue) {
  value = std::move(newValue);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////
Assignment *Assignment::clone_impl(AbstractNode *parent_) const {
  auto p = new Assignment(*this);
  if (parent_) { p->setParent(*parent_); }
  return p;
}

void Assignment::accept(IVisitor &v) {
  v.visit(*this);
}

AbstractNode::iterator Assignment::begin() {
  return AbstractNode::iterator(std::make_unique<AssignmentIteratorImpl<AbstractNode>>(*this, 0));
}

AbstractNode::const_iterator Assignment::begin() const {
  return AbstractNode::const_iterator(std::make_unique<AssignmentIteratorImpl<const AbstractNode>>(*this, 0));
}

AbstractNode::iterator Assignment::end() {
  return AbstractNode::iterator(std::make_unique<AssignmentIteratorImpl<AbstractNode>>(*this, countChildren()));
}

AbstractNode::const_iterator Assignment::end() const {
  return AbstractNode::const_iterator(std::make_unique<AssignmentIteratorImpl<const AbstractNode>>(*this,
                                                                                                   countChildren()));
}

size_t Assignment::countChildren() const {
  return hasValue() + hasTarget();
}

nlohmann::json Assignment::toJson() const {
  nlohmann::json j;
  j["type"] = getNodeType();
  j["target"] = target ? target->toJson() : "";
  j["value"] = value ? value->toJson() : "";
  return j;
}

std::string Assignment::toString(bool printChildren) const {
  return AbstractNode::toStringHelper(printChildren, {});
}

std::string Assignment::getNodeType() const {
  return "Assignment";
}
