#include "ast.h"

#include <stdio.h>

void Filler::print() {
  printf("%d", length);
  if (shares) {
    printf("s");
  }
  printf("'%c'", c);
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

void Block::addChild(SpecifiedLengthContentPtr child) {
  children.push_back(std::move(child));
}
void Block::addGreedyChild(const Words& words) {
  greedyChildIndex = children.size();
  greedyChild = words;
}
bool Block::hasGreedyChild() const {
  return (greedyChildIndex >= 0);
}
void Block::print() {
  length->print();
  printf("[");
  for (int i = 0; i <= children.size(); ++i) {
    if (i == greedyChildIndex) {
      printf(" ");
      greedyChild.print();
    }
    if (i != children.size()) {
      printf(" ");
      children[i]->print();
    }
  }
  printf(" ]^");
  topFiller.print();
  printf("v");
  bottomFiller.print();
}
