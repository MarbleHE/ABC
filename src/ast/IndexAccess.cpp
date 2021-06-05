#include "ast_opt/ast/IndexAccess.h"
#include "ast_opt/utilities/IVisitor.h"

IndexAccess::~IndexAccess() = default;

IndexAccess::IndexAccess(std::unique_ptr<AbstractTarget> &&target, std::unique_ptr<AbstractExpression> &&index)
    : target(std::move(target)), index(std::move(index)) {}

IndexAccess::IndexAccess(const IndexAccess &other) : target(other.target ? other.target->clone(this) : nullptr),
                                                     index(other.index ? other.index->clone(this) : nullptr) {}

IndexAccess::IndexAccess(IndexAccess &&other) noexcept: target(std::move(other.target)),
                                                        index(std::move(other.index)) {}

IndexAccess &IndexAccess::operator=(const IndexAccess &other) {
  target = other.target ? other.target->clone(this) : nullptr;
  index = other.index ? other.index->clone(this) : nullptr;
  return *this;
}

IndexAccess &IndexAccess::operator=(IndexAccess &&other) noexcept {
  target = std::move(other.target);
  index = std::move(other.index);
  return *this;
}
std::unique_ptr<IndexAccess> IndexAccess::clone(AbstractNode *parent_) const {
  return std::unique_ptr<IndexAccess>(clone_impl(parent_));
}

bool IndexAccess::hasTarget() const {
  return target!=nullptr;
}

bool IndexAccess::hasIndex() const {
  return index!=nullptr;
}

AbstractTarget &IndexAccess::getTarget() {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

std::unique_ptr<AbstractTarget> IndexAccess::takeTarget() {
  return std::move(target);
}

const AbstractTarget &IndexAccess::getTarget() const {
  if (hasTarget()) {
    return *target;
  } else {
    throw std::runtime_error("Cannot get null target.");
  }
}

AbstractExpression &IndexAccess::getIndex() {
  if (hasIndex()) {
    return *index;
  } else {
    throw std::runtime_error("Cannot get null index.");
  }
}

const AbstractExpression &IndexAccess::getIndex() const {
  if (hasIndex()) {
    return *index;
  } else {
    throw std::runtime_error("Cannot get null index.");
  }
}

void IndexAccess::setTarget(std::unique_ptr<AbstractTarget> &&newTarget) {
  target = std::move(newTarget);
}

void IndexAccess::setIndex(std::unique_ptr<AbstractExpression> &&newIndex) {
  index = std::move(newIndex);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////
IndexAccess *IndexAccess::clone_impl(AbstractNode *parent_) const {
  auto p = new IndexAccess(*this);
  if (parent_) { p->setParent(*parent_); }
  return p;
}

void IndexAccess::accept(IVisitor &v) {
  v.visit(*this);
}

AbstractNode::iterator IndexAccess::begin() {
  return AbstractNode::iterator(std::make_unique<IndexAccessIteratorImpl<AbstractNode>>(*this, 0));
}

AbstractNode::const_iterator IndexAccess::begin() const {
  return AbstractNode::const_iterator(std::make_unique<IndexAccessIteratorImpl<const AbstractNode>>(*this, 0));
}

AbstractNode::iterator IndexAccess::end() {
  return AbstractNode::iterator(std::make_unique<IndexAccessIteratorImpl<AbstractNode>>(*this, countChildren()));
}

AbstractNode::const_iterator IndexAccess::end() const {
  return AbstractNode::const_iterator(std::make_unique<IndexAccessIteratorImpl<const AbstractNode>>(*this,
                                                                                                    countChildren()));
}

size_t IndexAccess::countChildren() const {
  return hasTarget() + hasIndex();
}

nlohmann::json IndexAccess::toJson() const {
  nlohmann::json j;
  j["type"] = getNodeType();
  if (hasTarget()) j["target"] = getTarget().toJson();
  if (hasIndex()) j["index"] = getIndex().toJson();
  return j;
}

std::string IndexAccess::toString(bool printChildren) const {
  return AbstractNode::toStringHelper(printChildren, {});
}

std::string IndexAccess::getNodeType() const {
  return "IndexAccess";
}