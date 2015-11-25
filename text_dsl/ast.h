#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <assert.h>
#include <exception>

const int UNKNOWN_COL = -1;
typedef int(*LengthFunc)(int);

class DSLException : public std::exception {
public:
  class DSLException(const char* f_at, const char* const& what)
    : std::exception(what), f_at(f_at) {}

  const char* f_at;
};

// -------------------------------------------------------------------------------------------------

struct Length {
  Length() {}
  virtual void print() const = 0;
  virtual int getFixedLength() const = 0;
};

struct SpecifiedLength : public Length {
  SpecifiedLength(bool shares) : shares(shares) {}
  virtual void print() const override = 0;
  virtual int getFixedLength() const override = 0;

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(int value, bool shares)
    : SpecifiedLength(shares), value(value) {}
  void print() const override;
  int getFixedLength() const override { return shares ? UNKNOWN_COL : value; }

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(LengthFunc lengthFunc, bool shares)
    : SpecifiedLength(shares), lengthFunc(lengthFunc) {}
  void print() const override;
  int getFixedLength() const override { return UNKNOWN_COL; }

  LengthFunc lengthFunc;
};

struct GreedyLength : public Length {
  GreedyLength() {}
  void print() const override {};
  int getFixedLength() const override { return UNKNOWN_COL; }
};

// -------------------------------------------------------------------------------------------------
class Visitor;

enum NodeType:int{ STRING_LITERAL, REPEATED_CHAR, REPEATED_CHAR_FL, WORDS, BLOCK };

struct AST {
  AST(NodeType type, const char* f_at, Length* lengthPtr)
    : type(type), f_at(f_at), lengthPtr(lengthPtr), startCol(UNKNOWN_COL), numCols(UNKNOWN_COL) {}
  virtual void print() const = 0;
  virtual void accept(Visitor* v) = 0;

  virtual void computeConsistentPos(int startColHint, int numColsHint);
  
  NodeType type;
  const char* f_at;   // position in the format string where this node is specified
  Length* lengthPtr;

  int startCol;     // starting column of this content
  int numCols;      // width of this content
};

typedef std::unique_ptr<AST> ASTPtr;

// -------------------------------------------------------------------------------------------------

struct Filler : public AST {
  Filler(NodeType type, const char* f_at, const LiteralLength& length)
    : AST(type, f_at, &this->length), length(length) {}

  LiteralLength length;
};

typedef std::unique_ptr<Filler> FillerPtr;

// -------------------------------------------------------------------------------------------------

struct StringLiteral : public Filler {
  StringLiteral(const char* f_at, const std::string& str)
    : Filler(STRING_LITERAL, f_at, LiteralLength((int)str.length(), false)), str(str) {}
  void print() const override;
  void accept(Visitor* v) override;

  std::string str;
};

struct RepeatedCharLL : public Filler {
  RepeatedCharLL(const char* f_at, const LiteralLength& length, char c)
    : Filler(REPEATED_CHAR, f_at, length), c(c) {}
  void print() const override;
  void accept(Visitor* v) override;

  char c;
};

// -------------------------------------------------------------------------------------------------

struct RepeatedCharFL : public AST {
  RepeatedCharFL(const char* f_at, const FunctionLength& length, char c)
    : AST(REPEATED_CHAR_FL, f_at, &this->length), length(length), c(c) {}
  void print() const override;
  void accept(Visitor* v) override;

  FunctionLength length;
  char c;
};

struct Words : public AST {
  Words(const char* f_at, const std::string& source, char wordSilhouette = '\0')
    : AST(WORDS, f_at, &this->length), source(source), wordSilhouette(wordSilhouette) {}
  void print() const override;
  void accept(Visitor* v) override;

  GreedyLength length;
  std::string source;
  std::vector<FillerPtr> interwordFillers;
  char wordSilhouette;        // use '\0' if unused
};

typedef std::unique_ptr<Words> WordsPtr;

struct Block : public AST {
  Block(const char* f_at, const LiteralLength& length)
    : AST(BLOCK, f_at, &this->length), length(length), greedyChildIndex(-1), hasFLChild(false) {}
  void print() const override;
  void accept(Visitor* v) override;

  void addChild(ASTPtr child);
  void addGreedyChild(ASTPtr words);
  bool hasGreedyChild() const;

  void computeConsistentPos(int startColHint, int numColsHint) override;

  LiteralLength length;
  std::vector<ASTPtr> children;
  int greedyChildIndex;   // use value < 0 if no greedy child
  bool hasFLChild;        // whether or not any children have function-length.
  std::vector<FillerPtr> topFillers;
  std::vector<FillerPtr> bottomFillers;
};

// -------------------------------------------------------------------------------------------------

#endif
