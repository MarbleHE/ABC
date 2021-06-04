#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PRINTVISITOR_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PRINTVISITOR_H_

#include <list>
#include <sstream>
#include <string>
#include <utility>

#include "ast_opt/ast/AbstractNode.h"
#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/utilities/PlainVisitor.h"


/// Forward declaration of the class that will actually implement the PrintVisitor's logic
class SpecialPrintVisitor;

/// PrintVisitor uses the Visitor<T> template to allow specifying default behaviour
typedef Visitor<SpecialPrintVisitor, PlainVisitor> PrintVisitor;

class SpecialPrintVisitor : public PlainVisitor {
 private:
  /// Reference to the stream to which we write the output
  std::ostream& os;

  /// Current indentation level
  int indentation_level = 0;

  /// Compute the current required indentation string
  /// from the current indentation_level
  [[nodiscard]] std::string getIndentation() const;

 public:
  explicit SpecialPrintVisitor(std::ostream& os);

  void visit(AbstractNode&);

  void visit(LiteralBool&);

};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_PRINTVISITOR_H_
