
#ifndef MASTER_THESIS_CODE_LITERALINT_H
#define MASTER_THESIS_CODE_LITERALINT_H


#include "Literal.h"

class LiteralInt : public Literal {
public:

    int value;

    LiteralInt(int value);

    void print();
};


#endif //MASTER_THESIS_CODE_LITERALINT_H
