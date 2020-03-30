#include "For.h"
#include "LogicalExpr.h"
#include "Block.h"

std::string For::getNodeType() const {
  return "For";
}

void For::accept(Visitor &v) {
  v.visit(*this);
}

For::For(AbstractStatement *initializer,
         LogicalExpr *condition,
         AbstractStatement *update,
         AbstractStatement *statementToBeExecuted) {
  setAttributes(initializer, condition, update, statementToBeExecuted);
}

AbstractExpr *For::getInitializer() const {
  return reinterpret_cast<AbstractExpr *>(getChildAtIndex(0));
}

LogicalExpr *For::getCondition() const {
  return reinterpret_cast<LogicalExpr *>(getChildAtIndex(1));
}

AbstractExpr *For::getUpdateStatement() const {
  return reinterpret_cast<AbstractExpr *>(getChildAtIndex(2));
}

AbstractStatement *For::getStatementToBeExecuted() const {
  return reinterpret_cast<AbstractStatement *>(getChildAtIndex(3));
}

void For::setAttributes(AbstractStatement *initializer,
                        LogicalExpr *condition,
                        AbstractStatement *update,
                        AbstractStatement *statementToBeExecuted) {
  removeChildren();
  if (dynamic_cast<Block *>(statementToBeExecuted)==nullptr) {
    statementToBeExecuted = new Block(statementToBeExecuted);
  }
  addChildren({initializer, condition, update, statementToBeExecuted}, true);
}

int For::getMaxNumberChildren() {
  return 4;
}

AbstractNode *For::clone(bool keepOriginalUniqueNodeId) {
  auto clonedNode = new For(getInitializer()->clone(keepOriginalUniqueNodeId)->castTo<AbstractStatement>(),
                            getCondition()->clone(keepOriginalUniqueNodeId)->castTo<LogicalExpr>(),
                            getUpdateStatement()->clone(false)->castTo<AbstractStatement>(),
                            getStatementToBeExecuted()->clone(false)->castTo<AbstractStatement>());
  clonedNode->updateClone(keepOriginalUniqueNodeId, this);
  return clonedNode;
}

std::string For::toString(bool printChildren) const {
  return AbstractNode::generateOutputString(printChildren, {});
}

bool For::supportsCircuitMode() {
  return true;
}

json For::toJson() const {
  json j;
  j["type"] = getNodeType();
  j["condition"] = getCondition()->toJson();
  j["initializer"] = getInitializer()->toJson();
  j["update"] = getUpdateStatement()->toJson();
  j["statement"] = getStatementToBeExecuted()->toJson();
  return j;
}

bool For::isEqual(AbstractStatement *other) {
  if (auto otherFor = dynamic_cast<For *>(other)) {
    auto sameInitializer = getInitializer()->isEqual(otherFor->getInitializer());
    auto sameCondition = getCondition()->isEqual(otherFor->getCondition());
    auto sameUpdateStmt = getUpdateStatement()->isEqual(otherFor->getUpdateStatement());
    auto sameBody = getStatementToBeExecuted()->isEqual(otherFor->getStatementToBeExecuted());
    return sameInitializer && sameCondition && sameUpdateStmt && sameBody;
  }
  return false;
}
