#ifndef MASTER_THESIS_CODE_ASTTESTINGGENERATOR_H
#define MASTER_THESIS_CODE_ASTTESTINGGENERATOR_H

#include "Ast.h"

/// The ASTs generated by this class are used for tests. Hence, any modifications will break the tests!
class AstTestingGenerator {
 public:
  /// Generates a new demo AST.
  /// \param id The ID of the demo AST to generate.
  /// \param ast The AST object in which the demo AST is written into.
  static void generateAst(int id, Ast &ast);

  /// Returns the largest ID that is supported by method generateAst.
  /// \return An integer specifying the largest ID supported by generateAst.
  static size_t getLargestId();

  // Methods to generate sample ASTs to be used for tests.
  static void genSuperSimpleAst(Ast &ast);
  static void genAstRewritingOne(Ast &ast);
  static void genAstRewritingTwo(Ast &ast);
  static void genAstRewritingThree(Ast &ast);
  static void genAstRewritingFour(Ast &ast);
  static void genAstRewritingFive(Ast &ast);
  static void genAstRewritingSix(Ast &ast);
  static void genAstEvalOne(Ast &ast);
  static void genAstEvalTwo(Ast &ast);
  static void genAstEvalThree(Ast &ast);
  static void genAstEvalFour(Ast &ast);
  static void genAstEvalFive(Ast &ast);
  static void genAstEvalSix(Ast &ast);
  static void genAstEvalSeven(Ast &ast);
  static void genAstPrintVisitorOne(Ast &ast);
  static void genAstPrintVisitorTwo(Ast &ast);
  static void genAstMultDepthOne(Ast &ast);
  static void genAstMultDepthTwo(Ast &ast);
  static void genAstRewritingSimple(Ast &ast);
  static void genAstRewritingSimpleExtended(Ast &ast);
  static void genAstRewritingMultiInputY(Ast &ast);
  static void genAstRewritingTwoDepth2ConesButSingleVNode(Ast &ast);
  static void genAstForSecretTaintingWithMultipleNonSequentialStatements(Ast &ast);
};

#endif //MASTER_THESIS_CODE_ASTTESTINGGENERATOR_H
