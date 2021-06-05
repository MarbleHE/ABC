#ifndef AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VISITOR_H_
#define AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VISITOR_H_

// In order for the template magic to work,
// we must include ALL ast classes,
// so that the inheritance relations are known
#include "ast_opt/ast/AbstractExpression.h"
#include "ast_opt/ast/AbstractNode.h"
#include "ast_opt/ast/AbstractStatement.h"
#include "ast_opt/ast/AbstractTarget.h"
#include "ast_opt/ast/Assignment.h"
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
#include "ast_opt/ast/OperatorExpression.h"
#include "ast_opt/ast/Return.h"
#include "ast_opt/ast/TernaryOperator.h"
#include "ast_opt/ast/UnaryExpression.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/ast/VariableDeclaration.h"

#include "ast_opt/utilities/IVisitor.h"
#include "ast_opt/utilities/ScopedVisitor.h"

/// SFINAE based detection if T::visit(Args...) exists
/// Taken from https://stackoverflow.com/a/28309612
template<typename T, typename... Args>
class is_visit_available {
  template<typename C,
      typename = decltype(std::declval<C>().visit(std::declval<Args>()...))>
  static std::true_type test(int);

  template<typename C>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(0))::value;
};

/// Does T::visit(Args...) exist? (Syntactic sugar for is_visit_available)
template<typename T, typename... Args>
constexpr bool has_visit = is_visit_available<T, Args...>::value;

/// Helper Class to create Visitors.
/// This allows a SpecialVisitor to not only provide only *some* of the visit functions,
/// with the rest being handled by the default DefaultVisitor::visit() function
/// but it also allows the SpecialVisitor to target Superclasses, e.g. visit(AbstractExpression)
///
/// SpecialVisitor should inherit DefaultVisitor publicly,
/// should define at least one visit(Some AST Type) WITHOUT specifying override
/// (intentionally hiding all base class visit(...) overloads)
/// and should not include "using IVisitor::visit" or "using DefaultVisitor::visit" (which would un-hide them again)
///
/// This leads to only the defined visit(..) functions being accessible in SpecialVisitor
/// The Visitor<..> template than redirects any calls for which SpecialVisitor does not have a visit overload
/// to the default visit(elem) method in DefaultVisitor.
/// In addition, you can define visit(BaseClass &elem) in SpecialVisitor and,
/// unless there is a more specific visit(DerivedClass &elem) with DerivedClass : public BaseClass
/// All calls for classes derived from BaseClass will be handled by this visit.
/// This is not generally possible with normal visitors because they must have all possible visit functions
/// and therefore always have a more specific one for the derived classes.
///
/// SpecialVisitor should also specify any desired constructors, Visitor<SpecialVisitor> will inherit and expose them
///
/// Note that IVisitor, DefaultVisitor and the Visitor<..> template
/// only need functions corresponding to the concrete classes in the Node hierarchy.
/// Functions applying to parent classes like AbstractExpression need to appear only in the SpecialVisitor
/// The way this works is that Visitor<..>'s visit function for the concrete class will be called,
/// but if SpecialVisitor offers a function for the base class but not the concrete derived class,
/// the function for the (potentially abstract) base class will be called by Visitor<...>'s visit(concrete&)
///
/// \tparam SpecialVisitor  The class implementing the actual visiting logic

#define VISIT_SPECIAL_VISITOR_IF_EXISTS(AstClassName) if constexpr (has_visit<SpecialVisitor, AstClassName&>) { this->SpecialVisitor::visit(elem); } else { this->DefaultVisitor::visit(elem); }

template<typename SpecialVisitor, typename DefaultVisitor = ScopedVisitor>
class Visitor : public SpecialVisitor {
 public:
  /// Allow the SpecialVisitor class to operate on Visitor<SpecialVisitor> as if it was a SpecialVisitor
  friend SpecialVisitor;

  /// Ensure that SpecialVisitor is actually a visitor
  static_assert(std::is_base_of<IVisitor, SpecialVisitor>::value);

  /// Inherit Constructors from SpecialVisitor
  using SpecialVisitor::SpecialVisitor;

  void visit(BinaryExpression &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(BinaryExpression);
  }

  void visit(Block &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(Block);
  }

  void visit(ExpressionList &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(ExpressionList);
  }

  void visit(For &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(For);
  }

  void visit(Function &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(Function);
  }

  void visit(FunctionParameter &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(FunctionParameter);
  }

  void visit(If &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(If);
  }

  void visit(IndexAccess &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(IndexAccess);
  }

  void visit(LiteralBool &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralBool);
  }

  void visit(LiteralChar &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralChar);
  }

  void visit(LiteralInt &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralInt);
  }

  void visit(LiteralFloat &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralFloat);
  }

  void visit(LiteralDouble &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralDouble);
  }

  void visit(LiteralString &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(LiteralString);
  }

  void visit(OperatorExpression &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(OperatorExpression);
  }

  void visit(Return &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(Return);
  }

  void visit(TernaryOperator &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(TernaryOperator);
  }

  void visit(UnaryExpression &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(UnaryExpression);
  }

  void visit(Assignment &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(Assignment);
  }

  void visit(VariableDeclaration &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(VariableDeclaration);
  }

  void visit(Variable &elem) override {
    VISIT_SPECIAL_VISITOR_IF_EXISTS(Variable);
  }
};

#endif //AST_OPTIMIZER_INCLUDE_AST_OPT_UTILITIES_VISITOR_H_
