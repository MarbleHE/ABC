#include "ast_opt/visitor/ParentSettingVisitor.h"

void SpecialParentSettingVisitor::visit(AbstractNode &elem) {

  // If there is something on the stack, set it as the parent
  if(!stack.empty()) {
    if(stack.top() ==nullptr) throw std::runtime_error("nullptr not a valid parent.");

    if(elem.hasParent()) {
      if(stack.top() != &elem.getParent())
        throw std::runtime_error("Original parent and parent we would set do not match.");
    } else {
      elem.setParent(*stack.top());
    }
  }

  // Push elem on the stack
  stack.push(&elem);

  // Visit the children
  for(auto& c : elem) {
    c.accept(*this);
  }

  // Remove elem from the stack
  stack.pop();
}
