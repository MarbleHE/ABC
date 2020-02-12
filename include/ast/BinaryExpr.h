#ifndef AST_OPTIMIZER_BINARYEXPR_H
#define AST_OPTIMIZER_BINARYEXPR_H

#include "Operator.h"
#include "AbstractExpr.h"
#include "Literal.h"
#include "LiteralInt.h"
#include "LiteralBool.h"
#include "LiteralString.h"
#include <string>
#include <vector>

class BinaryExpr : public AbstractExpr {
 public:
  /// Represents an expression of the form "left op right", e.g., "2 + a" or "53 * 3".
  /// \param left is the left operand of the expression.
  /// \param op is the operator of the expression.
  /// \param right is the right operand of the expression.
  BinaryExpr(AbstractExpr* left, OpSymb::BinaryOp op, AbstractExpr* right);
  BinaryExpr();
  explicit BinaryExpr(OpSymb::BinaryOp op);

  template<typename T1, typename T2>
  BinaryExpr(T1 left, OpSymb::BinaryOp op, T2 right) {
    setAttributes(AbstractExpr::createParam(left), new Operator(op), AbstractExpr::createParam(right));
  }

  ~BinaryExpr() override;

  [[nodiscard]] json toJson() const override;

  [[nodiscard]] AbstractExpr* getLeft() const;

  [[nodiscard]] Operator* getOp() const;

  [[nodiscard]] AbstractExpr* getRight() const;

  void accept(IVisitor &v) override;

  [[nodiscard]] std::string getNodeName() const override;

  static void swapOperandsLeftAWithRightB(BinaryExpr* bexpA, BinaryExpr* bexpB);

  BinaryExpr* contains(BinaryExpr* bexpTemplate, AbstractExpr* excludedSubtree) override;

  bool contains(Variable* var) override;

  bool isEqual(AbstractExpr* other) override;

  Literal* evaluate(Ast &ast) override;

  int countByTemplate(AbstractExpr* abstractExpr) override;

  std::vector<std::string> getVariableIdentifiers() override;
  void setAttributes(AbstractExpr* leftOperand, Operator* operatore, AbstractExpr* rightOperand);

 private:
  int getMaxNumberChildren() override;
  bool supportsCircuitMode() override;
};

#endif //AST_OPTIMIZER_BINARYEXPR_H
