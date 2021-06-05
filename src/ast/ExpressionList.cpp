#include <iostream>
#include <exception>
#include "ast_opt/ast/ExpressionList.h"
#include "ast_opt/utilities/IVisitor.h"

/// Convenience typedef for conciseness
typedef std::unique_ptr<AbstractExpression> exprPtr;

ExpressionList::~ExpressionList() = default;

ExpressionList::ExpressionList(std::vector<std::unique_ptr<AbstractExpression>> && expressions) : expressions(std::move(expressions)) {}

ExpressionList::ExpressionList(const ExpressionList &other) {
  // deep-copy the expressions, including nullptrs
  expressions.reserve(other.expressions.size());
  for (auto &s: other.expressions) {
    expressions.emplace_back(s ? s->clone(this) : nullptr);
  }
}

ExpressionList::ExpressionList(ExpressionList &&other) noexcept:  expressions(std::move(other.expressions)) {}

ExpressionList &ExpressionList::operator=(const ExpressionList &other) {
  expressions.clear();
  // deep-copy the expressions, including nullptrs
  expressions.reserve(other.expressions.size());
  for (auto &s: other.expressions) {
    expressions.emplace_back(s ? s->clone(this) : nullptr);
  }
  return *this;
}
ExpressionList &ExpressionList::operator=(ExpressionList &&other) noexcept {
  expressions = std::move(other.expressions);
  return *this;
}
std::unique_ptr<ExpressionList> ExpressionList::clone(AbstractNode* parent_) const {
  return std::unique_ptr<ExpressionList>(clone_impl(parent_));
}

bool ExpressionList::hasNullExpressions() {
  // Because std::unique_ptr doesn't have copy, we can't use std::count_if
  size_t count = 0;
  for (auto &s : expressions) {
    if (s==nullptr) { count++; }
  }
  return count!=0;
}

std::vector<std::reference_wrapper<AbstractExpression>> ExpressionList::getExpressions() {
  std::vector<std::reference_wrapper<AbstractExpression>> r;
  for (auto &o: expressions) {
    if (o!=nullptr) { r.emplace_back(*o); }
  }
  return r;
}

std::vector<std::reference_wrapper<const AbstractExpression>> ExpressionList::getExpressions() const {
  std::vector<std::reference_wrapper<const AbstractExpression>> r;
  for (auto &o: expressions) {
    if (o!=nullptr) { r.emplace_back(*o); }
  }
  return r;
}

std::vector<std::unique_ptr<AbstractExpression>> &ExpressionList::getExpressionPtrs() {
  return expressions;
}

std::vector<std::unique_ptr<AbstractExpression>> ExpressionList::takeExpressions() {
  return std::move(expressions);
}

void ExpressionList::setExpressions(std::vector<std::unique_ptr<AbstractExpression>> new_expressions) {
  expressions = std::move(new_expressions);
}
void ExpressionList::appendExpression(std::unique_ptr<AbstractExpression> expression) {
  expressions.emplace_back(std::move(expression));
}

void ExpressionList::prependExpression(std::unique_ptr<AbstractExpression> expression) {
  expressions.insert(expressions.begin(),std::move(expression));
}

void ExpressionList::removeNullExpressions() {
  std::vector<exprPtr> new_expressions;
  for (auto &o: expressions) {
    if (o!=nullptr) { new_expressions.emplace_back(std::move(o)); }
  }
  expressions = std::move(new_expressions);
}

///////////////////////////////////////////////
////////// AbstractNode Interface /////////////
///////////////////////////////////////////////
ExpressionList *ExpressionList::clone_impl(AbstractNode* parent_) const {
  auto p = new ExpressionList(*this);
  if(parent_) {p->setParent(*parent_);}
  return p;
}

void ExpressionList::accept(IVisitor &v) {
  v.visit(*this);
}

AbstractNode::iterator ExpressionList::begin() {
  return AbstractNode::iterator(std::make_unique<ExpressionListIteratorImpl<AbstractNode>>(*this,
                                                                                  expressions.begin(),
                                                                                  expressions.end()));
}

AbstractNode::const_iterator ExpressionList::begin() const {
  return AbstractNode::const_iterator(std::make_unique<ExpressionListIteratorImpl<const AbstractNode>>(*this,
                                                                                              expressions.begin(),
                                                                                              expressions.end()));
}

AbstractNode::iterator ExpressionList::end() {
  return AbstractNode::iterator(std::make_unique<ExpressionListIteratorImpl<AbstractNode>>(*this,
                                                                                  expressions.end(),
                                                                                  expressions.end()));
}

AbstractNode::const_iterator ExpressionList::end() const {
  return AbstractNode::const_iterator(std::make_unique<ExpressionListIteratorImpl<const AbstractNode>>(*this,
                                                                                              expressions.end(),
                                                                                              expressions.end()));
}

size_t ExpressionList::countChildren() const {
  // Only non-null entries in the vector are counted as children
  // Because std::unique_ptr doesn't have copy, we can't use std::count_if
  size_t count = 0;
  for (auto &s : expressions) {
    if (s!=nullptr) { count++; }
  }
  return count;
}

nlohmann::json ExpressionList::toJson() const {
  std::vector<std::reference_wrapper<const AbstractExpression>> stmts = getExpressions();
  std::vector<nlohmann::json> expressionsJson;
  for(const AbstractExpression& o: stmts) {
    expressionsJson.push_back(o.toJson());
 }
  nlohmann::json j = {{"type", getNodeType()},
                      {"expressions", expressionsJson}};
  return j;
}

std::string ExpressionList::toString(bool printChildren) const {
  return AbstractNode::toStringHelper(printChildren, {});
}

std::string ExpressionList::getNodeType() const {
  return "ExpressionList";
}



