#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_AST_OPERATOREXPRESSION_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_AST_OPERATOREXPRESSION_H_

#include <string>
#include <vector>

#include "ast_opt/ast/AbstractExpression.h"
#include "ast_opt/utilities/Operator.h"

/// A OperatorExpression has multiple Operands and an Operator
/// E.g. if x+y is a binary expression +(x,y) would be the operator expression
/// Other than a binary expression, it can extend to be n-ary
/// i.e. +(x,y,z) or +(a,b,c,d,e)
class OperatorExpression : public AbstractExpression {

 private:

  /// Operator (not part of node class hierarchy, i.e. not a child)
  Operator op;

  /// Operands
  std::vector<std::unique_ptr<AbstractExpression>> operands;

  /// Creates a deep copy of the current node
  /// Should be used only by Nodes' clone()
  /// \return a copy of the current node
  OperatorExpression *clone_impl(AbstractNode* parent_) const override;
 public:
  /// Destructor
  ~OperatorExpression() override;

  /// Create a OperatorExpression with operator and operands
  /// \param operator Operator for this expression
  /// \param operands Operands for this expression (the OperatorExpression will take ownership)
  OperatorExpression(Operator op, std::vector<std::unique_ptr<AbstractExpression>> &&operands);

  /// Copy constructor
  /// \param other OperatorExpression to copy
  OperatorExpression(const OperatorExpression &other);

  /// Move constructor
  /// \param other OperatorExpression to copy
  OperatorExpression(OperatorExpression &&other) noexcept;

  /// Copy assignment
  /// \param other OperatorExpression to copy
  /// \return This object
  OperatorExpression &operator=(const OperatorExpression &other);

  /// Move assignment
  /// \param other OperatorExpression to move
  /// \return This object
  OperatorExpression &operator=(OperatorExpression &&other) noexcept;

  /// Deep copy of the current node
  /// \return A deep copy of the current node
  std::unique_ptr<OperatorExpression> clone(AbstractNode* parent = nullptr) const;

  /// Does the current OperatorExpression contain null operands?
  bool hasNullOperands();

  /// Get (a reference to) the Operator
  /// \return A reference to the operator variable
  Operator &getOperator();

  /// Get (a const reference to) the Operator
  /// \return A const reference to the operator variable
  const Operator &getOperator() const;

  /// Get (a vector of references to) all (non-null) operands
  /// Since std::vector cannot directly handle references,
  /// a wrapper is used, but this can be used exactly like
  /// std::vector<AbstractStatement&> could, if it were possible
  /// \return Vector of (references to) all non-null operands
  std::vector<std::reference_wrapper<AbstractExpression>> getOperands();

  /// Get (a vector of references to) all (non-null) operands
  /// Since std::vector cannot directly handle references,
  /// a wrapper is used, but this can be used exactly like
  /// std::vector<AbstractStatement&> could, if it were possible
  /// \return Vector of (const references to) all non-null operands
  std::vector<std::reference_wrapper<const AbstractExpression>> getOperands() const;

  /// Set the operator to newOperator
  /// \param newOperator new operator to set
  void setOperator(Operator newOperator);

  /// Add an operand as the last operand
  /// \param operand Operand to be appended, OperatorExpression will take ownership
  void appendOperand(std::unique_ptr<AbstractExpression> operand);

  /// Add an operand as the first operand
  /// \param operand Operand to be prepended, OperatorExpression will take ownership
  void prependOperand(std::unique_ptr<AbstractExpression> operand);

  /// Removes any potential nullptrs from the operands vector
  void removeNullOperands();

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
class OperatorExpressionIteratorImpl : public BaseIteratorImpl<T> {
 private:
  // Select const OperatorExpression / OperatorExpression depending on whether or not T is const/non-const
  typedef typename std::conditional<std::is_const<T>::value, const OperatorExpression, OperatorExpression>::type N;

  // Select const iterator / iterator depending on whether or not T is const/non-const
  typedef typename std::conditional<std::is_const<T>::value,
                                    std::vector<std::unique_ptr<AbstractExpression>>::const_iterator,
                                    std::vector<std::unique_ptr<AbstractExpression>>::iterator>::type I;

  /// The node object that this iterator belongs to
  N &node;

  /// Vector iterator
  I it;

  /// Vector iterator indicating end - this is required to avoid dereferencing past the end in increment()
  I end;

 public:

  OperatorExpressionIteratorImpl(N &node, I it, I end) : node(node), it(it), end(end) {};

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
      auto otherNodePtr = dynamic_cast<const OperatorExpressionIteratorImpl *>(&other);
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
    return std::make_unique<OperatorExpressionIteratorImpl>(node, it, end);
  }
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_AST_OPERATOREXPRESSION_H_
