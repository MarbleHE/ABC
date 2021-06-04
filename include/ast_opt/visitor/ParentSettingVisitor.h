#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PARENTSETTINGVISITOR_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PARENTSETTINGVISITOR_H_

#include <stack>
#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/utilities/PlainVisitor.h"

class SpecialParentSettingVisitor;

typedef Visitor<SpecialParentSettingVisitor, PlainVisitor> ParentSettingVisitor;

/// This is an ugly hack since the parser currently does not set parents!
/// TODO: Set parents in parser properly, then remove this visitor!
class SpecialParentSettingVisitor : public PlainVisitor {
 private:
  std::stack<AbstractNode*> stack;

 public:
  void visit(AbstractNode& elem);
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PARENTSETTINGVISITOR_H_
