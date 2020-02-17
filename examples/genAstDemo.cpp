#include "genAstDemo.h"
#include "Call.h"
#include <iostream>
#include "Ast.h"
#include "BinaryExpr.h"
#include "Block.h"
#include "CallExternal.h"
#include "Function.h"
#include "If.h"
#include "LiteralBool.h"
#include "LiteralInt.h"
#include "LiteralString.h"
#include "LogicalExpr.h"
#include "Return.h"
#include "UnaryExpr.h"
#include "VarAssignm.h"
#include "While.h"
#include "MultRewriteVisitor.h"
#include "PrintVisitor.h"

void runInteractiveDemo() {
  // ask which tree to be used for demo
  int treeNo = 0;
  bool isValid = false;
  Ast ast;
  while (!isValid) {
    std::cout << "Choose a demo tree by specifying a number between 1 and 7: ";
    std::cin >> treeNo;

    // generate AST
    std::map<int, std::function<void(Ast &)> > call = {
        {1, generateDemoOne},
        {2, generateDemoTwo},
        {3, generateDemoThree},
        {4, generateDemoFour},
        {5, generateDemoFive},
        {6, generateDemoSix},
        {7, generateDemoSeven}
    };
    auto it = call.find(treeNo);
    if (it == call.end()) {
      std::cout << "Invalid selection! Please choose another value." << std::endl;
    } else {
      isValid = true;
      it->second(ast);
    }
  }

  // this runs 'forever' as the users probably wants to first print the tree, perform some action on it,
  // and afterwards again print the transformed tree
  while (true) {
    // ask which action to perform
    int actionNo = 0;
    std::cout << "Please choose what to do with the generated tree: " << std::endl;
    std::cout << "\t1 Print the tree as indented text." << std::endl;
    std::cout << "\t2 Print the tree as pretty-printed JSON." << std::endl;
    std::cout << "\t3 Perform a simple rewrite operation (A*(B*C)) -> (C*(B*A))." << std::endl;
    std::cout << "Your choice [Press 0 to exit]: ";
    std::cin >> actionNo;

    // perform selected action
    switch (actionNo) {
      case 0: {
        exit(0);
      }
      case 1: {
        PrintVisitor pv;
        pv.visit(ast);
        break;
      }
      case 2: {
        std::cout << ast.getRootNode()->toJson().dump(2) << std::endl;
        break;
      }
      case 3: {
        MultRewriteVisitor mrv;
        mrv.visit(ast);
        break;
      }
      default: {
        exit(0);
      }
    }
  } // while (true)
}

void generateDemoOne(Ast &ast) {
  // int computePrivate(int x) { ... }
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("computePrivate")));
  func->addParameter(new FunctionParameter("int", new Variable("x")));

  // int a = 4;
  func->addStatement(new VarDecl("a", 4));

  // int k;
  func->addStatement(new VarDecl("k", TYPES::INT, nullptr));

  // if ( x > 32 ) { k = x * a; } else { k = (x * a) + 42; }
  func->addStatement(new If(
      // if (x > 32)
      new LogicalExpr(new Variable("x"), OpSymb::LogCompOp::greater, 32),
      // { k = x * a}
      new Block(
          new VarAssignm("k",
                         new BinaryExpr(
                             new Variable("x"),
                             OpSymb::BinaryOp::multiplication,
                             new Variable("a")))),
      // else { k = (x * a) + 42; }
      new Block(
          new VarAssignm("k",
                         new BinaryExpr(
                             new BinaryExpr(
                                 new Variable("x"),
                                 OpSymb::BinaryOp::multiplication,
                                 new Variable("a")),
                             OpSymb::BinaryOp::addition,
                             42)))));

  // return k
  func->addStatement(new Return(new Variable("k")));
}

void generateDemoTwo(Ast &ast) {
  // int determineSuitableX(int encryptedA, int encryptedB) {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("determineSuitableX")));
  func->addParameter(new FunctionParameter("int", new Variable("encryptedA")));
  func->addParameter(new FunctionParameter("int", new Variable("encryptedB")));

  // int randInt = rand() % 42;
  func->addStatement(
      new VarDecl("randInt", TYPES::INT,
                  new BinaryExpr(
                      new CallExternal("std::rand"),
                      OpSymb::BinaryOp::modulo,
                      new LiteralInt(42))));

  // bool b = encryptedA < 2;
  func->addStatement(
      new VarDecl("b", TYPES::BOOL,
                  new LogicalExpr(
                      new Variable("encryptedA"),
                      OpSymb::LogCompOp::smaller,
                      new LiteralInt(2))));

  // int sum = 0;
  func->addStatement(new VarDecl("sum", 0));

  // while (randInt > 0 && !b == true) { sum = sum + encryptedB; randInt--; };
  func->addStatement(new While(
      // (randInt > 0 && !b == true)
      new LogicalExpr(
          // randInt > 0
          new LogicalExpr(
              new Variable("randInt"),
              OpSymb::LogCompOp::greater,
              new LiteralInt(0)),
          OpSymb::LogCompOp::logicalAnd,
          // !b == true
          new LogicalExpr(
              new UnaryExpr(OpSymb::UnaryOp::negation, new Variable("b")),
              OpSymb::LogCompOp::equal,
              new LiteralBool(true))),
      // { sum = sum + encryptedB; randInt--; };
      new Block(
          new std::vector<AbstractStatement*>{
              // sum = sum + encryptedB
              new VarAssignm("sum", new BinaryExpr(
                  new Variable("sum"),
                  OpSymb::BinaryOp::addition,
                  new Variable("encryptedB"))),
              // randInt = randInt-1;
              new VarAssignm("randInt", new BinaryExpr(
                  new Variable("randInt"),
                  OpSymb::BinaryOp::subtraction,
                  new LiteralInt(1)))})));

  // String outStr = "Computation finished!";
  func->addStatement(new VarDecl("outStr", "Computation finished!"));

  // printf(outStr);
  func->addStatement(
      new CallExternal("printf",
                       {new FunctionParameter("string", new Variable("outStr"))}));

  // return sum;
  func->addStatement(
      new Return(new Variable("sum")));
}

void generateDemoThree(Ast &ast) {
  // void computeMult() {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("computeMult")));

  // int a = 3;
  func->addStatement(new VarDecl("a", 3));

  // int b = 7;
  func->addStatement(new VarDecl("b", 7));

  // int c = 9;
  func->addStatement(new VarDecl("c", 9));

  // int result = a * b;
  func->addStatement(
      new VarDecl("result", TYPES::INT,
                  new BinaryExpr(
                      new Variable("a"),
                      OpSymb::BinaryOp::multiplication,
                      new Variable("b"))));

  // result = result * c;
  func->addStatement(
      new VarAssignm("result",
                     new BinaryExpr(
                         new Variable("result"),
                         OpSymb::BinaryOp::multiplication,
                         new Variable("c"))));
}

void generateDemoFour(Ast &ast) {
  // int computeMult() {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("computeMult")));

  // int a = 3;
  func->addStatement(new VarDecl("a", 3));

  // int b = 7;
  func->addStatement(new VarDecl("b", 7));

  // int c = 9;
  func->addStatement(new VarDecl("c", 9));

  // int result = a * b;
  func->addStatement(
      new VarDecl("result", TYPES::INT,
                  new BinaryExpr(
                      new Variable("a"),
                      OpSymb::BinaryOp::multiplication,
                      new Variable("b"))));

  // if (4 > 3) {
  //   int exampleVal = 3
  // }
  func->addStatement(
      new If(
          new LogicalExpr(
              new LiteralInt(4),
              OpSymb::LogCompOp::greater,
              new LiteralInt(3)),
          new VarDecl("exampleVal", 3)));

  // result = result * c;
  func->addStatement(
      new VarAssignm("result",
                     new BinaryExpr(new Variable("result"), OpSymb::BinaryOp::multiplication, new Variable("c"))));
}

void generateDemoFive(Ast &ast) {
  // int computeMult() {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("multiMult")));

  // int result = (inA * (inB * inC));
  func->addStatement(new VarDecl("result", TYPES::INT,
                                 new BinaryExpr(
                                     new Variable("inA"),
                                     OpSymb::multiplication,
                                     new BinaryExpr(
                                         new Variable("inB"),
                                         OpSymb::multiplication,
                                         new Variable("inC")))));

  // return result;
  func->addStatement(new Return(new Variable("result")));
}

void generateDemoSix(Ast &ast) {
  //(Z * (A * (B * C))) --demo rewriter--> ((B * C) * (A * Z))
  // int computeMult() {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("multiMult")));

  // int result = (inZ * (inA * (inB * inC)));
  func->addStatement(new VarDecl("result", TYPES::INT,
                                 new BinaryExpr(new Variable("inZ"),
                                                OpSymb::multiplication,
                                                new BinaryExpr(
                                                    new Variable("inA"),
                                                    OpSymb::multiplication,
                                                    new BinaryExpr(
                                                        new Variable("inB"),
                                                        OpSymb::multiplication,
                                                        new Variable("inC"))))));


  // return result;
  func->addStatement(new Return(new Variable("result")));
}

void generateDemoSeven(Ast &ast) {
  // int determineSuitableX(int encryptedA, int encryptedB) {...}
  Function* func = dynamic_cast<Function*>(ast.setRootNode(new Function("determineSuitableX")));
  func->addParameter(new FunctionParameter("int", new Variable("encryptedA")));
  func->addParameter(new FunctionParameter("int", new Variable("encryptedB")));

  // int sum = encryptedA + encryptedB;
  func->addStatement(
      new VarDecl("sum", TYPES::INT,
                  new BinaryExpr(
                      new Variable("encryptedA"),
                      OpSymb::BinaryOp::addition,
                      new Variable("encryptedB"))));

  // return sum;
  func->addStatement(
      new Return(new Variable("sum")));
}
