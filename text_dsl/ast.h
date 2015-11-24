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


struct AST {
  AST(const char* f_at) : f_at(f_at) {}
  virtual void print() const = 0;
  const char* f_at;
};

// -------------------------------------------------------------------------------------------------

// Used for specifying interword and vertical fillers. It's a RepeatedChar with a literal length.
struct Filler : public AST {
  Filler(const char* f_at, int length, bool shares, char c) : AST(f_at),
    length(length), shares(shares), c(c) {}
  void print() const override;

  int length;
  bool shares;
  char c;
};

struct Words : public AST {
  Words(const char* f_at) : AST(f_at), interword(NULL, 1, false, ' '), wordSilhouette('\0') {}
  Words(const char* f_at, const std::string& source, const Filler& interword, char wordSilhouette = '\0')
    : AST(f_at), source(source), interword(interword),
    wordSilhouette(wordSilhouette) {}
  void print() const override;

  std::string source;
  Filler interword;
  char wordSilhouette;        // use '\0' if unused
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLength : public AST {
  SpecifiedLength(const char* f_at, bool shares) : AST(f_at), shares(shares) {}
  virtual ~SpecifiedLength() {}
  virtual void print() const = 0;

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(const char* f_at, int value, bool shares)
    : SpecifiedLength(f_at, shares), value(value) {}
  void print() const override;

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(const char* f_at, LengthFunc lengthFunc, bool shares)
    : SpecifiedLength(f_at, shares), lengthFunc(lengthFunc) {}
  void print() const override;

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLengthContent : public AST {
  SpecifiedLengthContent(const char* f_at, SpecifiedLengthPtr length)
    : AST(f_at), length(std::move(length)) {}
  virtual ~SpecifiedLengthContent() {}
  virtual void print() const = 0;

  SpecifiedLengthPtr length;
};

struct StringLiteral : public SpecifiedLengthContent {
  StringLiteral(const char* f_at, const std::string& str)
    : SpecifiedLengthContent(f_at, SpecifiedLengthPtr(new LiteralLength(NULL, (int)str.length(), false))),
    str(str) {}
  void print() const override;

  std::string str;
};

struct RepeatedChar : public SpecifiedLengthContent {
  RepeatedChar(const char* f_at, SpecifiedLengthPtr length, char c)
    : SpecifiedLengthContent(f_at, std::move(length)), c(c) {}
  void print() const override;

  char c;
};

struct Block : public SpecifiedLengthContent {
  Block(const char* f_at, SpecifiedLengthPtr length)
    : SpecifiedLengthContent(f_at, std::move(length)), greedyChildIndex(-1), greedyChild(NULL),
    topFiller(NULL, 0, false, ' '), bottomFiller(NULL, 0, false, ' ') {}
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
