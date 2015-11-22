#ifndef AST_H
#define AST_H

#include <vector>

struct GreedyLengthContent;

struct SpecifiedLength {
  int value;
  bool shares;
};

struct SpecifiedLengthContent {
  SpecifiedLength length;
};

struct StringLiteral : public SpecifiedLengthContent {
  std::string str;
};

struct RepeatedChar : public SpecifiedLengthContent {
  char c;
};

struct Block : public SpecifiedLengthContent {
  std::vector<SpecifiedLengthContent*> children;
  GreedyLengthContent* greedyChild;
  RepeatedChar topFiller;
  RepeatedChar bottomFiller;
};


struct GreedyLengthContent {
};

struct GreedyRepeatedChar : public GreedyLengthContent {
  char c;
};

struct Words : public GreedyLengthContent {
  char* source;
  SpecifiedLengthContent* interword;
  char wordSilhouette;        // use '\0' if unused
  char interwordSilhouette;   // use '\0' if unused
};

#endif
