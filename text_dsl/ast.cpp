#include "ast.h"

#include <stdio.h>

void Filler::print() const {
  printf("%d", length);
  if (shares) {
    printf("s");
  }
  printf("'%c'", c);
}

void Words::print() const {
  printf("{w");
  if (wordSilhouette) {
    printf("->'%c'", wordSilhouette);
  }
  printf(" ");
  interword.print();
  printf("}");
}

// -------------------------------------------------------------------------------------------------

void LiteralLength::print() const {
  printf("%d", value);
  if (shares) {
    printf("s");
  }
}

void FunctionLength::print() const {
  printf("#");
  if (shares) {
    printf("s");
  }
}

// -------------------------------------------------------------------------------------------------

void StringLiteral::print() const {
  length->print();
  printf("'%s'", str.c_str());
}

void RepeatedChar::print() const {
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
void Block::print() const {
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
