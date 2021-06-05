#ifndef AST_OPTIMIZER_SRC_VISITOR_CONTROLFLOWGRAPHVISITOR_H_
#define AST_OPTIMIZER_SRC_VISITOR_CONTROLFLOWGRAPHVISITOR_H_

#include <iostream>
#include <set>
#include "ast_opt/ast/AbstractStatement.h"
#include "ast_opt/utilities/Visitor.h"
#include "ast_opt/visitor/controlFlowGraph/GraphNode.h"

// Forward declaration
class SpecialControlFlowGraphVisitor;

/// ControlFlowGraphVisitor uses the Visitor<T> template to allow specifying default behaviour
typedef Visitor<SpecialControlFlowGraphVisitor> ControlFlowGraphVisitor;

class SpecialControlFlowGraphVisitor : public ScopedVisitor {
 private:
  // A list of all GraphNodes belonging to this CFG/DFG and created by this ControlFlowGraphVisitor.
  // Node at index 0 is the root node.
  std::vector<std::unique_ptr<GraphNode>> nodes;

  /// The nodes that were created most recently. Those are the parent nodes of the next node to be created.
  std::vector<std::reference_wrapper<GraphNode>> lastCreatedNodes;

  /// Creates a new GraphNode for the given AbstractNode and appends it as child to each of the last created
  /// GraphNodes (see lastCreatedNodes).
  /// \param astNode The node for which a new GraphNode object should be created for.
  /// \return (A reference to) the newly created GraphNode object.
  GraphNode &createGraphNodeAndAppendToCfg(AbstractNode &astNode);

  /// This constructor is required for testing purposes.
  /// Creates a new GraphNode for the given AbstractNode and appends it as child to each of the last created
  /// GraphNodes (see lastCreatedNodes).
  /// \param statement The node for which a new GraphNode object should be created for.
  /// \param parentNodes The node(s) that should be added as parent nodes for the newly created GraphNode.
  /// \return (A reference to) the newly created GraphNode object.
  GraphNode &createGraphNodeAndAppendToCfg(AbstractNode &statement,
                                           const std::vector<std::reference_wrapper<GraphNode>> &parentNodes);

  /// A set containing pairs of (scoped identifiers, access type) where the access type describes if a variable was read
  /// or written. This set collects all information of visited children of a statement and must be cleared before
  /// leaving a statement by calling storeAccessedVariables.
  VarAccessMapType variableAccesses;

  // This class has implemented a way to deal with variables that are declared in a part of the program
  // that is not in the given sub-AST and as such not visited by the visitor:
  //   If the visitor is initialized with ControlFlowGraphVisitor(ignoreNonDeclaredVariables = false), then variables
  // for which no declaration could be found will just be ignored. Consequently, also read/write accesses to them are
  // not being tracked in the variableAccesses map.

  /// Defines whether non-declared variables are ignored when calling Scope::resolveIdentifier. If this is not enabled
  /// (ignoreNonDeclaredVariables = False), then Scope::resolveIdentifier will raise an exception. Note that if non-
  /// declared variables are ignored, their read/write access is not tracked in the variableAccesses map.
  bool ignoreNonDeclaredVariables = false;

 public:
  SpecialControlFlowGraphVisitor() = default;

  explicit SpecialControlFlowGraphVisitor(bool ignoreNonDeclaredVariables);

  virtual ~SpecialControlFlowGraphVisitor() = default;

  void visit(Assignment &node) override;

  void visit(Block &node) override;

  void visit(For &node) override;

  void visit(Function &node) override;

  void visit(FunctionParameter &node) override;

  void visit(If &node) override;

  void visit(Return &node) override;

  void visit(Variable &node) override;

  void visit(VariableDeclaration &node) override;

  /// Stores the tracked variable accesses in the given GraphNode and clears the variable access map used for tracking.
  /// This assigns the variable access information, collected by visiting a statement's children nodes, to the
  /// corresponding GraphNode in the CFG.
  /// \param graphNode The GraphNode to that the tracked variable accessed should be assigned to.
  void storeAccessedVariables(GraphNode &graphNode);

  /// Marks a scoped identifier (e.g., Variable) as accessed by the given access type.
  /// \param scopedIdentifier The identifier that should be marked as accessed.
  /// \param accessType The access type that describes how the variable was accessed (read or write).
  void markVariableAccess(const ScopedIdentifier &scopedIdentifier, VariableAccessType accessType);

  /// Gets the root node of the control flow graph.
  /// \return (A reference to) the root node of the control flow graph.
  [[nodiscard]] GraphNode &getRootNode();

  /// Gets the root node of the control flow graph.
  /// \return (A const reference to) the root node of the control flow graph.
  [[nodiscard]] const GraphNode &getRootNode() const;

  /// Checks whether the given node is supported to be the first node where the ControlFlowGraphVisitor is called on.
  /// The ControlFlowGraphVisitor requires this to be a Block, For, of If node.
  /// \param node The node on that visit(...) was called on.
  void checkEntrypoint(AbstractNode &node);

  /// This method uses the information gained in the pass of this visitor (i.e., calling visit(..) on a node), in
  /// particular the information of variable reads and writes, and constructs a data flow graph. This graph has an edge
  /// from each statement where a variable is read to the last statement where the respective variable was written. By
  /// considering If statements, there might potentially exist multiple of such last writes to a variable. The following
  /// example demonstrates a program where variable access to "a" in line 06 would have two backward edges (to line 02
  /// and line 04) as both statements could cause a write to variable "a".
  ///
  ///  01:  if (c > 100) {
  ///  02:     a = 1;
  ///  03:  else {
  ///  04:     a = 0;
  ///  05:  }
  ///  06:  c = a*22;
  ///
  /// The data flow graph is constructed in two passes:
  ///   In the first pass, we traverse the control flow graph in a breadth-first search (BFS) style and propagate the
  /// knowledge at which node a all variables seen so far have been written the last time. This also considers joint
  /// points (i.e., nodes with two incoming edges) properly. In case that a loop is involved, it might be necessary to
  /// visit the loops body twice.
  ///   In the second pass, we iterate over all nodes that we have visited in the first pass and check whether the
  /// respective node reads a variable. If yes, we lookup when the respective variable was written the last time (i.e.,
  /// in which node) and add an backward edge (last_written_node -> read_node).
  void buildDataFlowGraph();
};

#endif //AST_OPTIMIZER_SRC_VISITOR_CONTROLFLOWGRAPHVISITOR_H_
