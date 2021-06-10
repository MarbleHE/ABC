#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_BatchingVisitor_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_BatchingVisitor_H_

#include <deque>
#include <set>
#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/runtime/RuntimeVisitor.h"

class SpecialBatchingVisitor;

/// The BatchingVisitor requires that the input is in Static Single Assignment (SSA) form
/// It also assumes that all public variables are scalar-valued
/// while secret variables are inherently treated as vectors
/// In order to disturb the AST as little as possible,
/// it transforms an index accesses x[i] into __rot__x[j]
/// where x_j is the required rotation to achieve the desired effect
/// and __rot__ is a reserved prefix used to indicate to the runtime system
/// that this is not a standard index access
typedef Visitor<SpecialBatchingVisitor, ScopedVisitor> BatchingVisitor;

class SpecialBatchingVisitor : public ScopedVisitor {
 private:

  struct TypedVariableValue {
    Datatype type;
    std::unique_ptr<AbstractExpression> value;
  };

  typedef VariableMap<TypedVariableValue> TypedVariableValueMap;

  /// A map from scoped identifiers to their datatype and value
  TypedVariableValueMap variableMap;

  /// Converts an AST into a string in our C-like input language
  /// Uses the ProgramPrintVisitor internally
  /// \param node root of AST to print
  /// \return C-like program representation of the AST
  static std::string printProgram(AbstractNode &node);

  /// Iterates over an ast (using child iterators instead of visitor pattern)
  /// and checks if there are any Variable(identifier) nodes present
  /// \param node root of an ast or expression tree to check
  /// \param identifier the identifiers to search for (use {"x"} to search for a single id)
  /// \return true iff at least one of the identifiers appears in ast or its children
  static bool containsVariable(const AbstractNode &node, const std::vector<std::string> &identifiers);

  /// This fakes "returns" from the visit calls
  /// This is only used for AbstractExpressions
  /// If this is set to something other than nullptr
  /// after a child node has been visited
  /// then the child node should be replaced with this node
  std::unique_ptr<AbstractExpression> replacementExpression;

  /// This fakes "returns" from the visit calls
  /// This is only used for AbstractStatements
  /// If this is set to true
  /// after a child node has been visited
  /// then the child node should be replaced with replacementStatement
  bool replaceStatement;

  /// This fakes "returns" from the visit calls
  /// This is only used for AbstractStatements
  /// If replaceStatement is true,
  /// after a child node has been visited
  //  then the child node should be replaced with replacementStatement
  std::unique_ptr<AbstractStatement> replacementStatement;

  /// This fakes passing an argument down to the child visiting calls
  /// This is set by the assignment and used to compute the offsets
  int targetSlot = 0;

  /// Contains the rotations required
  //  std::unordered_set<std::pair<ScopedIdentifier,int>> required_rotations;

 public:
  /// Custom iteration over statements that
  /// allows deleting & replacing child nodes
  /// safely from recursive calls
  /// without worrying about e.g. iterator invalidation
  void visit(Block &elem);

  /// Collects type information from the assignment
  void visit(VariableDeclaration &elem);


  /// Records the
  void visit(IndexAccess &elem);

  void visit(Assignment &elem);


};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_BatchingVisitor_H_