#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <assert.h>

typedef int(*LengthFunc)(int);

struct SpecifiedLength;
struct SpecifiedLengthContent;

typedef std::unique_ptr<SpecifiedLength> SpecifiedLengthPtr;
typedef std::unique_ptr<SpecifiedLengthContent> SpecifiedLengthContentPtr;


// Used for specifying interword and vertical fillers. It's a RepeatedChar with a literal length.
struct Filler {
  Filler(int length, bool shares, char c) : length(length), shares(shares), c(c) {}
  void print() const;

  int length;
  bool shares;
  char c;
};

struct Words {
  Words() : interword(1, false, ' '), wordSilhouette('\0') {}
  Words(const std::string& source, const Filler& interword, char wordSilhouette = '\0')
    : source(source), interword(interword),
    wordSilhouette(wordSilhouette) {}
  void print() const;

  std::string source;
  Filler interword;
  char wordSilhouette;        // use '\0' if unused
};


// -------------------------------------------------------------------------------------------------

struct SpecifiedLength {
  SpecifiedLength(bool shares = false) : shares(shares) {}
  virtual ~SpecifiedLength() {}
  virtual void print() const = 0;

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(int value, bool shares = false) : SpecifiedLength(shares), value(value) {}
  void print() const override;

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(LengthFunc lengthFunc, bool shares = false)
    : SpecifiedLength(shares), lengthFunc(lengthFunc) {}
  void print() const override;

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLengthContent {
  SpecifiedLengthContent(SpecifiedLengthPtr length) : length(std::move(length)) {}
  virtual ~SpecifiedLengthContent() {}
  virtual void print() const = 0;

  SpecifiedLengthPtr length;
};

struct StringLiteral : public SpecifiedLengthContent {
  StringLiteral(const std::string& str)
    : SpecifiedLengthContent(SpecifiedLengthPtr(new LiteralLength((int)str.length(), false)))
    , str(str) {}
  void print() const override;

  std::string str;
};

struct RepeatedChar : public SpecifiedLengthContent {
  RepeatedChar(SpecifiedLengthPtr length, char c)
    : SpecifiedLengthContent(std::move(length)), c(c) {}
  void print() const override;

  char c;
};

struct Block : public SpecifiedLengthContent {
  Block(SpecifiedLengthPtr length)
    : SpecifiedLengthContent(std::move(length)),
    greedyChildIndex(-1), topFiller(0, false, ' '), bottomFiller(0, false, ' ') {}
  void addChild(SpecifiedLengthContentPtr child);
  void addGreedyChild(const Words& words);
  bool hasGreedyChild() const;
  void print() const override;

  std::vector<SpecifiedLengthContentPtr> children;
  int greedyChildIndex;   // use value < 0 if no greedy child
  Words greedyChild;
  Filler topFiller;
  Filler bottomFiller;
};

#endif
