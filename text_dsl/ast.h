#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>

typedef int(*LengthFunc)(int);

struct SpecifiedLength;
struct SpecifiedLengthContent;
struct GreedyLengthContent;

typedef std::unique_ptr<SpecifiedLength> SpecifiedLengthPtr;
typedef std::unique_ptr<SpecifiedLengthContent> SpecifiedLengthContentPtr;
typedef std::unique_ptr<GreedyLengthContent> GreedyLengthContentPtr;

// -------------------------------------------------------------------------------------------------

struct SpecifiedLength {
  SpecifiedLength(bool shares = false) : shares(shares) {}
  virtual ~SpecifiedLength() {}

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(int value, bool shares = false) : SpecifiedLength(shares), value(value) {}

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(LengthFunc lengthFunc, bool shares = false)
    : SpecifiedLength(shares), lengthFunc(lengthFunc) {}

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLengthContent {
  SpecifiedLengthContent(SpecifiedLengthPtr length) : length(std::move(length)) {}
  virtual ~SpecifiedLengthContent() {}

  SpecifiedLengthPtr length;
};

struct StringLiteral : public SpecifiedLengthContent {
  StringLiteral(const std::string& str)
    : SpecifiedLengthContent(SpecifiedLengthPtr(new LiteralLength(str.length(), false)))
    , str(str) {}

  std::string str;
};

struct RepeatedChar : public SpecifiedLengthContent {
  RepeatedChar(SpecifiedLengthPtr length, char c)
    : SpecifiedLengthContent(std::move(length)), c(c) {}

  char c;
};

struct Block : public SpecifiedLengthContent {
  Block(SpecifiedLengthPtr length,
    std::vector<SpecifiedLengthContentPtr>& children, GreedyLengthContentPtr greedyChild,
    const RepeatedChar& topFiller, const RepeatedChar& bottomFiller)
    : SpecifiedLengthContent(std::move(length)),
    children(children), greedyChild(std::move(greedyChild)),
    topFiller(topFiller), bottomFiller(bottomFiller) {}

  std::vector<SpecifiedLengthContentPtr> children;
  GreedyLengthContentPtr greedyChild;
  RepeatedChar topFiller;
  RepeatedChar bottomFiller;
};

// -------------------------------------------------------------------------------------------------

struct GreedyLengthContent {
  virtual ~GreedyLengthContent() {};
};

struct GreedyRepeatedChar : public GreedyLengthContent {
  GreedyRepeatedChar(char c) : c(c) {}

  char c;
};

struct Words : public GreedyLengthContent {
  Words(const std::string& source, SpecifiedLengthContentPtr interWord,
    char wordSilhouette = '\0', char interwordSilhouette = '\0')
    : source(source), interword(std::move(interword)),
    wordSilhouette(wordSilhouette), interwordSilhouette(interwordSilhouette) {}

  std::string source;
  SpecifiedLengthContentPtr interword;
  char wordSilhouette;        // use '\0' if unused
  char interwordSilhouette;   // use '\0' if unused
};

#endif
