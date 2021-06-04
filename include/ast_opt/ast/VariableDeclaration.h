#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_AST_VARIABLEDECLARATION_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_AST_VARIABLEDECLARATION_H_

#include <string>
#include "ast_opt/ast/AbstractExpression.h"
#include "ast_opt/ast/AbstractStatement.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/utilities/Datatype.h"


/// A Variable Declaration associates a Variable with a type (Datatype)
/// and (optionally) a value (AbstractExpression)
class VariableDeclaration : public AbstractStatement {
 private:
  /// Type of variable
  /// Note: Datatype is not a part of the Node hierarchy,
  /// therefore this is also not a "child"
  Datatype datatype;

  /// Variable that is being declared
  std::unique_ptr<Variable> target;

  /// (Optional) initializer value
  std::unique_ptr<AbstractExpression> value;

  /// Creates a deep copy of the current node
  /// Should be used only by Nodes' clone()
  /// \return a copy of the current node
  VariableDeclaration *clone_impl(AbstractNode* parent_) const override;
 public:
  /// Destructor
  ~VariableDeclaration() override;

  /// Create a declaration with for a variable (target) of type datatype with optional initialization to value
  /// \param datatype The datatype of the variable being declared
  /// \param target The variable to declare (the declaration will take ownership)s
  /// \param value  (Optional) The value to assign (the declaration will take ownership)
   VariableDeclaration(Datatype datatype,
                       std::unique_ptr<Variable> target,
                       std::unique_ptr<AbstractExpression> value = nullptr);

  /// Copy constructor
  /// \param other VariableDeclaration to copy
  VariableDeclaration(const VariableDeclaration &other);

  /// Move constructor
  /// \param other VariableDeclaration to copy
  VariableDeclaration(VariableDeclaration &&other) noexcept ;

  /// Copy assignment
  /// \param other VariableDeclaration to copy
  /// \return This object
  VariableDeclaration &operator=(const VariableDeclaration &other);

  /// Move assignment
  /// \param other VariableDeclaration to move
  /// \return This object
  VariableDeclaration &operator=(VariableDeclaration &&other)  noexcept;

  /// Deep copy of the current node
  /// \return A deep copy of the current node
  std::unique_ptr<VariableDeclaration> clone(AbstractNode* parent_ = nullptr) const;

  /// Does this declaration have its target set?
  /// \return true iff the assignment has the target set
  bool hasTarget() const;

  /// Does this declaration have its datatype set?
  /// Note: Currently always true
  bool hasDatatype() const;

  /// Does this declaration have its value set?
  /// \return true iff the assignment has the value set
  bool hasValue() const;

  /// Get (a reference to) the target (if it exists)
  /// \return A reference to the target variable
  /// \throws std::runtime_error if no target exists
  Variable &getTarget();

  /// Get (a const reference to) the target (if it exists)
  /// \return A reference to the target variable
  /// \throws std::runtime_error if no target exists
  const Variable &getTarget() const;

  /// Transfer ownership of Target
  /// \return A unique_ptr that owns the AST that was previously this.target
  std::unique_ptr<AbstractExpression> takeTarget();

  /// Get (a reference to) the datatype (if it exists)
  /// \return A reference to the target variable
  /// \throws std::runtime_error if no target exists
  Datatype & getDatatype();

  /// Get (a const reference to) the datatype (if it exists)
  /// \return A reference to the target variable
  /// \throws std::runtime_error if no target exists
  const Datatype & getDatatype() const;

  /// Get (a reference to) the value (if it exists)
  /// \return A reference to the value variable
  /// \throws std::runtime_error if no value exists
  AbstractExpression &getValue();

  /// Get (a const reference to) the value (if it exists)
  /// \return A reference to the value variable
  /// \throws std::runtime_error if no value exists
  const AbstractExpression &getValue() const;

  /// Transfer ownership of Value
  /// \return A unique_ptr that owns the AST that was previously this.value
  std::unique_ptr<AbstractExpression> takeValue();

  /// Set the target to newTarget, taking ownership of newTarget
  /// This will delete the previous target!
  /// \param newTarget new target to set
  void setTarget(std::unique_ptr<Variable> newTarget);

  /// Set the value to newValue, taking ownership of newValue
  /// This will delete the previous value!
  /// \param newValue new value to set
  void setValue(std::unique_ptr<AbstractExpression> newValue);

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
class VariableDeclarationIteratorImpl : public PositionIteratorImpl<T, VariableDeclaration> {
 public:
  // Inherit the constructor from the base class since it does everything we need
  using PositionIteratorImpl<T,VariableDeclaration>::PositionIteratorImpl;

  T &operator*() override {
    switch (this->position) {
      case 0:
        if (this->node.hasTarget())
          return this->node.getTarget();
        else if (this->node.hasValue())
          return this->node.getValue();
        else
          throw std::runtime_error("Cannot dereference iterator since node has no children.");
      case 1:
        if (this->node.hasTarget())
          if (this->node.hasValue())
            return this->node.getValue();
        // If there is no target, then position 1 is past end even if value exists
        // If there is target, but no value, we're also past end, so just continue into default
      default:
        // calling dereference on higher elements is an error
        throw std::runtime_error("Trying to dereference iterator past end.");
    }
  }

  std::unique_ptr<BaseIteratorImpl<T>> clone() override {
    return std::make_unique<VariableDeclarationIteratorImpl>(this->node,this->position);
  }
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_AST_VARIABLEDECLARATION_H_
