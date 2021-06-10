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
  BatchingVisitor bv = BatchingVisitor();
  std::stringstream ss;
  ProgramPrintVisitor ppv = ProgramPrintVisitor(ss);
  BatchingTest() = default;
};

TEST_F(BatchingTest, setSameValueAllSlots) {
  /// Input program
  const char *programCode = R""""(
    secret int vec;
    int x;
    vec[0] = x;
    vec[1] = x;
    vec[2] = x;
    vec[3] = x;
    return x;
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
    vec = x;
    return vec;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}