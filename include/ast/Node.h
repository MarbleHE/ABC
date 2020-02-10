#ifndef AST_OPTIMIZER_NODE_H
#define AST_OPTIMIZER_NODE_H

#include <vector>
#include <sstream>
#include "nlohmann/json.hpp"
#include "Visitor.h"

using json = nlohmann::json;

class Literal;

class Ast;

class Node {
 protected:
  /// Temporarily stores the reserved node ID until the first call of getUniqueNodeId() at which the reserved ID is
  /// fetched and the node's ID is assigned (field uniqueNodeId) based on the node's name and this reserved ID.
  /// This is a workaround because getNodeName() is a virtual method that cannot be called from derived classes and
  /// their constructor. After retrieving the node ID and assigning it to the uniqueNodeId field, it is deleted from
  /// this map.
  std::map<Node*, int> assignedNodeIds{};

  /// Stores the children of the current node if the node supports the circuit mode (see supportsCircuitMode()).
  std::vector<Node*> children{};

  /// Stores the parent nodes of the current node if the node supports the circuit mode (see supportsCircuitMode()).
  std::vector<Node*> parents{};

  static int nodeIdCounter;

  /// An identifier that is unique among all nodes during runtime.
  std::string uniqueNodeId;

  /// This attributes is used to link back to the original Node in an overlay circuit.
  Node* underlyingNode{};

  /// Generates a new node ID in the form "<NodeTypeName>_nodeIdCounter++" where <NodeTypeName> is the value obtained by
  /// getNodeName() and nodeIdCounter an ongoing counter of created Node objects.
  /// \return An unique node ID to be used as uniqueNodeId for the current node.
  std::string genUniqueNodeId();

  static int getAndIncrementNodeId();

  /// This special variant of getChildAtIndex returns the n-th parent instead of n-th child if isEdgeDirectionAware is
  /// passed and is true, and the current node has the property isReversed set to True.
  /// \param idx The position of the child to be retrieved.
  /// \param isEdgeDirectionAware If the node's status of isReversed should be considered.
  /// \return A reference to the node at the specified index in the children or parent vector.
  [[nodiscard]] Node* getChildAtIndex(int idx, bool isEdgeDirectionAware) const;

 public:
  Node();

  virtual ~Node();

  [[nodiscard]] Node* getUnderlyingNode() const;

  void setUnderlyingNode(Node* uNode);

  [[nodiscard]] virtual std::string getNodeName() const;

  std::string getUniqueNodeId();

  static void resetNodeIdCounter();

  [[nodiscard]] const std::vector<Node*> &getParents() const;

  [[nodiscard]] const std::vector<Node*> &getChildren() const;

  /// Returns all the ancestor nodes of the current node.
  /// \return A list of ancestor nodes.
  std::vector<Node*> getAnc();

  // Functions for handling children
  void addChild(Node* child, bool addBackReference = false);
  void addChildBilateral(Node* child);
  void addChildren(const std::vector<Node*> &childrenToAdd, bool addBackReference = false);
  void setChild(std::vector<Node*>::const_iterator position, Node* value);
  void removeChild(Node* child);
  void removeChildren();
  [[nodiscard]] int countChildrenNonNull() const;

  /// Returns the child at the given index.
  /// \param idx The position of the children in the Node::children vector.
  /// \return The child at the given index of the children vector, or a nullptr if there is no child at this position.
  [[nodiscard]] Node* getChildAtIndex(int idx) const;

  // Functions for handling parents
  void addParent(Node* n);
  void removeParent(Node* node);
  void removeParents();
  bool hasParent(Node* n);

  static void addParentTo(Node* parentNode, std::vector<Node*> nodesToAddParentTo);

  void swapChildrenParents();

  virtual Literal* evaluate(Ast &ast);

  virtual void accept(Visitor &v);

  [[nodiscard]] virtual json toJson() const;

  [[nodiscard]] virtual std::string toString() const;

  friend std::ostream &operator<<(std::ostream &os, const std::vector<Node*> &v);

  [[nodiscard]] virtual Node* cloneFlat();

  [[nodiscard]] virtual Node* cloneRecursiveDeep(bool keepOriginalUniqueNodeId);

  void setUniqueNodeId(const std::string &unique_node_id);

  /// This method returns True iff the class derived from the Node class properly makes use of the child/parent fields
  /// as it would be expected in a circuit.
  virtual bool supportsCircuitMode();

  /// Indicates the number of children that are allowed for a specific node.
  /// For example, a binary expression accepts exactly three attributes and hence also exactly three children:
  /// left operand, right operand, and operator.
  /// If the node does not implement support for child/parent relationships, getMaxNumberChildren() return 0.
  /// \return An integer indicating the number of allowed children for a specific node.
  virtual int getMaxNumberChildren();

  /// Indicates whether the edges of this node are reversed compared to its initial state.
  bool isReversed{false};

  [[nodiscard]] std::vector<Node*> getChildrenNonNull() const;
  [[nodiscard]] std::vector<Node*> getParentsNonNull() const;

  /// Removes this node from all of its parents and children, and also removes all parents and children from this node.
  void isolateNode();

  /// Removes the node 'child' bilateral, i.e., on both ends of the edge. In other words, removes the node 'child' from
  /// this node, and this node from the parents list of 'child' node.
  /// \param child The child to be removed from this node.
  void removeChildBilateral(Node* child);

  [[nodiscard]] bool hasReversedEdges() const;

  /// Casts a node to type T which must be the specific derived class of the node to cast successfully.
  /// \tparam T The derived class of the node object.
  /// \return A pointer to the casted object, or a std::logic_error if cast was unsuccessful.
  template<typename T>
  T* castTo() {
    if (auto castedNode = dynamic_cast<T*>(this)) {
      return castedNode;
    } else {
      std::stringstream outputMsg;
      outputMsg << "Cannot cast object of type Node to given class ";
      outputMsg << typeid(T).name() << ".";
      outputMsg << "Because node (" << this->getUniqueNodeId() << ") is of type ";
      outputMsg << this->getNodeName() << ".";
      throw std::logic_error(outputMsg.str());
    }
  }
};

#endif //AST_OPTIMIZER_NODE_H
