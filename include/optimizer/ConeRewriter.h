#ifndef AST_OPTIMIZER_INCLUDE_OPTIMIZER_CONEREWRITER_H_
#define AST_OPTIMIZER_INCLUDE_OPTIMIZER_CONEREWRITER_H_

#include <Node.h>
#include <vector>
#include <random>
#include <Ast.h>
#include <utility>
#include "Operator.h"

/// This class implements the Cone Rewriting method described in
/// [Aubry, P. et al.: Faster Homomorphic Encryption Is Not Enough: Improved Heuristic for Multiplicative Depth
/// Minimization of Boolean Circuits. (2019)].
class ConeRewriter {
 private:
  std::map<std::string, int>* multiplicativeDepths;
  std::map<std::string, int>* multiplicativeDepthsReversed;
  int maximumMultiplicativeDepth{};
  Ast &ast;

  // ------------------------------------–
  // Internal (non-universal) helper methods
  // ------------------------------------–
  void rewriteCones(Ast &astToRewrite, const std::vector<Node*> &coneEndNodes);

  bool isCriticalNode(int lMax, Node* n);

  int getMaxMultDepth(Ast &ast);

  Node* getNonCriticalLeafNode();

  static void addElements(std::vector<Node*> &result, std::vector<Node*> newElements);

  static void flattenVectors(std::vector<Node*> &resultVector, std::vector<std::vector<Node*>> vectorOfVectors);

  static void reverseEdges(const std::vector<Node*> &nodes);

  std::vector<Node*> computeReducibleCones();

  static std::vector<Node*> sortTopologically(const std::vector<Node*> &nodes);

  std::vector<Node*>* getCriticalPredecessors(Node* v);

  int computeMinDepth(Node* v);

  // ------------------------------------–
  // Algorithms presented in the paper
  // ------------------------------------–
  /// Implements the recursive cone construction algorithm [see Algorithm 1, page 8]
  /// \param v The starting node for the cone construction procedure.
  /// \param minDepth The minimal multiplicative depth to which cone search will be performed.
  /// \return The cone to be rewritten, or an empty set if the cone ending at v cannot be reduced.
  std::vector<Node*> getReducibleCones(Node* v, int minDepth);

  std::vector<Node*> getReducibleCones();

  /// Calls and prints the output of getReducibleCones for each node in the given Ast
  void getReducibleConesForEveryPossibleStartingNode(Ast &ast);

  /// Implements the multiplicative depth minimization heuristic [see Algorithm 2, page 10]
  Ast &applyMinMultDepthHeuristic();

  /// Implements the algorithm that constructs the graph C_{AND} of critical AND nodes [see paragraph 3.2, page 10].
  static std::vector<Node*> getAndCriticalCircuit(std::vector<Node*> delta);

  /// Implements the cone selection algorithm [see Algorithm 3, page 11]
  static std::vector<Node*> selectCones(std::vector<Node*> cAndCkt);

 public:
  explicit ConeRewriter(Ast &ast);

  /// This is the only entry point that should be used to apply cone rewriting.
  Ast &applyConeRewriting();

  static void printConesAsGraphviz(std::vector<Node*> &nodes);

  static std::vector<Node*> rewriteMultiInputGate(std::vector<Node*> inputNodes, OpSymb::LogCompOp gateType);
  std::pair<Node*, Node*> getCriticalAndNonCriticalInput(int lMax, Node* n);
  std::pair<Node*, Node*> getCriticalAndNonCriticalInput(int lMax, Node* n,
                                                         std::map<std::string, int>* multDepthsMap,
                                                         std::map<std::string, int>* reverseMultDepthsMap);

  void precomputeMultDepths();
  void precomputeMultDepths(Ast &ast,
                            std::map<std::string, int>* multDepthsMap,
                            std::map<std::string, int>* reverseMultDepthsMap);
  virtual ~ConeRewriter();

  bool isCriticalNode(int lMax,
                      Node* n,
                      std::map<std::string, int>* multDepthsMap,
                      std::map<std::string, int>* reverseMultDepthsMap);
};

/// Returns a random element from a
/// Credits to Christopher Smith from stackoverflow, see https://stackoverflow.com/a/16421677/3017719.
template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
  std::advance(start, dis(gen));
  return start;
}

#endif //AST_OPTIMIZER_INCLUDE_OPTIMIZER_CONEREWRITER_H_
