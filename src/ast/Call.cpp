#include <typeindex>
#include <sstream>
#include <utility>
#include "Call.h"
#include "Function.h"
#include "AbstractLiteral.h"
#include "LiteralInt.h"
#include "LiteralBool.h"
#include "LiteralString.h"

json Call::toJson() const {
  json j = {{"type", getNodeType()},
            {"arguments", getArguments()},
            {"function", getFunc()->toJson()}};
  return j;
}

void Call::accept(Visitor &v) {
  v.visit(*this);
}

std::string Call::getNodeType() const {
  return "Call";
}

Call::Call(Function *func) {
  setAttributes({}, func);
}

Call::Call(std::vector<FunctionParameter *> parameterValuesForCalledFunction, Function *func) {
  setAttributes(std::move(parameterValuesForCalledFunction), func);
}

Function *Call::getFunc() const {
  return reinterpret_cast<Function *>(getChildAtIndex(1));
}

std::vector<FunctionParameter *> Call::getArguments() const {
  return reinterpret_cast<ParameterList *>(getChildAtIndex(0))->getParameters();
}

AbstractNode *Call::clone(bool keepOriginalUniqueNodeId) {
  std::vector<FunctionParameter *> clonedArgs;
  for (auto &arg : getArguments()) {
    clonedArgs.push_back(arg->clone(keepOriginalUniqueNodeId)->castTo<FunctionParameter>());
  }
  auto clonedNode = static_cast<AbstractExpr *>(
      new Call(clonedArgs, this->getFunc()->clone(keepOriginalUniqueNodeId)->castTo<Function>()));

  if (keepOriginalUniqueNodeId) clonedNode->setUniqueNodeId(this->AbstractExpr::getUniqueNodeId());
  if (this->AbstractExpr::isReversed) clonedNode->swapChildrenParents();
  return clonedNode;
}

void Call::setAttributes(std::vector<FunctionParameter *> functionCallParameters, Function *functionToBeCalled) {
  removeChildren();
  addChildren({new ParameterList(std::move(functionCallParameters)), functionToBeCalled});
}

int Call::getMaxNumberChildren() {
  return 2;
}

bool Call::supportsCircuitMode() {
  return true;
}

ParameterList *Call::getParameterList() const {
  return reinterpret_cast<ParameterList *>(getChildAtIndex(0));
}
