#include "ast_opt/visitor/ProgramPrintVisitor.h"

std::string SpecialProgramPrintVisitor::getIndentation() const {
  // Indent with two spaces per level
  return std::string(2*indentation_level, ' ');
}

SpecialProgramPrintVisitor::SpecialProgramPrintVisitor(std::ostream &os) : os(os) {}

void SpecialProgramPrintVisitor::visit(BinaryExpression &elem) {
  os << "(";
  if (elem.hasLeft()) elem.getLeft().accept(*this);
  os << " " << elem.getOperator().toString() << " ";
  if (elem.hasRight()) elem.getRight().accept(*this);
  os << ")";
}
void SpecialProgramPrintVisitor::visit(Block &elem) {
  os << getIndentation() << "{\n";
  ++indentation_level;
  for (auto &s: elem.getStatements()) {
    s.get().accept(*this);
  }
  --indentation_level;
  os << getIndentation() << "}\n";
}
void SpecialProgramPrintVisitor::visit(Call &elem) {
  os << elem.getIdentifier() << "(";
  auto args = elem.getArguments();
  if (!args.empty()) {
    args[0].get().accept(*this);
    for (size_t i = 1; i < args.size(); ++i) {
      os << ", ";
      args[i].get().accept(*this);
    }
  }
  os << ")";
}
void SpecialProgramPrintVisitor::visit(ExpressionList &elem) {
  os << "{";
  auto es = elem.getExpressions();
  if (!es.empty()) {
    es[0].get().accept(*this);
    for (size_t i = 1; i < es.size(); ++i) {
      os << ", ";
      es[i].get().accept(*this);
    }
  }
  os << "}";
}
void SpecialProgramPrintVisitor::visit(For &elem) {
  PlainVisitor::visit(elem);
}
void SpecialProgramPrintVisitor::visit(Function &elem) {
  os << getIndentation() << elem.getReturnType().toString() << " "
     << elem.getIdentifier() << "(";
  // Print parameters with correct commas between
  auto params = elem.getParameters();
  if (!params.empty()) {
    for (size_t i = 0; i < params.size() - 1; ++i) {
      params[i].get().accept(*this);
      os << ", ";
    }
    params[params.size() - 1].get().accept(*this);
  }
  os << ")\n";
  elem.getBody().accept(*this);
}
void SpecialProgramPrintVisitor::visit(FunctionParameter &elem) {
  os << elem.getParameterType().toString() << " " << elem.getIdentifier();
}
void SpecialProgramPrintVisitor::visit(If &elem) {
  os << getIndentation() << "if(";
  if (elem.hasCondition()) elem.getCondition().accept(*this);
  os << ")\n";
  if (elem.hasThenBranch()) elem.getThenBranch().accept(*this);
  if (elem.hasElseBranch()) {
    os << getIndentation() << "else\n";
    elem.getElseBranch().accept(*this);
  }
}
void SpecialProgramPrintVisitor::visit(IndexAccess &elem) {
  elem.getTarget().accept(*this);
  os << "[";
  elem.getIndex().accept(*this);
  os << "]";
}
void SpecialProgramPrintVisitor::visit(LiteralBool &elem) {
  if (elem.getValue()) os << "true";
  else os << "false";
}
void SpecialProgramPrintVisitor::visit(LiteralChar &elem) {
  os << elem.getValue();
}
void SpecialProgramPrintVisitor::visit(LiteralInt &elem) {
  os << elem.getValue();
}
void SpecialProgramPrintVisitor::visit(LiteralFloat &elem) {
  os << elem.getValue();
}
void SpecialProgramPrintVisitor::visit(LiteralDouble &elem) {
  os << elem.getValue();
}
void SpecialProgramPrintVisitor::visit(LiteralString &elem) {
  os << elem.getValue();
}
void SpecialProgramPrintVisitor::visit(OperatorExpression &elem) {
  os << elem.getOperator().toString() << "(";
  auto operands = elem.getOperands();
  if (!operands.empty()) {
    operands[0].get().accept(*this);
    for (size_t i = 1; i < operands.size(); ++i) {
      os << ", ";
      operands[i].get().accept(*this);
    }
  }
  os << ")";
}
void SpecialProgramPrintVisitor::visit(Return &elem) {
  os << getIndentation() << "return";
  if (elem.hasValue()) {
    os << " ";
    elem.getValue().accept(*this);
  }
  os << ";\n";
}
void SpecialProgramPrintVisitor::visit(TernaryOperator &elem) {
  elem.getCondition().accept(*this);
  os << " ? ";
  elem.getThenExpr().accept(*this);
  os << " : ";
  elem.getElseExpr().accept(*this);
}
void SpecialProgramPrintVisitor::visit(UnaryExpression &elem) {
  os << elem.getOperator().toString();
  elem.getOperand().accept(*this);
}
void SpecialProgramPrintVisitor::visit(Assignment &elem) {
  os << getIndentation();
  elem.getTarget().accept(*this);
  os << " = ";
  elem.getValue().accept(*this);
  os << ";\n";
}
void SpecialProgramPrintVisitor::visit(VariableDeclaration &elem) {
  os << getIndentation() << elem.getDatatype().toString() << " ";
  elem.getTarget().accept(*this);
  if (elem.hasValue()) {
    os << " = ";
    elem.getValue().accept(*this);
  }
  os << ";\n";
}
void SpecialProgramPrintVisitor::visit(Variable &elem) {
  os << elem.getIdentifier();
}
