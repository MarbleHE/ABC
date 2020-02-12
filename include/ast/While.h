#ifndef AST_OPTIMIZER_WHILE_H
#define AST_OPTIMIZER_WHILE_H

#include "AbstractStatement.h"
#include "AbstractExpr.h"
#include <string>

class While : public AbstractStatement {
 private:
  AbstractExpr* condition;
  AbstractStatement* body;

 public:
  While(AbstractExpr* condition, AbstractStatement* body);

  ~While() override;

  [[nodiscard]] json toJson() const override;

  void accept(IVisitor &v) override;

  [[nodiscard]] AbstractExpr* getCondition() const;

  [[nodiscard]] AbstractStatement* getBody() const;

  [[nodiscard]] std::string getNodeName() const override;

  Literal* evaluate(Ast &ast) override;
};

#endif //AST_OPTIMIZER_WHILE_H
