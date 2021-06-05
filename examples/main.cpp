#include <vector>
#include <iostream>
#include <fstream>

#include "main.h"
#include "ast_opt/ast/Literal.h"
#include "ast_opt/ast/Variable.h"
#include "ast_opt/ast/Assignment.h"

int main(int argc, char** argv) {

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

  if (argc < 3) {
    std::cerr << "Program ast_demo called with illegal arguments! Expected: ./ast_demo <benchmark_name> <output_filename> where "
              << "<benchmark_name> is any of: {demo}, and <output_filename> is the filename of the benchmark results."
              << std::endl;
    return EXIT_FAILURE;
  }

  std::string benchmark_name = argv[1];
  std::string target_filename = argv[2];
  if (benchmark_name == "demo") {
    std::ofstream file;
    file.open(target_filename);

    // TODO add code for running demo benchmark
    // for demonstration, we just write some random values (from SoK CSV) into a file here instead
    file << "t_keygen,t_input_encryption,t_computation,t_decryption\n"
              << "415,1117,38315,37"
              << std::endl;

    file.close();
  } else {
    std::cerr << "Given benchmark name " << benchmark_name << " is not valid." << std::endl;
    return EXIT_FAILURE;
  }
}


