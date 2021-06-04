#include <memory>
#include <stack>
#include <utility>

#include "ast_opt/ast/AbstractNode.h"
#include "ast_opt/ast/AbstractStatement.h"
#include "ast_opt/ast/AbstractExpression.h"
#include "ast_opt/ast/AbstractTarget.h"
#include "ast_opt/ast/BinaryExpression.h"
#include "ast_opt/ast/Block.h"
#include "ast_opt/ast/Call.h"
#include "ast_opt/ast/ExpressionList.h"
#include "ast_opt/ast/For.h"
#include "ast_opt/ast/Function.h"
#include "ast_opt/ast/FunctionParameter.h"
#include "ast_opt/ast/If.h"
#include "ast_opt/ast/IndexAccess.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/ast/UnaryExpression.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/ast/Assignment.h"
#include "ast_opt/ast/VariableDeclaration.h"
#include "ast_opt/parser/Errors.h"
#include "ast_opt/parser/Parser.h"
#include "ast_opt/parser/PushBackStream.h"
#include "../../test/parser/ParserTestHelpers.h"
#include "ast_opt/visitor/ParentSettingVisitor.h"

using std::to_string;

void addParsedNode(AbstractNode *parsedNode) {
  parsedNodes.push_back(std::ref(*parsedNode));
}

std::unique_ptr<AbstractNode> Parser::parse(std::string s) {
  parsedNodes.clear();

  // Setup Tokenizer from String
  auto getCharacter = stork::getCharacterFunc(s);
  stork::PushBackStream stream(&getCharacter);
  stork::tokens_iterator it(stream);

  auto block = std::make_unique<Block>();
  // Parse statements until end of file
  while (!it->isEof()) {
    auto statement = std::unique_ptr<AbstractStatement>(parseStatement(it));
    block->appendStatement(std::move(statement));
  }

  // TODO: Remove this workaround once parser sets parents properly
  ParentSettingVisitor p;
  block->accept(p);

  return std::move(block);
}

std::unique_ptr<AbstractNode> Parser::parse(std::string s,
                                            std::vector<std::reference_wrapper<AbstractNode>> &createdNodesList) {
  auto result = parse(std::move(s));
  createdNodesList = std::move(parsedNodes);
  return result;
}

AbstractStatement *Parser::parseStatement(stork::tokens_iterator &it, bool gobbleTrailingSemicolon) {
  AbstractStatement *parsedStatement;
  if (it->isReservedToken()) {
    switch (it->get_reserved_token()) {
      case stork::reservedTokens::kw_for:parsedStatement = parseForStatement(it);
        break;
      case stork::reservedTokens::kw_if:parsedStatement = parseIfStatement(it);
        break;
      case stork::reservedTokens::kw_return: {
        parsedStatement = parseReturnStatement(it);
        if (gobbleTrailingSemicolon) parseTokenValue(it, stork::reservedTokens::semicolon);
        break;
      }
      case stork::reservedTokens::open_curly:parsedStatement = parseBlockStatement(it);
        break;
      case stork::reservedTokens::kw_public:parsedStatement = parseFunctionStatement(it);
        break;

        // it starts with a data type or "secret" keyword (e.g., int, float, secret int, secret float)
      case stork::reservedTokens::kw_secret:
      case stork::reservedTokens::kw_bool:
      case stork::reservedTokens::kw_char:
      case stork::reservedTokens::kw_int:
      case stork::reservedTokens::kw_float:
      case stork::reservedTokens::kw_double:
      case stork::reservedTokens::kw_string:
      case stork::reservedTokens::kw_void: {
        parsedStatement = parseVariableDeclarationStatement(it);
        if (gobbleTrailingSemicolon) parseTokenValue(it, stork::reservedTokens::semicolon);
        break;
      }
      default: {
        // has to be an assignment
        parsedStatement = parseAssignmentStatement(it);
        if (gobbleTrailingSemicolon) parseTokenValue(it, stork::reservedTokens::semicolon);
        break;
      }
    }
  } else {
    // it start with an identifier -> must be an assignment
    parsedStatement = parseAssignmentStatement(it);;
    if (gobbleTrailingSemicolon) parseTokenValue(it, stork::reservedTokens::semicolon);
  }
  return parsedStatement;
}

bool isBinaryOperator(const stork::tokens_iterator &it) {
  return
      it->isReservedToken() &&
          (
              it->hasValue(stork::reservedTokens::add) ||
                  it->hasValue(stork::reservedTokens::sub) ||
                  it->hasValue(stork::reservedTokens::concat) ||
                  it->hasValue(stork::reservedTokens::mul) ||
                  it->hasValue(stork::reservedTokens::div) ||
                  it->hasValue(stork::reservedTokens::idiv) ||
                  it->hasValue(stork::reservedTokens::mod) ||
                  it->hasValue(stork::reservedTokens::bitwise_and) ||
                  it->hasValue(stork::reservedTokens::bitwise_or) ||
                  it->hasValue(stork::reservedTokens::bitwise_xor) ||
                  it->hasValue(stork::reservedTokens::shiftl) ||
                  it->hasValue(stork::reservedTokens::shiftr) ||
                  it->hasValue(stork::reservedTokens::logical_and) ||
                  it->hasValue(stork::reservedTokens::logical_or) ||
                  it->hasValue(stork::reservedTokens::eq) ||
                  it->hasValue(stork::reservedTokens::ne) ||
                  it->hasValue(stork::reservedTokens::lt) ||
                  it->hasValue(stork::reservedTokens::gt) ||
                  it->hasValue(stork::reservedTokens::le) ||
                  it->hasValue(stork::reservedTokens::ge) ||
                  it->hasValue(stork::reservedTokens::fhe_add) ||
                  it->hasValue(stork::reservedTokens::fhe_sub) ||
                  it->hasValue(stork::reservedTokens::fhe_mul)
          );
}

bool isUnaryOperator(const stork::tokens_iterator &it) {
  return
      it->isReservedToken() &&
          (
              it->hasValue(stork::reservedTokens::logical_not) ||
                  it->hasValue(stork::reservedTokens::bitwise_not)
          );
}

bool isPostFixOperator(const stork::tokens_iterator &it) {
  return
      it->isReservedToken() && (it->hasValue(stork::reservedTokens::inc) || it->hasValue(stork::reservedTokens::dec));
}

bool isLiteral(stork::tokens_iterator &it) {
  return it->isBool() || it->isChar() || it->isFloat() || it->isDouble() || it->isInteger() || it->isString()
      || it->hasValue(stork::reservedTokens::kw_true) || it->hasValue(stork::reservedTokens::kw_false);
}

AbstractExpression *Parser::parseExpression(stork::tokens_iterator &it) {

  // Shunting-yard algorithm: Keep a stack of operands and check precedence when you see an operator
  std::stack<AbstractExpression *, std::vector<AbstractExpression *>> operands;
  std::stack<Operator, std::vector<Operator>> operator_stack;

  bool running = true;
  while (running) {
    if (isBinaryOperator(it)) {
      Operator op1 = parseOperator(it);
      while (!operator_stack.empty()) {
        Operator op2 = operator_stack.top();
        if ((!op1.isRightAssociative() && comparePrecedence(op1, op2)==0) || comparePrecedence(op1, op2) < 0) {
          operator_stack.pop();
          AbstractExpression *rhs = operands.top();
          operands.pop();
          AbstractExpression *lhs = operands.top();
          operands.pop();
          auto binaryExpr = new BinaryExpression(std::unique_ptr<AbstractExpression>(lhs),
                                                 op2,
                                                 std::unique_ptr<AbstractExpression>(rhs));
          addParsedNode(binaryExpr);
          operands.push(binaryExpr);
        } else {
          break;
        }
      }
      operator_stack.push(op1);
    } else if (isUnaryOperator(it)) {
      operator_stack.push(parseOperator(it));
    } else if (isPostFixOperator(it)) {
      Operator op(ArithmeticOp::ADDITION);
      if (it->hasValue(stork::reservedTokens::inc)) {
        // already an add
      } else if (it->hasValue(stork::reservedTokens::dec)) {
        op = Operator(ArithmeticOp::SUBTRACTION);
      } else {
        throw stork::parsingError("Unexpected Postfix Operator", it->getLineNumber(), it->getCharIndex());
      }
      if (operands.empty()) {
        throw stork::expectedSyntaxError("operand for postfix operator", it->getLineNumber(), it->getCharIndex());
      } else {
        auto exp = operands.top();
        operands.pop();
        auto binaryExpr = new BinaryExpression(std::unique_ptr<AbstractExpression>(exp),
                                               op,
                                               std::make_unique<LiteralInt>(1));
        addParsedNode(binaryExpr);
        operands.push(binaryExpr);
      }
    } else if (isLiteral(it)) {
      // This handles the special case of negative values as the minus sign is recognized as separate token.
      // If we detected a minus sign but have not collected any lhs operand yet, we know that the minus does not act as
      // a binary operator but belong to the operand.
      if (operands.empty() && !operator_stack.empty()
          && operator_stack.top().toString()==Operator(SUBTRACTION).toString()) {
        operands.push(parseLiteral(it, true));
        operator_stack.pop();
      } else {
        operands.push(parseLiteral(it));
      }
    } else if (it->isIdentifier()) {
      // When we see an identifier, it could be a variable or a more general IndexAccess
      //TODO: It could also be a function call!!
      // Currently, the rotate "function" is implemented as a hardcoded token
      // The same could be done for the ctxt maintenance operations from https://github.com/MarbleHE/ABC/issues/7
      // However, this approach will not work for general functions, as in https://github.com/MarbleHE/ABC/issues/12
      operands.push(parseTarget(it));
    } else if (it->hasValue(stork::reservedTokens::open_round)) {
      // If we see an (, we have nested expressions going on, so use recursion.
      parseTokenValue(it, stork::reservedTokens::open_round);
      operands.push(parseExpression(it));
      parseTokenValue(it, stork::reservedTokens::close_round);
    } else if (it->hasValue(stork::reservedTokens::open_curly)) {
      // if it begins with an "{" it must be an expression list
      operands.push(parseExpressionList(it));
    } else if (it->hasValue(stork::reservedTokens::kw_rotate)) {
      // if it's the rotate keyword, it's a "fake" function call:
      parseTokenValue(it, stork::reservedTokens::kw_rotate);
      parseTokenValue(it, stork::reservedTokens::open_round);
      // the first argument can be a variable (e.g., ctxtA) or an expression (e.g., ctxtInput * 22)
      auto ciphertextToBeRotated = parseExpression(it);
      parseTokenValue(it, stork::reservedTokens::comma);
      std::vector<std::unique_ptr<AbstractExpression>> offset;
      offset.push_back(std::unique_ptr<AbstractExpression>(ciphertextToBeRotated));
      offset.push_back(std::unique_ptr<AbstractExpression>(parseExpression(it)));
      parseTokenValue(it, stork::reservedTokens::close_round);
      auto call = new Call(stork::to_string(stork::reservedTokens::kw_rotate), std::move(offset));
      addParsedNode(call);
      operands.push(call);
    } else {
      // Stop parsing tokens as soon as we see a closing ), a semicolon or anything else
      running = false;
    }

    // Check if we have a right-associative operator (currently only unary supported) ready to go on the stack
    if (!operator_stack.empty() && operator_stack.top().isRightAssociative() && !operands.empty()) {
      if (!operator_stack.top().isUnary()) {
        throw stork::parsingError("Cannot handle non-unary right-associative operators!",
                                  it->getLineNumber(),
                                  it->getCharIndex());
      } else {
        Operator op = operator_stack.top();
        operator_stack.pop();
        AbstractExpression *exp = operands.top();
        operands.pop();
        auto unaryExpression = new UnaryExpression(std::unique_ptr<AbstractExpression>(exp), op);
        addParsedNode(unaryExpression);
        operands.push(unaryExpression);
      }
    }

  } //end of while loop

  // cleanup any remaining operators
  while (!operator_stack.empty()) {
    Operator op = operator_stack.top();
    operator_stack.pop();

    // has to be binary?
    if (op.isUnary()) {
      throw stork::unexpectedSyntaxError("Unresolved Unary Operator", it->getLineNumber(), it->getCharIndex());
    } else {
      // Try to get two operands
      if (operands.size() < 2) {
        throw stork::unexpectedSyntaxError("Missing at least one Operand for Binary Operator",
                                           it->getLineNumber(),
                                           it->getCharIndex());
      } else {
        auto e1 = operands.top();
        operands.pop();
        auto e2 = operands.top();
        operands.pop();
        auto binaryExpression = new BinaryExpression(std::unique_ptr<AbstractExpression>(e2),
                                                     op,
                                                     std::unique_ptr<AbstractExpression>(e1));
        addParsedNode(binaryExpression);
        operands.push(binaryExpression);
      }
    }
  }

  if (operands.empty()) {
    throw stork::unexpectedSyntaxError("Empty Expression", it->getLineNumber(), it->getCharIndex());
  } else if (operands.size()==1) {
    return operands.top();
  } else {
    throw stork::unexpectedSyntaxError("Unresolved Operands", it->getLineNumber(), it->getCharIndex());
  }
}

AbstractTarget *Parser::parseTarget(stork::tokens_iterator &it) {
  //Any valid target must begin with a Variable as its "root"
  Variable *v = parseVariable(it);

  std::vector<AbstractExpression *> indices;
  // if the next token is a "[" we need to keep on parsing
  while (it->hasValue(stork::reservedTokens::open_square)) {
    parseTokenValue(it, stork::reservedTokens::open_square);
    indices.push_back(parseExpression(it));
    parseTokenValue(it, stork::reservedTokens::close_square);
  }

  if (indices.empty()) {
    return v;
  } else {
    auto cur = new IndexAccess(std::unique_ptr<AbstractTarget>(v), std::unique_ptr<AbstractExpression>(indices[0]));
    addParsedNode(cur);
    for (size_t i = 1; i < indices.size(); ++i) {
      cur = new IndexAccess(std::unique_ptr<AbstractTarget>(cur), std::unique_ptr<AbstractExpression>(indices[i]));
      addParsedNode(cur);
    }
    return cur;
  }
}

AbstractExpression *Parser::parseLiteral(stork::tokens_iterator &it, bool isNegative) {
  // Create literal by taking 'isNegative' flag into account. For example, "-3" would be parsed as {"-", "3"} and in
  // parseExpression it would be recognized that "-" is a binary operator but has a single operand only and as such this
  // operator negates the literal.

  // We cannot distinguish between integers and booleans as both use the numbers 0,1 in the input syntax. Because of
  // that, in case of a variable declaration, we also include the information of the parsed datatype here.

  AbstractExpression *l = nullptr;
  if (it->isString()) {
    l = new LiteralString(it->getString());
  } else if (it->isDouble()) {
    if (isNegative) l = new LiteralDouble(-it->getDouble());
    else l = new LiteralDouble(it->getDouble());
  } else if (it->isFloat()) {
    if (isNegative) l = new LiteralFloat(-it->getFloat());
    else l = new LiteralFloat(it->getFloat());
  } else if (it->isChar()) {
    l = new LiteralChar(it->getChar());
  } else if (it->isInteger()) {
    if (varAssignmentDatatype!=nullptr && varAssignmentDatatype->getType()==Type::BOOL) {
      auto value = it->getInteger();
      if (value!=0 && value!=1) {
        throw stork::semanticError("Variable declaration declares Bool but value found is neither 0 nor 1.",
                                   it->getLineNumber(),
                                   it->getCharIndex());
      }
      l = new LiteralBool(it->getInteger());
    } else {
      l = (isNegative) ? new LiteralInt(-it->getInteger()) : new LiteralInt(it->getInteger());
    }
  } else if (it->hasValue(stork::reservedTokens::kw_true)) {
    l = new LiteralBool(true);
  } else if (it->hasValue(stork::reservedTokens::kw_false)) {
    l = new LiteralBool(false);
  } else {
    throw stork::unexpectedSyntaxError(to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
  }

  addParsedNode(l);

  // A negative string|char|integer is not allowed
  if ((it->isString() || it->isChar() || it->isBool()) && isNegative) {
    throw stork::unexpectedSyntaxError("A minus sign ('-') in front of a string, char, or bool is not allowed. "
                                       "Current token: " + to_string(it->getValue()) + ".",
                                       it->getLineNumber(), it->getCharIndex());
  }

  ++it;
  return l;
}

AbstractExpression *Parser::parseLiteral(stork::tokens_iterator &it) {
  return parseLiteral(it, false);
}

ExpressionList *Parser::parseExpressionList(stork::tokens_iterator &it) {
  parseTokenValue(it, stork::reservedTokens::open_curly);
  std::vector<std::unique_ptr<AbstractExpression>> expressions;
  while (true) {
    expressions.push_back(std::unique_ptr<AbstractExpression>(parseExpression(it)));
    if (it->hasValue(stork::reservedTokens::comma)) {
      parseTokenValue(it, stork::reservedTokens::comma);
    } else {
      break;
    }
  }
  parseTokenValue(it, stork::reservedTokens::close_curly);
  auto pList = new ExpressionList(std::move(expressions));
  addParsedNode(pList);
  return pList;
}

Variable *Parser::parseVariable(stork::tokens_iterator &it) {
  auto identifier = parseIdentifier(it);
  auto variable = std::make_unique<Variable>(identifier);
  auto pVariable = new Variable(identifier);
  addParsedNode(pVariable);
  return pVariable;
}

Operator Parser::parseOperator(stork::tokens_iterator &it) {
  if (it->isReservedToken()) {
    if (it->hasValue(stork::reservedTokens::add)) {
      ++it;
      return Operator(ArithmeticOp::ADDITION);
    } else if (it->hasValue(stork::reservedTokens::sub)) {
      ++it;
      return Operator(ArithmeticOp::SUBTRACTION);
    } else if (it->hasValue(stork::reservedTokens::concat)) {
      throw stork::unexpectedSyntaxError("concatenation (not supported in AST)",
                                         it->getLineNumber(),
                                         it->getCharIndex());
    } else if (it->hasValue(stork::reservedTokens::mul)) {
      ++it;
      return Operator(ArithmeticOp::MULTIPLICATION);
    } else if (it->hasValue(stork::reservedTokens::div)) {
      ++it;
      return Operator(ArithmeticOp::DIVISION);
    } else if (it->hasValue(stork::reservedTokens::idiv)) {
      throw stork::unexpectedSyntaxError("integer division (not supported in AST)",
                                         it->getLineNumber(),
                                         it->getCharIndex());
    } else if (it->hasValue(stork::reservedTokens::mod)) {
      ++it;
      return Operator(ArithmeticOp::MODULO);
    } else if (it->hasValue(stork::reservedTokens::bitwise_and)) {
      ++it;
      return Operator(LogicalOp::BITWISE_AND);
    } else if (it->hasValue(stork::reservedTokens::bitwise_or)) {
      ++it;
      return Operator(LogicalOp::BITWISE_OR);
    } else if (it->hasValue(stork::reservedTokens::bitwise_xor)) {
      ++it;
      return Operator(LogicalOp::BITWISE_XOR);
    } else if (it->hasValue(stork::reservedTokens::shiftl)) {
      throw stork::unexpectedSyntaxError("shift left (not supported in AST)", it->getLineNumber(), it->getCharIndex());
    } else if (it->hasValue(stork::reservedTokens::shiftr)) {
      throw stork::unexpectedSyntaxError("shift right (not supported in AST)", it->getLineNumber(), it->getCharIndex());
    } else if (it->hasValue(stork::reservedTokens::logical_and)) {
      ++it;
      return Operator(LogicalOp::LOGICAL_AND);
    } else if (it->hasValue(stork::reservedTokens::logical_or)) {
      ++it;
      return Operator(LogicalOp::LOGICAL_OR);
    } else if (it->hasValue(stork::reservedTokens::eq)) {
      ++it;
      return Operator(LogicalOp::EQUAL);
    } else if (it->hasValue(stork::reservedTokens::ne)) {
      ++it;
      return Operator(LogicalOp::NOTEQUAL);
    } else if (it->hasValue(stork::reservedTokens::lt)) {
      ++it;
      return Operator(LogicalOp::LESS);
    } else if (it->hasValue(stork::reservedTokens::gt)) {
      ++it;
      return Operator(LogicalOp::GREATER);
    } else if (it->hasValue(stork::reservedTokens::le)) {
      ++it;
      return Operator(LogicalOp::LESS_EQUAL);
    } else if (it->hasValue(stork::reservedTokens::ge)) {
      ++it;
      return Operator(LogicalOp::GREATER_EQUAL);
    } else if (it->hasValue(stork::reservedTokens::logical_not)) {
      ++it;
      return Operator(UnaryOp::LOGICAL_NOT);
    } else if (it->hasValue(stork::reservedTokens::bitwise_not)) {
      ++it;
      return Operator(UnaryOp::BITWISE_NOT);
    } else if (it->hasValue(stork::reservedTokens::fhe_add)) {
      ++it;
      return Operator(ArithmeticOp::FHE_ADDITION);
    } else if (it->hasValue(stork::reservedTokens::fhe_sub)) {
      ++it;
      return Operator(ArithmeticOp::FHE_SUBTRACTION);
    } else if (it->hasValue(stork::reservedTokens::fhe_mul)) {
      ++it;
      return Operator(ArithmeticOp::FHE_MULTIPLICATION);
    }
  }
  // If we get here, it wasn't an operator
  throw stork::unexpectedSyntaxError(to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
}

/// consume token "value" and throw error if something different
void Parser::parseTokenValue(stork::tokens_iterator &it, const stork::token_value &value) {
  if (it->hasValue(value)) {
    ++it;
    return;
  }
  throw stork::expectedSyntaxError(to_string(value), it->getLineNumber(), it->getCharIndex());
}

Datatype Parser::parseDatatype(stork::tokens_iterator &it) {
  if (!it->isReservedToken()) {
    throw stork::unexpectedSyntaxError(to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
  }

  bool isSecret = false;
  if (it->hasValue(stork::reservedTokens::kw_secret)) {
    isSecret = true;
    parseTokenValue(it, stork::reservedTokens::kw_secret);
  }

  // just a placeholder as value-less constructor does not exist
  Datatype datatype(Type::VOID);
  switch (it->get_reserved_token()) {
    case stork::reservedTokens::kw_bool:datatype = Datatype(Type::BOOL, isSecret);
      break;
    case stork::reservedTokens::kw_char:datatype = Datatype(Type::CHAR, isSecret);
      break;
    case stork::reservedTokens::kw_int:datatype = Datatype(Type::INT, isSecret);
      break;
    case stork::reservedTokens::kw_float:datatype = Datatype(Type::FLOAT, isSecret);
      break;
    case stork::reservedTokens::kw_double:datatype = Datatype(Type::DOUBLE, isSecret);
      break;
    case stork::reservedTokens::kw_string:datatype = Datatype(Type::STRING, isSecret);
      break;
    case stork::reservedTokens::kw_void:datatype = Datatype(Type::VOID);
      break;
    default:throw stork::unexpectedSyntaxError(to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
  }

  ++it;

  return datatype;
}

std::string Parser::parseIdentifier(stork::tokens_iterator &it) {
  if (!it->isIdentifier()) {
    throw stork::unexpectedSyntaxError(to_string(it->getValue()), it->getLineNumber(), it->getCharIndex());
  }
  std::string ret = it->getIdentifier().name;
  ++it;
  return ret;
}

FunctionParameter *Parser::parseFunctionParameter(stork::tokens_iterator &it) {
  auto datatype = parseDatatype(it);
  auto identifier = parseIdentifier(it);

  auto functionParameter = new FunctionParameter(datatype, identifier);
  addParsedNode(functionParameter);

  // consume comma that separates this parameter from the next one
  // the caller is responsible for calling this method again for parsing the next parameter
  if (it->hasValue(stork::reservedTokens::comma)) {
    parseTokenValue(it, stork::reservedTokens::comma);
  }

  return functionParameter;
}

Function *Parser::parseFunctionStatement(stork::tokens_iterator &it) {
  // consume 'public'
  parseTokenValue(it, stork::reservedTokens::kw_public);

  // parse return type
  auto datatype = parseDatatype(it);

  // parse function name
  auto functionName = parseIdentifier(it);

  // parse function parameters
  parseTokenValue(it, stork::reservedTokens::open_round);
  std::vector<std::unique_ptr<FunctionParameter>> functionParams;
  while (!it->hasValue(stork::reservedTokens::close_round)) {
    auto functionParam = std::unique_ptr<FunctionParameter>(parseFunctionParameter(it));
    functionParams.push_back(std::move(functionParam));
  }
  parseTokenValue(it, stork::reservedTokens::close_round);

  // parse block/body statements
  auto block = std::unique_ptr<Block>(parseBlockStatement(it));

  auto pFunction = new Function(datatype, functionName, std::move(functionParams), std::move(block));
  addParsedNode(pFunction);
  return pFunction;
}

For *Parser::parseForStatement(stork::tokens_iterator &it) {
  // FOR (initialization statement(s); expression; update statement(s)) BLOCK
  parseTokenValue(it, stork::reservedTokens::kw_for);
  parseTokenValue(it, stork::reservedTokens::open_round);

  // initialization statement(s)
  std::vector<std::unique_ptr<AbstractStatement>> initializerStatements;
  // check if we have at least one statement in the initializer field
  if (!it->hasValue(stork::reservedTokens::semicolon)) {
    // parse all initialization statements, e.g., for (int i=0, int j=1, ...; - ; - )
    do {
      // if we parsed a statement before, we now need to gobble the comma before parsing the next statement
      if (initializerStatements.size() > 1) parseTokenValue(it, stork::reservedTokens::comma);
      initializerStatements.emplace_back(parseStatement(it));
    } while (it->hasValue(stork::reservedTokens::comma)); // only proceed parsing if there is a comma operator
  } else {
    // if there is no statement in the initializer field, we need to manually gobble the semicolon
    parseTokenValue(it, stork::reservedTokens::semicolon);
  }
  auto initializerStatementBlock = std::make_unique<Block>(std::move(initializerStatements));

  // expression (condition)
  auto condition = std::unique_ptr<AbstractExpression>(parseExpression(it));
  parseTokenValue(it, stork::reservedTokens::semicolon);

  // update statement(s)
  std::vector<std::unique_ptr<AbstractStatement>> updateStatements;
  // check if we have at least one statement in the update field
  if (!it->hasValue(stork::reservedTokens::close_round)) {
    // parse all update statements, e.g., for (- ; - ; i++, j++, ...)
    do {
      // if we parsed a statement before, we now need to gobble the comma before parsing the next statement
      if (updateStatements.size() > 1) parseTokenValue(it, stork::reservedTokens::comma);
      updateStatements.emplace_back(parseStatement(it, false));
    } while (it->hasValue(stork::reservedTokens::comma)); // only proceed parsing if there is a comma operator
  }
  parseTokenValue(it, stork::reservedTokens::close_round);
  auto updateStatementBlock = std::make_unique<Block>(std::move(updateStatements));

  // FOR loop's body
  auto body = std::unique_ptr<Block>(parseBlockStatement(it));

  auto pFor = new For(std::move(initializerStatementBlock),
                      std::move(condition),
                      std::move(updateStatementBlock),
                      std::move(body));
  addParsedNode(pFor);
  return pFor;
}

Block *Parser::parseBlockOrSingleStatement(stork::tokens_iterator &it) {
  // a helper method that checks whether there is a block (if so, parses the whole block) or otherwise only parses
  // the single statement and wraps it into a block
  Block *block;
  if (it->hasValue(stork::reservedTokens::open_curly)) {
    // multiple statements wrapped into a block -> parse whole block
    block = parseBlockStatement(it);
  } else {
    // a single statement, not wrapped into a block in the input file -> parse stmt. and manually wrap into a block
    block = new Block(std::unique_ptr<AbstractStatement>(parseStatement(it)));
    addParsedNode(block);
  }
  return block;
};

/**
 * Parses an If statement, looking for the following pattern, where [] indiciates optional, and <X> means X is another node type
 * if(<EXPRESSION>) <STATEMENT>|<BLOCK> [else <STATEMENT>|<BLOCK>]
 * //TODO: Specify in some kind of "real" EBNF or similar for each parsing function!!
 * @param it
 * @return
 */
If *Parser::parseIfStatement(stork::tokens_iterator &it) {
  // parse: if (condition)
  parseTokenValue(it, stork::reservedTokens::kw_if);
  parseTokenValue(it, stork::reservedTokens::open_round);
  auto condition = std::unique_ptr<AbstractExpression>(parseExpression(it));
  parseTokenValue(it, stork::reservedTokens::close_round);

  // check if there is an opening bracket (block)
  std::unique_ptr<Block> ifBlock = std::unique_ptr<Block>(parseBlockOrSingleStatement(it));

  // check if there is an "else" branch
  if (it->hasValue(stork::reservedTokens::kw_else)) {
    parseTokenValue(it, stork::reservedTokens::kw_else);
    std::unique_ptr<Block> elseBlock = std::unique_ptr<Block>(parseBlockOrSingleStatement(it));
    auto pIf = new If(std::move(condition), std::move(ifBlock), std::move(elseBlock));
    addParsedNode(pIf);
    return pIf;
  } else {
    auto pIf = new If(std::move(condition), std::move(ifBlock));
    addParsedNode(pIf);
    return pIf;
  }
}

Return *Parser::parseReturnStatement(stork::tokens_iterator &it) {
  parseTokenValue(it, stork::reservedTokens::kw_return);

  // Is it a return; i.e. no return value?
  if (it->hasValue(stork::reservedTokens::semicolon)) {
    auto pReturn = new Return();
    addParsedNode(pReturn);
    return pReturn;
  } else {
    AbstractExpression *p = parseExpression(it);
    auto pReturn = new Return(std::unique_ptr<AbstractExpression>(p));
    addParsedNode(pReturn);
    return pReturn;
  }
}

Block *Parser::parseBlockStatement(stork::tokens_iterator &it) {
  // parse block/body statements
  parseTokenValue(it, stork::reservedTokens::open_curly);
  std::vector<std::unique_ptr<AbstractStatement>> blockStatements;
  while (!it->hasValue(stork::reservedTokens::close_curly)) {
    blockStatements.push_back(std::unique_ptr<AbstractStatement>(parseStatement(it)));
  }
  parseTokenValue(it, stork::reservedTokens::close_curly);
  auto pBlock = new Block(std::move(blockStatements));
  addParsedNode(pBlock);
  return pBlock;
}

VariableDeclaration *Parser::parseVariableDeclarationStatement(stork::tokens_iterator &it) {
  // the variable's datatype
  auto datatype = parseDatatype(it);

  // the variable's name
  auto variable = std::unique_ptr<Variable>(parseVariable(it));

  // check if this is an array declaration
  if (it->hasValue(stork::reservedTokens::open_square)) {
    parseTokenValue(it, stork::reservedTokens::open_square);
    if (it->hasValue(stork::reservedTokens::close_square)) {
      parseTokenValue(it, stork::reservedTokens::close_square);
    } else {
      throw stork::unexpectedSyntaxError("Fixed-size array declarations not supported (e.g., int i[3]). "
                                         "Declare array without size, e.g., int i[];.",
                                         it->getLineNumber(),
                                         it->getCharIndex());
    }
  }

  // the variable's assigned value, if any assigned
  if (!it->hasValue(stork::reservedTokens::semicolon)) {
    parseTokenValue(it, stork::reservedTokens::assign);
    AbstractExpression *value = parseExpression(it);
    auto pDeclaration =
        new VariableDeclaration(datatype, std::move(variable), std::unique_ptr<AbstractExpression>(value));
    addParsedNode(pDeclaration);
    return pDeclaration;
  } else {
    auto pDeclaration = new VariableDeclaration(datatype, std::move(variable));
    addParsedNode(pDeclaration);
    return pDeclaration;
  }
}

Assignment *Parser::parseAssignmentStatement(stork::tokens_iterator &it) {
  // the target's name
  auto target = std::unique_ptr<AbstractTarget>(parseTarget(it));

  // the variable's assigned value
  parseTokenValue(it, stork::reservedTokens::assign);
  AbstractExpression *value = parseExpression(it);

  auto pAssignment = new Assignment(std::move(target), std::unique_ptr<AbstractExpression>(value));
  addParsedNode(pAssignment);
  return pAssignment;
}

