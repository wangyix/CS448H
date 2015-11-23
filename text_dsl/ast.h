#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <assert.h>

typedef int(*LengthFunc)(int);

struct SpecifiedLength;
struct SpecifiedLengthContent;
struct GreedyLengthContent;

typedef std::unique_ptr<SpecifiedLength> SpecifiedLengthPtr;
typedef std::unique_ptr<SpecifiedLengthContent> SpecifiedLengthContentPtr;
typedef std::unique_ptr<GreedyLengthContent> GreedyLengthContentPtr;

// Used for specifying interword and vertical fillers. It's a RepeatedChar with a literal length.
struct Filler {
  Filler(int length, bool shares, char c) : length(length), shares(shares), c(c) {}
  void print();

  int length;
  bool shares;
  char c;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLength {
  SpecifiedLength(bool shares = false) : shares(shares) {}
  virtual ~SpecifiedLength() {}
  virtual void print() = 0;

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(int value, bool shares = false) : SpecifiedLength(shares), value(value) {}
  void print() override;

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(LengthFunc lengthFunc, bool shares = false)
    : SpecifiedLength(shares), lengthFunc(lengthFunc) {}
  void print() override;

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLengthContent {
  SpecifiedLengthContent(SpecifiedLengthPtr length) : length(std::move(length)) {}
  virtual ~SpecifiedLengthContent() {}
  virtual void print() = 0;

  SpecifiedLengthPtr length;
};

struct StringLiteral : public SpecifiedLengthContent {
  StringLiteral(const std::string& str)
    : SpecifiedLengthContent(SpecifiedLengthPtr(new LiteralLength((int)str.length(), false)))
    , str(str) {}
  void print() override;

  std::string str;
};

struct RepeatedChar : public SpecifiedLengthContent {
  RepeatedChar(SpecifiedLengthPtr length, char c)
    : SpecifiedLengthContent(std::move(length)), c(c) {}
  void print() override;

  char c;
};

struct Block : public SpecifiedLengthContent {
  Block(SpecifiedLengthPtr length,
    std::vector<SpecifiedLengthContentPtr> children,
    GreedyLengthContentPtr greedyChild, int greedyChildIndex,
    const Filler& topFiller, const Filler& bottomFiller)
    : SpecifiedLengthContent(std::move(length)),
    children(std::move(children)), greedyChild(std::move(greedyChild)),
    greedyChildIndex(this->greedyChild ? greedyChildIndex : -1),
    topFiller(topFiller), bottomFiller(bottomFiller) {
    assert(!greedyChild || (0 <= greedyChildIndex && greedyChildIndex <= children.size()));
  }
  void print() override;

  std::vector<SpecifiedLengthContentPtr> children;
  GreedyLengthContentPtr greedyChild;
  int greedyChildIndex;
  Filler topFiller;
  Filler bottomFiller;
};

// -------------------------------------------------------------------------------------------------

struct GreedyLengthContent {
  virtual ~GreedyLengthContent() {};
  virtual void print() = 0;
};

struct GreedyRepeatedChar : public GreedyLengthContent {
  GreedyRepeatedChar(char c) : c(c) {}
  void print() override;

  char c;
};

struct Words : public GreedyLengthContent {
  Words(const std::string& source, const Filler& interword, char wordSilhouette = '\0')
    : source(source), interword(interword),
    wordSilhouette(wordSilhouette) {}
  void print() override;

  std::string source;
  Filler interword;
  char wordSilhouette;        // use '\0' if unused
};

#endif
