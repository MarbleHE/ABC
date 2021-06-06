#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/Call.h"
#include "ast_opt/ast/For.h"
#include "ast_opt/ast/Function.h"
#include "ast_opt/ast/If.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/visitor/ProgramTransformationVisitor.h"
#include "ast_opt/visitor/ProgramPrintVisitor.h"
#include "ast_opt/parser/Parser.h"
#include "gtest/gtest.h"

class ProgramTransformationVisitorTest : public ::testing::Test {
 protected:
  std::unique_ptr<AbstractNode> ast;
  ProgramTransformationVisitor ctes;
  std::stringstream ss;
  ProgramPrintVisitor ppv = ProgramPrintVisitor(ss);
  ProgramTransformationVisitorTest() = default;
};

TEST_F(ProgramTransformationVisitorTest, arithmeticExpr_literalsOnly_fullyEvaluable) {
  /// Input program
  const char *programCode = R""""(
  public int compute() {
    return 22 * 11;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 242;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, arithmeticExpr_variableUnknown_rhsOperandEvaluableOnly) {
  /// Input program
  const char *programCode = R""""(
  public int compute(secret int encryptedA) {
    return encryptedA * (4*7);
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret int encryptedA)
  {
    return (encryptedA * 28);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, arithmeticExpr_variableKnown_fullyEvaluable) {

  /// Input program
  const char *programCode = R""""(
   public int compute() {
     int parameterA = 43;
     return parameterA * (4*7);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 1204;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, arithmeticExpr_variablesUnknown_notAnythingEvaluable) {

  /// Input program
  const char *programCode = R""""(
  public int compute(secret int encryptedA, int plaintextB) {
      return encryptedA * (4*plaintextB);
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret int encryptedA, int plaintextB)
  {
    return (encryptedA * (4 * plaintextB));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, logicalExpr_literalsOnly_fullyEvaluable) {

  /// Input program
  const char *programCode = R""""(
  public int compute() {
    return true && false;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return false;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, DISABLED_logicalExpr_variableUnknown_lhsOperandEvaluableOnly) {
  //TODO: Implement short-circuiting logic
  /// Input program
  const char *programCode = R""""(
  public int compute(secret bool encryptedA) {
    return (true ^ false) || encryptedA;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool encryptedA)
  {
    return true;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, logicalExpr_variableKnown_fullyEvaluable) {
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    bool parameterA = true;
    return parameterA || (false && true);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return true;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, logicalExpr_variablesUnknown_notAnythingEvaluable) {
  //  -- input --
  // public int compute(secret bool encryptedA, bool paramB) {
  //  bool alpha = encryptedA && (true ^ paramB);
  // }
  //  -- expected --
  // variableValues['alpha'] = encryptedA && (true ^ paramB)
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool encryptedA, bool paramB) {
      return encryptedA && (true ^ paramB);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool encryptedA, bool paramB)
  {
    return (encryptedA && (true ^ paramB));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, unaryExpr_literalsOnly_fullyEvaluable) {
  // public int compute() {
  //  bool truthValue = !false;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      return !false;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return true;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, unaryExpr_variableKnown_fullyEvaluable) {
  // public int compute() {
  //  bool alpha = true;
  //  bool beta = !alpha;
  // }

  /// Input program
  const char *programCode = R""""(
   public int compute() {
      bool alpha = true;
      return !alpha;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return false;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, unaryExpr_variableUnknown_notEvaluable) {
  // public int compute(bool paramA) {
  //  bool beta = !paramA;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(bool paramA) {
    return !paramA;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(bool paramA)
  {
    return !paramA;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, varAssignm_variablesKnown_fullyEvaluable) {
  // public int compute() {
  //  float alpha = 1.23;
  //  alpha = 2.75;
  //  float beta = alpha;
  // }

  /// Input program
  const char *programCode = R""""(
   public int compute() {
      float alpha = 1.23;
      alpha = 2.75;
      float beta = alpha;
      return beta;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 2.75;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, varAssignm_previouslyDeclaredNonInitializedVariable) {
  // public int compute() {
  //  float alpha;
  //  alpha = 2.95;
  // }

  /// Input program
  const char *programCode = R""""(
   public int compute() {
      float alpha;
      alpha = 2.95;
      return alpha;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 2.95;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, varAssignm_assignmentToParameter) {
  // public int compute(float alpha) {
  //  alpha = 42.24;
  // }

  /// Input program
  const char *programCode = R""""(
   public int compute(float alpha) {
      alpha = 42.24;
      return alpha;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(float alpha)
  {
    return 42.24;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_varAssignm_symbolicTerms_circularDependency) {
  //TODO: Revisit once OperatorExpression support is added

  //  -- input --
  // int Foo(int x, int y) {
  //  x = y+3
  //  y = x+2
  //  return x+y
  // }
  //  -- expected --
  // int Foo(int x, int y) {
  //  return y+y+8
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int x, int y) {
      x = y+3;
      y = x+2;
      return x+y;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int x, int y)
  {
    return +(y, y, 8);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, return_literalOnly_expectedNoChange) {
  // float compute() {
  //  return 42.24;
  // }
  // -- expected –-
  // no change as cannot be simplified further
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    return 42.24;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 42.24;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       return_variableKnown_expectedSubstitutionAndStatementDeletion) {
  // int compute() {
  //  int b = 23;
  //  return b;
  // }
  // -- expected --
  // int compute() {
  //  return 23;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      int b = 23;
      return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 23;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       return_variableAndArithmeticExpressionKnown_expectedLiteralIntReturnValue) {
  // int compute() {
  //  int b = 23;
  //  return b + 99;
  // }
  // -- expected --
  // int compute() {
  //  return 122;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    int b = 23;
    return b + 99;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 122;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, return_variableUnknown_expectedNoChange) {
  // int compute(int b) {
  //  return b + 99;
  // }
  // -- expected --
  // no change
  /// Input program
  const char *programCode = R""""(
   public int compute(int b) {
      return b + 99;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int b)
  {
    return (b + 99);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, return_multipleReturnValues_expectedPartlyEvaluation) {
  // int,int,int compute(int a) {
  //  int b = 3 + 4;
  //  return a*b, 2-b, 21;
  // }
  // -- expected --
  // int,int,int compute(int a) {
  //  return a*7, -5, 21;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int a) {
    int b = 3 + 4;
    return {a*b, 2-b, 21};
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return {(a * 7), -5, 21};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}


TEST_F(ProgramTransformationVisitorTest, indexAccesses) {

/// Input program
  const char *programCode = R""""(
  public int compute(int img) {
    img[5] = 2;
    return img[5];
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img)
  {
    return 2;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}


TEST_F(ProgramTransformationVisitorTest, variableEliminated) {

/// Input program
  const char *programCode = R""""(
  public int compute(int img) {
    int img2;
    img2[5] = 2 * img[1];
    return img2;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img)
  {
    return {-, -, -, -, -, (2 * img[1])};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, complexVariableEliminated) {
//TODO: Once we have a better way of dealing with declaration tracking, update this test
/// Input program
  const char *programCode = R""""(
  public int compute(int img, int x) {
    int img2;
    img2[x] = 2 * img[x];
    return img2;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img, int x)
  {
    img2[x] = (2 * img[x]);
    return img2;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsKnown_thenIsAlwaysExecutedNoElseIsPresent_expectedIfRemoval) {
  //TODO: Implement IfStatement rewriting

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
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    int a = 512;
    int b = 22;
    if (b+12 > 20) {
      a = a*2;
    }
    return a*32;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 32768;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsKnown_thenIsAlwaysExecutedAndElseIsPresent_expectedIfRemoval) {

  //TODO: Implement IfStatement rewriting

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
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      int a = 512;
      int b = 22;
      if (b+12 > 20) {
        a = a*2;
      } else {
        a = 1;
      }
      return a*32;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 32768;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsKnown_elseIsAlwaysExecuted_expectedIfRemoval) {
  //TODO: Implement IfStatement rewriting

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
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    int a = 512;
    int b = 22;
    if (b+12 < 20) {
      a = a*2;
    } else {
      a = 1;
    }
    return a*32;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 32;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_thenBranchEvaluable_expectedRewriting) {
  //TODO: Implement IfStatement rewriting

  //  -- input --
  //  int compute(int a) {
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
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      int b = 22;
      if (a > 20) {
        b = 2*b;
      }
      return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return (((a > 20) *** 44) + ((1 - (a > 20)) *** 22));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_expectedRemovalOfElseClauseInResultBecauseVariableBIsNull) {
  //TODO: Implement IfStatement rewriting


  //  -- input --
  //  int compute(int a) {
  //    int b = 0;
  //    if (a > 20) {
  //      int c = 642;
  //      b = 2*c-1;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [a>20]*1'283;
  //  }
  /// Input program
  const char *programCode = R""""(
   public int compute(int a) {
      int b = 0;
      if (a > 20) {
        int c = 642;
        b = 2*c-1;
      }
      return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return ((a > 20) *** 1283);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsUnknown_thenBranchOnlyExists_varDeclInThenBranch_expectedRewritingOfIfStatement) {
  //TODO: Implement IfStatement rewriting


  //  -- input --
  //  int compute(int a) {
  //    int b = 42;
  //    if (a > 20) {
  //      int c = 642;
  //      b = 2*c-1;
  //    }
  //    return b;
  //  }
  //  -- expected --
  //  int compute() {
  //    return [a>20]*1'283+[1-[a>20]]*42;
  //  }
  /// Input program
  const char *programCode = R""""(
   public int compute(int a) {
      int b = 42;
      if (a > 20) {
        int c = 642;
        b = 2*c-1;
      }
      return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return (((a > 20) *** 1283) + ((1 - (a > 20)) *** 42));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_conditionValueIsUnknown_thenAndElseExists_returnValueIsInputVariable_expectedRewritingOfIfStatement) {
  //TODO: Implement IfStatement rewriting

  //  -- input --
  //  int compute(int factor, int threshold) {
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
  /// Input program
  const char *programCode = R""""(
   public int compute(int factor, int threshold) {
      int b;
      if (threshold < 11) {
        b = 2*factor;
      } else {
        b = factor;
      }
      return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return (((threshold < 11) *** (2 * factor)) + ((1 - (threshold < 11)) * factor));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest,
       DISABLED_ifStmt_nestedIfStatements_expectedRewritingOfBothIfStatement) {
  //TODO: Implement IfStatement rewriting
  //TODO: Update Test

  //  -- input --
  //  int compute(int factor, int threshold) {
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
  /// Input program
  const char *programCode = R""""(
   public int compute() {
    int b = 99;
    if (threshold > 11) {
        b = b/3;
      if (factor > 9) {
        b = b*2*factor;
      } else {
        b = b*factor;
      }
    }
    return b;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  // TODO: still in old style
  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return [factor>9]*[threshold>11]*66*factor + [1-[factor>9]]*[threshold>11]*33*factor + [1-[threshold>11]]*99;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_partiallyEvaluableOnly) {

  //  -- input --
  // int f(int x) {
  //  int y = 42;
  //  x = x+29
  //  return x+y
  // }
  //  -- expected --
  // int f(int x) {
  //  return x+71;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int x) {
       int y = 42;
       x = x+29;
       return x+y;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int x)
  {
    return +(x, 71);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_includingOperatorExprs) {

  //  -- input --
  // int f(int x, int a) {
  //  int y = 42+34+x+a;  // 76+x+a
  //  int x = 11+29;  // 40
  //  return x+y
  // }
  //  -- expected --
  // int f(int x) {
  //  return 116+x+a;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      int y = 42+34+x+a;
      int x = 11+29;
      return x+y;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return  +(116, x, a);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_nestedDivisionOperator) {

  //  -- input --
  // int f(int a) {
  //  return 9 + (34 + (22 / (a / (11 * 42))));
  // }
  //  -- expected --
  // int f(int a) {
  //  return 43 + (22 / (a / 462));
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int a) {
      return 9 + (34 + (22 / (a / (11 * 42))));
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return  +(43, (22 / (a / 462)));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_nestedOperatorsSimplifiableOnOneSideOnly) {

  //  -- input --
  // int f(int a) {
  //  return (4 + (3 + 8) - (a / (2 * 4))
  // }
  //  -- expected --
  // int f(int a) {
  //  return 15 - (a / 8)
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      return (4 + (3 + 8) - (a / (2 * 4)));
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return (15 - (a / 8));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_nestedLogicalOperators) {

  //  -- input --
  // int f(secret bool a, bool b, secret bool c) {
  //  return (a ^ (b ^ false)) && ((true || false) || true)
  // }
  //  -- expected --
  // int f(int a) {
  //  return a ^ b;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b, secret bool c) {
    return (a ^ (b ^ false)) && ((true || false) || true);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b, secret bool c)
  {
    return (a ^ b);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));

}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_ANDtrue) {

  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a && (true && b);
  // }
  //  -- expected --
  // int f(int a) {
  //  return a && b;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
    return a && (true && b);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b)
  {
    return (a && b);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_ANDfalse) {
  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a && (false && b);
  // }
  //  -- expected --
  // int f(int a) {
  //  return false;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
      return a && (false && b);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b)
  {
    return false;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_ORfalse) {

  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a || (b || false);
  // }
  //  -- expected --
  // int f(int a) {
  //  return a || b;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
      return a || (b || false);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b)
  {
    return (a || b);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_ORtrue) {
  //TODO: Update Test

  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a || (b || true);
  // }
  //  -- expected --
  // int f(int a) {
  //  return true;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
      return a || (b || true);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a, bool b)
  {
    return true;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_XORtrue) {

  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a ^ (b ^ true);
  // }
  //  -- expected --
  // int f(int a) {
  //  return a ^ !b;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
      return a ^ (b ^ true);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b)
  {
    return (a ^ !b);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, symbolicTerms_logicalAndSimplification_XORfalse) {

  //  -- input --
  // int f(secret bool a, bool b) {
  //  return a ^ (b ^ false);
  // }
  //  -- expected --
  // int f(int a) {
  //  return a ^ b;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(secret bool a, bool b) {
      return a ^ (b ^ false);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(secret bool a, bool b)
  {
    return (a ^ b);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_WhileLoop_compileTimeKnownExpression_removalExpected) {
  // TODO: Revisit once While loop support is added

  //  -- input --
  // int f(int a) {
  //  int i = 2;
  //  while (i > 10) {
  //    a = a * a;
  //    i = i-1;
  //  }
  //  return a;
  // }
  //  -- expected --
  // int f(int a) {
  //  return a;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int a) {
      int i = 2;
      while (i > 10) {
        a = a * a;
        i = i-1;
      }
      return a;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return a;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_Call_inliningExpected) {
  // TODO: Calls are currently not supported by the compiler
  //  -- input --
  // int f(int a) {
  //  return computeX(a);       --> int computeX(int x) { return x + 111; }
  // }
  //  -- expected --
  // int f(int a) {
  //  return a + 111;
  // }
  /// Input program
  const char *programCode = R""""(
   public int computeX(int x) {
      return x + 111;
   }

   public int compute() {
      return computeX(a);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int a)
  {
    return (a + 11);
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_Call_inliningExpected2) {
  //TODO: Re-enable once arguments for Call are implemnted correctly
  //  -- input --
  // int f() {
  //  return computeX(232);       --> int computeX(int x) { return x + 111; }
  // }
  //  -- expected --
  // int f() {
  //  return 343;
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_Rotate_executionExpected) {
  //TODO: Rotation syntax has changed, update once its done!

  //  -- input --
  // rotateVec(int inputA) {
  //   int sumVec = {{1, 7, 3}};   // [1 7 3]
  //   return sumVec.rotate(1);    // [3 1 7]
  // }
  //  -- expected --
  // rotateVec(int inputA) {
  //   return LiteralInt{{3, 1, 7}};
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute(int inputA) {
      int sumVec = {{1, 7, 3}};
      return sumVec.rotate(1);
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return {{3, 1, 7}};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_transpose) {
  //TODO: Matrices currently not supported

  //  -- input --
  // transposeMatrix() {
  //   return [11 2 3; 4 2 3; 2 1 3].transpose();
  // }
  //  -- expected --
  // transposeMatrix() {
  //   return [11 4 2; 2 2 1; 3 3 3];
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_getMatrixElementSimpleBool) {
  //TODO: Matrices currently not supported

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
  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_partiallySimplifiableMatrix) {
  //TODO: Matrices currently not supported

  // -- input –-
  // extractArbitraryMatrixElements(bool y {
  //   int M = [ true y false ];
  //   return [ M[0][1];      // y
  //            M[0][0];      // true
  //            M[0][2] ];    // false
  // }
  // -- expected –-
  // extractArbitraryMatrixElements {
  //   return [ y; LiteralBool(true); LiteralBool(false) ];
  // }
  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_partialforLoopUnrolling) {

  //TODO: Re-enable once partial loop unrolling is re-implemented

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
  /// Input program
  const char *programCode = R""""(
   public int compute(int numIterations) {
      int M = {54, 32, 63, 38, 13, 20};
      int sum = 0;
      for (int i = 0; i < numIterations; i++) {
        sum = sum + M[i];
      }
      return sum;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  //TODO: Update with what the output should really be
  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int numIterations)
  {
    int M = {54, 32, 63, 38, 13, 20};
    int sum;
    int i;
    ....
    return sum;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}


TEST_F(ProgramTransformationVisitorTest, trivialLoop) {
/// Input program
  const char *programCode = R""""(
   public int compute() {
      int x = 0;
      for(int i = 0; i < 3; i = i + 1) {
        x = 42;
      }
      return x;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 42;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}


TEST_F(ProgramTransformationVisitorTest, complexLoop) {
  //TODO: Once OperatorExpressions are implemented, this can be simplified even further

/// Input program
  const char *programCode = R""""(
  public int compute(int img, int imgSize) {
    int img2;
    for (int i = 0; i < 2; i = i + 1) {
      img2[i] = img[i+1];
    }
    return img2;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img, int imgSize)
  {
    return {img[1], img[2]};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, foldLoop) {
  //TODO: Once OperatorExpressions are implemented, this can be simplified even further

/// Input program
  const char *programCode = R""""(
  public int compute(int x) {
    int sum = 0;
    for (int i = 1; i < 3; i = i + 1) {
      sum = sum + i*x;
    }
    return sum;
  }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int x)
  {
    return ((((((0 + (1 * x)) + (1 * x)) + (2 * x)) + (2 * x)) + (2 * x)) + (3 * x));
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, trivialNestedLoops) {
  //  int trivialLoop() {
  //    int x = 0;
  //    for(int j = 0; j < 3; j = j + 1) {
  //      for(int i = 0; i < 3; i = i + 1) {
  //        x = 42;
  //      }
  //    }
  //    return x;
  //  }
  /// Input program
  const char *programCode = R""""(
   public int compute() {
      int x = 0;
      for(int j = 0; j < 3; j = j + 1) {
        for(int i = 0; i < 3; i = i + 1) {
          x = 42;
        }
      }
      return x;
    }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return 42;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, nestedFold) {
  /// Input program
  const char *programCode = R""""(
   public int compute(int img, int imgSize) {
      int sum = 0;
      for (int j = 0; j < 3; j = j + 1) {
         for (int i = 0; i < 3; i = i + 1) {
            sum = sum + img[imgSize*i+j];
         }
      }
      return sum;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  //TODO: add expected code
  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img, int imgSize, int x, int y)
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, complexNestedLoops) {
  /// Input program
  const char *programCode = R""""(
   public int compute(int img) {
      int x;
      for(int j = 0; j < 2; j = j + 1) {
        for(int i = 0; i < 2; i = i + 1) {
          x[i+j] = img[i + j];
        }
      }
      return x;
    }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img)
  {
    return {img[0], img[1], img[2]};
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, fullForLoopUnrolling) {
  /// Input program
  const char *programCode = R""""(
   public int compute(int img, int imgSize, int x, int y) {
      int weightMatrix = {1, 1, 1, 1, -8, 1, 1, 1, 1};
      int img2;
      int value = 0;
      for (int j = -1; j < 2; j = j + 1) {
         for (int i = -1; i < 2; i = i + 1) {
            value = value + weightMatrix[(i+1)*3 + j+1] * img[imgSize*(x+i)+y+j];
         }
      }
      img2[imgSize*x+y] = 2 * img[imgSize*x+y] - (value);
      return img2;
   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute(int img, int imgSize, int x, int y)
  {
    int img2;
    img2[((imgSize * x) + y)] = img[((imgSize * x) + y)] - (img[((imgSize * (x - 1)) + (y - 1))] + img[imgSize*x+y-1]        + img[imgSize*(x+1)+y-1]
          + img[imgSize*(x-1)+y]   + img[imgSize*x+y] * (-8)   + img[imgSize*(x+1)+y]
          + img[imgSize*(x-1)+y+1] + img[imgSize*x+y+1]        + img[imgSize*(x+1)+y+1]  ) / 2;
    return img2;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, fourNestedLoopsLaplacianSharpeningFilter) {
  //TODO: Update Test

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
  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

TEST_F(ProgramTransformationVisitorTest, DISABLED_maxNumUnrollings) {
  //TODO: Update Test and re-enable once there's an easy way to set maxUnrollings in the visitor

  //  int maxNumUnrollings() {
  //    int x = 0;
  //    for(int j = 0; j < 3; j = j + 1) { //do not unroll this
  //      for(int i = 0; i < 3; i = i + 1) {
  //        x = 42;
  //      }
  //    }
  //    return x;
  //  }

  /// Input program
  const char *programCode = R""""(
   public int compute() {

   }
)"""";
  auto code = std::string(programCode);
  ast = Parser::parse(code);

  // perform the compile-time expression simplification
  ast->accept(ctes);

  // get the transformed code
  ast->accept(ppv);
  std::cout << ss.str() << std::endl;

  /// Expected program
  const char *expectedCode = R""""(
{
  int compute()
  {
    return;
  }
}
)"""";

  EXPECT_EQ("\n" + ss.str(), std::string(expectedCode));
}

//TODO: Update AstTestingGenerator based tests
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_fullyEvaluable) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(35, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new LiteralInt(77);
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_partiallyEvaluable) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(37, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new OperatorExpr(new Operator(MULTIPLICATION), {new Variable("a"), new LiteralInt(1054)});
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalAndFalse) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(38, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new LiteralBool(false);
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalAndTrue_oneRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(39, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new Variable("a");
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalAndTrue_twoRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(40, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_AND), {new Variable("b"), new Variable("a")});
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalOrTrue) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(41, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new LiteralBool(true);
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalOrFalse_oneRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(42, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new Variable("a");
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalOrFalse_twoRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(43, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_OR), {new Variable("b"), new Variable("a")});
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalXorTrue) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(44, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new LiteralBool(true)});
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalXorFalse_oneRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(45, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new Variable("a");
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, operatorExpr_logicalXorFalse_twoRemainingOperand) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(46, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedReturnExpr = new OperatorExpr(new Operator(LOGICAL_XOR), {new Variable("a"), new Variable("b")});
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(expectedReturnExpr));
//}
//
//TEST_F(ProgramTransformationVisitorTest, nestedOperatorExprsTest) { 
//  auto func = new Function("sumVectorElements");
//  // (27 / a / 12 / 3 / 1) or in prefix notation: (/ 27 (/ a 12) 3 1)
//  auto opExpr0 = new OperatorExpr(new Operator(DIVISION), {new Variable("a"), new LiteralInt(12)});
//  auto opExpr1 =
//      new OperatorExpr(new Operator(DIVISION), {new LiteralInt(27), opExpr0, new LiteralInt(3), new LiteralInt(1)});
//  auto ret = new Return(opExpr1);
//  func->addStatement(ret);
//  func->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("a")));
//
//  Ast ast;
//  ast.setRootNode(func);
//  ctes.visit(ast);
//
//  // expected result: (/ 27 a 4)
//  auto returnStatement = ast.getRootNode()->castTo<Function>()->getBodyStatements().back()->castTo<Return>();
//  EXPECT_TRUE(returnStatement->getReturnExpressions().front()->isEqual(
//      new OperatorExpr(new Operator(DIVISION), {new LiteralInt(27), new Variable("a"), new LiteralInt(4)})));
//}
//
//TEST_F(ProgramTransformationVisitorTest, getMatrixSizeOfKnownMatrix) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(52, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunction = new Function("returnLastVectorElement");
//  expectedFunction->addStatement(new Return(new LiteralInt(44)));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, getMatrixSizeOfAbstractMatrix) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(53, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunction = new Function("getNumElementsPerDimension");
//  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("factor")));
//  expectedFunction->addStatement(new Return(
//      new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt(1), new LiteralInt(5), new LiteralInt(0)}}))));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, getMatrixSizeOfUnknownMatrix) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(54, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunction = new Function("getNumElementsPerDimension");
//  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("inputMatrix")));
//  expectedFunction->addParameter(new FunctionParameter(new Datatype(Types::INT), new Variable("dimension")));
//  expectedFunction
//      ->addStatement(new Return(new GetMatrixSize(new Variable("inputMatrix"), new Variable("dimension"))));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, nestedFullLoopUnrolling_matrixAssignmAndGetMatrixSize) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(55, ast);
//  // Matrix<int> extendMatrixAddingElements() {
//  //   Matrix<int> m;   // size: 0x0
//  //   for (int i = 0; i < 3; ++i) {
//  //     Vector<int> t;
//  //     for (int j = 0; j < 3; ++j) {
//  //       t[0][t.dimSize(1)] = i*j;
//  //     }
//  //     m[m.dimSize(0)] = t;
//  //   }
//  //   return m;  // m = [0*0 0*1 0*2; 1*0 1*1 1*2; 2*0 2*1 2*2] = [0 0 0; 0 1 2; 0 2 4], size: 3x3
//  // }
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunction = new Function("extendMatrixAddingElements");
//  expectedFunction->addStatement(new Return(new LiteralInt(
//      new Matrix<AbstractExpression *>({{new LiteralInt(0), new LiteralInt(0), new LiteralInt(0)},
//                                        {new LiteralInt(0), new LiteralInt(1), new LiteralInt(2)},
//                                        {new LiteralInt(0), new LiteralInt(2), new LiteralInt(4)}}))));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, matrixAssignmIncludingPushBack) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(59, ast);
//
//  // Matrix<int> extendMatrixAddingElements() {
//  //  Matrix<int> m;   // size: 0x0
//  //  Vector<int> t;
//  //  for (int i = 0; i < 3; ++i) {
//  //    t[0][t.dimSize(1)] = i*i;
//  //  }
//  //  m[m.dimSize(0)] = t;
//  //  return m;  // m = [0*0 1*1 2*2] = [0 1 4], size: 1x3
//  //}
//
//  // perform the compile-time expression simplification
//  PrintVisitor p;
//  p.visit(ast);
//  ctes.visit(ast);
//  p.visit(ast);
//
//  auto expectedFunction = new Function("extendMatrixAddingElements");
//  expectedFunction->addStatement(
//      new Return(new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt(0),
//                                                                   new LiteralInt(1),
//                                                                   new LiteralInt(4)}}))));
//  auto expectedAst = Ast(expectedFunction);
//  p.visit(expectedAst);
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunction->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, matrixAssignmentUnknownThenKnown) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(56, ast);
//
//  // public int computeMatrix(int k, int a) {
//  //   Matrix<int> M;
//  //   M[k][0] = 4;
//  //   M[0][0] = 21 + a;
//  //   return M;
//  // }
//
//  // perform the compile-time expression simplification
//  PrintVisitor p;
//  p.visit(ast);
//  ctes.visit(ast);
//  p.visit(ast);
//
//  auto expectedFunc = new Function("computeMatrix");
//  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("k")));
//  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("a")));
//  expectedFunc->addStatement(new VarDecl("M", new Datatype(Types::INT, false), new LiteralInt()));
//  expectedFunc->addStatement(new MatrixAssignm(
//      new MatrixElementRef(new Variable("M"), new Variable("k"), new LiteralInt(0)), new LiteralInt(4)));
//  expectedFunc->addStatement(new MatrixAssignm(new MatrixElementRef(new Variable("M"), 0, 0),
//                                               new OperatorExpr(new Operator(ADDITION),
//                                                                {new Variable("a"), new LiteralInt(21)})));
//  expectedFunc->addStatement(new Return(new Variable("M")));
//  auto expectedAst = Ast(expectedFunc);
//  p.visit(expectedAst);
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, matrixAssignmentKnownThenUnknown) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(57, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunc = new Function("computeMatrix");
//  expectedFunc->addParameter(new FunctionParameter(new Datatype(Types::INT, false), new Variable("k")));
//  expectedFunc->addStatement(new VarDecl("M", new Datatype(Types::INT, false)));
//  // This assignment may look weird but is expected because matrix M lacks an initialization and as such is
//  // default-initialized as Matrix<AbstractExpr*>, hence we need to store integers as LiteralInts
//  expectedFunc->addStatement(new VarAssignm("M",
//                                            new LiteralInt(new Matrix<AbstractExpression *>({{new LiteralInt((21))}}))));
//  expectedFunc->addStatement(new MatrixAssignm(
//      new MatrixElementRef(new Variable("M"), new LiteralInt(0), new Variable("k")), new LiteralInt(4)));
//  expectedFunc->addStatement(new Return(new Variable("M")));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
//}
//
//TEST_F(ProgramTransformationVisitorTest, fullAssignmentToMatrix) { 
//  Ast ast;
//  AstTestingGenerator::generateAst(58, ast);
//
//  // perform the compile-time expression simplification
//  ctes.visit(ast);
//
//  auto expectedFunc = new Function("computeMatrix");
//  expectedFunc->addStatement(new Return(new LiteralInt(
//      new Matrix<AbstractExpression *>({{new LiteralInt(11), new LiteralInt(1), new LiteralInt(1)},
//                                        {new LiteralInt(3), new LiteralInt(2), new LiteralInt(2)}}))));
//
//  // get the body of the AST on that the CompileTimeExpressionSimplifier was applied on
//  auto simplifiedAst = ast.getRootNode()->castTo<Function>()->getBody();
//  EXPECT_TRUE(simplifiedAst->isEqual(expectedFunc->getBody()));
//}