#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ExpressionList_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ExpressionList_H_

#include <string>
#include <vector>

#include "ast_opt/ast/AbstractExpression.h"
#include "ast_opt/utilities/Operator.h"

/// An ExpressionList is an order list of (potentially null) expressions.
/// However, more specifically, it's intended to be a rough equivalent to the {a,b,c} notation in C++
/// Note that this is NOT a target (lvalue) but instead an rvalue
/// So we could assign an ExpressionList into a Variable using a VariableAssignment
/// And this would turn the Variable into a vector-valued Variable
/// However, we cannot assign something to an ExpressionList
class ExpressionList : public AbstractExpression {

 private:
  /// Expressions
  std::vector<std::unique_ptr<AbstractExpression>> expressions;

  /// Creates a deep copy of the current node
  /// Should be used only by Nodes' clone()
  /// \return a copy of the current node
  ExpressionList *clone_impl(AbstractNode* parent_) const override;
 public:
  /// Destructor
  ~ExpressionList() override;

  /// Create a List of Expressions from a vector of expressions
  /// \param expressions Expressions for this expression (the ExpressionList will take ownership)
  ExpressionList(std::vector<std::unique_ptr<AbstractExpression>> &&expressions);

  /// Copy constructor
  /// \param other ExpressionList to copy
  ExpressionList(const ExpressionList &other);

  /// Move constructor
  /// \param other ExpressionList to copy
  ExpressionList(ExpressionList &&other) noexcept;

  /// Copy assignment
  /// \param other ExpressionList to copy
  /// \return This object
  ExpressionList &operator=(const ExpressionList &other);

  /// Move assignment
  /// \param other ExpressionList to move
  /// \return This object
  ExpressionList &operator=(ExpressionList &&other) noexcept;

  /// Deep copy of the current node
  /// \return A deep copy of the current node
  std::unique_ptr<ExpressionList> clone(AbstractNode* parent = nullptr) const;

  /// Does the current ExpressionList contain null expressions?
  bool hasNullExpressions();

  /// Get (a vector of references to) all (non-null) expressions
  /// Since std::vector cannot directly handle references,
  /// a wrapper is used, but this can be used exactly like
  /// std::vector<AbstractStatement&> could, if it were possible
  /// \return vector of (references to) all non-null expressions
  std::vector<std::reference_wrapper<AbstractExpression>> getExpressions();

  /// Get (a vector of references to) all (non-null) expressions
  /// Since std::vector cannot directly handle references,
  /// a wrapper is used, but this can be used exactly like
  /// std::vector<AbstractStatement&> could, if it were possible
  /// \return vector of (const references to) all non-null expressions
  std::vector<std::reference_wrapper<const AbstractExpression>> getExpressions() const;

  /// Get (a vector of ptrs to) all expressions
  /// \return vec of (pointers to) all expressions
  std::vector<std::unique_ptr<AbstractExpression>>& getExpressionPtrs();

  /// Take Ownership of all expressions, removing them from this ExpressionsList
  /// \return vector of all unique_ptrs to expressions
  std::vector<std::unique_ptr<AbstractExpression>> takeExpressions();

  /// Replace existing expression list with new one
  /// \param expression Expressions to be set, ExpressionList will take ownership
  void setExpressions(std::vector<std::unique_ptr<AbstractExpression>> new_expressions);

  /// Add an expression as the last expression
  /// \param expression Expression to be appended, ExpressionList will take ownership
  void appendExpression(std::unique_ptr<AbstractExpression> expression);

  /// Add an expression as the first expression
  /// \param expression Expression to be prepended, ExpressionList will take ownership
  void prependExpression(std::unique_ptr<AbstractExpression> expression);

  /// Removes any potential nullptrs from the expressions vector
  void removeNullExpressions();

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
// This essentially just wraps the vector iterator
// jumping over null positions on operator++
template<typename T>
class ExpressionListIteratorImpl : public BaseIteratorImpl<T> {
 private:
  // Select const ExpressionList / ExpressionList depending on whether or not T is const/non-const
  typedef typename std::conditional<std::is_const<T>::value, const ExpressionList, ExpressionList>::type N;

  // Select const iterator / iterator depending on whether or not T is const/non-const
  typedef typename std::conditional<std::is_const<T>::value,
                                    std::vector<std::unique_ptr<AbstractExpression>>::const_iterator,
                                    std::vector<std::unique_ptr<AbstractExpression>>::iterator>::type I;

  /// The node object that this iterator belongs to
  N &node;

  /// ExpressionList iterator
  I it;

  /// ExpressionList iterator indicating end - this is required to avoid dereferencing past the end in increment()
  I end;

 public:

  ExpressionListIteratorImpl(N &node, I it, I end) : node(node), it(it), end(end) {};

  const T &getNode() const override {
    return node;
  }

  T &getNode() override {
    return node;
  }

  void increment() override {
    // First, increment the underlying iterator normally
    ++it;
    // Then, keep advancing until we stop seeing nullptrs or hit the end
    while (it!=end && *it==nullptr) {
      ++it;
    }
  }

  bool equal(const BaseIteratorImpl<T> &other) override {
    if (node==other.getNode()) {
      auto otherNodePtr = dynamic_cast<const ExpressionListIteratorImpl *>(&other);
      assert(otherNodePtr); // If the other node has the same type, the Iterator must be the same type, too
      return (it==otherNodePtr->it);
    } else {
      return false;
    }
  }

  T &operator*() override {
    //simply forward to vector iterator and follow the unique_ptr
    return **it;
  }

  std::unique_ptr<BaseIteratorImpl<T>> clone() override {
    return std::make_unique<ExpressionListIteratorImpl>(node, it, end);
  }
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_AST_ExpressionList_H_
