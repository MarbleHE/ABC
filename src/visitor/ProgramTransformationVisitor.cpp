#include <stdexcept>
#include "ast_opt/visitor/ProgramTransformationVisitor.h"
#include "ast_opt/visitor/controlFlowGraph/ControlFlowGraphVisitor.h"
#include "ast_opt/visitor/ProgramPrintVisitor.h"

template<typename T>
/// This function goes through all possible types of operator,
/// selects the correct on and tries to apply it to the operands
/// \tparam T Type of the Operands
/// \param lhsOperand
/// \param rhsOperand
/// \param op
/// \return result of computing "lhs op rhs"
T applyOperator(T lhsOperand, T rhsOperand, Operator op) {
  auto operatorEqualsAnyOf = [&](std::initializer_list<OperatorVariant> list) -> bool {
    return std::any_of(list.begin(), list.end(), [&op](OperatorVariant oper) { return op==Operator(oper); });
  };
  auto operatorEquals = [&op](OperatorVariant oper) -> bool { return op==Operator(oper); };
  if (operatorEqualsAnyOf({ADDITION, FHE_ADDITION})) {
    return (lhsOperand + rhsOperand);
  } else if (operatorEqualsAnyOf({SUBTRACTION, FHE_SUBTRACTION})) {
    return (lhsOperand - rhsOperand);
  } else if (operatorEqualsAnyOf({MULTIPLICATION, FHE_MULTIPLICATION})) {
    return (lhsOperand*rhsOperand);
  } else if (operatorEquals(DIVISION)) {
    if constexpr (!std::is_same<T, bool>::value) {
      return (lhsOperand/rhsOperand);
    } else {
      throw std::runtime_error("Cannot perform modulo (%) on bool.");
    }
  } else if (operatorEquals(MODULO)) {
    if constexpr (std::is_integral<T>::value && !std::is_same<T, bool>::value) {
      return (lhsOperand%rhsOperand);
    } else {
      throw std::runtime_error("Cannot perform modulo (%) and on non-integral types.");
    }
  } else if (operatorEquals(LOGICAL_AND)) {
    return (lhsOperand && rhsOperand);
  } else if (operatorEquals(LOGICAL_OR)) {
    return (lhsOperand || rhsOperand);
  } else if (operatorEquals(LESS)) {
    return (lhsOperand < rhsOperand);
  } else if (operatorEquals(LESS_EQUAL)) {
    return (lhsOperand <= rhsOperand);
  } else if (operatorEquals(GREATER)) {
    return (lhsOperand > rhsOperand);
  } else if (operatorEquals(GREATER_EQUAL)) {
    return (lhsOperand >= rhsOperand);
  } else if (operatorEquals(EQUAL)) {
    return (lhsOperand==rhsOperand);
  } else if (operatorEquals(NOTEQUAL)) {
    return (lhsOperand!=rhsOperand);
  } else if (operatorEquals(BITWISE_AND)) {
    if constexpr (std::is_integral<T>::value) {
      return (lhsOperand & rhsOperand && !std::is_same<T, bool>::value);
    } else {
      throw std::runtime_error("Cannot perform bitwise AND on non-integral types.");
    }
  } else if (operatorEquals(BITWISE_XOR)) {
    if constexpr (std::is_integral<T>::value) {
      return (lhsOperand ^ rhsOperand);
    } else {
      throw std::runtime_error("Cannot perform bitwise XOR on non-integral types.");
    }
  } else if (operatorEquals(BITWISE_OR)) {
    if constexpr (std::is_integral<T>::value) {
      return (lhsOperand | rhsOperand);
    } else {
      throw std::runtime_error("Cannot perform bitwise OR on non-integral types.");
    }
  } else {
    throw std::runtime_error("Unknown binary operator encountered. Cannot continue!");
  }
}

template<typename TO, typename FROM>
/// \tparam TO Target Type
/// \tparam FROM Source Type
/// \param old Pointer to convert, needs to be std::move()'d in
/// \return
std::unique_ptr<TO> dynamic_cast_unique_ptr(std::unique_ptr<FROM> &&old) {
  return std::unique_ptr<TO>{dynamic_cast<TO *>(old.release())};
  //conversion: unique_ptr<FROM>->FROM*->TO*->unique_ptr<TO>
}

std::string SpecialProgramTransformationVisitor::printProgram(AbstractNode &node) {
  std::stringstream ss;
  ProgramPrintVisitor ppv(ss);
  node.accept(ppv);
  return ss.str();
}

bool SpecialProgramTransformationVisitor::containsVariable(const AbstractNode &node,
                                                           const std::vector<std::string> &identifiers) {
  if (auto var_ptr = dynamic_cast<const Variable *>(&node)) {
    for (auto &id: identifiers) {
      if (var_ptr->getIdentifier()==id)
        return true;
    }
  } else {
    for (auto &c: node) {
      if (containsVariable(c, identifiers))
        return true;
    }
  }
  return false;
}

void SpecialProgramTransformationVisitor::visit(BinaryExpression &elem) {
  std::string original_code = printProgram(elem);
  elem.getLeft().accept(*this);
  if (replacementExpression) {
    elem.setLeft(std::move(replacementExpression));
  }
  elem.getRight().accept(*this);
  if (replacementExpression) {
    elem.setRight(std::move(replacementExpression));
  }

  // Try out all possible types of literal we could combine
  // if "lhs op rhs" can be computed at compile time
  // then we replace elem with a literal
  // does not use apply when implicit conversions would be required, because handling them is messy
  // TODO (constant folding): This needs to be split into operations that return bool and operations that return the input type!
  AbstractNode *lhs_ptr = &elem.getLeft();
  AbstractNode *rhs_ptr = &elem.getRight();
  if (auto literal_int_lhs_ptr = (dynamic_cast<LiteralInt *>(lhs_ptr))) {
    if (auto lit_rhs_ptr = (dynamic_cast<LiteralInt *>(rhs_ptr))) {
      auto v = applyOperator(literal_int_lhs_ptr->getValue(), lit_rhs_ptr->getValue(), elem.getOperator());
      replacementExpression = std::make_unique<LiteralInt>(v);
    }
  } else if (auto literal_bool_lhs_ptr = (dynamic_cast<LiteralBool *>(lhs_ptr))) {
    if (auto lit_rhs_ptr = (dynamic_cast<LiteralBool *>(rhs_ptr))) {
      auto v = applyOperator(literal_bool_lhs_ptr->getValue(), lit_rhs_ptr->getValue(), elem.getOperator());
      replacementExpression = std::make_unique<LiteralBool>(v);
    }
  } else if (auto literal_char_lhs_ptr = (dynamic_cast<LiteralChar *>(lhs_ptr))) {
    if (auto lit_rhs_ptr = (dynamic_cast<LiteralChar *>(rhs_ptr))) {
      auto v = applyOperator(literal_char_lhs_ptr->getValue(), lit_rhs_ptr->getValue(), elem.getOperator());
      replacementExpression = std::make_unique<LiteralChar>(v);
    }
  } else if (auto literal_float_lhs_ptr = (dynamic_cast<LiteralFloat *>(lhs_ptr))) {
    if (auto lit_rhs_ptr = (dynamic_cast<LiteralFloat *>(rhs_ptr))) {
      auto v = applyOperator(literal_float_lhs_ptr->getValue(), lit_rhs_ptr->getValue(), elem.getOperator());
      replacementExpression = std::make_unique<LiteralFloat>(v);
    }
  } else if (auto literal_double_lhs_ptr = (dynamic_cast<LiteralDouble *>(lhs_ptr))) {
    if (auto lit_rhs_ptr = (dynamic_cast<LiteralDouble *>(rhs_ptr))) {
      auto v = applyOperator(literal_double_lhs_ptr->getValue(), lit_rhs_ptr->getValue(), elem.getOperator());
      replacementExpression = std::make_unique<LiteralDouble>(v);
    }
  } // else: nothing needs to be done
}

void SpecialProgramTransformationVisitor::visit(UnaryExpression &elem) {

  // visit children
  elem.getOperand().accept(*this);
  if (replacementExpression) {
    elem.setOperand(std::move(replacementExpression));
  }

  // try to apply the operator
  if (elem.getOperator()==Operator(LOGICAL_NOT)) {
    if (auto bool_ptr = dynamic_cast<LiteralBool *>(&elem.getOperand())) {
      replacementExpression = std::make_unique<LiteralBool>(!bool_ptr->getValue());
    }
  } else if (elem.getOperator()==Operator(BITWISE_NOT)) {
    if (auto bool_ptr = dynamic_cast<LiteralBool *>(&elem.getOperand())) {
      replacementExpression = std::make_unique<LiteralBool>(!bool_ptr->getValue());
    } else if (auto char_ptr = dynamic_cast<LiteralChar *>(&elem.getOperand())) {
      replacementExpression = std::make_unique<LiteralChar>(~char_ptr->getValue());
    } else if (auto int_ptr = dynamic_cast<LiteralInt *>(&elem.getOperand())) {
      replacementExpression = std::make_unique<LiteralInt>(~int_ptr->getValue());
    }
  }
  // for all "else" case above: don't do anything
  // if the program is valid (type checked)
  // this will never happen anyway

}

void SpecialProgramTransformationVisitor::visit(VariableDeclaration &elem) {
  // TODO (inlining): Introduce a depth threshold (#nodes) to stop inlining if a variable's symbolic value reached a certain depth.
  // Get the value, if it exists
  std::unique_ptr<AbstractExpression> expr_ptr = nullptr;
  if (elem.hasValue()) {
    // visit the expression to simplify it
    elem.getValue().accept(*this);
    if (replacementExpression) {
      expr_ptr = std::move(replacementExpression);
    } else {
      // Note: Since we're deleting this VariableDeclaration after this call,
      // we can "move" the expression into the map, setting elem.value = nullptr
      expr_ptr = elem.takeValue();
    }
  }


  // Build the ScopedIdentifier
  auto scopedIdentifier = ScopedIdentifier(getCurrentScope(), elem.getTarget().getIdentifier());


  // Check that variable wasn't declared before
  if (variableMap.has(scopedIdentifier))
    throw std::runtime_error("Redeclaration of a variable that already exists in this scope: " + printProgram(elem));

  // Insert the variable
  variableMap.insert_or_assign(scopedIdentifier, {elem.getDatatype(), std::move(expr_ptr)});

  // Register the Variable in the current scope (doesn't matter if its already there, because it's a set)
  getCurrentScope().addIdentifier(scopedIdentifier.getId());

  // This VariableDeclaration is now redundant and needs to be removed from the program.
  // However, our parent visit(Block&) will have to handle this
  replaceStatement = true;
  replacementStatement = nullptr;
}

void SpecialProgramTransformationVisitor::visit(Assignment &elem) {
  // visit the value (rhs) expression to simplify it
  elem.getValue().accept(*this);
  if (replacementExpression) elem.setValue(std::move(replacementExpression));

  if (auto var_ptr = dynamic_cast<Variable *>(&elem.getTarget())) {
    // Resolve the identifier to a scopedVariable
    auto scopedIdentifier = getCurrentScope().resolveIdentifier(var_ptr->getIdentifier());

    // Check that we know the type!
    if (!variableMap.has(scopedIdentifier))
      throw std::runtime_error("No type information found for " + scopedIdentifier.getId() + " (Scope: "
                                   + scopedIdentifier.getScope().getScopeName() + ") in : " + printProgram(elem));

    // Update or insert the value
    // Since we're removing this statement after this call, we can "steal" the expression from elem
    auto type = variableMap.get(scopedIdentifier).type;
    variableMap.insert_or_assign(scopedIdentifier, {type, std::move(elem.takeValue())});

    // This VariableDeclaration is now redundant and needs to be removed from the program.
    // However, our parent visit(Block&) will have to handle this
    replaceStatement = true;
    replacementStatement = nullptr;

  } else if (auto ind_ptr = dynamic_cast<IndexAccess *>(&elem.getTarget())) {

    // Simplify the index expression
    ind_ptr->getIndex().accept(*this);
    if (replacementExpression) ind_ptr->setIndex(std::move(replacementExpression));

    // For now, we only support a single level of index access, so target must be a variable
    // TODO (extension): Support matrix accesses, i.e. x[i][j]!
    if (auto ind_var_ptr = dynamic_cast<Variable *>(&ind_ptr->getTarget())) {
      if (!getCurrentScope().identifierExists(ind_var_ptr->getIdentifier())) {
        throw std::runtime_error("Cannot assign to non-declared variable: " + printProgram(elem));
      }

      // Find the variable we are assigning to
      auto scopedIdentifier = getCurrentScope().resolveIdentifier(ind_var_ptr->getIdentifier());

      // If the index could be resolved to a LiteralInt, we might be able to remove this expression
      if (auto lit_ptr = dynamic_cast<LiteralInt *>(&ind_ptr->getIndex())) {
        auto index = lit_ptr->getValue();

        // check if we have a value for this variable already
        if (variableMap.has(scopedIdentifier) && variableMap.get(scopedIdentifier).value) { //update value

          // Create a copy of the value, since we sadly can't take it out of the variableMap :(
          // TODO (extension): Consider finding or creating a more unique_ptr-friendly data structure?
          auto new_val = variableMap.get(scopedIdentifier).value->clone(&elem);

          // check if it's an expression list
          if (auto list = dynamic_cast_unique_ptr<ExpressionList>(std::move(new_val))) {
            // steal the vector for a moment
            auto vec = list->takeExpressions();

            // Make sure it's large enough
            for (size_t i = vec.size(); i <= index; ++i) {
              vec.emplace_back(nullptr);
            }

            // Put in the new value. Since we'll remove elem after this function, we can steal it
            vec.at(index) = elem.takeValue();

            // put the vec back into list
            list->setExpressions(std::move(vec));

            // Put new_val into the variableMap
            auto type = variableMap.get(scopedIdentifier).type;
            variableMap.insert_or_assign(scopedIdentifier, {type, std::move(list)});
          } else {
            // This can occur if we had, e.g., int x = 6; x[7] = 8; which we don't allow for now
            throw std::runtime_error(
                "Cannot assign index of variable that is not vector valued already: " + printProgram(elem));
          }

        } else if (variableMap.has(scopedIdentifier)) { // no values stored so far -> build a new one!
          // let's build a new index_expression that's mostly "undefined" (nullptr)
          // except at the index we are assigning to
          std::vector<std::unique_ptr<AbstractExpression>> list(index + 1);
          // Since we're removing this statement after this call, we can "steal" the expression from elem
          list.at(index) = std::move(elem.takeValue());
          // Now build the ExpressionList
          auto exprlist = std::make_unique<ExpressionList>(std::move(list));
          // and put it into the variableMap
          variableMap.insert_or_assign(scopedIdentifier, {variableMap.get(scopedIdentifier).type, std::move(exprlist)});
        } else {
          throw std::runtime_error(
              "Variable is assigned to but no information found in variableMap: " + printProgram(elem));
        }

        // This VariableDeclaration is now redundant and needs to be removed from the program.
        // However, our parent visit(Block&) will have to handle this
        replaceStatement = true;
        replacementStatement = nullptr;

      } else {
        // if index is a more general expression, we can't do anything
        // TODO (inlining): Emit the declaration (ideally only if it hasn't been done before)
      }

    } else {
      throw std::runtime_error("Cannot handle non-variable-target in index access: " + printProgram(elem));
    }
  }

}

void SpecialProgramTransformationVisitor::visit(Variable &elem) {
  // take scope into account
  auto scopedIdentifier = getCurrentScope().resolveIdentifier(elem.getIdentifier());

  // check  if variable has been declared and error if not
  if (!variableMap.has(scopedIdentifier))
    throw std::runtime_error(
        "Variable not declared: " + printProgram(elem) + " in " + printProgram(elem.getParent()));

  // check if variable has a value and ask the parent to replace it with this, if yes
  if (variableMap.at(scopedIdentifier).value!=nullptr) {
    // Make the parent replace this node with (a copy of) the current value
    // NOTE: Copy-on-Write (COW) would be a useful optimization here
    replacementExpression = std::move(variableMap.at(scopedIdentifier).value->clone(&elem.getParent()));
  }

}

void SpecialProgramTransformationVisitor::visit(IndexAccess &elem) {

  // try to simplify the index
  elem.getIndex().accept(*this);
  if (replacementExpression) elem.setIndex(std::move(replacementExpression));

  // check if the index is compile time known
  if (auto lit_int_ptr = dynamic_cast<LiteralInt *>(&elem.getIndex())) {

    // For now, we only support index access over variables
    if (auto var_ptr = dynamic_cast<Variable *>(&elem.getTarget())) {

      auto scopedIdentifier = getCurrentScope().resolveIdentifier(var_ptr->getIdentifier());

      // check if variable has a value and ask the parent to replace it with the index-th element
      if (variableMap.at(scopedIdentifier).value!=nullptr) {
        auto &expr = variableMap.at(scopedIdentifier).value;
        if (auto list_ptr = dynamic_cast<ExpressionList *>(&*expr)) {
          auto &vec = list_ptr->getExpressionPtrs();
          if (lit_int_ptr->getValue() < vec.size()) {
            if (auto &expr_ptr = vec.at(lit_int_ptr->getValue())) {
              replacementExpression = std::move(expr_ptr->clone(expr_ptr->getParentPtr()));
            }
          }
        }
      } // in all other cases: just leave this be

    } else {
      throw std::runtime_error("Index access must target a variable: " + printProgram(elem));
    }

  } // if it's not compile time known, we can't really simplify it

}

void SpecialProgramTransformationVisitor::visit(Function &elem) {
  enterScope(elem);

  // Register the Variables from the parameters in the variableMap & scope
  for (auto &p : elem.getParameters()) {
    auto scopedIdentifier = ScopedIdentifier(this->getCurrentScope(), p.get().getIdentifier());
    if (variableMap.has(scopedIdentifier))
      throw std::runtime_error(
          "Redeclaration of a variable that already exists in this scope: " + printProgram(elem));
    variableMap.insert_or_assign(scopedIdentifier, {p.get().getParameterType(), nullptr});
    getCurrentScope().addIdentifier(p.get().getIdentifier());
  }

  // Visit the Body
  if (elem.hasBody()) {
    elem.getBody().accept(*this);
  }

  exitScope();
}

void SpecialProgramTransformationVisitor::visit(Block &elem) {
  enterScope(elem);

  // Iterate through statements

  // Because recursive calls might end up re-introducing declarations into the scope (prependStatement),
  // we can't use nice iterators here since they would get invalidated => Undefined Behaviour

  // Instead, we iterate through by index
  // we keep track of the current element we are about to visit
  // and if the number of children changed, we go forward until we see that element again

  auto &stmts = elem.getStatementPointers();
  replaceStatement = false;
  for (size_t i = 0; i < stmts.size(); ++i) {
    if (stmts[i]!=nullptr) {
      // save info
      auto cur_size = stmts.size();
      auto cur_stmt = &*stmts[i];

      // visit child
      cur_stmt->accept(*this);

      // check if someone introduced something into the block
      // if yes, forward our counter by however many were introduced
      if (stmts.size()!=cur_size) {
        // if is mostly pointless, but gives nice line for break point
        i += (stmts.size() - cur_size);
      }

      // Now perform the usual post-child cleanup
      if (replaceStatement) { /*NOLINT NOT always false */
        // Collapse empty block
        if (replacementStatement!=nullptr) {
          if (auto block_ptr = dynamic_cast<Block *>(&*replacementStatement)) {
            if (block_ptr->isEmpty()) {
              replacementStatement = nullptr;
            }
          }
        }

        // Replace statement
        stmts[i] = std::move(replacementStatement);
        replaceStatement = false;
      }
    }
  }

  // Now let the Block itself perform actual removal
  elem.removeNullStatements();

  exitScope();
}

void SpecialProgramTransformationVisitor::visit(ExpressionList &elem) {
  // Temporarily steal the expression vector
  auto vec = elem.takeExpressions();

  // Simplify children
  for (auto &expr : vec) {
    expr->accept(*this);
    if (replacementExpression) expr = std::move(replacementExpression);
  }

  // Return the expression vector
  elem.setExpressions(std::move(vec));
}

void SpecialProgramTransformationVisitor::visit(Return &elem) {
  if (elem.hasValue()) {
    elem.getValue().accept(*this);
    if (replacementExpression) {
      elem.setValue(std::move(replacementExpression));
    }
  }
}

void SpecialProgramTransformationVisitor::visit(For &elem) {
  //Update LoopDepth tracking.
  enteredForLoop();

  // Handle scope
  enterScope(elem);

  /// Before we start simplifying stuff away, we need to let the CFGV do its thing
  /// Otherwise, we would delete, e.g. "int i = 0;" since it's put into the varMap as as i -> LiteralInt(0)

  /// Loop Variables are variables that are both written to and read from during the loop
  auto loopVariables = identifyReadWriteVariables(elem);


  // INITIALIZER

  // Visit initializer. Visiting this is important, in case it affects variables that won't be detected as "loop variables"
  // If we did not visit it, the recursive visit of the body might go wrong!
  // Manually visit the statements in the block, since otherwise Visitor::visit would create a new scope!
  replaceStatement = false;
  for (auto &s : elem.getInitializer().getStatementPointers()) {
    s->accept(*this);
    if (replaceStatement) { /*NOLINT not actually always false */
      s = std::move(replacementStatement);
      replaceStatement = false;
    }
  }
  elem.getInitializer().removeNullStatements();

  // Now, int i = 0 and similar things might have been deleted from AST and are in VariableValuesMap
  // We need to emit Assignments (or Declarations with value if needed) for each of the loop variables Variables into the initializer
  if (!elem.hasInitializer()) { elem.setBody(std::make_unique<Block>()); };
  for (auto &sv : loopVariables) {
    if (&sv.getScope()==&getCurrentScope()) {
      elem.getInitializer().prependStatement(generateVariableDeclaration(sv, &elem.getInitializer()));
    } else {
      elem.getInitializer().prependStatement(generateVariableAssignment(sv, &elem.getInitializer()));
    }
  }

  // The CFGV also returns variables that are read&written in inner loops, which we might not yet be aware of
  { // scope to delete temp variable
    std::unordered_set<ScopedIdentifier> filteredLoopVariables;
    for (const auto &si : loopVariables) {
      if (getCurrentScope().identifierExists(si.getId())) {
        if (getCurrentScope().resolveIdentifier(si.getId()).getScope().getScopeName()==si.getScope().getScopeName()) {
          filteredLoopVariables.insert(si);
        }
      }
    }
    loopVariables = filteredLoopVariables;
  }

  // The values of loop variables we got from the initializer should not be substituted inside the loop
  // Since they will be different in each iteration, CTES should treat them as "compile time unknown"
  // Therefore, we need to remove their values from the variableValues map
  for (auto &si: loopVariables) {
    if (variableMap.has(si)) { // the check is necessary because of nested loops!
      variableMap.insert_or_assign(si, {variableMap.get(si).type, nullptr});
    }
  }

  // CONDITION

  // Visit the condition to simplify it - it's important that we have removed the loop variables before!
  elem.getCondition().accept(*this);
  if (replacementExpression) elem.setCondition(std::move(replacementExpression));


  // BODY

  // Visit Body to simplify it + recursively deal with nested loops
  // This will also update the maxLoopDepth in case there are nested loops
  // This in turn allows us to determine if this For-Loop should be unrolled - see isUnrollLoopAllowed()
  if (elem.hasBody()) {
    elem.getBody().accept(*this);

    // Now, parts of the body statements (e.g. x++) might have been deleted and are only in VariableValuesMap
    // Re-emit all the statements that contain a loop variable!
    for (auto&[si, tv] : variableMap) {
      for (auto &loop_var : loopVariables) {
        if (tv.value && containsVariable(*tv.value, {loop_var.getId()})) {
          auto s = generateVariableAssignment(si, &elem.getBody());
          elem.getBody().appendStatement(std::move(s));
        }
      }
    }

  }

  // UPDATE

  // Visit Update to simplify it + recursively deal with nested loops
  // This will also update the maxLoopDepth in case there are nested loops (which would be weird but possible)
  // This in turn allows us to determine if this For-Loop should be unrolled - see isUnrollLoopAllowed()
  if (elem.hasUpdate()) elem.getUpdate().accept(*this);
  // Now, parts of the body statements (e.g. x++) might have been deleted and are only in VariableValuesMap

  // We have potentially removed stmts from body and update (loop-variable init has already been re-emitted)
  // Go through and re-emit any loop variables into the body:
  if (!elem.hasBody()) { elem.setBody(std::make_unique<Block>()); };
  for (auto &si : loopVariables) {
    // TODO (inlining): Decide when to emit variables
    elem.getBody().appendStatement(generateVariableAssignment(si, &elem.getBody()));
  }


  // At this point, the loop has been visited and some parts have been simplified.
  // Now we are ready to analyze if this loop can additionally also be unrolled:
  // Are we even set to do unrolling at this LoopDepth?
  if (isUnrollLoopAllowed()) {
    // We cannot know if the loop can be fully unrolled without evaluating it
    // So we speculatively unroll the loop until its either too long or we cannot determine something at compile time

    /// Backup of variableMap in case unrolling fails
    TypedVariableValueMap mapBackup;
    for (auto &[sv, tv] : variableMap) {
      mapBackup.insert_or_assign(sv, {tv.type, tv.value ? tv.value->clone(tv.value->getParentPtr()) : nullptr});
    }

    /// Backup of the initializer in case unrolling fails
    auto initializerBackup =
        elem.getInitializer().clone(elem.getInitializer().getParentPtr());

    // Visit the initializer (this will load the loop variables back into variableValues)
    // Manually visit the statements in the block, since otherwise Visitor::visit would create a new scope!
    replaceStatement = false;
    for (auto &s: elem.getInitializer().getStatementPointers()) {
      if (s) {
        s->accept(*this);
        if (replaceStatement) {
          s = std::move(replacementStatement);
          replaceStatement = false;
        }
      }
    }
    elem.getInitializer().removeNullStatements();

    /// Evaluates (a copy of) the condition, returning either 0/1 or -1 if the condition is not compile time known
    auto evaluateCondition = [&](AbstractExpression &condition) -> int {
      /// Operate on a copy, so we can re-use the original condition in the next iteration!
      auto condition_copy = condition.clone(condition.getParentPtr());

      // Evaluate it by visiting
      condition_copy->accept(*this);
      if (replacementExpression) condition_copy = std::move(replacementExpression);

      // Check if it's known.
      // TODO (constant folding): Once BinaryExp handles this better, type should always be bool?
      if (auto bool_ptr = dynamic_cast<LiteralBool *>(&*condition_copy)) {
        return bool_ptr->getValue();
      } else if (auto char_ptr = dynamic_cast<LiteralChar *>(&*condition_copy)) {
        return char_ptr->getValue();
      } else if (auto int_ptr = dynamic_cast<LiteralInt *>(&*condition_copy)) {
        return int_ptr->getValue();
      } else if (auto float_ptr = dynamic_cast<LiteralFloat *>(&*condition_copy)) {
        return (int) float_ptr->getValue();
      } else if (auto double_ptr = dynamic_cast<LiteralDouble *>(&*condition_copy)) {
        return (int) double_ptr->getValue();
      } else {
        return -1; //indicates  unknown at compile time
      }
    };

    /// Block for created unrolled Statements
    auto unrolledBlock = std::make_unique<Block>();

    /// Track iterations
    int numIterations = 0;

    /// Result of testing condition
    int condition = evaluateCondition(elem.getCondition());
    while (condition==1 && numIterations < fullyUnrollLoopMaxNumIterations) {

      // BODY
      if (elem.hasBody()) {
        /// Operate on a copy of the body, so we can reuse it in later iterations
        auto clonedBody = elem.getBody().clone(elem.getBody().getParentPtr());

        // Visit and simplify - again, have to do visit statements manually because of scopes
        for (auto &s: clonedBody->getStatementPointers()) {
          replaceStatement = false;
          s->accept(*this);
          if (replaceStatement) {
            // Empty Block merging
            if (replacementStatement) {
              if (auto block_ptr = dynamic_cast<Block *>(&*replacementStatement)) {
                if (block_ptr->isEmpty()) {
                  replacementStatement = nullptr;
                }
              }
            }
            s = std::move(replacementStatement);
            replaceStatement = false;
          }
        }
        clonedBody->removeNullStatements();

        // If there are any stmts left, transfer them to the unrolledBlock
        for (auto &s: clonedBody->getStatementPointers()) {
          // Empty Block merging
          if (auto block_ptr = dynamic_cast<Block *>(&*s)) {
            if (block_ptr->isEmpty()) {
              s = nullptr;
            }
          }
          unrolledBlock->appendStatement(std::move(s));
        }
      }

      // UPDATE
      if (elem.hasUpdate()) {
        /// Operate on a copy of the update, so we can reuse it in later iterations
        auto clonedUpdate = elem.getUpdate().clone(elem.getUpdate().getParentPtr());

        // Visit and simplify - again, have to do visit statements manually because of scopes
        for (auto &s: clonedUpdate->getStatementPointers()) {
          replaceStatement = false;
          s->accept(*this);
          if (replaceStatement) {
            s = std::move(replacementStatement);
            replaceStatement = false;
          }
        }
        clonedUpdate->removeNullStatements();

        // If there are any stmts left, transfer them to the unrolledBlock
        for (auto &s: clonedUpdate->getStatementPointers()) {
          unrolledBlock->appendStatement(std::move(s));
        }
      }

      // Re-compute condition
      condition = evaluateCondition(elem.getCondition());

      // Increase iteration count
      numIterations++;
    }

    // Cleanup the Block we just created, in case it's empty/has NULL stmts left
    unrolledBlock->removeNullStatements();

    if (condition==0 && numIterations < fullyUnrollLoopMaxNumIterations) {
      // Loop unrolling was successful

      // If we are not the outermost loop, we must re-emit statements
      // we currently emit everything, but probably only need to emit
      // statments that contain loop variables?
      if (currentLoopDepth_maxLoopDepth.first > 1) {

        // TODO (inlining) How to figure out order/dependencies? Order in variable map?
        for (auto&[si, expr] : variableMap) {
          // Don't emit loop variables, since we just got rid of them!
          if (loopVariables.find(si)==loopVariables.end()) {
            //TODO: (inlining) Figure out which statements to emit and how, rather than dumping all
            auto s = generateVariableAssignment(si, &*unrolledBlock);
            unrolledBlock->prependStatement(std::move(s));
          }
        }
      }
      unrolledBlock->removeNullStatements();

      // Inform the parent
      replaceStatement = true;
      replacementStatement = std::move(unrolledBlock);

    } else if (numIterations > fullyUnrollLoopMaxNumIterations) {
      // PARTIAL UNROLLING
      throw std::runtime_error("Partial loop unrolling currently not supported.");
    } else {
      // We don't clean up any excess scopes due to, e.g. if/else statements in the body
      // This is a bit unfortunate, but doesn't seem worth fixing at this point
      // Since they will just exist, but not disturb anything (variable resolution will never touch them)

      // Reset & Restore VariableValueMap
      variableMap = TypedVariableValueMap();
      for (auto &[sv, tv] : mapBackup) {
        variableMap.insert_or_assign(sv, {tv.type, tv.value ? tv.value->clone(tv.value->getParentPtr()) : nullptr});
      }

      /// Restore the initializer
      elem.setInitializer(std::move(initializerBackup));
    }
  }

  // Manual scope handling
  exitScope();

  // Update LoopDepth tracking
  leftForLoop();
}

void SpecialProgramTransformationVisitor::leftForLoop() {
  if (currentLoopDepth_maxLoopDepth.first==1) {
    // if the outermost loop is left, reset the loop depth tracking counter
    currentLoopDepth_maxLoopDepth = std::pair(0, 0);
  } else {
    // if we ascended back to a higher level we only decrement the current depth level counter
    --currentLoopDepth_maxLoopDepth.first;
  }
}

void SpecialProgramTransformationVisitor::enteredForLoop() {
  // Only increase the maximum if we're currently in the deepest level
  // Otherwise, things like for() { for() {}; ...; for() {}; } would give wrong level
  if (currentLoopDepth_maxLoopDepth.first==currentLoopDepth_maxLoopDepth.second) {
    ++currentLoopDepth_maxLoopDepth.second;
  }

  ++currentLoopDepth_maxLoopDepth.first;
}

bool SpecialProgramTransformationVisitor::isUnrollLoopAllowed() const {
  return (currentLoopDepth_maxLoopDepth.second - currentLoopDepth_maxLoopDepth.first
      < maxNumLoopUnrollings);
}

std::unordered_set<ScopedIdentifier>
SpecialProgramTransformationVisitor::identifyReadWriteVariables(For &forLoop) {

  /// Visitor to create Control- and Data-Flow Graphs used to analyze which variables are read and written in Block
  ControlFlowGraphVisitor cfgv;

  // Temporarily "give away" our scope hierarchy to the CFGV
  cfgv.setRootScope(std::move(takeRootScope()));
  // The parent scope is necessary, as the visit would otherwise create a new for block inside this for block!
  cfgv.overrideCurrentScope(&getCurrentScope().getParentScope());

  // Create Control-Flow Graph for blockStmt
  cfgv.visit(forLoop);

  // Build Data-Flow Graph from Control-Flow Graph.
  auto variablesReadAndWritten = cfgv.buildDataFlowGraph();

  // Take back our scope hierarchy from the CFGV
  setRootScope(std::move(cfgv.takeRootScope()));

  return variablesReadAndWritten;
}

std::unique_ptr<VariableDeclaration> SpecialProgramTransformationVisitor::generateVariableDeclaration(const ScopedIdentifier &variable,
                                                                                                      AbstractNode *parent) {
  auto &tv = variableMap.get(variable);
  auto var = std::make_unique<Variable>(variable.getId());
  return std::make_unique<VariableDeclaration>(tv.type, std::move(var), std::move(tv.value->clone(parent)));

}

std::unique_ptr<Assignment> SpecialProgramTransformationVisitor::generateVariableAssignment(const ScopedIdentifier &variable,
                                                                                            AbstractNode *parent) {

  if (!variableMap.has(variable) or variableMap.get(variable).value==nullptr) {
    // if the variable has no value, there's no need to create a variable assignment
    return nullptr;
  } else {
    // get the value and
    auto &tv = variableMap.get(variable);
    auto var = std::make_unique<Variable>(variable.getId());
    return std::make_unique<Assignment>(std::move(var), std::move(tv.value->clone(parent)));
  }

}

void SpecialProgramTransformationVisitor::visit(AbstractExpression &elem) {
  for (auto &c : elem) {
    c.accept(*this);
    replacementExpression = nullptr;
  }
}

// BELOW IS OLD CODE FOR REFERENCE!

//void ProgramTransformationVisitor::visit(Rotate &elem) {
//  Visitor::visit(elem);
//  // if the Rotate's operand is known at compile-time, we can execute the rotation and replace this node by the
//  // rotation's result (i.e., rotated operand)
//  if (hasKnownValue(elem.getOperand()) && hasKnownValue(elem.getRotationFactor())) {
//    auto val = getKnownValue(elem.getOperand());
//    // we need a AbstractLiteral to be able to perform the rotation
//    if (auto valAsAbstractLiteral = dynamic_cast<AbstractLiteral *>(val)) {
//      // clone the AbstractLiteral (including its value)
//      auto clonedVal = valAsAbstractLiteral->clone()->castTo<AbstractLiteral>();
//      // perform rotation on the cloned literal
//      clonedVal->getMatrix()->rotate(getKnownValue(elem.getRotationFactor())->castTo<LiteralInt>()->getValue(), true);
//      // replace this Rotate node by a new node containing the rotated operand
//      elem.getParent()->replaceChild(&elem, clonedVal);
//      enqueueNodeForDeletion(&elem);
//    }
//  }
//}
//
//void ProgramTransformationVisitor::visit(Transpose &elem) {
//  Visitor::visit(elem);
//  // if the Transpose' operand is known at compile-time, we can execute the transpose cmd and replace this node by the
//  // transpose result (i.e., transposed operand)
//  if (hasKnownValue(elem.getOperand())) {
//    auto val = getKnownValue(elem.getOperand());
//    // we need a AbstractLiteral to be able to perform the rotation
//    if (auto valAsAbstractLiteral = dynamic_cast<AbstractLiteral *>(val)) {
//      // clone the AbstractLiteral (including its value)
//      auto clonedVal = valAsAbstractLiteral->clone()->castTo<AbstractLiteral>();
//      // perform transpose on the cloned literal
//      clonedVal->getMatrix()->transpose(true);
//      // replace this Rotate node by a new node containing the rotated operand
//      elem.getParent()->replaceChild(&elem, clonedVal);
//      enqueueNodeForDeletion(&elem);
//    }
//  }
//}
//
//void ProgramTransformationVisitor::visit(MatrixAssignm &elem) {
//  // Do not visit the MatrixElementRef because it would replace the node by a copy of the retrieved value but in a
//  // MatrixAssignm we need to modify the value at the given position instead. However, our implementation does not
//  // allow to retrieve a real (assignable) reference using MatrixElementRef.
//  // Visitor::visit(elem);
//
//  // visit the row and column index
//  auto assignmTarget = elem.getAssignmTarget();
//  assignmTarget->getRowIndex()->accept(*this);
//  if (assignmTarget->getColumnIndex()!=nullptr) {
//    assignmTarget->getColumnIndex()->accept(*this);
//  }
//  elem.getValue()->accept(*this);
//
//  // flag to mark whether to delete this MatrixAssignm node after it has been visited
//  bool enqueueNodeForDeletion_ = false;
//
//  // get operand (matrix) where assignment is targeted to
//  auto operandAsVariable = dynamic_cast<Variable *>(assignmTarget->getOperand());
//  if (operandAsVariable==nullptr) {
//    throw std::logic_error("MatrixAssignm's operand must be a Variable!");
//  }
//
//  // check if the given variable was declared-only and not initialized, i.e., the variable refers to a literal that
//  // has dimension (0,0)
//  auto isNullDimensionLiteral = [&](Variable *var) -> bool {
//    auto varValue =
//        variableValues.getVariableValueDeclaredInThisOrOuterScope(operandAsVariable->getIdentifier(), curScope);
//    if (varValue==nullptr) return false;
//    auto literal = dynamic_cast<AbstractLiteral *>(varValue);
//    if (literal==nullptr) return false;
//    return literal->getMatrix()->getDimensions().equals(0, 0);
//  };
//
//  bool isKnownExecutableMatrixElementAssignment = hasKnownValue(assignmTarget->getRowIndex())
//      && hasKnownValue(assignmTarget->getColumnIndex())
//      && hasKnownValue(assignmTarget->getOperand());
//  bool isKnownExecutableMatrixRowColumnAssignment = hasKnownValue(assignmTarget->getRowIndex())
//      && assignmTarget->getColumnIndex()==nullptr
//      && hasKnownValue(assignmTarget->getOperand());
//
//  if ((isKnownExecutableMatrixElementAssignment || isKnownExecutableMatrixRowColumnAssignment)
//      // Matrix must either have dimension (0,0) or a value of anything != nullptr, otherwise there was a
//      // previous MatrixAssignm that could not be executed, hence it does not make sense to store this assigned value.
//      && (isNullDimensionLiteral(operandAsVariable) || getKnownValue(operandAsVariable)!=nullptr)) {
//
//    // if both indices are literals and we know the referred matrix (i.e., is not an input parameter), we can
//    // execute the assignment and mark this node for deletion afterwards
//    auto rowIdx = getKnownValue(assignmTarget->getRowIndex())->castTo<LiteralInt>()->getValue();
//
//    if (auto valueAsLiteral = dynamic_cast<AbstractLiteral *>(elem.getValue())) {
//      if (isKnownExecutableMatrixElementAssignment) {
//        auto colIdx = getKnownValue(assignmTarget->getColumnIndex())->castTo<LiteralInt>()->getValue();
//        setMatrixVariableValue(operandAsVariable->getIdentifier(), rowIdx, colIdx, valueAsLiteral);
//      } else if (isKnownExecutableMatrixRowColumnAssignment) {
//        appendVectorToMatrix(operandAsVariable->getIdentifier(), rowIdx, valueAsLiteral);
//      }
//    } else {
//      auto sv = variableValues
//          .getVariableEntryDeclaredInThisOrOuterScope(elem.getAssignmTarget()->getVariableIdentifiers()[0], curScope);
//      auto targetVV = variableValues.getVariableValue(sv);
//      if (auto m = dynamic_cast<AbstractLiteral *>(targetVV.getValue())) {
//        // Matrix is currently stored as a Literal
//        //OLD_TODO: Implement
//        throw std::runtime_error("Updating Literal Matrix with AbstractExpr not yet implemented.");
//      } else if (auto m = dynamic_cast<AbstractMatrix *>(targetVV.getValue())) {
//        // Matrix already contains expr elements
//        //OLD_TODO: Implement, should be simpler
//        throw std::runtime_error("Updating AbstractExpr Matrix not yet implemented.");
//      } else {
//        throw std::logic_error("CTES encountered an unexpected Matrix state while trying to perform MatrixAssignm.");
//      }
//    }
//    enqueueNodeForDeletion_ = true;
//  } else { // matrix/indices are not known or there was a previous assignment that could not be executed
//    auto var = variableValues.getVariableEntryDeclaredInThisOrOuterScope(operandAsVariable->getIdentifier(), curScope);
//    auto varValue = variableValues.getVariableValue(var);
//    auto varValueExpr = varValue.getValue();
//    if (varValueExpr==nullptr) {
//      // The Matrix already has the UNKNOWN value (nullptr) assigned, i.e., a MatrixAssignm was visited before that
//      // could not be executed as there were unknown indices involved.
//      // -> Do nothing as we cannot execute or simplify that MatrixAssignm.
//    } else if (varValueExpr->castTo<AbstractLiteral>()->getMatrix()->getDimensions().equals(0, 0)) {
//      // The variable's value is EMPTY (default-initialized literal without any value), i.e., this is the first
//      // MatrixAssignm that could not be executed because there is an unknown index involved.
//      // -> Remove the variable's value in variableValues map to prevent execution of any future MatrixAssignms.
//      // emit a variable declaration statement if there's none present yet because it was marked for deletion after
//      // it has been visited
//      if (emittedVariableDeclarations.count(var)==0) {
//        emitVariableDeclaration(var);
//        // add this MatrixAssignm as dependency to the emitted variable declaration otherwise it will wrongly be
//        // classified as obsolete and deleted after finishing the AST traversal
//        emittedVariableDeclarations.at(var)->addVarAssignm(&elem);
//      }
//      // and then mark the value as UNKNOWN
//      variableValues.setVariableValue(var, VariableValue(varValue.getDatatype(), nullptr));
//    } else {
//      // The variable's value is set, i.e., a previous MatrixAssignm was executed by CTES. As we now encountered a
//      // MatrixAssignm that cannot be executed due to unknown indices, we need to undo the removal of any previous
//      // MatrixAssignms that we executed. This is done by emitting the matrix's current value as a VarAssignm.
//      // Afterwards, we set the matrix's value to UNKNOWN (nullptr) to prevent execution of any future MatrixAssignms.
//      auto varAssignm = emitVariableAssignment(
//          variableValues.getVariableEntryDeclaredInThisOrOuterScope(operandAsVariable->getIdentifier(), curScope));
//
//      //OLD_TODO: THIS CAUSES ITERATOR INVALIDATION ISSUES!!
//      //  INSTEAD: REPLACE CURRENT ELEM WITH NEW BLOCK, LET PARENT DEAL WITH INLINING
//      // and attach the assignment statement immediately before this MatrixAssignm
//      if (auto parentAsBlock = dynamic_cast<Block *>(elem.getParent())) {
//        for (auto &s: varAssignm) {
//          parentAsBlock->addStatement(s);
//        }
//      } else {
//        throw std::logic_error("Cannot add Statement into non-Block environment.");
//      }
//      // and remove the value in variableValues map to avoid saving any further assignments
//      variableValues.setVariableValue(var, VariableValue(varValue.getDatatype(), nullptr));
//    }
//  }
//  if (enqueueNodeForDeletion_) {
//    elem.getParent()->replaceChild(&elem, nullptr);
//    enqueueNodeForDeletion(&elem);
//  }
//}
//
//void ProgramTransformationVisitor::visit(MatrixElementRef &elem) {
//  Visitor::visit(elem);
//  // if this is an expression like "matrix[a][b]" where the operand (matrix) as well as both indices (a,b) are known
//  if (hasKnownValue(elem.getOperand()) && hasKnownValue(elem.getRowIndex()) && hasKnownValue(elem.getColumnIndex())) {
//    // get the row index
//    int rowIndex = getKnownValue(elem.getRowIndex())->castTo<LiteralInt>()->getValue();
//    // get the column index
//    int columnIndex = getKnownValue(elem.getColumnIndex())->castTo<LiteralInt>()->getValue();
//    // get the element at position (row, column)
//    auto matrix = dynamic_cast<AbstractLiteral *>(getKnownValue(elem.getOperand()))->getMatrix();
//    auto retrievedElement = matrix->getElementAt(rowIndex, columnIndex);
//    // replace this MatrixElementRef referred by the parent node by the retrieved element
//    elem.getParent()->replaceChild(&elem, retrievedElement);
//  }
//}
//
//void ProgramTransformationVisitor::visit(GetMatrixSize &elem) {
//  Visitor::visit(elem);
//
//  // if this is an expression like "GetMatrixSize(M, N)" where the matrix M and the dimension N are known
//  if (hasKnownValue(elem.getMatrixOperand()) && hasKnownValue(elem.getDimensionParameter())) {
//    auto matrix = getKnownValue(elem.getMatrixOperand());
//    auto requestedDimension = getKnownValue(elem.getDimensionParameter())->castTo<LiteralInt>();
//    auto matrixAsAbstractLiteral = dynamic_cast<AbstractLiteral *>(matrix);
//    if (matrixAsAbstractLiteral==nullptr) {
//      throw std::logic_error("GetMatrixSize requires an AbstractLiteral subtype as operand. Aborting.");
//    }
//    auto dimSize = matrixAsAbstractLiteral->getMatrix()->getDimensions()
//        .getNthDimensionSize(requestedDimension->getValue());
//    elem.getParent()->replaceChild(&elem, new LiteralInt(dimSize));
//  }
//}
//
//// =====================
//// Simplifiable Statements
//// =====================
//
//void ProgramTransformationVisitor::visit(VarDecl &elem) {
//  // Visit and simplify datatype and initializer (if present)
//  Visitor::visit(elem);
//
//  // determine the variable's value
//  AbstractExpression *variableValue;
//  auto variableInitializer = elem.getInitializer();
//  if (variableInitializer==nullptr) {
//    // Default initialization
//    variableValue = AbstractLiteral::createLiteralBasedOnDatatype(elem.getDatatype());
//  } else {
//    variableValue = variableInitializer;
//  }
//  // store the variable's value
//  auto sv = ScopedVariable(elem.getIdentifier(), curScope);
//  auto vv = VariableValue(*elem.getDatatype(), variableValue);
//  variableValues.addDeclaredVariable(sv, vv);
//
//  // we no longer need this node or its children, since the value is now in the variableValues map
//  elem.getParent()->replaceChild(&elem, nullptr);
//  enqueueNodeForDeletion(&elem);
//}
//
//void ProgramTransformationVisitor::visit(VarAssignm &elem) {
//  Visitor::visit(elem);
//  // store the variable's value
//  auto var = variableValues.getVariableEntryDeclaredInThisOrOuterScope(elem.getVarTargetIdentifier(), curScope);
//  auto newVV = VariableValue(variableValues.getVariableValue(var).getDatatype(), elem.getValue());
//  variableValues.setVariableValue(var, newVV);
//
//  // Delete this node
//  elem.getParent()->replaceChild(&elem, nullptr);
//  enqueueNodeForDeletion(&elem);
//}
//
//void ProgramTransformationVisitor::visit(ArithmeticExpr &elem) {
//  // transform this ArithmeticExpr into an OperatorExpr
//  if (elem.hasParent()) {
//    auto op = elem.getOperator();
//    op->removeFromParent();
//    std::vector<AbstractExpression *> operands{elem.getLeft(), elem.getRight()};
//    elem.getLeft()->takeFromParent();
//    elem.getRight()->takeFromParent();
//    auto operatorExpr = new OperatorExpr(op, operands);
//    elem.getParent()->replaceChild(&elem, operatorExpr);
//    enqueueNodeForDeletion(&elem);
//    operatorExpr->accept(*this);
//  } else {
//    Visitor::visit(elem);
//  }
//}
//
//void ProgramTransformationVisitor::visit(LogicalExpr &elem) {
//  // transform this LogicalExpr into an OperatorExpr
//  if (elem.hasParent()) {
//    auto op = elem.getOperator();
//    op->removeFromParent();
//    std::vector<AbstractExpression *> operands{elem.getLeft(), elem.getRight()};
//    elem.getLeft()->takeFromParent();
//    elem.getRight()->takeFromParent();
//    auto operatorExpr = new OperatorExpr(op, operands);
//    elem.getParent()->replaceChild(&elem, operatorExpr);
//    enqueueNodeForDeletion(&elem);
//    operatorExpr->accept(*this);
//  } else {
//    Visitor::visit(elem);
//  }
//}
//
//void ProgramTransformationVisitor::visit(UnaryExpr &elem) {
//  Visitor::visit(elem);
//
//  // check if the unary expression can be evaluated such that we can replace the whole node by its evaluation result
//  if (dynamic_cast<AbstractLiteral *>(elem.getRight())) {
//    // if operand value is known -> evaluate the expression and store its result
//    auto result = evaluateNodeRecursive(&elem, getTransformedVariableMap());
//    // replace this UnaryExpr by the evaluation's result
//    elem.getParent()->replaceChild(&elem, result.front());
//  } else if (elem.hasParent()) {
//    // if this UnaryExpr cannot be evaluated, replace the UnaryExpr by an OperatorExpr
//    auto op = elem.getOperator();
//    op->removeFromParent();
//    std::vector<AbstractExpression *> operands{elem.getRight()};
//    elem.getRight()->takeFromParent();
//    auto operatorExpr = new OperatorExpr(op, operands);
//    elem.getParent()->replaceChild(&elem, operatorExpr);
//  }
//  enqueueNodeForDeletion(&elem);
//}
//
//void ProgramTransformationVisitor::visit(OperatorExpr &elem) {
//  // In case that this OperatorExpr has been created recently by transforming an Arithmetic-/LogicalExpr into this
//  // OperatorExpr, the operands will be visited again. This is needless but acceptable. In other cases it is
//  // important to revisit the operands, for example, if this statement was modified or cloned (e.g., during loop
//  // unrolling) and we have new knowledge (e.g., variable values) that must be taken into account while visiting the
//  // operands.
//  Visitor::visit(elem);
//
//  // Check if any of the operands is itself an OperatorExpr with the same symbol such that its operands can be merged
//  // into this OperatorExpr. As we do not consider the operator's commutativity, we need to take care of the operands'
//  // order while merging. For example:
//  //
//  //   Before merging:                After merging:
//  //   ┌───┬───┬───┬───┬───┬───┐     ┌───┬───┬───┬───┬───┬───┬───┐
//  //   │ + │ a │ │ │ b │ c │ d │     │ + │ a │ e │ f │ b │ c │ d │
//  //   └───┴───┴─┼─┴───┴───┴───┘     └───┴───┴───┴───┴───┴───┴───┘
//  //             │
//  //       ┌───┬─▼─┬───┐
//  //       │ + │ e │ f │
//  //       └───┴───┴───┘
//  std::vector<AbstractExpression *> newOperands;
//  auto operatorAndOperands = elem.getChildren();
//  // start by operatorAndOperands.begin() + 1 to skip the first child (operator)
//  for (auto it = operatorAndOperands.begin() + 1; it!=operatorAndOperands.end(); ++it) {
//    auto operandAsOperatorExpr = dynamic_cast<OperatorExpr *>(*it);
//    // check if this operand is an OperatorExpr of the same symbol
//    if (operandAsOperatorExpr!=nullptr
//        && operandAsOperatorExpr->getOperator()->equals(elem.getOperator()->getOperatorSymbol())) {
//      auto operandsToBeAdded = operandAsOperatorExpr->getOperands();
//      // go through all operands of this sub-OperatorExpr, remove each from its current parent and add it as operand to
//      // this OperatorExpr
//      for (auto &operand : operandsToBeAdded) {
//        operand->takeFromParent();
//        newOperands.push_back(operand->castTo<AbstractExpression>());
//      }
//      // mark the obsolete OperatorExpr child for deletion
//      elem.replaceChild(*it, nullptr);
//      enqueueNodeForDeletion(*it);
//    } else {
//      // if this operand is not an OperatorExpr, we also need to remove it from this OperatorExpr because re-adding it
//      // as operand would otherwise lead to having two times the same parent
//      (*it)->takeFromParent();
//      newOperands.push_back((*it)->castTo<AbstractExpression>());
//    }
//  }
//  // replaced the operands by the merged list of operands also including those operands that were not an OperatorExpr
//  auto curOperator = elem.getOperator();
//  curOperator->removeFromParent();
//  elem.setAttributes(curOperator, newOperands);
//
//  // if this OperatorExpr is a logical expression, try to apply Boolean laws to further simplify this expression
//  if (elem.isLogicalExpr()) simplifyLogicalExpr(elem);
//
//  // If there is only one operand in this OperatorExpr left then we already applied the operator on all operands, hence
//  // we can replace the whole OperatorExpr by its operand.
//  // Exception: If this is an unary operator (e.g., !a) containing an unknown operand (e.g., Variable a) then this
//  // replacement is not legal. This case is excluded by !elem.getOperator()->isUnaryOp().
//  if (elem.getOperands().size()==1 && !elem.getOperator()->isUnaryOp()) {
//    elem.getParent()->replaceChild(&elem, elem.getOperands().at(0));
//  }
//}
//
//void ProgramTransformationVisitor::simplifyLogicalExpr(OperatorExpr &elem) {
//  // Simplifying this OperatorExpr using Boolean laws is only applicable if this OperatorExpr contains a logical
//  // operator and there are at least two operands because we'll potentially remove one operand. If there exists already
//  // only one operand, the operand must be moved up to the parent instead (not handled by simplifyLogicalExpr).
//  if (!elem.isLogicalExpr() || elem.getOperands().size() <= 1) {
//    return;
//  }
//
//  // collect all known operands from current OperatorExpr
//  std::vector<AbstractLiteral *> knownOperands;
//  for (auto &c : elem.getOperands()) {
//    auto valueAsAbstractLiteral = dynamic_cast<AbstractLiteral *>(c);
//    if (valueAsAbstractLiteral!=nullptr && !valueAsAbstractLiteral->getMatrix()->containsAbstractExprs()) {
//      knownOperands.push_back(valueAsAbstractLiteral);
//    }
//  }
//  // The following logic requires that there is exactly one known operand. This should be the case if there are any
//  // known operands because logical operators (AND, OR, XOR) are commutative and thus adding new operands always
//  // performs aggregation. For example, adding (true XOR false XOR false XOR a) to OperatorExpr will partially evaluate
//  // the expression and add (true XOR a) instead. Hence there should be at most one known operand, if there is any
//  // known operand at all.
//  if (knownOperands.size()!=1) return;
//
//  // retrieve the known operand and the logical operator
//  auto knownOperand = knownOperands.at(0);
//  LogCompOp logicalOperator = std::get<LogCompOp>(elem.getOperator()->getOperatorSymbol());
//
//  // In case that the expression can be simplified, resulting in a single-element expression (e.g., AND false) then it
//  // is the caller's responsibility to accordingly replace this OperatorExpr by the only operand.
//
//  if (logicalOperator==LOGICAL_AND) {
//    // - knownOperand == false: replace the whole expression by False as <anything> AND False is always False
//    // - knownOperand == true: remove True from the expression as <anything> AND True only depends on <anything>
//    if (knownOperand->isEqual(new LiteralBool(false))) {
//      elem.setAttributes(elem.getOperator(), {knownOperand});
//    } else if (knownOperand->isEqual(new LiteralBool(true))) {
//      elem.removeOperand(knownOperand);
//    }
//  } else if (logicalOperator==LOGICAL_OR) {
//    // - knownOperand == true: replace whole expression by True as <anything> OR True is always True
//    // - knownOperand == false: remove False from the expression as <anything> OR False only depends on <anything>
//    if (knownOperand->isEqual(new LiteralBool(true))) {
//      elem.setAttributes(elem.getOperator(), {knownOperand});
//    } else if (knownOperand->isEqual(new LiteralBool(false))) {
//      elem.removeOperand(knownOperand);
//    }
//  } else if (logicalOperator==LOGICAL_XOR && knownOperand->isEqual(new LiteralBool(false))) {
//    // - knownOperand == false: remove False from the expression as <anything> XOR False always depends on <False>
//    // - knownOperand == true [not implemented]: rewrite <anything> XOR True to !<anything>
//    elem.removeOperand(knownOperand);
//  }
//}
//
//void ProgramTransformationVisitor::visit(Block &elem) {
//  Visitor::visit(elem);
//  cleanUpBlock(elem);
//
//  //OLD_TODO: If we come to the end of a scope, do we need to emit variables?
//}
//
//void ProgramTransformationVisitor::visit(Call &elem) {
//  Return *returnStmt = (elem.getFunc()!=nullptr) ? dynamic_cast<Return *>(elem.getFunc()->getBodyStatements().back())
//                                                 : nullptr;
//  // only perform inlining if...
//  // there is a Return in the called function
//  if (returnStmt!=nullptr) {
//    auto returnStatementDescendants = returnStmt->getDescendants();
//    if (// the Return statement does not have more than 20 descendant nodes ("threshold")
//        returnStatementDescendants.size() <= 20
//            // their is exactly one return value (because assignment of multiple values cannot be expressed yet
//            && returnStmt->getReturnExpressions().size()==1) {
//      // replace variables values of Call with those in called function
//      auto parameterValues = elem.getParameterList()->getParameters();
//      auto expectedFunctionParameters = elem.getFunc()->getParameterList()->getParameters();
//      if (parameterValues.size()!=expectedFunctionParameters.size()) {
//        throw std::invalid_argument("Number of given and expected parameters in Call does not match!");
//      }
//      // generate a map consisting of "variableIdentifier : variableValue" entries where variableIdentifier is the name
//      // of the variable within the called function and variableValue the value (literal or variable) that is passed as
//      // value for that identifier as part of the function call
//      std::unordered_map<std::string, AbstractExpression *> varReplacementMap;
//      for (int i = 0; i < parameterValues.size(); ++i) {
//        auto variable = expectedFunctionParameters[i]->getValue()->castTo<Variable>();
//        auto entry = std::make_pair(variable->getIdentifier(), parameterValues[i]->getValue());
//        varReplacementMap.insert(entry);
//      }
//      for (auto &node : returnStatementDescendants) {
//        // if the current node is a Variable node and it is a function parameter -> replace it
//        auto nodeAsVariable = dynamic_cast<Variable *>(node);
//        if (nodeAsVariable!=nullptr && varReplacementMap.count(nodeAsVariable->getIdentifier()) > 0) {
//          node->getParent()->replaceChild(node,
//                                          varReplacementMap.at(nodeAsVariable->getIdentifier())->clone());
//        }
//      }
//
//      // remove return expression from its parent (return statement) and replace Call by extracted return statement
//      auto parentNode = elem.getParent();
//      auto returnExpr = returnStmt->getReturnExpressions().front();
//      returnExpr->takeFromParent();
//      parentNode->replaceChild(&elem, returnExpr);
//
//      // revisit subtree as new simplification opportunities may exist now
//      parentNode->accept(*this);
//    }
//  }
//  Visitor::visit(elem);
//}
//
//void ProgramTransformationVisitor::visit(If &elem) {
//  // Bypass the base Visitor's logic and directly visit the condition only because we need to know whether it is
//  // evaluable at runtime (or not) and its result.
//  elem.getCondition()->accept(*this);
//
//  // OLD_TODO: Rewriting should only happen if the condition is runtime-known and secret.
//  //  If the condition is public and runtime-known, the If-statement should NOT be rewritten because can be handled
//  //  more efficient by the runtime system. This check requires information from the ControlFlowGraphVisitor that
//  //  does not support variable-scoping yet.
//
//  // ================
//  // Case 1: Condition's evaluation result is KNOWN at compile-time
//  // -> we can delete the branch that is not executed and move or remove contained statements prior deleting the whole
//  //    If statement including all of its children.
//  // ================
//  if (dynamic_cast<LiteralBool *>(elem.getCondition())!=nullptr) {
//    // If we know the condition's value, we can eliminate the branch that is never executed.
//    // We need to detach this branch immediately, otherwise the variableValues map will only contain the values of the
//    // last visited branch (but we need the values of the branch that is always executed instead).
//    auto thenAlwaysExecuted = elem.getCondition()->castTo<LiteralBool>()->getValue();
//    if (thenAlwaysExecuted) { // the Then-branch is always executed
//      // recursively remove the Else-branch (sanity-check, may not necessarily exist)
//      if (elem.getElseBranch()!=nullptr) {
//        auto elseBranch = elem.getElseBranch();
//        // we also unlink it from the If statement such that it will not be visited
//        elem.setElseBranch(nullptr);
//        enqueueNodeForDeletion(elseBranch);
//      }
//    } else {  // the Else-branch is always executed
//      // recursively remove the Then-branch (always exists)
//      auto then = elem.getThenBranch();
//      elem.setThenBranch(nullptr);
//      enqueueNodeForDeletion(then);
//      // negate the condition and delete the conditions stored value (is now invalid)
//      auto condition = elem.getCondition();
//      condition->takeFromParent();
//      auto newCondition = new OperatorExpr(new Operator(UnaryOp::NEGATION), {condition});
//      // replace the If statement's Then branch by the Else branch
//      auto newThen = elem.getElseBranch();
//      elem.setElseBranch(nullptr); //necessary to avoid double parent link
//      elem.setAttributes(newCondition, newThen, nullptr);
//    }
//
//    // continue visiting the remaining branches: the condition will be visited again, but that's ok
//    Visitor::visit(elem);
//
//    // we can remove the whole If statement if...
//    if ( // the Then-branch is always executed and it is empty after simplification (thus queued for deletion)
//        (thenAlwaysExecuted && elem.getThenBranch()==nullptr)
//            // the Then-branch is never executed but there is no Else-branch
//            || (!thenAlwaysExecuted && elem.getElseBranch()==nullptr)
//                // the Else-branch is always executed but it is empty after simplification  (thus queued for deletion)
//            || (!thenAlwaysExecuted && elem.getElseBranch()==nullptr)) {
//      // enqueue the If statement and its children for deletion
//      elem.getParent()->replaceChild(&elem, nullptr);
//      enqueueNodeForDeletion(&elem);
//    }
//  }
//    // ================
//    // Case 2: Condition's evaluation result is UNKNOWN at compile-time (i.e., known at runtime)
//    // -> rewrite variables values that are modified in either one or both of the If statement's branches such that the
//    //    variable's value depends on the If statement's condition evaluation result.
//    // ================
//  else { // if we don't know the evaluation result of the If statement's condition -> rewrite the If statement
//    // create a copy of the variableValues map and removableNodes map
//    auto originalVariableValues = variableValues;
//
//    // visit the thenBranch and store its modifications
//    elem.getThenBranch()->accept(*this);
//    auto variableValuesAfterVisitingThen = variableValues;
//
//    // check if there is an Else-branch that we need to visit
//    bool hadElseBranch = elem.getElseBranch()!=nullptr;
//    if (hadElseBranch) {
//      // restore the original map via copy assignment prior visiting Else-branch
//      variableValues = originalVariableValues;
//      // visit the Else-branch
//      elem.getElseBranch()->accept(*this);
//    }
//
//    // rewrite those entries that were modified in either one or both maps
//    // note: up to this point (and beyond), the Else-branch's modifications are in variableValues
//    for (auto &[variableIdentifier, originalValue] : originalVariableValues.getMap()) {
//      // check if the variable was changed in the Then-branch
//      auto thenBranchValue = variableValuesAfterVisitingThen.getVariableValue(variableIdentifier).getValue();
//      auto thenBranchModifiedCurrentVariable = (thenBranchValue!=originalValue.getValue());
//      // check if the variable was changed in the Else-branch
//      // if there is no Else-branch, elseBranchModifiedCurrentVariable stays False
//      bool elseBranchModifiedCurrentVariable = false;
//      AbstractExpression *elseBranchValue = nullptr;
//      //Check elseBranch directly longer works because the else branch might have self-eliminated
//      if (hadElseBranch) {
//        elseBranchValue = variableValues.getVariableValue(variableIdentifier).getValue();
//        elseBranchModifiedCurrentVariable = (elseBranchValue!=originalValue.getValue());
//      }
//
//      // Determine if an If statement-dependent value needs to be assigned to the variable.
//      // The following approach only rewrites those variables that were modified in the Then-branch, the Else-branch, or
//      // both branches. It does, however, drop new variables (i.e., variable declarations) that happened in one of the
//      // branches. Those variables are anyway out of scope when leaving the branch, so there's no need to store their
//      // value in variableValues map.
//      AbstractExpression *newValue;
//      if (thenBranchModifiedCurrentVariable && elseBranchModifiedCurrentVariable) {
//        newValue = generateIfDependentValue(elem.getCondition(), thenBranchValue, elseBranchValue);
//      } else if (thenBranchModifiedCurrentVariable) {
//        newValue = generateIfDependentValue(elem.getCondition(), thenBranchValue, originalValue.getValue());
//      } else if (elseBranchModifiedCurrentVariable) {
//        newValue = generateIfDependentValue(elem.getCondition(), originalValue.getValue(), elseBranchValue);
//      } else {
//        // otherwise neither one of the two branches modified the variable's value and we can keep it unchanged
//        continue;
//      }
//      // assign the new If statement-dependent value (e.g., myVarIdentifier = condition*32+[1-condition]*11)
//      auto newVariableValue =
//          VariableValue(originalVariableValues.getVariableValue(variableIdentifier).getDatatype(), newValue);
//      originalVariableValues.setVariableValue(variableIdentifier, newVariableValue);
//    }
//    // restore the original map that contains the merged changes from the visited branches
//    variableValues = originalVariableValues;
//
//    // enqueue the If statement and its children for deletion
//    elem.getParent()->replaceChild(&elem, nullptr);
//    enqueueNodeForDeletion(&elem);
//  }
//}
//
//void ProgramTransformationVisitor::visit(While &elem) {
//  // visit the condition only
//  elem.getCondition()->accept(*this);
//
//  // check if we know the While condition's truth value at compile time
//  auto conditionValue = dynamic_cast<LiteralBool *>(elem.getCondition());
//  if (conditionValue!=nullptr && !conditionValue->getValue()) {
//    // While is never executed: remove While-loop including contained statements
//    elem.getParent()->replaceChild(&elem, nullptr);
//    enqueueNodeForDeletion(&elem);
//    return;
//  }
//
//  // visit body (and condition again, but that's acceptable)
//  Visitor::visit(elem);
//}
//
////AbstractNode *ProgramTransformationVisitor::doPartialLoopUnrolling(For &elem) {
////  // create a new Block statemeny: necessary for cleanup loop and to avoid overriding user-defined variables that expect
////  // the initalizer to be within the for-loop's scope only
////  auto blockEmbeddingLoops = new Block();
////
////  // move initializer from For-loop into newly added block
////  auto forLoopInitializer = elem.getInitializer();
////  forLoopInitializer->removeFromParent();
////  blockEmbeddingLoops->addChild(forLoopInitializer);
////
////  // replace this For-loop in its parent node by the new block and move the For-loop into the block
////  elem.getParent()->replaceChild(&elem, blockEmbeddingLoops);
////  blockEmbeddingLoops->addChild(&elem);
////
////  // create copies to allow reverting changes made by visiting nodes
////  // - a copy of known variables with their values
////  auto variableValuesBackup = getClonedVariableValuesMap();
////  // - a copy of all nodes marked for deletion
////  auto nodesQueuedForDeletionCopy = nodesQueuedForDeletion;
////  // - a copy of the whole For-loop including initializer, condition, update stmt, body (required for cleanup loop)
////  auto cleanupForLoop = elem.clone(false)->castTo<For>();
////  auto ignrd =
////      identifyReadWriteVariables(*cleanupForLoop->getBody()->castTo<Block>(), VariableValuesMapType());
////  // visit the condiiton, body, and update statement to make required replacements (e.g., Arithmetic/LogicalExpr to
////  // OperatorExpr)
////  cleanupForLoop->getCondition()->accept(*this);
////  cleanupForLoop->getBody()->accept(*this);
////  cleanupForLoop->getUpdate()->accept(*this);
////  // undo changes made by visiting the condition, body, and update statement: we need them for the cleanup loop and
////  // do not want them to be deleted
////  variableValues = variableValuesBackup;
////  nodesQueuedForDeletion = nodesQueuedForDeletionCopy;
////
////  // visit the intializer
////  forLoopInitializer->accept(*this);
////
////  // update the nodesQueuedForDeletion as the initializer's VarDecl will be emitted later by calling
////  // emitVariableAssignments
////  nodesQueuedForDeletionCopy = nodesQueuedForDeletion;
////  auto variableValuesAfterVisitingInitializer = getClonedVariableValuesMap();
////  // determine the loop variables, i.e., variables changed in the initializer statement
////  auto loopVariablesMap = getChangedVariables(variableValuesBackup);
////
////  // the new for-loop body containing the unrolled statements
////  auto unrolledForLoopBody = new Block();
////  // Generate the unrolled loop statements that consists of:
////  //   <all statements of the body with symbolic loop variable for iteration 1>
////  //   <update statement>
////  //   <loop condition>   <- this will be moved out of the body into the For-loop's condition field afterwards
////  //   ... repeat NUM_CIPHERTEXT_SLOTS times ...
////  std::vector<Return *> tempReturnStmts;
////  for (int i = 0; i < ctes.partiallyUnrollLoopNumCipherslots; i++) {
////    // for each statement in the for-loop's body
////    for (auto &stmt : elem.getBody()->castTo<Block>()->getStatements()) {
////      // clone the body statement and append the statement to the unrolledForLoop Body
////      unrolledForLoopBody->addChild(stmt->clone(false));
////    }
////    // temporarily add the condition such that the variables are replaced (e.g., i < 6 -> i+1 < 6 -> i+2 < 6 -> ...)
////    // we use a Return statement here as it does not write anything into the variableValues map
////    auto retStmt = new Return(elem.getCondition()->clone());
////    tempReturnStmts.push_back(retStmt);
////    unrolledForLoopBody->addChild(retStmt);
////
////    // add a copy of the update statement, visiting the body then automatically handles the iteration variable in the
////    // cloned loop body statements - no need to manually adapt them
////    unrolledForLoopBody->addChild(elem.getUpdate()->clone(false));
////  }
////  // replace the for loop's body by the unrolled statements
////  elem.replaceChild(elem.getBody(), unrolledForLoopBody);
////
////  // delete update statement from loop since it's now incorporated into the body but keep a copy since we need it
////  // for the cleanup loop
////  elem.getUpdate()->removeFromParent();
////
////  // Erase any variable from variableValues that is written in the loop's body such that it is not replaced by any
////  // known value while visiting the body's statements. This includes the iteration variable as we moved the update
////  // statement into the loop's body.
////  auto readAndWrittenVars =
////      identifyReadWriteVariables(*elem.getBody()->castTo<Block>(), VariableValuesMapType());
////
////  variableValuesBackup = getClonedVariableValuesMap();
////
////  // restore the copy, otherwise the initializer visited after creating this copy would be marked for deletion
////  nodesQueuedForDeletion = nodesQueuedForDeletionCopy;
////
////  // visit the for-loop's body to do inlining
////  auto variableValuesBeforeVisitingLoopBody = getClonedVariableValuesMap();
////  visitingUnrolledLoopStatements = true;
////  elem.getBody()->accept(*this);
////  visitingUnrolledLoopStatements = false;
////
////
////  // Move the expressions of the temporarily added Return statements into the For-loop's condition by combining all
////  // conditions using a logical-AND. Then remove all temporarily added Return statements.
////  std::vector<AbstractExpr *> newConds;
////  for (auto rStmt : tempReturnStmts) {
////    rStmt->removeFromParent(true);
////    auto expr = rStmt->getReturnExpressions().front();
////    expr->removeFromParent(true);
////    newConds.push_back(expr);
////    enqueueNodeForDeletion(rStmt);
////  }
////  auto originalCondition = elem.getCondition();
////  elem.replaceChild(originalCondition, new OperatorExpr(new Operator(LOGICAL_AND), newConds));
////  enqueueNodeForDeletion(originalCondition);
////
////  // OLD_TODO: Future work:  Make this entire thing flexible with regard to num_slots_in_ctxt, i.e., allow changing how long
////  //  unrolled loops are. Idea: generate all loops (see below cleanup loop ideas) starting from ludacriously large
////  //  number, later disable/delete the ones that are larger than actually selected cipheretxt size determined from
////  //  parameters?
////  // ISSUE: Scheme parameters might depend on loop length? This is a general issue (no loop bound => no bounded depth)
////
////  // find all variables that were changed in the for-loop's body - even iteration vars (important for condition!) - and
////  // emit them, i.e, create a new variable assignment for each variable
////  // OLD_TODO: Do not emit any variable assignments in the for-loop's body if a variable's maximum depth is reached as this
////  //  leads to wrong results. This must be considered when introducing the cut-off for "deep variables".
////  auto bodyChangedVariables = getChangedVariables(variableValuesBeforeVisitingLoopBody);
////  std::list<AbstractNode *> emittedVarAssignms;
////  for (auto it = bodyChangedVariables.begin(); it!=bodyChangedVariables.end(); ++it) {
////    // Create exactly one statement for each variable that is changed in the loop's body. This implicitly is the
////    // statement with the "highest possible degree" of inlining for this variable. Make sure that we emit the loop
////    // variables AFTER the body statements.
////    auto emittedVarAssignm = emitVariableAssignment(it);
////    if (emittedVarAssignm!=nullptr) {
////      if (loopVariablesMap.count(it->first) > 0) {
////        // if this is a loop variable - add the statement at the *end* of the body
////        emittedVarAssignms.push_back(emittedVarAssignm);
////      } else {
////        // if this is not a loop variable - add the statement at the *beginning* of the body
////        emittedVarAssignms.push_front(emittedVarAssignm);
////      }
////    }
////    // Remove all the variables that were changed within the body from variableValues as inlining them in any statement
////    // after/outside the loop does not make any sense. We need to keep the variable and only remove the value by
////    // setting it to nullptr, otherwise we'll lose information.
////    (*it).second->value = nullptr;
////  }
////  // append the emitted loop body statements
////  elem.getBody()->castTo<Block>()->addChildren(
////      std::vector<AbstractNode *>(emittedVarAssignms.begin(), emittedVarAssignms.end()), true);
////
////  // OLD_TODO: Future work (maybe): for large enough num_..., e.g. 128 or 256, it might make sense to have a binary series
////  //  of cleanup loops, e.g., if 127 iterations are left, go to 64-unrolled loop, 32-unrolled loop, etc.
////  //  When to cut off? -> empirical decision?
////
////  // Handle remaining iterations using a "cleanup loop": place statements in a separate loop for remaining iterations
////  // and attach the generated cleanup loop to the newly added Block. The cleanup loop consists of:
////  // - initializer: reused from the unrolled loop as the loop variable is declared out of the first for-loop thus
////  // still accessible by the cleanup loop.
////  // - condition, update statement, body statements: remain the same as in the original For loop.
////  blockEmbeddingLoops->addChild(cleanupForLoop);
////
////  variableValues = variableValuesBackup;
////
////  return blockEmbeddingLoops;
////}
//
//// =====================
//// Helper methods
//// =====================
//
//AbstractExpression *ProgramTransformationVisitor::generateIfDependentValue(
//    AbstractExpression *condition, AbstractExpression *trueValue, AbstractExpression *falseValue) {
//  // We need to handle the case where trueValue or/and falseValue are null because in that case the dependent
//  // statement can be simplified significantly by removing one/both operands of the arithmetic expression.
//
//  // determine whether one or both of the provided expressions (trueValue, falseValue) are null
//  auto exprIsNull = [](AbstractExpression *expr) {
//    if (expr==nullptr) return true;
//    auto castedExpr = dynamic_cast<AbstractLiteral *>(expr);
//    return castedExpr!=nullptr && castedExpr->isNull();
//  };
//  auto trueValueIsNull = exprIsNull(trueValue);
//  auto falseValueIsNull = exprIsNull(falseValue);
//
//  // check whether both values are null
//  if (trueValueIsNull && falseValueIsNull) {
//    // case: trueValue == 0 && falseValue == 0 => expression always equals null, independent of condition's eval result
//    // return a cloned copy of trueValue because we cannot directly create a new object (e.g., LiteralInt) as we do
//    // not exactly know which subtype of AbstractLiteral trueValue has
//    // return "0" (where 0 is of the respective input type)
//    return trueValue->clone();
//  }
//
//  // check if exactly one of both values are null
//  if (trueValueIsNull) {
//    // factorIsFalse = [1-ifStatementCondition]
//    auto factorIsFalse = new OperatorExpr(new Operator(ArithmeticOp::SUBTRACTION),
//                                          {new LiteralInt(1), condition->clone()});
//    // case: trueValue == 0 && falseValue != 0 => value is 0 if the condition is True
//    // -> return (1-b)*falseValue
//    return new OperatorExpr(new Operator(MULTIPLICATION), {factorIsFalse, falseValue});
//  } else if (falseValueIsNull) {
//    // factorIsTrue = ifStatementCondition
//    auto factorIsTrue = condition->clone();
//    // case: trueValue != 0 && falseValue == 0 => value is 0 if the condition is False
//    // -> return condition * trueValue
//    return new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION), {factorIsTrue, trueValue});
//  }
//
//  // default case: trueValue != 0 && falseValue != 0 => value is changed in both branches of If statement
//  // -> return condition*trueValue + (1-b)*falseValue.
//  // factorIsTrue = ifStatementCondition
//  auto factorIsTrue = condition->clone();
//  // factorIsFalse = [1-ifStatementCondition]
//  auto factorIsFalse = new OperatorExpr(new Operator(ArithmeticOp::SUBTRACTION),
//                                        {new LiteralInt(1), condition->clone()});
//  return new OperatorExpr(
//      new Operator(ArithmeticOp::ADDITION),
//      {new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
//                        {factorIsTrue, trueValue->clone()}),
//       new OperatorExpr(new Operator(ArithmeticOp::MULTIPLICATION),
//                        {factorIsFalse, falseValue->clone()})});
//}
//
//void ProgramTransformationVisitor::appendVectorToMatrix(const std::string &variableIdentifier, int posIndex,
//                                                           AbstractLiteral *matrixRowOrColumn) {
//  auto pMatrix = matrixRowOrColumn->getMatrix();
//  AbstractMatrix *vec = pMatrix->clone();
//
//  auto var = variableValues.getVariableEntryDeclaredInThisOrOuterScope(variableIdentifier, curScope);
////  if (iterator->second->value==nullptr) {
////    std::stringstream errorMsg;
////    errorMsg << "appendVectorToMatrix failed: ";
////    errorMsg << "Could not find entry in variableValues for variable identifier " << iterator->first.first << " ";
////    errorMsg << "by starting search from scope " << iterator->first.second->getScopeIdentifier() << ".";
////    throw std::runtime_error(errorMsg.str());
////  }
//
//  // on contrary to simple scalars, we do not need to replace the variable in the variableValues map, instead we
//  // need to retrieve the associated matrix and set the element at the specified (row, column)
//  auto literal = dynamic_cast<AbstractLiteral *>(variableValues.getVariableValue(var).getValue());
//  if (literal==nullptr) {
//    std::stringstream errorMsg;
//    errorMsg << "appendVectorToMatrix failed: " << "Current value of matrix " << var.getIdentifier() << " ";
//    errorMsg << "in variableValues is nullptr. ";
//    errorMsg << "This should never happen and indicates that an earlier visited MatrixAssignm could not be executed.";
//    errorMsg << "Because of that any subsequent MatrixAssignms should not be executed too.";
//    throw std::runtime_error(errorMsg.str());
//  }
//
//  // store the value at the given position - matrix must handle indices and make sure that matrix is large enough
//  literal->getMatrix()->appendVectorAt(posIndex, vec);
//  // Update the VariableValuesMap with this new matrix
//  variableValues.setVariableValue(var, VariableValue(variableValues.getVariableValue(var).getDatatype(), literal));
//
//}
//
//void ProgramTransformationVisitor::setMatrixVariableValue(const std::string &variableIdentifier,
//                                                             int row,
//                                                             int column,
//                                                             AbstractExpression *matrixElementValue) {
//  AbstractExpression *valueToStore = nullptr;
//  if (matrixElementValue!=nullptr) {
//    // clone the given value and detach it from its parent
//    valueToStore = matrixElementValue->clone();
//    valueToStore->takeFromParent();
//  }
//
//  auto var = variableValues.getVariableEntryDeclaredInThisOrOuterScope(variableIdentifier, curScope);
//
//  // on contrary to simple scalars, we do not need to replace the variable in the variableValues map, instead we
//  // need to retrieve the associated matrix and set the element at the specified (row, column)
//  auto literal = dynamic_cast<AbstractLiteral *>(variableValues.getVariableValue(var).getValue());
//  if (literal==nullptr) {
//    std::stringstream errorMsg;
//    errorMsg << "setMatrixValue failed: ";
//    errorMsg << "Current value of matrix " << var.getIdentifier() << " ";
//    errorMsg << "in variableValues is nullptr. ";
//    errorMsg << "This should never happen and indicates that an earlier visited MatrixAssignm could not be executed.";
//    errorMsg << "Because of that any subsequent MatrixAssignms should not be executed too.";
//    throw std::runtime_error(errorMsg.str());
//  }
//
//  // store the value at the given position - matrix must handle indices and make sure that matrix is large enough
//  literal->getMatrix()->setElementAt(row, column, valueToStore);
//  // Update the VariableValuesMap with this new matrix
//  variableValues.setVariableValue(var, VariableValue(variableValues.getVariableValue(var).getDatatype(), literal));
//
//}
//
//void ProgramTransformationVisitor::emitVariableDeclaration(ScopedVariable variableToEmit) {
//  auto parent = variableToEmit.getScope()->getScopeOpener();
//  auto children = parent->getChildren();
//  auto varValue = variableValues.getVariableValue(variableToEmit);
//  // if this variable is not a scalar, we need to emit the variable value too, otherwise the information about the
//  // matrix dimension will be lost!
//  VarDecl *newVarDeclaration;
//  auto varAsLiteral = dynamic_cast<AbstractLiteral *>(varValue.getValue());
//  if (varAsLiteral!=nullptr && !varAsLiteral->getMatrix()->getDimensions().equals(1, 1)) {
//    newVarDeclaration = new VarDecl(variableToEmit.getIdentifier(),
//                                    new Datatype(varValue.getDatatype()),
//                                    varValue.getValue()->clone());
//  } else {
//    Datatype *d = new Datatype(varValue.getDatatype());
//    newVarDeclaration = new VarDecl(variableToEmit.getIdentifier(), d);
//  }
//
//  // passing position in children vector is req. to prepend the new VarAssignm (i.e., as new first child of parent)
//  auto parentAsBlock = parent->castTo<Block>();
//  //OLD_TODO: Solve prepending invalidation issues/maybe not there because whenever we NULL something we leave a slot for re-insertion?
//  parentAsBlock->addStatement({newVarDeclaration});
//  emittedVariableDeclarations.emplace(variableToEmit, new EmittedVariableData(newVarDeclaration));
//}
//
