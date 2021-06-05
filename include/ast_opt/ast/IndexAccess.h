#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_AST_INDEXACCESS_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_AST_INDEXACCESS_H_

#include <map>
#include <string>
#include <vector>
#include "ast_opt/ast/AbstractTarget.h"

/// An IndexAccess represents the [] operator in C++
/// It has a target (e.g. a Variable or anther IndexAccess) and an index value
class IndexAccess : public AbstractTarget {
 private:
  /// Value (expression) of which the index is being accessed
  std::unique_ptr<AbstractTarget> target;

  /// Expression defining the index
  std::unique_ptr<AbstractExpression> index;

  /// Creates a deep copy of the current node
  /// Should be used only by Nodes' clone()
  /// \return a copy of the current node
  IndexAccess *clone_impl(AbstractNode* parent) const override;

 public:
  /// Destructor
  ~IndexAccess() override;

  /// Create an IndexAccess that accesses index of target
  /// \param target The target
  /// \param index The value that resolves to the index
  IndexAccess(std::unique_ptr<AbstractTarget> &&target, std::unique_ptr<AbstractExpression> &&index);

  /// Copy constructor
  /// \param other IndexAccess to copy
  IndexAccess(const IndexAccess &other);

  /// Move constructor
  /// \param other IndexAccess to copy
  IndexAccess(IndexAccess &&other) noexcept;

  /// Copy assignment
  /// \param other IndexAccess to copy
  /// \return This object
  IndexAccess &operator=(const IndexAccess &other);

  /// Move assignment
  /// \param other IndexAccess to move
  /// \return This object
  IndexAccess &operator=(IndexAccess &&other) noexcept;

  /// Deep copy of the current node
  /// \return A deep copy of the current node
  std::unique_ptr<IndexAccess> clone(AbstractNode* parent = nullptr) const;

  /// Checks if the target is set
  /// \return true iff the target is set
  bool hasTarget() const;

  /// Checks if the index is set
  /// \return true iff the index is set
  bool hasIndex() const;

  /// Get the target, if it exists
  /// \return (a const reference to) the target
  /// \throws std::runtime_error if no target exists
  const AbstractTarget& getTarget() const;


  /// Get the target, if it exists
  /// \return (a reference to) the target
  /// \throws std::runtime_error if no target exists
  AbstractTarget& getTarget();

  /// Take ownership of the target
  /// \return the target
  std::unique_ptr<AbstractTarget> takeTarget();

  /// Get the index, if it exists
  /// \return (a const reference to) the index
  /// \throws std::runtime_error if no index exists
  const AbstractExpression& getIndex() const;


  /// Get the index, if it exists
  /// \return (a reference to) the index
  /// \throws std::runtime_error if no index exists
  AbstractExpression& getIndex();

  /// Set a new target
  /// \param newTarget condition to set, IndexAccess takes ownership
  void setTarget(std::unique_ptr<AbstractTarget> &&newTarget);

  /// Set a new index
  /// \param newIndex condition to set, IndexAccess takes ownership
  void setIndex(std::unique_ptr<AbstractExpression> &&newIndex);


  ///////////////////////////////////////////////
  ////////// AbstractNode Interface /////////////
  ///////////////////////////////////////////////
  void accept(IVisitor &v) override;
  iterator begin() override;
  const_iterator begin() const override;
  iterator end() override;
  const_iterator end() const override;
  size_t countChildren() const override;
  nlohmann::json toJson() const override;
  std::string toString(bool printChildren) const override;
 protected:
  std::string getNodeType() const override;
};

// Designed to be instantiated only with T = (const) AbstractNode
template<typename T>
class IndexAccessIteratorImpl : public PositionIteratorImpl<T, IndexAccess> {
 public:
  // Inherit the constructor from the base class since it does everything we need
  using PositionIteratorImpl<T, IndexAccess>::PositionIteratorImpl;

  T &operator*() override {
    switch (this->position) {
      case 0:
        if (this->node.hasTarget())
          return this->node.getTarget();
        else if (this->node.hasIndex())
          return this->node.getIndex();
        else
          throw std::runtime_error("Cannot dereference iterator since node has no children.");
      case 1:
        if (this->node.hasTarget()) {
          if (this->node.hasIndex())
            return this->node.getIndex();
          // Else: continue to default
        }
      default:
        // calling dereference on higher elements is an error
        throw std::runtime_error("Trying to dereference iterator past end.");
    }
  }

  std::unique_ptr<BaseIteratorImpl<T>>
  clone()
  override {
    return std::make_unique<IndexAccessIteratorImpl>(this->node, this->position);
  }
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_AST_INDEXACCESS_H_
