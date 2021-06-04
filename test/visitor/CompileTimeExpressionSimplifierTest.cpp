#include "AstTestingGenerator.h"
#include "ast_opt/ast/AbstractLiteral.h"
#include "ast_opt/ast/ArithmeticExpr.h"
#include "ast_opt/ast/Ast.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/Call.h"
#include "ast_opt/ast/For.h"
#include "ast_opt/ast/Function.h"
#include "ast_opt/ast/GetMatrixSize.h"
#include "ast_opt/ast/If.h"
#include "ast_opt/ast/LiteralFloat.h"
#include "ast_opt/ast/LiteralInt.h"
#include "ast_opt/ast/Matrix.h"
#include "ast_opt/ast/MatrixAssignm.h"
#include "ast_opt/ast/OpSymbEnum.h"
#include "ast_opt/ast/OperatorExpr.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/ast/UnaryExpr.h"
#include "ast_opt/ast/VarAssignm.h"
#include "ast_opt/ast/VarDecl.h"
#include "ast_opt/ast/While.h"
#include "ast_opt/visitor/CompileTimeExpressionSimplifier.h"
#include "ast_opt/visitor/PrintVisitor.h"
#include "gtest/gtest.h"

class CompileTimeExpressionSimplifierFixture : public ::testing::Test {
 protected:
  Ast ast;
  CompileTimeExpressionSimplifier ctes;

  CompileTimeExpressionSimplifierFixture() = default;

  AbstractExpression *getVariableValueByUniqueName(const std::string &varIdentifier) {
    // NOTE: This method does not work if there are multiple variables with the same variable identifier but in
    // different scopes. In that case the method always returns the value of the variable in the outermost scope.
    for (auto &[scopedVariable, varValue] : ctes.variableValues.getMap()) {
      if (scopedVariable.getIdentifier()==varIdentifier) {
        return varValue.getValue();
      }
    }
    throw std::logic_error("Variable identifier '" + varIdentifier + "' not found!");
  }
};

TEST_F(CompileTimeExpressionSimplifierFixture, arithmeticExpr_literalsOnly_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //  plaintext_int alpha = 22 * 11;
  // }
  auto function = new Function("compute");
  auto arithmeticExpr = new ArithmeticExpr(
      new LiteralInt(22),
      ArithmeticOp::MULTIPLICATION,
      new LiteralInt(11));
  auto varAssignm = new VarDecl("alpha",
                                new Datatype(Types::INT, false),
                                arithmeticExpr);

  // connect objects
  function->addStatement(varAssignm);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralInt>()->getValue(), 242);
  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, arithmeticExpr_variableUnknown_rhsOperandEvaluableOnly) { /* NOLINT */
  // void compute(encrypted_int encryptedA) {
  //  plaintext_int alpha = encryptedA * (4*7);
  // }
  auto function = new Function("compute");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(new Datatype(Types::INT, true),
                             new Variable("encryptedA"))});
  auto arithmeticExpr = new ArithmeticExpr(
      new Variable("encryptedA"),
      ArithmeticOp::MULTIPLICATION,
      new ArithmeticExpr(new LiteralInt(4),
                         ArithmeticOp::MULTIPLICATION,
                         new LiteralInt(7)));
  auto varAssignm = new VarDecl("alpha", new Datatype(Types::INT, false), arithmeticExpr);

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varAssignm);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // variableValues is expected to contain: [encryptedA, alpha][
  EXPECT_EQ(ctes.variableValues.getMap().size(), 2);
  // check that the rhs operand of arithmeticExpr is simplified
  auto expected = new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
                                   {new Variable("encryptedA"),
                                    new LiteralInt(28)});
  EXPECT_TRUE(getVariableValueByUniqueName("alpha")->isEqual(expected));
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, arithmeticExpr_variableKnown_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //   plaintext_int parameterA = 43;
  //   plaintext_int alpha = parameterA * (4*7);
  // }
  auto function = new Function("compute");
  auto varDeclParameterA = new VarDecl("parameterA", 43);
  auto arithmeticExpr = new ArithmeticExpr(
      new Variable("parameterA"),
      ArithmeticOp::MULTIPLICATION,
      new ArithmeticExpr(new LiteralInt(4),
                         ArithmeticOp::MULTIPLICATION,
                         new LiteralInt(7)));
  auto varDeclAlpha = new VarDecl("alpha",
                                  new Datatype(Types::INT, false),
                                  arithmeticExpr);

  // connect objects
  function->addStatement(varDeclParameterA);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralInt>()->getValue(), 1'204);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, arithmeticExpr_variablesUnknown_notAnythingEvaluable) { /* NOLINT */
  // void compute(encrypted_int encryptedA, plaintext_int plaintextB) {
  //  plaintext_int alpha = encryptedA * (4*plaintextB);
  // }
  auto function = new Function("compute");
  auto functionParameters =
      std::vector<FunctionParameter *>(
          {
              new FunctionParameter(new Datatype(Types::INT, true),
                                    new Variable("encryptedA")),
              new FunctionParameter(new Datatype(Types::INT, false),
                                    new Variable("plaintextB"))});
  function->setParameterList(new ParameterList(functionParameters));

  auto arithmeticExpr = new ArithmeticExpr(
      new Variable("encryptedA"),
      ArithmeticOp::MULTIPLICATION,
      new ArithmeticExpr(new LiteralInt(4),
                         ArithmeticOp::MULTIPLICATION,
                         new Variable("plaintextB")));
  auto varDeclAlpha = new VarDecl("alpha",
                                  new Datatype(Types::INT, false),
                                  arithmeticExpr);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);
  auto numberOfNodesBeforeSimplification = ast.getAllNodes().size();

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto expected = new OperatorExpr(
      new Operator(ArithmeticOp::MULTIPLICATION),
      {
          new Variable("encryptedA"),
          new Variable("plaintextB"),
          new LiteralInt(4)
      });
  EXPECT_TRUE(getVariableValueByUniqueName("alpha")->isEqual(expected));

  // check that 10 nodes were deleted and the function's body is empty
  EXPECT_EQ(numberOfNodesBeforeSimplification - 10, ast.getAllNodes().size());
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, logicalExpr_literalsOnly_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //  plaintext_bool alpha = true && false;
  // }
  auto function = new Function("compute");
  auto logicalExpr = new LogicalExpr(
      new LiteralBool(true),
      LogCompOp::LOGICAL_AND,
      new LiteralBool(false));
  auto varAssignm = new VarDecl("alpha",
                                new Datatype(Types::BOOL, false),
                                logicalExpr);

  // connect objects
  function->addStatement(varAssignm);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralBool>()->getValue(), false);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, logicalExpr_variableUnknown_lhsOperandEvaluableOnly) { /* NOLINT */
  //  -- input --
  // void compute(encrypted_bool encryptedA) {
  //  plaintext_bool alpha = (true ^ false) || encryptedA;
  // }
  //  -- expected --
  // variableValues['alpha'] = true
  auto function = new Function("compute");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(new Datatype(Types::BOOL, true),
                             new Variable("encryptedA"))});
  auto logicalExpr = new LogicalExpr(
      new LogicalExpr(new LiteralBool(true),
                      LogCompOp::LOGICAL_XOR,
                      new LiteralBool(false)),
      LogCompOp::LOGICAL_OR,
      new Variable("encryptedA"));
  auto varAssignm = new VarDecl("alpha",
                                new Datatype(Types::BOOL, false),
                                logicalExpr);

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varAssignm);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that alpha's lhs operand is computed
  auto expected = new LiteralBool(true);
  EXPECT_TRUE(getVariableValueByUniqueName("alpha")->isEqual(expected));
  // check that the variable declaration statement is deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, logicalExpr_variableKnown_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //   plaintext_bool parameterA = true;
  //   plaintext_bool alpha = parameterA || (false && true);
  // }
  auto function = new Function("compute");
  auto varDeclParameterA = new VarDecl("parameterA", true);
  auto logicalExpr = new LogicalExpr(
      new Variable("parameterA"),
      LogCompOp::LOGICAL_OR,
      new LogicalExpr(new LiteralBool(false),
                      LogCompOp::LOGICAL_AND,
                      new LiteralBool(true)));
  auto varDeclAlpha = new VarDecl("alpha",
                                  new Datatype(Types::BOOL, false),
                                  logicalExpr);

  // connect objects
  function->addStatement(varDeclParameterA);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralBool>()->getValue(), true);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, logicalExpr_variablesUnknown_notAnythingEvaluable) { /* NOLINT */
  //  -- input --
  // void compute(encrypted_bool encryptedA, plaintext_bool paramB) {
  //  plaintext_bool alpha = encryptedA && (true ^ encryptedB);
  // }
  //  -- expected --
  // variableValues['alpha'] = encryptedA && (true ^ encryptedB)
  auto function = new Function("compute");
  auto functionParameters = std::vector<FunctionParameter *>(
      {new FunctionParameter(new Datatype(Types::BOOL, true),
                             new Variable("encryptedA")),
       new FunctionParameter(new Datatype(Types::BOOL, false),
                             new Variable("paramB"))});
  function->setParameterList(new ParameterList(functionParameters));

  auto logicalExpr = new LogicalExpr(
      new Variable("encryptedA"),
      LogCompOp::LOGICAL_AND,
      new LogicalExpr(new LiteralBool(true),
                      LogCompOp::LOGICAL_XOR,
                      new Variable("encryptedB")));
  auto varDeclAlpha = new VarDecl("alpha",
                                  new Datatype(Types::BOOL, false),
                                  logicalExpr);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);
  auto numberOfNodesBeforeSimplification = ast.getAllNodes().size();

  // perform the compile-time expression simplification
  // Should fail due to unknown variable
  EXPECT_THROW(ctes.visit(ast), std::invalid_argument);

}

TEST_F(CompileTimeExpressionSimplifierFixture, unaryExpr_literalsOnly_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //  plaintext_bool truthValue = !false;
  // }
  auto function = new Function("compute");
  auto unaryExpr = new UnaryExpr(UnaryOp::NEGATION, new LiteralBool(false));
  auto varAssignm = new VarDecl("truthValue",
                                new Datatype(Types::BOOL, false),
                                unaryExpr);

  // connect objects
  function->addStatement(varAssignm);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'truthValue' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("truthValue");
  EXPECT_EQ(alphaValue->castTo<LiteralBool>()->getValue(), true);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, unaryExpr_variableKnown_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //  plaintext_bool alpha = true;
  //  plaintext_bool beta = !alpha;
  // }
  auto function = new Function("compute");
  auto varDeclParameterA = new VarDecl("alpha", true);
  auto unaryExpr = new UnaryExpr(UnaryOp::NEGATION, new Variable("alpha"));
  auto varDeclAlpha = new VarDecl("beta",
                                  new Datatype(Types::BOOL, false),
                                  unaryExpr);

  // connect objects
  function->addStatement(varDeclParameterA);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is stored correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralBool>()->getValue(), true);

  // check that 'beta' is computed correctly
  auto betaValue = getVariableValueByUniqueName("beta");
  EXPECT_EQ(betaValue->castTo<LiteralBool>()->getValue(), false);

  // check that both statements and their children are deleted
  EXPECT_EQ(function->getBody(), nullptr);
}

TEST_F(CompileTimeExpressionSimplifierFixture, unaryExpr_variableUnknown_notEvaluable) { /* NOLINT */
  // void compute(plaintext_bool paramA) {
  //  plaintext_bool beta = !paramA;
  // }
  auto function = new Function("compute");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(new Datatype(Types::BOOL, false),
                             new Variable("paramA"))});
  auto unaryExpr = new UnaryExpr(UnaryOp::NEGATION, new Variable("paramA"));
  auto varDeclAlpha = new VarDecl("beta",
                                  new Datatype(Types::BOOL, false),
                                  unaryExpr);

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varDeclAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // variableValues is expected to contain: [paramA, beta]
  EXPECT_EQ(ctes.variableValues.getMap().size(), 2);
  EXPECT_TRUE(getVariableValueByUniqueName("beta")
                  ->isEqual(new OperatorExpr(new Operator(UnaryOp::NEGATION), {new Variable("paramA")})));
  // check that statements is deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, varAssignm_variablesKnown_fullyEvaluable) { /* NOLINT */
  // void compute() {
  //  plaintext_float alpha = 1.23;
  //  alpha = 2.75;
  //  plaintext_float beta = alpha;
  // }
  auto function = new Function("compute");
  auto varDeclAlpha = new VarDecl("alpha", 1.23f);
  auto varAssignmAlpha = new VarAssignm("alpha", new LiteralFloat(2.75));
  auto varDeclBeta = new VarDecl("beta",
                                 new Datatype(Types::FLOAT), new Variable("alpha"));

  // connect objects
  function->addStatement(varDeclAlpha);
  function->addStatement(varAssignmAlpha);
  function->addStatement(varDeclBeta);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is stored correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralFloat>()->getValue(), 2.75);

  // check that 'beta' is assigned correctly
  auto betaValue = getVariableValueByUniqueName("beta");
  EXPECT_EQ(betaValue->castTo<LiteralFloat>()->getValue(), 2.75);

  // check that the statements and their children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, varAssignm_previouslyDeclaredNonInitializedVariable) { /* NOLINT */
  // void compute() {
  //  plaintext_float alpha;
  //  alpha = 2.95;
  // }
  auto function = new Function("compute");
  auto varDeclAlpha = new VarDecl("alpha", Types::FLOAT, nullptr);
  auto varAssignmAlpha = new VarAssignm("alpha", new LiteralFloat(2.95));

  // connect objects
  function->addStatement(varDeclAlpha);
  function->addStatement(varAssignmAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralFloat>()->getValue(), 2.95f);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, varAssignm_assignmentToParameter) { /* NOLINT */
  // void compute(plaintext_float alpha) {
  //  alpha = 42.24;
  // }
  auto function = new Function("compute");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(new Datatype(Types::FLOAT, false),
                             new Variable("alpha"))});
  auto varAssignmAlpha = new VarAssignm("alpha", new LiteralFloat(42.24));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varAssignmAlpha);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that 'alpha' is computed correctly
  auto alphaValue = getVariableValueByUniqueName("alpha");
  EXPECT_EQ(alphaValue->castTo<LiteralFloat>()->getValue(), 42.24f);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody(), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture,  /* NOLINT */
       varAssignm_symbolicTerms_circularDependency) {
  //  -- input --
  // int Foo(plaintext_int x, plaintext_int y) {
  //  x = y+3
  //  y = x+2
  //  return x+y
  // }
  //  -- expected --
  // int Foo(plaintext_int x, plaintext_int y) {
  //  return y+y+8
  // }
  auto function = new Function("Foo");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(
          new Datatype(Types::INT, false), new Variable("x")),
       new FunctionParameter(
           new Datatype(Types::INT, false), new Variable("y"))});
  auto varAssignmX = new VarAssignm("x",
                                    new ArithmeticExpr(
                                        new Variable("y"),
                                        ArithmeticOp::ADDITION,
                                        3));
  auto varAssignmY = new VarAssignm("y",
                                    new ArithmeticExpr(
                                        new Variable("x"),
                                        ArithmeticOp::ADDITION,
                                        2));
  auto returnStmt = new Return(
      new ArithmeticExpr(
          new Variable("x"),
          ArithmeticOp::ADDITION,
          new Variable("y")));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varAssignmX);
  function->addStatement(varAssignmY);
  function->addStatement(returnStmt);
  ast.setRootNode(function);
  auto originalNumberOfNodes = ast.getAllNodes().size();

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that the number of nodes decreased
  EXPECT_TRUE(ast.getAllNodes().size() < originalNumberOfNodes);

  // check that simplification generated the expected simplified AST
  auto expectedAst = new OperatorExpr(
      new Operator(ADDITION),
      {new Variable("y"), new Variable("y"), new LiteralInt(8)});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedAst));

}

TEST_F(CompileTimeExpressionSimplifierFixture, return_literalOnly_expectedNoChange) { /* NOLINT */
  // float compute() {
  //  return 42.24;
  // }
  // -- expected –-
  // no change as cannot be simplified further
  auto function = new Function("compute");
  auto returnStatement = new Return(new LiteralFloat(42.24f));

  // connect objects
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that none of the nodes are deleted
  auto numberOfNodesBeforeSimplification = ast.getAllNodes().size();
  EXPECT_EQ(numberOfNodesBeforeSimplification, ast.getAllNodes().size());

}

TEST_F(CompileTimeExpressionSimplifierFixture,  /* NOLINT */
       return_variableKnown_expectedSubstitutionAndStatementDeletion) {
  // int compute() {
  //  int b = 23;
  //  return b;
  // }
  // -- expected --
  // int compute() {
  //  return 23;
  // }
  auto function = new Function("compute");
  auto varDeclB = new VarDecl("b", 23);
  auto returnStatement = new Return(new Variable("b"));

  // connect objects
  function->addStatement(varDeclB);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody()->getStatements().size(), 1);
  ASSERT_EQ(function->getBody()->getStatements().front(), returnStatement);

  // check that the variable 'b' in the Return statement was replaced by b's value
  auto firstReturnExpr = returnStatement->getReturnExpressions().front();
  auto newLiteralIntNode = dynamic_cast<LiteralInt *>(firstReturnExpr);
  ASSERT_NE(newLiteralIntNode, nullptr);
  EXPECT_EQ(newLiteralIntNode->getValue(), 23);

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       return_variableAndArithmeticExpressionKnown_expectedLiteralIntReturnValue) {
  // int compute() {
  //  int b = 23;
  //  return b + 99;
  // }
  // -- expected --
  // int compute() {
  //  return 122;
  // }
  auto function = new Function("compute");
  auto varDeclB = new VarDecl("b", 23);
  auto returnStatement = new Return(
      new ArithmeticExpr(new Variable("b"),
                         ArithmeticOp::ADDITION,
                         new LiteralInt(99)));

  // connect objects
  function->addStatement(varDeclB);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that the statement VarDecl and its children are deleted
  EXPECT_EQ(function->getBody()->getStatements().size(), 1);
  ASSERT_EQ(function->getBody()->getStatements().front(), returnStatement);

  // check that the expression b+99 was simplified by its value
  auto firstReturnExpr = returnStatement->getReturnExpressions().front();
  auto newLiteralIntNode = dynamic_cast<LiteralInt *>(firstReturnExpr);
  ASSERT_NE(newLiteralIntNode, nullptr);
  EXPECT_EQ(newLiteralIntNode->getValue(), 122);

}

TEST_F(CompileTimeExpressionSimplifierFixture, return_variableUnknown_expectedNoChange) { /* NOLINT */
  // int compute(plaintext_int b) {
  //  return b + 99;
  // }
  // -- expected --
  // no change
  auto function = new Function("compute");
  auto returnStatement = new Return(
      new ArithmeticExpr(new Variable("b"),
                         ArithmeticOp::ADDITION,
                         new LiteralInt(99)));

  // connect objects
  function->addStatement(returnStatement);
  function->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("b")));
  ast.setRootNode(function);
  auto numberOfNodesBeforeSimplification = ast.getAllNodes().size();

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that none of the nodes are deleted
  EXPECT_EQ(numberOfNodesBeforeSimplification, ast.getAllNodes().size());

  // check that 'b' remains unknown
  EXPECT_EQ(getVariableValueByUniqueName("b"), nullptr);

}

TEST_F(CompileTimeExpressionSimplifierFixture, return_multipleReturnValues_expectedPartlyEvaluation) { /* NOLINT */
  // int,int,int compute(plaintext_int a) {
  //  int b = 3 + 4;
  //  return a*b, 2-b, 21;
  // }
  // -- expected --
  // int,int,int compute(plaintext_int a) {
  //  return a*7, -5, 21;
  // }
  auto function = new Function("compute");
  auto functionParam = new ParameterList(
      {new FunctionParameter(new Datatype(Types::INT), new Variable("a"))});
  auto varDecl = new VarDecl("b",
                             new Datatype(Types::INT),
                             new ArithmeticExpr(
                                 new LiteralInt(3),
                                 ArithmeticOp::ADDITION,
                                 new LiteralInt(4)));
  auto returnStatement =
      new Return({
                     new ArithmeticExpr(
                         new Variable("a"),
                         ArithmeticOp::MULTIPLICATION,
                         new Variable("b")),
                     new ArithmeticExpr(
                         new LiteralInt(2),
                         ArithmeticOp::SUBTRACTION,
                         new Variable("b")),
                     new LiteralInt(21)});

  // connect objects
  function->setParameterList(functionParam);
  function->addStatement(varDecl);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that the Return statement has three return values
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 3);

  // check return expression 1: a*7
  EXPECT_TRUE(returnStatement->getReturnExpressions().at(0)
                  ->isEqual(new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
                                             {new Variable("a"), new LiteralInt(7)})));
  // check return expression 2: -5
  EXPECT_TRUE(returnStatement->getReturnExpressions().at(1)->isEqual(new LiteralInt(-5)));
  // check return expression 3: 21
  EXPECT_TRUE(returnStatement->getReturnExpressions().at(2)->isEqual(new LiteralInt(21)));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsKnown_thenIsAlwaysExecutedNoElseIsPresent_expectedIfRemoval) {
  //  -- input --
  //  int compute() {
  //    int a = 512;
  //    int b = 22;
  //    if (b+12 > 20) {
  //      a = a*2;
  //    }
  //    return a*32;
  //  }
  //  -- expected --
  //  int compute() {
  //    return 32'768;
  //  }
  auto function = new Function("compute");
  auto varDeclA = new VarDecl("a",
                              new Datatype(Types::INT),
                              new LiteralInt(512));
  auto varDeclB = new VarDecl("b",
                              new Datatype(Types::INT),
                              new LiteralInt(22));
  auto ifStmt = new If(
      new LogicalExpr(
          new ArithmeticExpr(
              new Variable("b"),
              ArithmeticOp::ADDITION,
              new LiteralInt(12)),
          LogCompOp::GREATER,
          new LiteralInt(20)),
      new VarAssignm("a", new ArithmeticExpr(
          new Variable("a"),
          ArithmeticOp::MULTIPLICATION,
          new LiteralInt(2))));
  auto returnStatement =
      new Return(new ArithmeticExpr(new Variable("a"),
                                    ArithmeticOp::MULTIPLICATION,
                                    new LiteralInt(32)));

  // connect objects
  function->addStatement(varDeclA);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(new LiteralInt(32'768)));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsKnown_thenIsAlwaysExecutedAndElseIsPresent_expectedIfRemoval) {
  //  -- input --
  //  int compute() {
  //    int a = 512;
  //    int b = 22;
  //    if (b+12 > 20) {
  //      a = a*2;
  //    } else {
  //      a = 1;
  //    }
  //    return a*32;
  //  }
  //  -- expected --
  //  int compute() {
  //    return 32'768;
  //  }
  auto function = new Function("compute");
  auto varDeclA = new VarDecl("a",
                              new Datatype(Types::INT),
                              new LiteralInt(512));
  auto varDeclB = new VarDecl("b",
                              new Datatype(Types::INT),
                              new LiteralInt(22));

  auto condition = new LogicalExpr(
      new ArithmeticExpr(new Variable("b"), ArithmeticOp::ADDITION, new LiteralInt(12)),
      LogCompOp::GREATER,
      new LiteralInt(20));
  auto thenBranch = new VarAssignm("a", new ArithmeticExpr(
      new Variable("a"),
      ArithmeticOp::MULTIPLICATION,
      new LiteralInt(2)));
  auto elseBranch = new VarAssignm("a", new LiteralInt(1));
  auto ifStmt = new If(condition, thenBranch, elseBranch);

  auto returnStatement =
      new Return(new ArithmeticExpr(new Variable("a"),
                                    ArithmeticOp::MULTIPLICATION,
                                    new LiteralInt(32)));

  // connect objects
  function->addStatement(varDeclA);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  EXPECT_EQ(function->getBodyStatements().front(), returnStatement);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(new LiteralInt(32'768)));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsKnown_elseIsAlwaysExecuted_expectedIfRemoval) {
  //  -- input --
  //  int compute() {
  //    int a = 512;
  //    int b = 22;
  //    if (b+12 < 20) {
  //      a = a*2;
  //    } else {
  //      a = 1;
  //    }
  //    return a*32;
  //  }
  //  -- expected --
  //  int compute() {
  //    return 32;
  //  }
  auto function = new Function("compute");
  auto varDeclA = new VarDecl("a",
                              new Datatype(Types::INT),
                              new LiteralInt(512));
  auto varDeclB = new VarDecl("b",
                              new Datatype(Types::INT),
                              new LiteralInt(22));

  auto condition = new LogicalExpr(
      new ArithmeticExpr(new Variable("b"), ArithmeticOp::ADDITION, new LiteralInt(12)),
      LogCompOp::SMALLER,
      new LiteralInt(20));
  auto thenBranch = new VarAssignm("a", new ArithmeticExpr(
      new Variable("a"),
      ArithmeticOp::MULTIPLICATION,
      new LiteralInt(2)));
  auto elseBranch = new VarAssignm("a", new LiteralInt(1));
  auto ifStmt = new If(condition, thenBranch, elseBranch);

  auto returnStatement =
      new Return(new ArithmeticExpr(new Variable("a"),
                                    ArithmeticOp::MULTIPLICATION,
                                    new LiteralInt(32)));

  // connect objects
  function->addStatement(varDeclA);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  PrintVisitor p;
  p.visit(ast);
  ctes.visit(ast);
  p.visit(ast);

  // check that 'a' was memorized correctly
  EXPECT_EQ(getVariableValueByUniqueName("a")->castTo<LiteralInt>()->getValue(), 1);
  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  EXPECT_EQ(function->getBodyStatements().front(), returnStatement);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(new LiteralInt(32)));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_thenBranchEvaluable_expectedRewriting) {
  //  -- input --
  //  int compute(plaintext_int a) {
  //    int b = 22;
  //    if (a > 20) {
  //      b = 2*b;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [a>20]*44+[1-[a>20]]*22;
  //  }
  auto function = new Function("compute");
  auto functionParameter = new ParameterList({
                                                 new FunctionParameter(new Datatype(Types::INT),
                                                                       new Variable("a"))});
  auto varDeclB = new VarDecl("b",
                              new Datatype(Types::INT),
                              new LiteralInt(22));
  auto ifStmtCondition = new LogicalExpr(new Variable("a"),
                                         LogCompOp::GREATER,
                                         new LiteralInt(20));
  auto ifStmt = new If(
      ifStmtCondition,
      new VarAssignm("b", new ArithmeticExpr(
          new LiteralInt(2),
          ArithmeticOp::MULTIPLICATION,
          new Variable("b"))));
  auto returnStatement =
      new Return(new Variable("b"));

  // connect objects
  function->setParameterList(functionParameter);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  auto expectedResult = new OperatorExpr(
      new Operator(ArithmeticOp::ADDITION),
      {new OperatorExpr(
          new Operator(ArithmeticOp::MULTIPLICATION),
          {new OperatorExpr(new Operator(LogCompOp::GREATER),
                            {new Variable("a"), new LiteralInt(20)}),
           new LiteralInt(44)}),
       new OperatorExpr(
           new Operator(ArithmeticOp::MULTIPLICATION),
           {new OperatorExpr(
               new Operator(ArithmeticOp::SUBTRACTION),
               {new LiteralInt(1),
                new OperatorExpr(new Operator(LogCompOp::GREATER), {new Variable("a"), new LiteralInt(20)})}),
            new LiteralInt(22)})});
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedResult));
  // check that 'b' was memorized correctly
  EXPECT_TRUE(getVariableValueByUniqueName("b")->isEqual(expectedResult));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_expectedRemovalOfElseClauseInResultBecauseVariableBIsNull) {
  //  -- input --
  //  int compute(plaintext_int a) {
  //    plaintext_int b = 0;
  //    if (a > 20) {
  //      plaintext_int c = 642;
  //      b = 2*c-1;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [a>20]*1'283;
  //  }
  auto function = new Function("compute");
  auto functionParameter = new ParameterList({
                                                 new FunctionParameter(new Datatype(Types::INT),
                                                                       new Variable("a"))});
  auto varDeclB = new VarDecl("b", new Datatype(Types::INT), new LiteralInt(0));
  auto ifStmtCondition = new LogicalExpr(new Variable("a"),
                                         LogCompOp::GREATER,
                                         new LiteralInt(20));
  auto thenStatements = std::vector<AbstractStatement *>(
      {new VarDecl("c", 642),
       new VarAssignm("b",
                      new ArithmeticExpr(
                          new ArithmeticExpr(
                              new LiteralInt(2),
                              ArithmeticOp::MULTIPLICATION,
                              new Variable("c")),
                          ArithmeticOp::SUBTRACTION,
                          new LiteralInt(1)))});
  auto thenBranch = new Block(thenStatements);
  auto ifStmt = new If(ifStmtCondition, thenBranch);
  auto returnStatement =
      new Return(new Variable("b"));

  // connect objects
  function->setParameterList(functionParameter);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  auto expectedResult =
      new OperatorExpr(
          new Operator(ArithmeticOp::MULTIPLICATION),
          {new OperatorExpr(
              new Operator(LogCompOp::GREATER), {new Variable("a"), new LiteralInt(20)}),
           new LiteralInt(1'283)});
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedResult));
  // check that variable values were memorized correctly
  EXPECT_TRUE(getVariableValueByUniqueName("b")->isEqual(expectedResult));
  // variable 'c' is not expected to be memorized because it's declared in the Then-branch only
  EXPECT_THROW(getVariableValueByUniqueName("c"), std::logic_error);

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_varDeclInThenBranch_expectedRewritingOfIfStatement) {
  //  -- input --
  //  int compute(plaintext_int a) {
  //    plaintext_int b = 42;
  //    if (a > 20) {
  //      plaintext_int c = 642;
  //      b = 2*c-1;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [a>20]*1'283+[1-[a>20]]*42;
  //  }
  auto function = new Function("compute");
  auto functionParameter = new ParameterList({
                                                 new FunctionParameter(new Datatype(Types::INT),
                                                                       new Variable("a"))});
  auto varDeclB = new VarDecl("b", new Datatype(Types::INT), new LiteralInt(42));
  auto ifStmtCondition = new LogicalExpr(new Variable("a"),
                                         LogCompOp::GREATER,
                                         new LiteralInt(20));
  auto thenStatements = std::vector<AbstractStatement *>(
      {new VarDecl("c", 642),
       new VarAssignm("b",
                      new ArithmeticExpr(
                          new ArithmeticExpr(
                              new LiteralInt(2),
                              ArithmeticOp::MULTIPLICATION,
                              new Variable("c")),
                          ArithmeticOp::SUBTRACTION,
                          new LiteralInt(1)))});
  auto thenBranch = new Block(thenStatements);
  auto ifStmt = new If(ifStmtCondition, thenBranch);
  auto returnStatement =
      new Return(new Variable("b"));

  // connect objects
  function->setParameterList(functionParameter);
  function->addStatement(varDeclB);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  auto expectedResult = new OperatorExpr(new Operator(ArithmeticOp::ADDITION),
                                         {new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
                                                           {new OperatorExpr(new Operator(LogCompOp::GREATER),
                                                                             {new Variable("a"), new LiteralInt(20)}),
                                                            new LiteralInt(1'283)}),
                                          new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
                                                           {new OperatorExpr(new Operator(ArithmeticOp::SUBTRACTION),
                                                                             {new LiteralInt(1),
                                                                              new OperatorExpr(new Operator(LogCompOp::GREATER),
                                                                                               {new Variable("a"),
                                                                                                new LiteralInt(20)})}),
                                                            new LiteralInt(42)})});
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedResult));
  // check that 'b' was memorized correctly
  EXPECT_TRUE(getVariableValueByUniqueName("b")->isEqual(expectedResult));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_conditionValueIsUnknown_thenAndElseExists_returnValueIsInputVariable_expectedRewritingOfIfStatement) {
  //  -- input --
  //  int compute(plaintext_int factor, plaintext_int threshold) {
  //    int b;
  //    if (threshold < 11) {
  //      b = 2*factor;
  //    } else {
  //      b = factor;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [threshold<11]*2*factor + [1-[threshold<11]]*factor;
  //  }
  auto function = new Function("compute");
  auto functionParameter = new ParameterList(
      {new FunctionParameter(new Datatype(Types::INT), new Variable("factor")),
       new FunctionParameter(new Datatype(Types::INT), new Variable("threshold"))});
  auto varDecl = new VarDecl("b", new Datatype(Types::INT));
  auto ifStmtCondition = new LogicalExpr(new Variable("threshold"),
                                         LogCompOp::SMALLER,
                                         new LiteralInt(11));
  auto thenBranch = new VarAssignm("b",
                                   new ArithmeticExpr(
                                       new LiteralInt(2),
                                       ArithmeticOp::MULTIPLICATION,
                                       new Variable("factor")));
  auto elseBranch = new VarAssignm("b", new Variable("factor"));

  auto ifStmt = new If(ifStmtCondition, thenBranch, elseBranch);
  auto returnStatement =
      new Return(new Variable("b"));

  // connect objects
  function->addStatement(varDecl);
  function->setParameterList(functionParameter);
  function->addStatement(ifStmt);
  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStatement->getReturnExpressions().size(), 1);
  // check that the return value is the expected one
  auto expectedResult = new OperatorExpr(
      new Operator(ArithmeticOp::ADDITION),
      {new OperatorExpr(
          new Operator(ArithmeticOp::MULTIPLICATION),
          {new OperatorExpr(new Operator(LogCompOp::SMALLER), {new Variable("threshold"), new LiteralInt(11)}),
           new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION), {new LiteralInt(2), new Variable("factor")})}),
       new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
                        {new OperatorExpr(new Operator(ArithmeticOp::SUBTRACTION),
                                          {new LiteralInt(1),
                                           new OperatorExpr(
                                               new Operator(LogCompOp::SMALLER),
                                               {new Variable("threshold"), new LiteralInt(11)})}),
                         new Variable("factor")})});
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedResult));
  // check that 'b' was memorized correctly
  EXPECT_TRUE(getVariableValueByUniqueName("b")->isEqual(expectedResult));

}

TEST_F(CompileTimeExpressionSimplifierFixture, /* NOLINT */
       ifStmt_nestedIfStatements_expectedRewritingOfBothIfStatement) {
  //  -- input --
  //  int compute(plaintext_int factor, plaintext_int threshold) {
  //    int b = 99;
  //    if (threshold > 11) {
  //        b = b/3;          // b = 33;
  //      if (factor > 9) {
  //        b = b*2*factor;   // b = 33*2*factor = 66*factor;
  //      } else {
  //        b = b*factor;     // b = 33*factor
  //      }
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [factor>9]*[threshold>11]*66*factor + [1-[factor>9]]*[threshold>11]*33*factor + [1-[threshold>11]]*99;
  //  }
  auto function = new Function("compute");
  auto functionParameter = new ParameterList(
      {new FunctionParameter(new Datatype(Types::INT), new Variable("factor")),
       new FunctionParameter(new Datatype(Types::INT), new Variable("threshold"))});

  auto varDeclB = new VarDecl("b", 99);

  auto innerIfStatementCondition = new LogicalExpr(new Variable("factor"), LogCompOp::GREATER, new LiteralInt(9));
  auto innerIfStatement = new If(innerIfStatementCondition,
                                 new VarAssignm("b", new ArithmeticExpr(
                                     new ArithmeticExpr(new Variable("b"), ArithmeticOp::MULTIPLICATION, 2),
                                     ArithmeticOp::MULTIPLICATION,
                                     new Variable("factor"))),
                                 new VarAssignm("b",
                                                new ArithmeticExpr(new Variable("b"),
                                                                   ArithmeticOp::MULTIPLICATION,
                                                                   new Variable("factor"))));

  auto outerIfStmtThenBlock =
      new Block({new VarAssignm("b", new ArithmeticExpr(new Variable("b"), ArithmeticOp::DIVISION, new LiteralInt(3))),
                 innerIfStatement});
  auto outerIfStatementCondition = new LogicalExpr(new Variable("threshold"), LogCompOp::GREATER, new LiteralInt(11));
  auto outerIfStmt = new If(outerIfStatementCondition, outerIfStmtThenBlock);

  auto returnStmt = new Return(new Variable("b"));

  // connect objects
  function->setParameterList(functionParameter);
  function->addStatement(varDeclB);
  function->addStatement(outerIfStmt);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that there is only one statement left
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  // check that the return statement contains exactly one return value
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  // check that the return value equals the expected one
  // return
  //    [threshold>11]*[
  //            [factor>9]*66*factor        // expectedResultLhsTerm
  //          + [1-[factor>9]]*33*factor]   // expectedResultMiddleTerm
  //    + [1-[threshold>11]]*99;            // expectedResultRhsTerm

  auto expectedResultLhsTerm = new OperatorExpr(
      new Operator(ArithmeticOp::MULTIPLICATION),
      {new OperatorExpr(new Operator(LogCompOp::GREATER), {new Variable("factor"), new LiteralInt(9)}),
       new OperatorExpr(
           new Operator(ArithmeticOp::MULTIPLICATION),
           {new LiteralInt(66),
            new Variable("factor")})});
  auto expectedResultMiddleTerm = new OperatorExpr(
      new Operator(ArithmeticOp::MULTIPLICATION),
      {new OperatorExpr(
          new Operator(ArithmeticOp::SUBTRACTION),
          {new LiteralInt(1),
           new OperatorExpr(new Operator(LogCompOp::GREATER), {new Variable("factor"), new LiteralInt(9)})}),
       new OperatorExpr(
           new Operator(ArithmeticOp::MULTIPLICATION),
           {new LiteralInt(33),
            new Variable("factor")})});
  auto expectedResultRhsTerm = new OperatorExpr(
      new Operator(ArithmeticOp::MULTIPLICATION),
      {new OperatorExpr(
          new Operator(ArithmeticOp::SUBTRACTION),
          {new LiteralInt(1),
           new OperatorExpr(new Operator(LogCompOp::GREATER), {new Variable("threshold"), new LiteralInt(11)})}),
       new LiteralInt(99)});

  auto expectedResult = new OperatorExpr(
      new Operator(ArithmeticOp::ADDITION),
      {new OperatorExpr(
          new Operator(ArithmeticOp::MULTIPLICATION),
          {new OperatorExpr(new Variable("threshold"), new Operator(LogCompOp::GREATER), new LiteralInt(11)),
           new OperatorExpr(new Operator(ArithmeticOp::ADDITION), {expectedResultLhsTerm, expectedResultMiddleTerm})}),
       expectedResultRhsTerm});
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedResult));
  // check that 'b' was memorized correctly
  EXPECT_TRUE(getVariableValueByUniqueName("b")->isEqual(expectedResult));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_partiallyEvaluableOnly) { /* NOLINT */
  //  -- input --
  // int f(plaintext_int x) {
  //  int y = 42;
  //  x = x+29
  //  return x+y
  // }
  //  -- expected --
  // int f(plaintext_int x) {
  //  return x+71;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(
          new Datatype(Types::INT, false), new Variable("x"))});
  auto varDeclY = new VarDecl("y", 42);
  auto varAssignmX =
      new VarAssignm("x", new ArithmeticExpr(new Variable("x"), ArithmeticOp::ADDITION, new LiteralInt(29)));
  auto returnStmt = new Return(
      new ArithmeticExpr(new Variable("x"), ArithmeticOp::ADDITION, new Variable("y")));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varDeclY);
  function->addStatement(varAssignmX);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(ADDITION), {new Variable("x"), new LiteralInt(71)});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_includingOperatorExprs) { /* NOLINT */
  //  -- input --
  // int f(plaintext_int x, plaintext_int a) {
  //  int y = 42+34+x+a;  // 76+x+a
  //  int x = 11+29;  // 40
  //  return x+y
  // }
  //  -- expected --
  // int f(plaintext_int x) {
  //  return 116+x+a;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::INT, false), new Variable("x")),
          new FunctionParameter(new Datatype(Types::INT, false), new Variable("a"))
      });

  auto varDeclY = new VarDecl("y",
                              new Datatype(Types::INT),
                              new ArithmeticExpr(new LiteralInt(42), ArithmeticOp::ADDITION,
                                                 new ArithmeticExpr(new LiteralInt(34), ArithmeticOp::ADDITION,
                                                                    new ArithmeticExpr(new Variable("x"),
                                                                                       ArithmeticOp::ADDITION,
                                                                                       new Variable("a")))));

  auto varAssignmX =
      new VarAssignm("x",
                     new ArithmeticExpr(new LiteralInt(11), ArithmeticOp::ADDITION, new LiteralInt(29)));
  auto returnStmt = new Return(
      new ArithmeticExpr(new Variable("x"), ArithmeticOp::ADDITION, new Variable("y")));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(varDeclY);
  function->addStatement(varAssignmX);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal =
      new OperatorExpr(new Operator(ADDITION), {new Variable("x"), new Variable("a"), new LiteralInt(116)});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_nestedDivisionOperator) { /* NOLINT */
  //  -- input --
  // int f(plaintext_int a) {
  //  return 9 + (34 + (22 / (a / (11 * 42))));
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return 43 + (22 / (a / 462));
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(
          new Datatype(Types::INT, false), new Variable("a"))});
  auto returnStmt = new Return(
      new ArithmeticExpr(new LiteralInt(9), ADDITION,
                         new ArithmeticExpr(new LiteralInt(34), ADDITION,
                                            new ArithmeticExpr(new LiteralInt(22), DIVISION,
                                                               new ArithmeticExpr(new Variable("a"), DIVISION,
                                                                                  new ArithmeticExpr(new LiteralInt(11),
                                                                                                     MULTIPLICATION,
                                                                                                     42))))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(
      new Operator(ADDITION), {new LiteralInt(43),
                               new OperatorExpr(new Operator(DIVISION),
                                                {new LiteralInt(22), new Variable("a"), new LiteralInt(462)})});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_nestedOperatorsSimplifiableOnOneSideOnly) { /* NOLINT */
  //  -- input --
  // int f(plaintext_int a) {
  //  return (4 + (3 + 8) - (a / (2 * 4))
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return 15 - (a / 8)
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(
          new Datatype(Types::INT, false), new Variable("a"))});
  auto returnStmt = new Return(
      new ArithmeticExpr(
          new ArithmeticExpr(new LiteralInt(4), ADDITION,
                             new ArithmeticExpr(new LiteralInt(3), ADDITION, new LiteralInt(8))),
          SUBTRACTION,
          new ArithmeticExpr(new Variable("a"), DIVISION,
                             new ArithmeticExpr(new LiteralInt(2), MULTIPLICATION, new LiteralInt(4)))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(SUBTRACTION),
                                            {new LiteralInt(15),
                                             new OperatorExpr(new Operator(DIVISION),
                                                              {new Variable("a"), new LiteralInt(8)})});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_nestedLogicalOperators) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b, encrypted_bool c) {
  //  return (a ^ (b ^ false)) && ((true || false) || true)
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a ^ b;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b")),
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("c"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(new LogicalExpr(new Variable("a"), LOGICAL_XOR,
                                      new LogicalExpr(
                                          new Variable("b"),
                                          LOGICAL_XOR,
                                          new LiteralBool(false))),
                      LOGICAL_AND,
                      new LogicalExpr(new LogicalExpr(
                          new LiteralBool(true),
                          LOGICAL_OR,
                          new LiteralBool(false)),
                                      LOGICAL_OR,
                                      new LiteralBool(true))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new Variable("b")});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));

}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_ANDtrue) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a && (true && b);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a && b;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_AND,
          new LogicalExpr(
              new LiteralBool(true),
              LOGICAL_AND,
              new Variable("b"))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(LOGICAL_AND), {new Variable("a"), new Variable("b")});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_ANDfalse) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a && (false && b);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return false;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_AND,
          new LogicalExpr(
              new LiteralBool(false),
              LOGICAL_AND,
              new Variable("b"))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new LiteralBool(false);
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_ORfalse) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a || (b || false);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a || b;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_OR,
          new LogicalExpr(
              new Variable("b"),
              LOGICAL_OR,
              new LiteralBool(false))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(LOGICAL_OR), {new Variable("a"), new Variable("b")});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_ORtrue) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a || (b || true);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return true;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_OR,
          new LogicalExpr(
              new Variable("b"),
              LOGICAL_OR,
              new LiteralBool(true))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new LiteralBool(true);
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_XORtrue) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a ^ (b ^ true);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a ^ !b;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_XOR,
          new LogicalExpr(
              new Variable("b"),
              LOGICAL_XOR,
              new LiteralBool(true))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(LOGICAL_XOR),
                                            {new Variable("a"), new Variable("b"), new LiteralBool(true)});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, symbolicTerms_logicalAndSimplification_XORfalse) { /* NOLINT */
  //  -- input --
  // int f(encrypted_bool a, plaintext_bool b) {
  //  return a ^ (b ^ false);
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a ^ b;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {
          new FunctionParameter(new Datatype(Types::BOOL, true), new Variable("a")),
          new FunctionParameter(new Datatype(Types::BOOL, false), new Variable("b"))
      });
  auto returnStmt = new Return(
      new LogicalExpr(
          new Variable("a"),
          LOGICAL_XOR,
          new LogicalExpr(
              new Variable("b"),
              LOGICAL_XOR,
              new LiteralBool(false))));

  // connect objects
  function->setParameterList(functionParamList);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  auto expectedReturnVal = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new Variable("b")});
  EXPECT_EQ(returnStmt->getReturnExpressions().size(), 1);
  EXPECT_TRUE(returnStmt->getReturnExpressions().front()->isEqual(expectedReturnVal));
}

TEST_F(CompileTimeExpressionSimplifierFixture, WhileLoop_compileTimeKnownExpression_removalExpected) { /* NOLINT */
  //  -- input --
  // int f(plaintext_int a) {
  //  int i = 2;
  //  while (i > 10) {
  //    a = a * a;
  //    i = i-1;
  //  }
  //  return a;
  // }
  //  -- expected --
  // int f(plaintext_int a) {
  //  return a;
  // }
  auto function = new Function("f");
  auto functionParamList = new ParameterList(
      {new FunctionParameter(new Datatype(Types::INT, true), new Variable("a"))});
  function->setParameterList(functionParamList);

  function->addStatement(new VarDecl("i", 2));

  auto whileBody =
      new Block({
                    new VarAssignm("a",
                                   new ArithmeticExpr(
                                       new Variable("a"),
                                       MULTIPLICATION,
                                       new Variable("a"))),
                    new VarAssignm("i",
                                   new ArithmeticExpr(
                                       new Variable("i"),
                                       SUBTRACTION,
                                       new LiteralInt(1)))});
  function->addStatement(new While(new LogicalExpr(
      new Variable("i"),
      GREATER,
      new LiteralInt(10)), whileBody));

  auto *returnStatement = new Return(new Variable("a"));
  function->addStatement(returnStatement);

  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // check that simplification generated the expected simplified AST
  EXPECT_EQ(function->getBodyStatements().size(), 1);
  EXPECT_TRUE(function->getBodyStatements().front()->isEqual(new Return(new Variable("a"))));
}

TEST_F(CompileTimeExpressionSimplifierFixture, Call_inliningExpected) { /* NOLINT */
  //  -- input --
  // int f(int a) {
  //  return computeX(a);       --> int computeX(plaintext_int x) { return x + 111; }
  // }
  //  -- expected --
  // int f(int a) {
  //  return a + 111;
  // }
  auto function = new Function("f");
  function->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("a")));
  auto funcComputeX =
      new Function("computeX",
                   new ParameterList({new FunctionParameter(new Datatype(Types::INT, false), new Variable("x"))}),
                   new Block(new Return(new ArithmeticExpr(new Variable("x"), ADDITION, new LiteralInt(111)))));
  auto callStmt = new Call(
      {new FunctionParameter(new Datatype(Types::INT, false), new Variable("a"))}, funcComputeX);

  auto returnStatement = new Return(callStmt);

  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(ADDITION), {new Variable("a"), new LiteralInt(111)});
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, Call_inliningExpected2) { /* NOLINT */
  //TODO: Re-enable once arguments for Call are implemnted correctly
  GTEST_SKIP();
  //  -- input --
  // int f() {
  //  return computeX(232);       --> int computeX(plaintext_int x) { return x + 111; }
  // }
  //  -- expected --
  // int f() {
  //  return 343;
  // }
  auto function = new Function("f");
  auto funcComputeX =
      new Function("computeX",
                   new ParameterList({new FunctionParameter(new Datatype(Types::INT, false), new Variable("x"))}),
                   new Block(new Return(new ArithmeticExpr(new Variable("x"), ADDITION, new LiteralInt(111)))));
  auto callStmt = new Call(
      {new FunctionParameter(
          new Datatype(Types::INT, false), new LiteralInt(232))}, funcComputeX);

  auto returnStatement = new Return(callStmt);

  function->addStatement(returnStatement);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralInt(343);
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, Rotate_executionExpected) { /* NOLINT */
  //  -- input --
  // rotateVec(int inputA) {
  //   int sumVec = {{1, 7, 3}};   // [1 7 3]
  //   return sumVec.rotate(1);    // [3 1 7]
  // }
  //  -- expected --
  // rotateVec(int inputA) {
  //   return LiteralInt{{3, 1, 7}};
  // }
  Ast ast;
  AstTestingGenerator::generateAst(24, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralInt(new Matrix<int>({{3, 1, 7}}));
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, transpose) { /* NOLINT */
  //  -- input --
  // transposeMatrix() {
  //   return [11 2 3; 4 2 3; 2 1 3].transpose();
  // }
  //  -- expected --
  // transposeMatrix() {
  //   return [11 4 2; 2 2 1; 3 3 3];
  // }
  Ast ast;
  AstTestingGenerator::generateAst(26, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralInt(new Matrix<int>({{11, 4, 2}, {2, 2, 1}, {3, 3, 3}}));
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, getMatrixElementSimpleBool) { /* NOLINT */
  // -- input –-
  // extractArbitraryMatrixElements() {
  //   int M = [ true true false ];
  //   int B = [ false true true ];
  //   return [ M[0][1];      // true
  //            B[0][0];      // false
  //            B[0][2] ];    // true
  // }
  // -- expected –-
  // extractArbitraryMatrixElements {
  //   return [ true; false; true ];
  // }
  Ast ast;
  AstTestingGenerator::generateAst(30, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralBool(
      new Matrix<AbstractExpression *>({{new LiteralBool(true), new LiteralBool(false), new LiteralBool(true)}}));
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, partiallySimplifiableMatrix) { /* NOLINT */
  // -- input –-
  // extractArbitraryMatrixElements(plaintext_bool y {
  //   int M = [ true y false ];
  //   return [ M[0][1];      // y
  //            M[0][0];      // true
  //            M[0][2] ];    // false
  // }
  // -- expected –-
  // extractArbitraryMatrixElements {
  //   return [ y; LiteralBool(true); LiteralBool(false) ];
  // }
  Ast ast;
  AstTestingGenerator::generateAst(34, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto mx = new Matrix<AbstractExpression *>({{new Variable("y"), new LiteralBool(true), new LiteralBool(false)}});
  auto expectedReturnExpr = new LiteralBool(mx);
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_fullyEvaluable) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(35, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralInt(77);
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_partiallyEvaluable) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(37, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(MULTIPLICATION), {new Variable("a"), new LiteralInt(1054)});
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalAndFalse) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(38, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralBool(false);
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalAndTrue_oneRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(39, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new Variable("a");
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalAndTrue_twoRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(40, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_AND), {new Variable("b"), new Variable("a")});
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalOrTrue) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(41, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new LiteralBool(true);
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalOrFalse_oneRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(42, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new Variable("a");
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalOrFalse_twoRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(43, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_OR), {new Variable("b"), new Variable("a")});
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalXorTrue) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(44, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new LiteralBool(true)});
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalXorFalse_oneRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(45, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new Variable("a");
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, operatorExpr_logicalXorFalse_twoRemainingOperand) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(46, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new Variable("b")});
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();

  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
}

TEST_F(CompileTimeExpressionSimplifierFixture, nestedOperatorExprsTest) { /* NOLINT */
  auto func = new Function("sumVectorElements");
  // (27 / a / 12 / 3 / 1) or in prefix notation: (/ 27 (/ a 12) 3 1)
  auto opExpr0 = new OperatorExpr(new Operator(DIVISION), {new Variable("a"), new LiteralInt(12)});
  auto opExpr1 =
      new OperatorExpr(new Operator(DIVISION), {new LiteralInt(27), opExpr0, new LiteralInt(3), new LiteralInt(1)});
  auto ret = new Return(opExpr1);
  func->addStatement(ret);
  func->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("a")));

  Ast ast;
  ast.setRootNode(func);
  ctes.visit(ast);

  // expected result: (/ 27 a 4)
  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(
      new OperatorExpr(new Operator(DIVISION), {new LiteralInt(27), new Variable("a"), new LiteralInt(4)})));
}

TEST_F(CompileTimeExpressionSimplifierFixture, partialforLoopUnrolling) { /* NOLINT */
  //TODO: Re-implement partial unrolling
  GTEST_SKIP();
  // -- input --
  // int sumVectorElements(int numIterations) {
  //    Matrix<int> M = [54; 32; 63; 38; 13; 20];
  //    int sum = 0;
  //    for (int i = 0; i < numIterations; i++) {
  //      sum = sum + M[i];
  //    }
  //    return sum;
  // }
  // -- expected --
  // int sumVectorElements(int numIterations) {
  //    Matrix<int> M = [54; 32; 63; 38; 13; 20];
  //    int sum;
  //    int i;
  //    {
  //      for (int i = 0; i < numIterations && i+1 < numIterations && i+2 < numIterations;) {
  //        sum = sum + M[i] + M[i+1] + M[i+2];
  //        i = i+3;
  //      }
  //      for (; i < numIterations; i++) {
  //        sum = sum + M[i];
  //      }
  //    }
  //    return sum;
  // }
  Ast ast;
  AstTestingGenerator::generateAst(48, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // build expected AST
  auto func = new Function("sumVectorElements");
  func->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("numIterations")));

  // int sum;
  func->addStatement(new VarDecl("sum", new Datatype(Types::INT, false)));
  func->addStatement(new VarDecl("i", new Datatype(Types::INT, false)));

  // unrolled loop and cleanup loop are embedded into new Block
  // { ...
  auto newBlock = new Block();
  func->addStatement(newBlock);
  // int i = 0;
//  newBlock->addChild(new VarDecl("i", new Datatype(Types::INT)));
  // for (; i+1 < numIterations && i+2 < numIterations && i+3 < numIterations; )  // unrolled loop
  auto unrolledLoopInitializer = nullptr;
  auto unrolledLoopCondition =
      new OperatorExpr(
          new Operator(LOGICAL_AND),
          {
              new OperatorExpr(new Operator(SMALLER), {new Variable("i"), new Variable("numIterations")}),
              new OperatorExpr(new Operator(SMALLER), {
                  new OperatorExpr(new Operator(ADDITION),
                                   {new Variable("i"), new LiteralInt(1)}),
                  new Variable("numIterations")}),
              new OperatorExpr(new Operator(SMALLER), {
                  new OperatorExpr(new Operator(ADDITION), {new Variable("i"), new LiteralInt(2)}), new Variable
                      ("numIterations")})
          });

  auto unrolledLoopUpdater = nullptr;
  auto createVariableM = []() -> LiteralInt * {
    return new LiteralInt(new Matrix<int>({{54}, {32}, {63}, {38}, {13}, {20}}));
  };
  auto genGetMatrixElement = [&createVariableM](int rowIdx) -> MatrixElementRef * {
    return new MatrixElementRef(createVariableM(),
                                new OperatorExpr(new Operator(ADDITION),
                                                 {new Variable("i"), new LiteralInt(rowIdx)}), new LiteralInt(0));
  };
  // {
  //    sum = M[i] + M[i+1] + M[i+2];
  //    i = i + 3;
  // }
  auto unrolledLoopBodyStatements = new Block(
      {new VarAssignm("sum",
                      new OperatorExpr(new Operator(ADDITION),
                                       {new Variable("sum"),
                                           // M[i]
                                        new MatrixElementRef(createVariableM(), new Variable("i"), new LiteralInt(0)),
                                           // M[i+1]
                                        genGetMatrixElement(1),
                                           // M[i+2]
                                        genGetMatrixElement(2)})),
       new VarAssignm("i", new OperatorExpr(new Operator(ADDITION), {new Variable("i"), new LiteralInt(3)}))
      });
  auto forLoop =
      new For(unrolledLoopInitializer, unrolledLoopCondition, unrolledLoopUpdater, unrolledLoopBodyStatements);
  newBlock->addStatement(forLoop);

  // for (; i < numIterations; i=i+1)  // cleanup loop
  auto cleanupLoopInitializer = nullptr;
  auto cleanupLoopCondition = new OperatorExpr(new Operator(SMALLER), {new Variable("i"), new Variable
      ("numIterations")});
  auto cleanupLoopUpdater =
      new VarAssignm("i", new OperatorExpr(new Operator(ADDITION), {new Variable("i"), new LiteralInt(1)}));
  auto cleanupLoopBodyStatements = new Block(
      new VarAssignm("sum", new OperatorExpr(
          new Operator(ADDITION),
          {new Variable("sum"),
           new MatrixElementRef(createVariableM(), new Variable("i"), new LiteralInt(0))})));
  auto child = new For(cleanupLoopInitializer,
                       cleanupLoopCondition->castTo<AbstractExpression>(),
                       cleanupLoopUpdater->castTo<AbstractStatement>(),
                       cleanupLoopBodyStatements);
  newBlock->addStatement(child);
  // ...}  // end of the new block

  // Return sum;
  func->addStatement(new Return(new Variable("sum")));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();

  EXPECT_TRUE(simplifiedAst->isEqual(func->getBody()));
}

//TODO: Why is the exepected output VarDecl, VarAssignm, Return(Variable) instead of Return(Expression)?
TEST_F(CompileTimeExpressionSimplifierFixture, fullForLoopUnrolling) { /* NOLINT */
  // -- input --
  //  VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize, int x, int y) {
  //     Matrix<int> weightMatrix = [1 1 1; 1 -8 1; 1 1 1];
  //     Vector<int> img2;
  //     int value = 0;
  //     for (int j = -1; j < 2; ++j) {
  //        for (int i = -1; i < 2; ++i) {
  //           value = value + weightMatrix[i+1][j+1] * img[imgSize*(x+i)+y+j];
  //        }
  //     }
  //     img2[imgSize*x+y] = img[imgSize*x+y] - (value/2);
  //     return img2;
  //  }
  // -- expected --
  // VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize, int x, int y) {
  //   Matrix<int> img2;
  //   img2[imgSize*x+y] = img[imgSize*x+y]
  //      - (  img[imgSize*(x-1)+y-1] + img[imgSize*x+y-1]        + img[imgSize*(x+1)+y-1]
  //         + img[imgSize*(x-1)+y]   + img[imgSize*x+y] * (-8)   + img[imgSize*(x+1)+y]
  //         + img[imgSize*(x-1)+y+1] + img[imgSize*x+y+1]        + img[imgSize*(x+1)+y+1]  ) / 2;
  //   return img2;
  // }
  Ast ast;
  AstTestingGenerator::generateAst(49, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  // VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize, int x, int y) {
  auto expectedFunction = new Function("runLaplacianSharpeningAlgorithm");
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, true), new Variable("img")));
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("imgSize")));
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("x")));
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("y")));

  expectedFunction->addStatement(new VarDecl("img2", new Datatype(Types::INT), new LiteralInt()));

  // a helper to generate img[imgSize*(x-i)+y+j] terms
  auto createImgIdx = [](int i, int j) -> AbstractExpression * {
    auto buildTermI = [](int i) -> AbstractExpression * {
      if (i==0) {
        return new Variable("x");
      } else {
        return new OperatorExpr(new Operator(ADDITION), {new Variable("x"), new LiteralInt(i)});
      }
    };

    auto buildTermJ = [&](int j) -> AbstractExpression * {
      if (j==0) {
        return new OperatorExpr(new Operator(ADDITION),
                                {new OperatorExpr(new Operator(MULTIPLICATION),
                                                  {new Variable("imgSize"),
                                                   buildTermI(i)}),
                                 new Variable("y")});
      } else {
        return new OperatorExpr(new Operator(ADDITION),
                                {new OperatorExpr(new Operator(MULTIPLICATION),
                                                  {new Variable("imgSize"),
                                                   buildTermI(i)}),
                                 new Variable("y"),
                                 new LiteralInt(j)});
      }
    };
    return new MatrixElementRef(new Variable("img"), new LiteralInt(0), buildTermJ(j));
  };

  // img[imgSize*(x-1)+y-1]  * 1 + ... + img[imgSize*(x+1)+y+1]  * 1;
  auto varValue =
      new OperatorExpr(
          new Operator(ADDITION),
          {createImgIdx(-1, -1),
           createImgIdx(0, -1),
           createImgIdx(1, -1),
           createImgIdx(-1, 0),
           new OperatorExpr(new Operator(MULTIPLICATION), {createImgIdx(0, 0), new LiteralInt(-8)}),
           createImgIdx(1, 0),
           createImgIdx(-1, 1),
           createImgIdx(0, 1),
           createImgIdx(1, 1)});

  // img2[imgSize*x+y] = img[imgSize*x+y] - (value/2);
  expectedFunction->addStatement(
      new MatrixAssignm(new MatrixElementRef(new Variable("img2"),
                                             new LiteralInt(0),
                                             new OperatorExpr(
                                                 new Operator(ADDITION),
                                                 {new OperatorExpr(new Operator(MULTIPLICATION),
                                                                   {new Variable("imgSize"), new Variable("x")}),
                                                  new Variable("y")})),
                        new OperatorExpr(
                            new Operator(SUBTRACTION),
                            {new MatrixElementRef(
                                new Variable("img"),
                                new LiteralInt(0),
                                new OperatorExpr(
                                    new Operator(ADDITION),
                                    {new OperatorExpr(new Operator(MULTIPLICATION),
                                                      {new Variable("imgSize"), new Variable("x")}),
                                     new Variable("y")})),
                             new OperatorExpr(new Operator(DIVISION), {varValue, new LiteralInt(2)})})));

  // return img2;
  expectedFunction->addStatement(new Return(new Variable("img2")));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, getMatrixSizeOfKnownMatrix) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(52, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunction = new Function("returnLastVectorElement");
  expectedFunction->addStatement(new Return(new LiteralInt(44)));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, getMatrixSizeOfAbstractMatrix) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(53, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunction = new Function("getNumElementsPerDimension");
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("factor")));
  expectedFunction->addStatement(new Return(
      new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt(1), new LiteralInt(5), new LiteralInt(0)}}))));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, getMatrixSizeOfUnknownMatrix) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(54, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunction = new Function("getNumElementsPerDimension");
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("inputMatrix")));
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("dimension")));
  expectedFunction
      ->addStatement(new Return(new GetMatrixSize(new Variable("inputMatrix"), new Variable("dimension"))));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, nestedFullLoopUnrolling_matrixAssignmAndGetMatrixSize) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(55, ast);
  // Matrix<int> extendMatrixAddingElements() {
  //   Matrix<int> m;   // size: 0x0
  //   for (int i = 0; i < 3; ++i) {
  //     Vector<int> t;
  //     for (int j = 0; j < 3; ++j) {
  //       t[0][t.dimSize(1)] = i*j;
  //     }
  //     m[m.dimSize(0)] = t;
  //   }
  //   return m;  // m = [0*0 0*1 0*2; 1*0 1*1 1*2; 2*0 2*1 2*2] = [0 0 0; 0 1 2; 0 2 4], size: 3x3
  // }

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunction = new Function("extendMatrixAddingElements");
  expectedFunction->addStatement(new Return(new LiteralInt(
      new Matrix<AbstractExpression *>({{new LiteralInt(0), new LiteralInt(0), new LiteralInt(0)},
                                        {new LiteralInt(0), new LiteralInt(1), new LiteralInt(2)},
                                        {new LiteralInt(0), new LiteralInt(2), new LiteralInt(4)}}))));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, matrixAssignmIncludingPushBack) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(59, ast);

  // Matrix<int> extendMatrixAddingElements() {
  //  Matrix<int> m;   // size: 0x0
  //  Vector<int> t;
  //  for (int i = 0; i < 3; ++i) {
  //    t[0][t.dimSize(1)] = i*i;
  //  }
  //  m[m.dimSize(0)] = t;
  //  return m;  // m = [0*0 1*1 2*2] = [0 1 4], size: 1x3
  //}

  // perform the compile-time expression simplification
  PrintVisitor p;
  p.visit(ast);
  ctes.visit(ast);
  p.visit(ast);

  auto expectedFunction = new Function("extendMatrixAddingElements");
  expectedFunction->addStatement(
      new Return(new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt(0),
                                                                   new LiteralInt(1),
                                                                   new LiteralInt(4)}}))));
  auto expectedAst = Ast(expectedFunction);
  p.visit(expectedAst);
  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, matrixAssignmentUnknownThenKnown) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(56, ast);

  // void computeMatrix(int k, int a) {
  //   Matrix<int> M;
  //   M[k][0] = 4;
  //   M[0][0] = 21 + a;
  //   return M;
  // }

  // perform the compile-time expression simplification
  PrintVisitor p;
  p.visit(ast);
  ctes.visit(ast);
  p.visit(ast);

  auto expectedFunc = new Function("computeMatrix");
  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("k")));
  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("a")));
  expectedFunc->addStatement(new VarDecl("M", new Datatype(Types::INT, false), new LiteralInt()));
  expectedFunc->addStatement(new MatrixAssignm(
      new MatrixElementRef(new Variable("M"), new Variable("k"), new LiteralInt(0)), new LiteralInt(4)));
  expectedFunc->addStatement(new MatrixAssignm(new MatrixElementRef(new Variable("M"), 0, 0),
                                               new OperatorExpr(new Operator(ADDITION),
                                                                {new Variable("a"), new LiteralInt(21)})));
  expectedFunc->addStatement(new Return(new Variable("M")));
  auto expectedAst = Ast(expectedFunc);
  p.visit(expectedAst);

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, matrixAssignmentKnownThenUnknown) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(57, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunc = new Function("computeMatrix");
  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("k")));
  expectedFunc->addStatement(new VarDecl("M", new Datatype(Types::INT, false)));
  // This assignment may look weird but is expected because matrix M lacks an initialization and as such is
  // default-initialized as Matrix<AbstractExpr*>, hence we need to store integers as LiteralInts
  expectedFunc->addStatement(new VarAssignm("M",
                                            new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt((21))}}))));
  expectedFunc->addStatement(new MatrixAssignm(
      new MatrixElementRef(new Variable("M"), new LiteralInt(0), new Variable("k")), new LiteralInt(4)));
  expectedFunc->addStatement(new Return(new Variable("M")));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, fullAssignmentToMatrix) { /* NOLINT */
  Ast ast;
  AstTestingGenerator::generateAst(58, ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunc = new Function("computeMatrix");
  expectedFunc->addStatement(new Return(new LiteralInt(
      new Matrix<AbstractExpression *>({{new LiteralInt(11), new LiteralInt(1), new LiteralInt(1)},
                                        {new LiteralInt(3), new LiteralInt(2), new LiteralInt(2)}}))));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, fourNestedLoopsLaplacianSharpeningFilter) { /* NOLINT */
  // -- input --
  // VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize) {
  //     Vector<int> img2 = {0, 0, .... ,0};
  //     Matrix<int> weightMatrix = [1 1 1; 1 -8 1; 1 1 1];
  //     for (int x = 1; x < imgSize - 1; ++x) {
  //         for (int y = 1; y < imgSize - 1; ++y) {
  //             int value = 0;
  //             for (int j = -1; j < 2; ++j) {
  //                 for (int i = -1; i < 2; ++i) {
  //                     value = value + weightMatrix[i+1][j+1] * img[imgSize*(x+i)+y+j];
  //                 }
  //             }
  //             img2[imgSize*x+y] = img[imgSize*x+y] - (value/2);
  //         }
  //     }
  //     return img2;
  // }
  // -- expected --
  // VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize) {
  //     Matrix<int> img2 = {0, 0, .... ,0};
  //     for (int x = 1; x < imgSize - 1; ++x) {
  //         for (int y = 1; y < imgSize - 1; ++y) {
  //            img2[imgSize*x+y] = img[imgSize*x+y] - (
  //                    + img[imgSize*(x-1)+y-1] + img[imgSize*x+y-1]        + img[imgSize*(x+1)+y-1]
  //                    + img[imgSize*(x-1)+y]   + img[imgSize*x+y] * (-8)   + img[imgSize*(x+1)+y]
  //                    + img[imgSize*(x-1)+y+1] + img[imgSize*x+y+1]        + img[imgSize*(x+1)+y+1] ) / 2;
  //         }
  //     }
  //     return img2;
  // }
  Ast ast;
  AstTestingGenerator::generateAst(60, ast);

  // run CTES and limit the number of unrolled loops to 2 (the two innermost loops are unrolled only)
  CompileTimeExpressionSimplifier ctes((CtesConfiguration(2)));
  ctes.visit(ast);

  // VecInt2D runLaplacianSharpeningAlgorithm(Vector<int> img, int imgSize, int x, int y) {
  auto expectedFunction = new Function("runLaplacianSharpeningAlgorithm");
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, true), new Variable("img")));
  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("imgSize")));

  expectedFunction->addStatement(new VarDecl("img2", new Datatype(Types::INT),
                                             new LiteralInt(new Matrix<int>(std::vector<std::vector<int>>(1,
                                                                                                          std::vector<
                                                                                                              int>(1024))))));

  // a helper to generate img[imgSize*(x-i)+y+j] terms
  auto createImgIdx = [](int i, int j) -> AbstractExpression * {
    auto buildTermI = [](int i) -> AbstractExpression * {
      if (i==0) {
        return new Variable("x");
      } else {
        return new OperatorExpr(new Operator(ADDITION), {new Variable("x"), new LiteralInt(i)});
      }
    };

    auto buildTermJ = [&](int j) -> AbstractExpression * {
      if (j==0) {
        return new OperatorExpr(new Operator(ADDITION),
                                {new OperatorExpr(new Operator(MULTIPLICATION),
                                                  {new Variable("imgSize"),
                                                   buildTermI(i)}),
                                 new Variable("y")});
      } else {
        return new OperatorExpr(new Operator(ADDITION),
                                {new OperatorExpr(new Operator(MULTIPLICATION),
                                                  {new Variable("imgSize"),
                                                   buildTermI(i)}),
                                 new Variable("y"),
                                 new LiteralInt(j)});
      }
    };
    return new MatrixElementRef(new Variable("img"), new LiteralInt(0), buildTermJ(j));
  };

  // img[imgSize*(x-1)+y-1]  * 1 + ... + img[imgSize*(x+1)+y+1]  * 1;
  auto varValue =
      new OperatorExpr(
          new Operator(ADDITION),
          {createImgIdx(-1, -1),
           createImgIdx(0, -1),
           createImgIdx(1, -1),
           createImgIdx(-1, 0),
           new OperatorExpr(new Operator(MULTIPLICATION), {createImgIdx(0, 0), new LiteralInt(-8)}),
           createImgIdx(1, 0),
           createImgIdx(-1, 1),
           createImgIdx(0, 1),
           createImgIdx(1, 1)});

  // img2[imgSize*x+y] = img[imgSize*x+y] - (value/2);
  auto secondLoopBody = new Block(
      new MatrixAssignm(new MatrixElementRef(new Variable("img2"),
                                             new LiteralInt(0),
                                             new OperatorExpr(
                                                 new Operator(ADDITION),
                                                 {new OperatorExpr(new Operator(MULTIPLICATION),
                                                                   {new Variable("imgSize"), new Variable("x")}),
                                                  new Variable("y")})),
                        new OperatorExpr(
                            new Operator(SUBTRACTION),
                            {new MatrixElementRef(
                                new Variable("img"),
                                new LiteralInt(0),
                                new OperatorExpr(
                                    new Operator(ADDITION),
                                    {new OperatorExpr(new Operator(MULTIPLICATION),
                                                      {new Variable("imgSize"), new Variable("x")}),
                                     new Variable("y")})),
                             new OperatorExpr(new Operator(DIVISION), {varValue, new LiteralInt(2)})})));

  // for (int y = 1; y < imgSize - 1; ++y)  -- 2nd level loop
  auto firstLoopBody = new Block(new For(new VarDecl("y", 1),
                                         new LogicalExpr(new Variable("y"),
                                                         SMALLER,
                                                         new ArithmeticExpr(new Variable("imgSize"), SUBTRACTION, 1)),
                                         new VarAssignm("y",
                                                        new ArithmeticExpr(new Variable("y"),
                                                                           ADDITION,
                                                                           new LiteralInt(1))),
                                         secondLoopBody));

  // for (int x = 1; x < imgSize - 1; ++x)  -- 1st level loop
  expectedFunction->addStatement(new For(new VarDecl("x", 1),
                                         new LogicalExpr(new Variable("x"),
                                                         SMALLER,
                                                         new ArithmeticExpr(new Variable("imgSize"), SUBTRACTION, 1)),
                                         new VarAssignm("x",
                                                        new ArithmeticExpr(new Variable("x"),
                                                                           ADDITION,
                                                                           new LiteralInt(1))),
                                         firstLoopBody));

  // return img2;
  expectedFunction->addStatement(new Return(new Variable("img2")));

  PrintVisitor pv;
  pv.visit(ast);

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, trivialLoop) { /* NOLINT */

  //  int trivialLoop() {
  //    int x = 0;
  //    for(int i = 0; i < 3; i = i + 1) {
  //      x = 42;
  //    }
  //    return x;
  //  }
  Ast ast;
  auto function = new Function("trivialLoop");
  auto loop = new For(new VarDecl("i", Types::INT, new LiteralInt(0)),
                      new LogicalExpr(new Variable("i"), LogCompOp::SMALLER, new LiteralInt(3)),
                      new VarAssignm("i",
                                     new ArithmeticExpr(new Variable("i"),
                                                        ArithmeticOp::ADDITION,
                                                        new LiteralInt(1))),
                      new VarAssignm("x", new LiteralInt(42)));
  auto varDecl = new VarDecl("x", Types::INT, new LiteralInt(0));
  auto returnStmt = new Return(new Variable("x"));
  function->addStatement(varDecl);
  function->addStatement(loop);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  PrintVisitor p;
  p.visit(ast);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  p.visit(ast);

  auto expectedFunc = new Function("trivialLoop");
  expectedFunc->addStatement(new Return(new LiteralInt(42)));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAstBody = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAstBody->isEqual(expectedFunc->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, trivialNestedLoops) { /* NOLINT */
  //  int trivialLoop() {
  //    int x = 0;
  //    for(int j = 0; j < 3; j = j + 1) {
  //      for(int i = 0; i < 3; i = i + 1) {
  //        x = 42;
  //      }
  //    }
  //    return x;
  //  }
  Ast ast;
  auto function = new Function("trivialNestedLoops");
  auto varDecl = new VarDecl("x", Types::INT, new LiteralInt(0));
  auto innerloop = new For(new VarDecl("i", Types::INT, new LiteralInt(0)),
                           new LogicalExpr(new Variable("i"), LogCompOp::SMALLER, new LiteralInt(3)),
                           new VarAssignm("i",
                                          new ArithmeticExpr(new Variable("i"),
                                                             ArithmeticOp::ADDITION,
                                                             new LiteralInt(1))),
                           new VarAssignm("x", new LiteralInt(42)));
  auto outerloop = new For(new VarDecl("j", Types::INT, new LiteralInt(0)),
                           new LogicalExpr(new Variable("j"), LogCompOp::SMALLER, new LiteralInt(3)),
                           new VarAssignm("j",
                                          new ArithmeticExpr(new Variable("j"),
                                                             ArithmeticOp::ADDITION,
                                                             new LiteralInt(1))),
                           innerloop);
  auto returnStmt = new Return(new Variable("x"));
  function->addStatement(varDecl);
  function->addStatement(outerloop);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // perform the compile-time expression simplification
  ctes.visit(ast);

  auto expectedFunc = new Function("trivialNestedLoops");
  expectedFunc->addStatement(new Return(new LiteralInt(42)));

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAstBody = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAstBody->isEqual(expectedFunc->getBody()));
}

TEST_F(CompileTimeExpressionSimplifierFixture, maxNumUnrollings) { /* NOLINT */
  Ast ast;
  //  int maxNumUnrollings() {
  //    int x = 0;
  //    for(int j = 0; j < 3; j = j + 1) { //do not unroll this
  //      for(int i = 0; i < 3; i = i + 1) {
  //        x = 42;
  //      }
  //    }
  //    return x;
  //  }
  auto function = new Function("maxNumUnrollings");
  auto innerloop = new For(new VarDecl("i", Types::INT, new LiteralInt(0)),
                           new LogicalExpr(new Variable("i"), LogCompOp::SMALLER, new LiteralInt(3)),
                           new VarAssignm("i",
                                          new ArithmeticExpr(new Variable("i"),
                                                             ArithmeticOp::ADDITION,
                                                             new LiteralInt(1))),
                           new VarAssignm("x", new LiteralInt(42)));
  auto outerloop = new For(new VarDecl("j", Types::INT, new LiteralInt(0)),
                           new LogicalExpr(new Variable("j"), LogCompOp::SMALLER, new LiteralInt(3)),
                           new VarAssignm("j",
                                          new ArithmeticExpr(new Variable("j"),
                                                             ArithmeticOp::ADDITION,
                                                             new LiteralInt(1))),
                           innerloop);
  auto varDecl = new VarDecl("x", Types::INT, new LiteralInt(0));
  auto returnStmt = new Return(new Variable("x"));
  function->addStatement(varDecl);
  function->addStatement(outerloop);
  function->addStatement(returnStmt);
  ast.setRootNode(function);

  // Function: (maxNumUnrollings)	[global]
  //	ParameterList:	[Function_0]
  //	Block:
  //		VarDecl: (x)	[Block_2]
  //			Datatype: (plaintext int)
  //			LiteralInt: (0)
  //		For:
  //			VarDecl: (j)
  //				Datatype: (plaintext int)
  //				LiteralInt: (0)
  //			LogicalExpr:
  //				Variable: (j)
  //				Operator: (<)
  //				LiteralInt: (3)
  //			VarAssignm: (j)
  //				ArithmeticExpr:
  //					Variable: (j)
  //					Operator: (add)
  //					LiteralInt: (1)
  //			Block:	[Block_39]
  //				For:	[Block_39]
  //					VarDecl: (i)
  //						Datatype: (plaintext int)
  //						LiteralInt: (0)
  //					LogicalExpr:
  //						Variable: (i)
  //						Operator: (<)
  //						LiteralInt: (3)
  //					VarAssignm: (i)
  //						ArithmeticExpr:
  //							Variable: (i)
  //							Operator: (add)
  //							LiteralInt: (1)
  //					Block:	[Block_22]
  //						VarAssignm: (x)	[Block_22]
  //							LiteralInt: (42)
  //		Return:	[Block_2]
  //			Variable: (x)

  // perform the compile-time expression simplification
  CompileTimeExpressionSimplifier ctes((CtesConfiguration(1)));
  ctes.visit(ast);
  PrintVisitor p;
  p.visit(ast);

  auto expectedFunc = new Function("maxNumUnrollings");
  auto expectedOuterloop = new For(new VarDecl("j", Types::INT, new LiteralInt(0)),
                                   new LogicalExpr(new Variable("j"), LogCompOp::SMALLER, new LiteralInt(3)),
                                   new Block(new VarAssignm("j",
                                                            new ArithmeticExpr(new Variable("j"),
                                                                               ArithmeticOp::ADDITION,
                                                                               new LiteralInt(1)))),
                                   nullptr); // no content here
  expectedFunc->addStatement(expectedOuterloop);
  expectedFunc->addStatement(returnStmt);

  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
  auto simplifiedAstBody = ast.getRootNode()->castTo<Function>()->getBody();
  EXPECT_TRUE(simplifiedAstBody->isEqual(expectedFunc->getBody()));
}