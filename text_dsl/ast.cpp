#include "ast.h"

#include <stdio.h>

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
  length.print();
  printf("'%s'", str.c_str());
}

void RepeatedChar::print() const {
  length.print();
  printf("'%c'", c);
}

// -------------------------------------------------------------------------------------------------

void RepeatedCharFuncLength::print() const {
  length.print();
  printf("'%c'", c);
}

void Words::print() const {
  printf("{w");
  if (wordSilhouette) {
    printf("->'%c'", wordSilhouette);
  }
  if (!interwordFillers.empty()) {
    printf(" ");
    for (const FillerPtr& filler : interwordFillers) {
      filler->print();
    }
  }
  printf("}");
}

void Block::print() const {
  length.print();
  printf("[");
  for (const ASTPtr& child : children) {
    printf(" ");
    child->print();
  }
  printf(" ]^{");
  for (const FillerPtr& filler : topFillers) {
    filler->print();
  }
  printf("}v{");
  for (const FillerPtr& filler : bottomFillers) {
    filler->print();
  }
  printf("}");
}
void Block::addChild(ASTPtr child) {
  children.push_back(std::move(child));
}
void Block::addGreedyChild(ASTPtr words) {
  assert(!hasGreedyChild());
  greedyChildIndex = children.size();
  children.push_back(std::move(words));
}
bool Block::hasGreedyChild() const {
  return greedyChildIndex >= 0;
}
