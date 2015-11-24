#ifndef AST_H
#define AST_H

#include <vector>
#include <memory>
#include <assert.h>
#include <exception>

typedef int(*LengthFunc)(int);

class DSLException : public std::exception {
public:
  class DSLException(const char* f_at, const char* const& what)
    : std::exception(what), f_at(f_at) {}

  const char* f_at;
};

// -------------------------------------------------------------------------------------------------

struct SpecifiedLength {
  SpecifiedLength(bool shares) : shares(shares) {}
  virtual ~SpecifiedLength() {}
  virtual void print() const = 0;

  bool shares;
};

struct LiteralLength : public SpecifiedLength {
  LiteralLength(int value, bool shares)
    : SpecifiedLength(shares), value(value) {}
  void print() const override;

  int value;
};

struct FunctionLength : public SpecifiedLength {
  FunctionLength(LengthFunc lengthFunc, bool shares)
    : SpecifiedLength(shares), lengthFunc(lengthFunc) {}
  void print() const override;

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------

enum NodeType:int{ STRING_LITERAL, REPEATED_CHAR, REPEATED_CHAR_FL, WORDS, BLOCK };

struct AST {
  AST(NodeType type, const char* f_at) : type(type), f_at(f_at), startCol(-1), numCols(-1) {}
  virtual void print() const = 0;

  NodeType type;
  const char* f_at;   // position in the format string where this node is specified
};

struct Filler : public AST {
  Filler(NodeType type, const char* f_at, const LiteralLength& length)
    : AST(type, f_at), length(length) {}
  virtual void print() const override = 0;

  LiteralLength length;
};

typedef std::unique_ptr<AST> ASTPtr;
typedef std::unique_ptr<Filler> FillerPtr;

// -------------------------------------------------------------------------------------------------

struct StringLiteral : public Filler {
  StringLiteral(const char* f_at, const std::string& str)
    : Filler(STRING_LITERAL, f_at, LiteralLength((int)str.length(), false)), str(str) {}
  void print() const override;

  std::string str;
};

struct RepeatedChar : public Filler {
  RepeatedChar(const char* f_at, const LiteralLength& length, char c)
    : Filler(REPEATED_CHAR, f_at, length), c(c) {}
  void print() const override;

  char c;
};

// -------------------------------------------------------------------------------------------------

struct RepeatedCharFuncLength : public AST {
  RepeatedCharFuncLength(const char* f_at, const FunctionLength& length, char c)
    : AST(REPEATED_CHAR_FL, f_at), length(length), c(c) {}
  void print() const override;

  FunctionLength length;
  char c;
};

struct Words : public AST {
  Words(const char* f_at, const std::string& source, char wordSilhouette = '\0')
    : AST(WORDS, f_at), source(source), wordSilhouette(wordSilhouette) {}
  void print() const override;

  std::string source;
  std::vector<FillerPtr> interwordFillers;
  char wordSilhouette;        // use '\0' if unused
};

typedef std::unique_ptr<Words> WordsPtr;

struct Block : public AST {
  Block(const char* f_at, const LiteralLength& length)
    : AST(BLOCK, f_at), length(length), greedyChildIndex(-1) {}
  void print() const override;

  void addChild(ASTPtr child);
  void addGreedyChild(ASTPtr words);
  bool hasGreedyChild() const;

  LiteralLength length;
  std::vector<ASTPtr> children;
  int greedyChildIndex;   // use value < 0 if no greedy child
  std::vector<FillerPtr> topFillers;
  std::vector<FillerPtr> bottomFillers;
};

// -------------------------------------------------------------------------------------------------

#endif
