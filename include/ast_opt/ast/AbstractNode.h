#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ABSTRACTNODE_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ABSTRACTNODE_H_

#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <typeinfo>
#include <nlohmann/json.hpp>

/// forward declaration of Visitor interface from ast_opt/visitor/IVisitor.h
class IVisitor;

/// Forward Iterator that redirects all calls to a (polymorphic) BaseIteratorImpl iterator
template<typename T>
class NodeIterator;

/// AbstractNode defines the common interface for all nodes.
///
/// DIRECTED GRAPH:
/// Nodes form a directed graph, with one end of each edge being the parent and the other being the child.
/// Nodes have exactly one pointer to their parent. If null, we say the node does not have a parent.
/// Nodes might have an arbitrary number of children, including none.
/// Some derived classes have a fixed number of children, while this changes dynamically for others.
/// Interaction with children should primarily happen via derived classes' specific getter/setter methods.
/// However, it is also possible to iterate over the children of any node with a NodeIterator
/// The order of the children is up to the derived class.
/// Note that some derived classes might internally use nullptr to represent "empty slots"
/// (e.g. deleted stmts in a Block, or an empty else-stmt in an If stmt)
/// If a child is null, we say it does not exist and it is not exposed externally.
///
/// LIFECYCLE MANAGEMENT:
/// A node owns its children and its children are deleted when the node itself is deleted.
/// Derived classes MUST implement this behavior, for example by using std::unique_ptr<>
/// Note that the parent is a raw pointer, however the above semantics should exclude dangling parent pointers.
/// Deleting a node will invalidate all iterators over this node and its children.
///
/// Changing Ownership of a Node (e.g. when moving a node around in a tree)
/// should proceed by first taking ownership to the local scope via takeNode()
/// Then, adding the node to its new parent via a derived class's setter method
/// Internally, the setter should first add the child and then use setParent()
///
class AbstractNode {
 private:
  /// Stores the parent nodes of the current node.
  AbstractNode *parent = nullptr;

  /// Clones a node recursively, i.e., by including all of its children.
  /// Because return-type covariance does not work with smart pointers,
  /// derived classes are expected to both override this function (for usage with base class ptrs/refs/etc)
  /// and introduce a std::unique_ptr<DerivedNode> clone() method that hides AbstractNode::clone() (for use with derived class ptrs/refs)
  /// WARNING: This method should never be called outside of this clone() method for memory safety reasons
  /// \return A clone of the node including clones of all of its children.
  [[nodiscard]] virtual AbstractNode *clone_impl(AbstractNode *parent_) const = 0;

 public:
  /// Virtual Destructor, force class to be abstract
  virtual ~AbstractNode() = 0;

  /// Clones a node recursively, i.e., by including all of its children.
  /// Because return-type covariance does not work with smart pointers,
  /// derived classes are expected to introduce a std::unique_ptr<DerivedNode> clone() method that hides this (for use with derived class ptrs/refs)
  /// \return A clone of the node including clones of all of its children.
  std::unique_ptr<AbstractNode> clone(AbstractNode *parent = nullptr) const;

  /// Compares two nodes for equality
  /// For efficiency, this currently considers only two exact same objects (i.e. same address) to be equal
  /// \param other
  /// \return true iff this and other are the exact same object
  bool operator==(const AbstractNode &other) const noexcept;

  /// Compares two nodes for equality
  /// For efficiency, this currently considers only two exact same objects (i.e. same address) to be equal
  /// \param other
  /// \return false iff this and other are the exact same object
  bool operator!=(const AbstractNode &other) const noexcept;

  /// Part of the visitor pattern.
  /// Must be overridden in derived classes and must call v.visit(node).
  /// This allows the correct overload for the derived class to be called in the visitor.
  /// \param v Visitor that offers a visit() method
  virtual void accept (IVisitor &v) = 0;

  /** @defgroup DAG Methods for handling parent/child relationship
   *  @{
   */
 public:

  /// Forward Iterator through Nodes
  typedef NodeIterator<AbstractNode> iterator;

  /// Const Forward Iterator through Nodes
  typedef NodeIterator<const AbstractNode> const_iterator;

  /// Forward Iterator marking begin of children
  virtual iterator begin() = 0;

  /// Forward Const Iterator marking begin of children
  virtual const_iterator begin() const = 0;

  /// Forward Iterator marking end of children
  virtual iterator end() = 0;

  /// Forward Const Iterator marking end of children
  virtual const_iterator end() const = 0;

  // Returns the number of (non-null) children nodes
  /// \return An integer indicating the number of children nodes.
  [[nodiscard]] virtual size_t countChildren() const = 0;

  /// Checks whether this node has a parent set
  /// \return True if this node has a parent, otherwise returns False.
  bool hasParent() const;

  /// Returns a reference to the only parent node.
  /// \return A reference to the node's parent node.
  /// \throws std::runtime_error if the node has no parent
  AbstractNode &getParent();

  /// Returns a pointer to the only parent node.
  /// \return A pointer to the node's parent node.
  AbstractNode* getParentPtr();

  /// Returns a pointer to the only parent node.
  /// \return A pointer to the node's parent node.
  /// \throws std::runtime_error if the node has no parent
  const AbstractNode &getParent() const;

  /// Set the parent of this node.
  /// Derived classes must ensure that a node's parent has the node as its child!
  /// This is also why this function does not take a const AbstractNode& parameter
  /// Because one should not call this on a node that one isn't also modifying
  /// by adding this node as a child
  /// \param newParent The new parent
  /// \throws std::logic_error if this node already has a parent
  void setParent(AbstractNode &newParent);

  /** @} */ // End of DAG group

  /** @defgroup output Methods for output
  *  @{
  */
 public:

  /// Get the nlohmann::json representation of the node including all of its children.
  /// \return nlohmann::json representation of the node
  [[nodiscard]] virtual nlohmann::json toJson() const = 0;

  /// Returns a string representation of the node,
  /// if printChildren is True then calls toString for its children too.
  /// Hence, toString(false) is equivalent to printing a node's attributes only.
  /// \return A textual representation of the node, including a newline at the end
  [[nodiscard]] virtual std::string toString(bool printChildren) const;

  /// Prints the output of node.toString(true) to the stream os
  /// \param os Stream to print to
  /// \param node Any node
  /// \return The written to stream
  friend std::ostream &operator<<(std::ostream &os, const AbstractNode &node);

 protected:
  /// Generates an output string to be used by the toString() method.
  /// \param printChildren Specifies whether to print details of this node only (False) or also its children (True).
  /// \param attributes The node's attributes to be printed (fields in the node's class).
  /// \return A string representation of the node.
  [[nodiscard]] std::string toStringHelper(bool printChildren, std::vector<std::string> attributes) const;

  /// Returns the node's type, which is the name of the object in the AST. This method must be overridden by all classes
  /// that inherit from AbstractNode by their respective name (e.g., ArithmeticExp, Function, Variable).
  /// \return The name of the node type.
  [[nodiscard]] virtual std::string getNodeType() const = 0;

  /** @} */ // End of output group

  /** @defgroup nodeID Methods to support unique node ids
  *  @{
  */

 private:
  /// Stores the reserved node ID until the first call of getUniqueNodeId() at which the reserved ID is
  /// fetched and the node's ID is assigned (field uniqueNodeId) based on the node's name and this reserved ID.
  /// This is a workaround because getNodeType() is a virtual method that cannot be called from derived classes'
  /// constructor.
  int assignedNodeId{-1};

  /// A static ongoing counter that is incremented after creating a new AbstractNode object. The counter's value is used
  /// to build the unique node ID.
  static int nodeIdCounter;

  /// An identifier that is unique among all nodes during runtime.
  /// Needs to be mutable, since it is constructed on-demand by the first call of getUniqueNode()
  mutable std::string uniqueNodeId;

  /// Generates a new node ID in the form "<NodeTypeName>_nodeIdCounter" where <NodeTypeName> is the value obtained by
  /// getNodeType() and nodeIdCounter an ongoing counter of created AbstractNode objects.
  /// \return An unique node ID to be used as uniqueNodeId for the current node.
  std::string generateUniqueNodeId() const;

  /// Returns the current ID (integer) and increments the ID by one. The ID is an ongoing counter of created
  /// AbstractNode objects and is used to build an AbstractNode's unique ID (see getUniqueNodeId()).
  /// \return The current ID as integer.
  static int getAndIncrementNodeId();

 protected:
  /// Default Constructor, defines some default behavior for subclasses related to IDs
  AbstractNode();

 public:
  /// Returns a node's unique ID, or generates it by calling generateUniqueNodeId() if the name was not defined yet.
  /// \return The node's name consisting of the node type and an ongoing number (e.g., Function_1).
  std::string getUniqueNodeId() const;

  /** @} */ // End of nodeID group
};

/// BaseIteratorImpl is an abstract class that simply specifies the functions required in the wrapper
template<typename T>
class BaseIteratorImpl;

/// Forward Iterator that redirects all calls to a (polymorphic) BaseIteratorImpl iterator
template<typename T>
class NodeIterator {
 private:
  /// Pointer to the actual iterator implementation, which depends on the derived class' data layout
  std::unique_ptr<BaseIteratorImpl<T>> impl;

 public:
  typedef std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef std::forward_iterator_tag iterator_category;

  /// A default constructed iterator is uninitialized and should not be used
  NodeIterator() = default;

  /// Create a NodeIterator Wrapper from an IteratorImpl, taking ownership of the Impl
  /// \param impl Pointer to an IteratorImpl
  explicit NodeIterator(std::unique_ptr<BaseIteratorImpl<T>> impl) : impl(std::move(impl)) {};

  /// Copy constructor
  NodeIterator(const NodeIterator &other) : impl(other.impl->clone()) {};

  /// Move constructor
  NodeIterator(NodeIterator &&other) noexcept: impl(std::move(other.impl)) {};

  /// Copy assignment
  NodeIterator &operator=(const NodeIterator &other) {
    impl = other.impl->clone(nullptr);
  }

  /// Move assignment
  NodeIterator &operator=(NodeIterator &&other) noexcept {
    impl = std::move(other.impl);
  }

  /// Pre-increment
  NodeIterator &operator++() {
    impl->increment();
    return *this;
  }

  /// Post-increment
  /// l-value ref-qualified because of
  /// https://stackoverflow.com/questions/52871026/overloaded-operator-returns-a-non-const-and-clang-tidy-complains
  NodeIterator<T> operator++(int) &{
    NodeIterator tmp(*this);
    impl->increment();
    return tmp;
  }

  /// Equality
  bool operator==(const NodeIterator &other) const {
    return impl->equal(*other.impl);
  }

  /// Inequality
  bool operator!=(const NodeIterator &other) const {
    return !(this->operator==(other));
  }

  /// Dereference
  T &operator*() {
    return **impl;
  }

  /// Arrow
  T* operator->() {
    return &**impl;
  }

};

/// BaseIteratorImpl is an abstract class that simply specifies the functions required in the wrapper
template<typename T>
class BaseIteratorImpl {
 public:
  virtual ~BaseIteratorImpl() = default;
  virtual const T &getNode() const = 0;
  virtual T &getNode() = 0;
  virtual std::unique_ptr<BaseIteratorImpl> clone() = 0;
  virtual void increment() = 0;
  virtual bool equal(const BaseIteratorImpl &other) = 0;
  virtual T &operator*() = 0;
};

/// EmptyIteratorImpl is a "dummy" iterator that can be used for node classes that do not have children
/// Any two EmptyIteratorImpl for the same node will always be equal, i.e begin() == end() if used for begin/end
template<typename T>
class EmptyIteratorImpl : public BaseIteratorImpl<T> {
 private:
  T &node;
 public:
  virtual ~EmptyIteratorImpl() = default;
  explicit EmptyIteratorImpl(T &node) : node(node) {};
  const T &getNode() const override {
    return node;
  };
  T &getNode() override {
    return node;
  }
  std::unique_ptr<BaseIteratorImpl<T>> clone() override {
    return std::make_unique<EmptyIteratorImpl<T>>(node);
  }
  void increment() override { /* do nothing */ };
  bool equal(const BaseIteratorImpl<T> &other) override {
    return getNode()==other.getNode();
  };
  T &operator*() override {
    throw std::logic_error("Cannot dereference dummy iterator EmptyIteratorImpl.");
  };
};

/// PositionIteratorImpl is a "default" iterator that can be used as a base class for iterators for node classes
/// that have a fixed number of children held as named member variables.
// Designed to be instantiated only with T = (const) AbstractNode
template<typename T, typename NodeType>
class PositionIteratorImpl : public BaseIteratorImpl<T> {
 protected:
  // Select const NodeType / NodeType depending on whether or not T is const/non-const
  typedef typename std::conditional<std::is_const<T>::value, const NodeType, NodeType>::type N;

  /// The node object that this iterator belongs to
  N &node;

  /// Indicates the current position
  size_t position = 0;

  T &getNode() override {
    return node;
  };
  const T &getNode() const override {
    return node;
  };

 public:
  virtual ~PositionIteratorImpl() = default;

  PositionIteratorImpl(N &node, size_t position) : node(node), position(position) {};

  void increment() override {
    ++position;
  }

  bool equal(const BaseIteratorImpl<T> &other) override {
    if (node==other.getNode()) {
      auto otherNodePtr = dynamic_cast<const PositionIteratorImpl *>(&other);
      assert(otherNodePtr); // If the other node has the same type, the Iterator must be the same type, too
      return (position==otherNodePtr->position);
    } else {
      return false;
    }
  }

  T &operator*() override  = 0;
  /* The dereference function needs to be overridden, similar to the example below:
    switch (position) {
      case 0:
        if (node.hasTarget())
          return node.getTarget();
        else if (node.hasValue())
          return node.getValue();
        else
          throw std::runtime_error("Cannot dereference iterator since node has no children.");
      case 1:
        if (node.hasTarget())
          if (node.hasValue())
            return node.getValue();
        // If there is no target, then position 1 is past end even if value exists
        // If there is target, but no value, we're also past end, so just continue into default
      default:
        // calling dereference on higher elements is an error
        throw std::runtime_error("Trying to dereference iterator past end.");
    }
  }
  */

  std::unique_ptr<BaseIteratorImpl<T>> clone() override = 0;
  // Deriving class needs to implement this, returning a new copy of themselves
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ABSTRACTNODE_H_
