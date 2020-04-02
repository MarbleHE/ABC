#ifndef AST_OPTIMIZER_INCLUDE_VISITOR_COMPILETIMEEXPRESSIONSIMPLIFIER_H_
#define AST_OPTIMIZER_INCLUDE_VISITOR_COMPILETIMEEXPRESSIONSIMPLIFIER_H_

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <deque>
#include "NodeUtils.h"
#include "Visitor.h"
#include "EvaluationVisitor.h"
#include "ArithmeticExpr.h"
#include "LiteralFloat.h"
#include "Variable.h"
#include "OperatorExpr.h"

class CompileTimeExpressionSimplifier : public Visitor {
 private:
  /// A EvaluationVisitor instance that is used to evaluate parts of the AST in order to simplify them.
  EvaluationVisitor evalVisitor;

 public:
  CompileTimeExpressionSimplifier();

  /// Contains all nodes that can potentially be removed. The decision of a node's removal is to be made by its parent
  /// statement. If this decision is made (i.e., at the end of processing the statement), the node must be deleted from
  /// removableNodes.
  /// - AbstractNode*: A reference to the removable node.
  std::unordered_set<AbstractNode *> removableNodes;

  /// Stores the latest value of a variable while traversing through the AST.
  /// - std::string: The variable's identifier.
  /// - AbstractExpr*: The variable's value.
  std::unordered_map<std::string, AbstractExpr *> variableValues;

  /// Contains pointer to those nodes for which full or partial evaluation could be performed and hence can be deleted
  /// at the end of this simplification traversal.
  /// For example, the arithmetic expression represented by
  ///   ArithmeticExpr( LiteralInt(12), OpSymb::add, LiteralInt(42) )
  /// will be evaluated to 12+42=54.
  /// The node ArithmeticExpr (and all of its children) will be deleted and replaced by a new node LiteralInt(54).
  std::deque<AbstractNode *> nodesQueuedForDeletion;

  /// A flag that indicates whether variables should be replaced by their known value. The value can be a concrete
  /// value (i.e., subtype of AbstractLiteral) or a symbolic value containing unknown variables (e.g., x = y+4).
  /// This is not to be confused with evaluation of expressions where yet unknown expressions are computed.
  bool replaceVariablesByValues{true};

  /** @defgroup visit Methods implementing the logic of the visitor for each node type.
  *  @{
  */
  void visit(AbstractExpr &elem) override;

  void visit(AbstractNode &elem) override;

  void visit(AbstractStatement &elem) override;

  void visit(ArithmeticExpr &elem) override;

  void visit(AbstractMatrix &elem) override;

  void visit(Ast &elem) override;

  void visit(Block &elem) override;

  void visit(Call &elem) override;

  void visit(CallExternal &elem) override;

  void visit(Datatype &elem) override;

  void visit(Function &elem) override;

  void visit(FunctionParameter &elem) override;

  void visit(For &elem) override;

  void visit(GetMatrixElement &elem) override;

  void visit(If &elem) override;

  void visit(LiteralBool &elem) override;

  void visit(LiteralFloat &elem) override;

  void visit(LiteralInt &elem) override;

  void visit(LiteralString &elem) override;

  void visit(LogicalExpr &elem) override;

  void visit(Operator &elem) override;

  void visit(OperatorExpr &elem) override;

  void visit(ParameterList &elem) override;

  void visit(Return &elem) override;

  void visit(UnaryExpr &elem) override;

  void visit(VarAssignm &elem) override;

  void visit(VarDecl &elem) override;

  void visit(Variable &elem) override;

  void visit(While &elem) override;

  void visit(Rotate &elem) override;

  void visit(Transpose &elem) override;
  /** @} */ // End of visit group

  /// Checks whether there is a known value for this node. A value can either be an AbstractLiteral or an AbstractExpr
  /// with unknown variables (e.g., function parameters).
  /// \param node The node for which the existence of a value should be determined.
  /// \return True if the node's value is known, otherwise False.
  bool valueIsKnown(AbstractNode *node);

  /// Marks a node as a candidate for deletion. The node's first ancestor that is a statement has to decide whether its
  /// children that are marked to be removed can be deleted or not.
  /// \param node The node for which the evaluation result should be stored.
  void markNodeAsRemovable(AbstractNode *node);

  /// Returns the (first) known value of a node, i.e., the first element of the result vector. If a node has multiple
  /// values (e.g., Return statements), this method will return an exception.
  /// \param node The node for which the first value should be retrieved.
  /// \return The node's value.
  AbstractExpr *getFirstValue(AbstractNode *node);

  /// Evaluates a subtree, i.e., a node and all of its children by using the EvaluationVisitor.
  /// \param node The subtree's root node to be evaluated.
  /// \param valuesOfVariables The variable values required to evaluate this subtree.
  /// \return The evaluation's result as a vector of AbstractLiteral pointers. If the subtree does not include a Return
  ///         statement, then the result vector should always have one element only.
  std::vector<AbstractLiteral *> evaluateNodeRecursive(
      AbstractNode *node, std::unordered_map<std::string, AbstractLiteral *> valuesOfVariables);

  /// Takes the variableValues map and creates a new map containing a copy of all AbstractLiteral values. This map
  /// can then be passed to the EvaluationVisitor to evaluate a given subtree. The original variableValues map, however,
  /// contains AbstractExprs of non-evaluable variables too, for example, x=y+7 where y is a function parameter.
  /// \return A map of (variable identifier, variable value) pairs where variable values are AbstractLiterals.
  std::unordered_map<std::string, AbstractLiteral *> getTransformedVariableMap();

  /// A helper method to transform an If statement into a dependent assignment, for example:
  ///     if (condition) { x = trueValue; } else { x = falseValue; }
  /// is converted into
  ///     x = condition*trueValue + (1-b)*falseValue.
  /// This method takes the required parts of this expression, the condition, the value that should be assigned in case
  /// that the condition evaluates to True (trueValue) or to False (falseValue). It then generates and returns the
  /// following expression: condition*trueValue + (1-b)*falseValue.
  /// The method also considers the case where trueValue and/or falseValue are null and appropriately removes the
  /// irrelevant subtree from the resulting expression.
  /// \param condition The condition the assignment depends on, e.g., the condition of the If statement.
  /// \param trueValue The value to be used for the case that the condition evaluates to True.
  /// \param falseValue The value to be used for the case that the condition evaluates to False.
  /// \return An arithmetic expression of the form condition*trueValue + (1-b)*falseValue.
  static AbstractExpr *generateIfDependentValue(
      AbstractExpr *condition, AbstractExpr *trueValue, AbstractExpr *falseValue);

  /// This method must be called at the end of the visit(...) of each statement. The method makes sure that the binary
  /// expression accumulator is reset and if specified by enqueueStatementForDeletion=True, also marks the given
  /// statement for deletion.
  /// \param statement The statement for which visit(...) was executed.
  /// \param enqueueStatementForDeletion True if this statement should be marked for deletion, otherwise False
  /// (default).
  void cleanUpAfterStatementVisited(AbstractNode *statement, bool enqueueStatementForDeletion = false);

  /// Adds a new (variable identifier, variable value) pair to the map of variable values.
  /// \param variableIdentifier The variable identifier ("name" of the variable).
  /// \param valueAnyLiteralOrAbstractExpr The variable's value. This can be any kind of AbstractLiteral or
  void addVariableValue(const std::string &variableIdentifier, AbstractExpr *valueAnyLiteralOrAbstractExpr);

  /// Checks whether the given node is queued for deletion. Deletion will be carried out at the end of the traversal.
  /// \param node The node to be checked for deletion.
  /// \return True if this node is enqueued for deletion, otherwise False.
  bool isQueuedForDeletion(const AbstractNode *node);

  /// A helper method that takes a copy of the variableValues map that was created before visiting a node and determines
  /// the changes made by visiting the node. The changes recognized are newly declared variables (added variables) and
  /// variables whose value changed.
  /// \param variableValuesBeforeVisitingNode A copy of the variable values map.
  /// \return The changes between the map variableValuesBeforeVisitingNode and the current variableValues map.
  std::unordered_map<std::string, AbstractExpr *> getChangedVariables(
      std::unordered_map<std::string, AbstractExpr *> variableValuesBeforeVisitingNode);

  /// Takes an OperatorExpr consisting of a logical operator (i.e., AND, XOR, OR) and applies the Boolean laws to
  /// simplify the expression. For example, the expression <anything> AND False always evaluates to False, hence we can
  /// replace this OperatorExpr by the boolean value (LiteralBool) False. Other considered rules include:
  ///   * <anything> AND False ⟹ False
  ///   * <anything> AND True  ⟹ <anything>
  ///   * <anything> OR True   ⟹ True
  ///   * <anything> OR False  ⟹ <anything>
  ///   * <anything> XOR False ⟹ <anything>
  ///   * <anything> XOR True  ⟹ !<anything>  [not implemented yet]
  /// where <anything> denotes an arbitrary logical expression of the same logical operator.
  /// \param elem The OperatorExpr that should be simplified using Boolean laws.
  static void simplifyLogicalExpr(OperatorExpr &elem);

  std::unordered_set<std::string> removeVarsWrittenInStatementsFromVarValuesMap(Block &blockStmt);
};

/// Takes a Literal (e.g., LiteralInt) and checks whether its values are defined using a Matrix<AbstractExpr*>. In
/// this case, checks if the value (e.g., of type int) is known of each element such that Matrix<AbstractExpr*> can
/// be replaced by a Matrix<T> (e.g., Matrix<int>) where T is the Literal's associated primitive type (e.g., int).
/// If the matrix could be simplified, the current Literal is replaced by a new Literal of type U containing the
/// simplified matrix.
/// \tparam T The type of the elements of the respective AbstractLiteral subtype, e.g., int, float.
/// \tparam U The AbstractLiteral subtype, e.g., LiteralInt, LiteralFloat.
/// \param elem The AbstractLiteral subtype that should be simplified.
template<typename T, typename U>
void simplifyAbstractExprMatrix(U &elem);

#endif //AST_OPTIMIZER_INCLUDE_VISITOR_COMPILETIMEEXPRESSIONSIMPLIFIER_H_
