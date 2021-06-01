#include <vector>

#include "main.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/ast/Assignment.h"

int main() {

  // Create Literal nodes directly
  Literal<bool> a(false);
  Literal<char> b('b');
  Literal<int> c(0);
  Literal<float> d(0.f);
  Literal<double> e(0.);
  Literal<std::string> f("");

  // Create Variable node
  Variable v("foo");

  // Create a VariableAssignment node (boo = false;)
  Assignment va(std::make_unique<Variable>("boo"), std::make_unique<Literal<bool>>(false));
}


