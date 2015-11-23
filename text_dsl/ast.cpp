#include "ast.h"

#include <stdio.h>

void Filler::print() {
  printf("%d", length);
  if (shares) {
    printf("s");
  }
  printf("'%c'", c);
}

// -------------------------------------------------------------------------------------------------

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
  length->print();
  printf("[");
  for (int i = 0; i < children.size(); ++i) {
    if (i == greedyChildIndex) {
      printf(" ");
      greedyChild->print();
    }
    printf(" ");
    children[i]->print();
  }
  if (greedyChildIndex == children.size()) {
    printf(" ");
    greedyChild->print();
  }
  printf(" ]^");
  topFiller.print();
  printf("v");
  bottomFiller.print();
}

// -------------------------------------------------------------------------------------------------

void GreedyRepeatedChar::print() {
  printf("{%c}", c);
}

void Words::print() {
  printf("{w");
  if (wordSilhouette) {
    printf("->'%c'", wordSilhouette);
  } 
  printf(" ");
  interword.print();
  printf("}");
}
