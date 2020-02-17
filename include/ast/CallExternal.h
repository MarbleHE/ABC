#ifndef AST_OPTIMIZER_INCLUDE_CALLEXTERNAL_H
#define AST_OPTIMIZER_INCLUDE_CALLEXTERNAL_H

#include "AbstractStatement.h"
#include <vector>
#include <string>

class CallExternal : public AbstractExpr, public AbstractStatement {
private:
    std::string functionName;
    std::vector<FunctionParameter *> arguments;

    Node *createClonedNode(bool keepOriginalUniqueNodeId) override;

public:
    explicit CallExternal(std::string functionName);

    CallExternal(std::string functionName, std::vector<FunctionParameter *> arguments);

    [[nodiscard]] json toJson() const override;

    void accept(Visitor &v) override;

    [[nodiscard]] const std::string &getFunctionName() const;

    [[nodiscard]] const std::vector<FunctionParameter *> &getArguments() const;

    [[nodiscard]] std::string getNodeName() const override;

    std::vector<Literal *> evaluate(Ast &ast) override;
};

#endif //AST_OPTIMIZER_INCLUDE_CALLEXTERNAL_H
