#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/Call.h"
#include "ast_opt/ast/For.h"
#include "ast_opt/ast/Function.h"
#include "ast_opt/ast/If.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/visitor/BatchingVisitor.h"
#include "ast_opt/visitor/ProgramPrintVisitor.h"
#include "ast_opt/parser/Parser.h"
#include "gtest/gtest.h"

class BatchingTest : public ::testing::Test {
 protected:
  std::unique_ptr<AbstractNode> ast;
  BatchingVisitor bv;
  std::stringstream ss;
  ProgramPrintVisitor ppv = ProgramPrintVisitor(ss);
  BatchingTest() = default;
};

TEST_F(BatchingTest, setConstantValueAllSlots) {
  /// Input program
  const char *programCode = R""""(
  public int compute(secret int vec) {
    vec[0] = 5;
    vec[1] = 5;
    vec[2] = 5;
    vec[3] = 5;
    return vec;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(bv);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret int vec)
  {
    return {5,5,5,5};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}