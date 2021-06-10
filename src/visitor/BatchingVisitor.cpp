#include <stdexcept>
#include "ast_opt/visitor/BatchingVisitor.h"
#include "ast_opt/visitor/controlFlowGraph/ControlFlowGraphVisitor.h"
#include "ast_opt/visitor/ProgramPrintVisitor.h"


std::string SpecialBatchingVisitor::printProgram(AbstractNode &node) {
  std::stringstream ss;
  ProgramPrintVisitor ppv(ss);
  node.accept(ppv);
  return ss.str();
}


void SpecialBatchingVisitor::visit(Block &elem) {
//  enterScope(elem);
//
//  // Iterate through statements
//
//  // Because recursive calls might end up re-introducing declarations into the scope (prependStatement),
//  // we can't use nice iterators here since they would get invalidated => Undefined Behaviour
//
//  // Instead, we iterate through by index
//  // we keep track of the current element we are about to visit
//  // and if the number of children changed, we go forward until we see that element again
//
//  auto &stmts = elem.getStatementPointers();
//  replaceStatement = false;
//  for (size_t i = 0; i < stmts.size(); ++i) {
//    if (stmts[i]!=nullptr) {
//      // save info
//      auto cur_size = stmts.size();
//      auto cur_stmt = &*stmts[i];
//
//      // visit child
//      cur_stmt->accept(*this);
//
//      // check if someone introduced something into the block
//      // if yes, forward our counter by however many were introduced
//      if (stmts.size()!=cur_size) {
//        // if is mostly pointless, but gives nice line for break point
//        i += (stmts.size() - cur_size);
//      }
//
//      // Now perform the usual post-child cleanup
//      if (replaceStatement) { /*NOLINT NOT always false */
//        // Collapse empty block
//          if (auto block_ptr = dynamic_cast<Block *>(&*replacementStatement)) {
//            if (block_ptr->isEmpty()) {
//              replacementStatement = nullptr;
//            }
//          }
//        }
//
//        // Replace statement
//        stmts[i] = std::move(replacementStatement);
//        replaceStatement = false;
//      }
//    }
//  }
//
//  // Now let the Block itself perform actual removal
//  elem.removeNullStatements();
//
//  exitScope();
}

bool SpecialBatchingVisitor::containsVariable(const AbstractNode &node,
                                              const std::vector<std::string> &identifiers) {
//  if (auto var_ptr = dynamic_cast<const Variable *>(&node)) {
//    for (auto &id: identifiers) {
//      if (var_ptr->getIdentifier()==id)
//        return true;
//    }
//  } else {
//    for (auto &c: node) {
//      if (containsVariable(c, identifiers))
//        return true;
//    }
//  }
//  return false;
}

void SpecialBatchingVisitor::visit(VariableDeclaration &elem) {
//  // Build the ScopedIdentifier
//  auto scopedIdentifier = ScopedIdentifier(getCurrentScope(), elem.getTarget().getIdentifier());
//
//  // Check that variable wasn't declared before
//  if (variableMap.has(scopedIdentifier))
//    throw std::runtime_error("Redeclaration of a variable that already exists in this scope: " + printProgram(elem));
//
//  // Insert the variable
//  // TODO: Decide scalar/vector by looking at if it's secret!
//  variableMap.insert_or_assign(scopedIdentifier, {elem.getDatatype(), nullptr});
//
//  // Register the Variable in the current scope (doesn't matter if its already there, because it's a set)
//  getCurrentScope().addIdentifier(scopedIdentifier.getId());
}

void SpecialBatchingVisitor::visit(Assignment &elem) {
//  // visit the value (rhs) expression to simplify it
//  elem.getValue().accept(*this);
//  if (replacementExpression) elem.setValue(std::move(replacementExpression));
//
//  // We only care about index access
//  if (auto ind_ptr = dynamic_cast<IndexAccess *>(&elem.getTarget())) {
//
//    // Simplify the index expression
//    ind_ptr->getIndex().accept(*this);
//    if (replacementExpression) ind_ptr->setIndex(std::move(replacementExpression));
//
//    // For now, we only support a single level of index access, so target must be a variable
//    // TODO (extension): Support matrix accesses, i.e. x[i][j]!
//    if (auto ind_var_ptr = dynamic_cast<Variable *>(&ind_ptr->getTarget())) {
//      if (!getCurrentScope().identifierExists(ind_var_ptr->getIdentifier())) {
//        throw std::runtime_error("Cannot assign to non-declared variable: " + printProgram(elem));
//      }
//
//      // Find the variable we are assigning to
//      auto scopedIdentifier = getCurrentScope().resolveIdentifier(ind_var_ptr->getIdentifier());
//
//      // If the index could be resolved to a LiteralInt, we might be able to remove this expression
//      if (auto lit_ptr = dynamic_cast<LiteralInt *>(&ind_ptr->getIndex())) {
//        auto index = lit_ptr->getValue();
//
//        // check if we have a value for this variable already
//        if (variableMap.has(scopedIdentifier) && variableMap.get(scopedIdentifier).value) { //update value
//
//          // Create a copy of the value, since we sadly can't take it out of the variableMap :(
//          // TODO (extension): Consider finding or creating a more unique_ptr-friendly data structure?
//          auto new_val = variableMap.get(scopedIdentifier).value->clone(&elem);
//
//          // check if it's an expression list
//          if (auto list = dynamic_cast_unique_ptr<ExpressionList>(std::move(new_val))) {
//            // steal the vector for a moment
//            auto vec = list->takeExpressions();
//
//            // Make sure it's large enough
//            for (size_t i = vec.size(); i <= index; ++i) {
//              vec.emplace_back(nullptr);
//            }
//
//            // Put in the new value. Since we'll remove elem after this function, we can steal it
//            vec.at(index) = elem.takeValue();
//
//            // put the vec back into list
//            list->setExpressions(std::move(vec));
//
//            // Put new_val into the variableMap
//            auto type = variableMap.get(scopedIdentifier).type;
//            variableMap.insert_or_assign(scopedIdentifier, {type, std::move(list)});
//          } else {
//            // This can occur if we had, e.g., int x = 6; x[7] = 8; which we don't allow for now
//            throw std::runtime_error(
//                "Cannot assign index of variable that is not vector valued already: " + printProgram(elem));
//          }
//
//        } else if (variableMap.has(scopedIdentifier)) { // no values stored so far -> build a new one!
//          // let's build a new index_expression that's mostly "undefined" (nullptr)
//          // except at the index we are assigning to
//          std::vector<std::unique_ptr<AbstractExpression>> list(index + 1);
//          // Since we're removing this statement after this call, we can "steal" the expression from elem
//          list.at(index) = std::move(elem.takeValue());
//          // Now build the ExpressionList
//          auto exprlist = std::make_unique<ExpressionList>(std::move(list));
//          // and put it into the variableMap
//          variableMap.insert_or_assign(scopedIdentifier, {variableMap.get(scopedIdentifier).type, std::move(exprlist)});
//        } else {
//          throw std::runtime_error(
//              "Variable is assigned to but no information found in variableMap: " + printProgram(elem));
//        }
//
//        // This VariableDeclaration is now redundant and needs to be removed from the program.
//        // However, our parent visit(Block&) will have to handle this
//        replaceStatement = true;
//        replacementStatement = nullptr;
//
//      } else {
//        // if index is a more general expression, we can't do much
//
//        //  throw std::runtime_error("Currently unsupported");
//        //  // Emit the declaration if one doesn't exist?
//        //  // This needs to go *before* our current statement
//        //  // and it will include all the known state about it (if that exists)
//        //  if (&scopedIdentifier.getScope()==&getCurrentScope()) {
//        //    auto d = generateVariableDeclaration(scopedIdentifier, elem.getParentPtr());
//        //    if (auto block_ptr = dynamic_cast<Block *>(&elem.getParent())) {
//        //      block_ptr->prependStatement(std::move(d));
//        //      // TODO (inlining): This should do something with the variable map, right?
//        //    } else if (auto for_ptr = dynamic_cast<For *>(&elem.getParent())) {
//        //      for_ptr->getBody().prependStatement(std::move(d));
//        //      // TODO (inlining): Should this be erased from the variableMap?
//        //      //  that would probably lead to similar issues with undeclared variables
//        //      //  as when we did that for the initializer emits
//        //      //  instead, we might need to have a "fake return" for emitted declarations, too!
//        //
//        //      // TODO (inlining): updating the parent means we have to do the same annoying loop thing as in visit(Block&)
//        //      throw std::runtime_error("Currently unsupported, requires more handling in visit(For&)");
//        //    } else {
//        //      throw std::runtime_error("Unexpected parent for : " + printProgram(elem));
//        //    }
//        //
//        //  } else {
//        //    // since we're leaving this update statement here,
//        //    // we have to set the value (for the entire vector) to unknown in the variableMap
//        //  }
//
//      }
//
//    } else {
//      throw std::runtime_error("Cannot handle non-variable-target in index access: " + printProgram(elem));
//    }
//  }

}

void SpecialBatchingVisitor::visit(IndexAccess &elem) {
//
//  // try to simplify the index
//  elem.getIndex().accept(*this);
//  if (replacementExpression) elem.setIndex(std::move(replacementExpression));
//
//  // check if the index is compile time known
//  if (auto lit_int_ptr = dynamic_cast<LiteralInt *>(&elem.getIndex())) {
//
//    // For now, we only support index access over variables
//    if (auto var_ptr = dynamic_cast<Variable *>(&elem.getTarget())) {
//
//      auto scopedIdentifier = getCurrentScope().resolveIdentifier(var_ptr->getIdentifier());
//
//      // check if variable has a value and ask the parent to replace it with the index-th element
//      if (variableMap.at(scopedIdentifier).value!=nullptr) {
//        auto &expr = variableMap.at(scopedIdentifier).value;
//        if (auto list_ptr = dynamic_cast<ExpressionList *>(&*expr)) {
//          auto &vec = list_ptr->getExpressionPtrs();
//          if (lit_int_ptr->getValue() < vec.size()) {
//            if (auto &expr_ptr = vec.at(lit_int_ptr->getValue())) {
//              replacementExpression = std::move(expr_ptr->clone(expr_ptr->getParentPtr()));
//            }
//          }
//        }
//      } // in all other cases: just leave this be
//
//    } else {
//      throw std::runtime_error("Index access must target a variable: " + printProgram(elem));
//    }
//
//  } // if it's not compile time known, we can't really simplify it

}

//void SpecialBatchingVisitor::visit(AbstractExpression &elem) {
//  for (auto &c : elem) {
//    c.accept(*this);
//    replacementExpression = nullptr;
//  }
//}
