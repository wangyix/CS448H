#include "ast.h"

#include <stdio.h>

void LiteralLength::print() {
  printf("%d", value);
  if (shares) {
    printf("s");
  }
}

void FunctionLength::print() {
  printf("#");
  if (shares) {
    printf("s");
  }
}

// -------------------------------------------------------------------------------------------------


void StringLiteral::print() {
  length->print();
  printf("'%s'", str.c_str());
}

void RepeatedChar::print() {
  length->print();
  printf("'%c'", c);
}

void Block::print() {
  printf("[");
  for (int i = 0; i < children.size(); ++i) {
    if (i == greedyChildIndex) {
      printf(" ");
      greedyChild->print();
    }
    printf(" ");
    children[i]->print();
  }
  printf(" ]^");
  topFiller->print();
  printf("v");
  bottomFiller->print();
}

// -------------------------------------------------------------------------------------------------

void GreedyRepeatedChar::print() {
  printf("{%c}", c);
}

void Words::print() {
  printf("{w ");
  if (wordSilhouette) {
    printf("->%c ", wordSilhouette);
  }
  interword->print();
  printf("}");
}
