#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_ProgramTransformationVisitor_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_ProgramTransformationVisitor_H_

#include <deque>
#include <set>
#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/runtime/RuntimeVisitor.h"

class SpecialProgramTransformationVisitor;

/// The ProgramTransformationVisitor
typedef Visitor<SpecialProgramTransformationVisitor, ScopedVisitor> ProgramTransformationVisitor;

class SpecialProgramTransformationVisitor : public ScopedVisitor {
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
  static bool containsVariable(const AbstractNode& node, const std::vector<std::string>& identifiers);

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

  /// A counter that keeps track of the nesting level while visiting For-loops. The first value indicates the
  /// depth of the currently visiting loop body. The second value the depth of the deepest loop. For example:
  ///   ...
  ///   for (...) {     // currentLoopDepth_maxLoopDepth = (1,1)
  ///      ...
  ///      for (...) {   // currentLoopDepth_maxLoopDepth = (2,2)
  ///         ...
  ///      }             // currentLoopDepth_maxLoopDepth = (1,2)
  ///   }                // currentLoopDepth_maxLoopDepth = (0,0)
  ///   ...
  std::pair<int, int> currentLoopDepth_maxLoopDepth = {std::pair(0, 0)};

  int fullyUnrollLoopMaxNumIterations = 1 << 20; // two to the power of 20

  int maxNumLoopUnrollings = 4;

  /// A method to be called immediately after entering the For-loop's visit method.
  /// updates currentLoopDepth_maxLoopDepth
  void enteredForLoop();

  /// A method to be called before leaving the For-loop's visit method.
  /// updates currentLoopDepth_maxLoopDepth
  void leftForLoop();

  /// Determines whether the number of total allowed loop unrollings is already exhausted.
  /// \return True if the maxNumLoopUnrollings is not reached yet or if there is no limitation in the number of total
  /// loop unrollings. Otherwise returns False.
  bool isUnrollLoopAllowed() const;

  /// Identify all variables that are written and read from in the body + condition of a for-loop
  /// \param forLoop The for loop to investigate
  /// \param VariableValues Initial VariableValues BEFORE the block
  /// \return Variables, with their associated scopes, that match the criteria
  std::unordered_set<ScopedIdentifier> identifyReadWriteVariables(For &forLoop);

  /// Emits a variableDeclaration statement for variable into its origin scope
  /// uses the value from variableMap, if the value is not nullptr
  /// \param variable variable to emit
  /// \return
  std::unique_ptr<VariableDeclaration> generateVariableDeclaration(const ScopedIdentifier &variable,
                                                                   AbstractNode *parent);

  /// Builds an Assignment for the variable
  /// uses the value from variableMap (if not nullptr, otherwise this returns nullptr, too)
  /// \param variable variable to assign to
  /// \return (unique ptr to) Assignment or nullptr (if no value in variableMap)
  std::unique_ptr<Assignment> generateVariableAssignment(const ScopedIdentifier &variable,
                                                         AbstractNode *parent);

 public:

  void visit(Function &elem);

  void visit(Block &elem);

  void visit(VariableDeclaration &elem);

  void visit(Assignment &elem);

  void visit(Variable &elem);

  void visit(IndexAccess& elem);

  void visit(BinaryExpression &elem);

  void visit(UnaryExpression &elem);

  void visit(ExpressionList &elem);

  void visit(Return &elem);

  void visit(For &expression);

  void visit(AbstractExpression &elem);
};

//  /// Keeps track of all emitted variable declarations and maps each to an associated EmittedVariableData pointer.
//  std::map<ScopedVariable, EmittedVariableData *> emittedVariableDeclarations;
//
//  /// Maps emitted Assignments to their corresponding VarDecl statement in emittedVariableDeclarations.
//  std::map<AbstractNode *,
//           std::map<ScopedVariable, EmittedVariableData *>::iterator> emittedAssignments;

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
//  static AbstractExpression *generateIfDependentValue(
//      AbstractExpression *condition, AbstractExpression *trueValue, AbstractExpression *falseValue);

/// Takes an OperatorExpression consisting of a logical operator (i.e., AND, XOR, OR) and applies the Boolean laws to
/// simplify the expression. For example, the expression <anything> AND False always evaluates to False, hence we can
/// replace this OperatorExpression by the boolean value (LiteralBool) False. Other considered rules include:
///   * <anything> AND False ⟹ False
///   * <anything> AND True  ⟹ <anything>
///   * <anything> OR True   ⟹ True
///   * <anything> OR False  ⟹ <anything>
///   * <anything> XOR False ⟹ <anything>
///   * <anything> XOR True  ⟹ !<anything>  [not implemented yet]
/// where <anything> denotes an arbitrary logical expression of the same logical operator.
/// \param elem The OperatorExpression that should be simplified using Boolean laws.
//  static void simplifyLogicalExpr(OperatorExpression &elem);

/// Creates a new Assignment statement of the variable that the given iterator (variableToEmit) is pointing to.
/// The method ensures that there exists a variable declaration statement (VarDecl) in the scope where this
/// variable was originally declared.
/// Returns a set, because emitting a variable might require emitting temporary variables at the same time
/// \param variableToEmit The variable to be emitted, i.e., for that a variable assignment statement should be
/// generated.
/// \return A  set of variable assignment statements for the given variable (variableToEmit).
//  std::set<Assignment *> emitAssignment(ScopedVariable variableToEmit);

/// Emit variable assignments (and/or declaration, if necessary) for each of variables
/// The VarDecl will be created in the scope associated with the variable
/// \param variables  Variables for which VarDecls are to be emitted, and their associated scope
/// \return Set of variable assignment statements for the given variables.
//  std::set<Assignment *> emitAssignments(std::set<ScopedVariable> variables);

/// Creates a new VarDecl statements of the variable that the given iterator (variableToEmit) is pointing to.
/// The variable declaration is emitted as the first statement in the scope where the variable was initially
/// declared. The generated declaration statement is added to the emittedVariableDeclarations map to keep track of
/// it. On contrary to emitAssignment, this method automatically adds the statement to the AST instead of
/// returning the generated statement.
/// \param variableToEmit The variable to be emitted, i.e., for that a variable declaration statement should be
/// generated.
//  void emitVariableDeclaration(ScopedVariable variableToEmit);


/// Sets a new value matrixElementValue to the position indicated by (row, column) in matrix referred by
/// variableIdentifier. This implements variable assignments of the form M[rowIdx][colIdx] = value; where value is
/// a single element (AbstractExpr or primitive, e.g., int).
/// \param variableIdentifier A variable identifier that must refer to a matrix, i.e., any subtype of an
/// AbstractLiteral.
/// \param row The row index where the new value should be written to.
/// \param column The column index where the new value should be written to.
/// \param matrixElementValue The matrix value that should be written to the index given as (row, column).
//  void setMatrixVariableValue(const std::string &variableIdentifier,
//                              int row,
//                              int column,
//                              AbstractExpression *matrixElementValue);

//  /// Appends a row/column to a matrix or overwrites an existing row/column. This implements variable assignments of
//  /// the form M[idx] = vec; where vec is either a row vector, e.g., [4 2 1] or a column vector, e.g., [4; 2; 1].
//  /// \param variableIdentifier A variable identifier that must refer to a matrix, i.e., any subtype of an
//  /// AbstractLiteral.
//  /// \param posIndex The index where the row/column should be appended to. If matrixRowOrColumn is a row vector,
//  /// this index is considered as row index. Otherwise, if matrixRowOrColumn is a column vector, this index is
//  /// considered as column index.
//  /// \param matrixRowOrColumn An AbstractLiteral consisting of a (1,x) or (x,1) matrix.
//  void appendVectorToMatrix(const std::string &variableIdentifier, int posIndex, AbstractLiteral *matrixRowOrColumn);


/// Handles the partial loop unrolling to enable batching of the loop's body statements.
/// \param elem The For-loop to be unrolled.
/// \return A pointer to the new node if the given For-loop was replaced in the children vector of the For-loop's
/// parent.
//  AbstractNode *doPartialLoopUnrolling(For &elem);

///**
// * A helper struct that is used by emittedVariableDeclarations and helps to keep track of the relationship between a
// * variable (given as pair of identifier an scope), the associated (emitted) variable declaration statement, and a
// * reference to all emitted variable assignments that depend on this variable declaration. This allows to determine
// * at the end of the traversal if the emitted Assignment was meanwhile deleted and we do not need the VarDecl anymore.
// */
//struct EmittedVariableData {
//  /// the emitted variable declaration statement
//  AbstractNode *varDeclStatement;
//  /// the emitted variable assignment statements that require this declaration
//  std::unordered_set<AbstractNode *> emittedAssignments;
//  /// other nodes (e.g., MatrixAssignment) that depend on this variable declaration
//  std::unordered_set<AbstractNode *> dependentAssignments;
//
// public:
//  explicit EmittedVariableData(AbstractNode *varDeclStatement) : varDeclStatement(varDeclStatement) {}
//
//  void addAssignment(AbstractNode *varAssignm) { emittedAssignments.insert(varAssignm); }
//
//  void removeAssignment(AbstractNode *varAssignm) { emittedAssignments.erase(varAssignm); }
//
//  bool hasNoReferringAssignments() { return emittedAssignments.empty() && dependentAssignments.empty(); }
//
//  AbstractNode *getVarDeclStatement() { return varDeclStatement; }
//};
//

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_VISITOR_ProgramTransformationVisitor_H_