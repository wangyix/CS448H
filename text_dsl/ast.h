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
  Length(bool shares) : shares(shares) {}
  virtual void print() const = 0;

  bool shares;
};

struct LiteralLength : public Length {
  LiteralLength(int value, bool shares)
    : Length(shares), value(value) {}
  void print() const override;

  int value;
};

struct FunctionLength : public Length {
  FunctionLength(LengthFunc lengthFunc, bool shares)
    : Length(shares), lengthFunc(lengthFunc) {}
  void print() const override;
  LiteralLength toLiteralLength(int line) const { return LiteralLength((*lengthFunc)(line), shares); }

  LengthFunc lengthFunc;
};

// -------------------------------------------------------------------------------------------------
enum NodeType :int{ STRING_LITERAL, REPEATED_CHAR_LL, REPEATED_CHAR_FL, WORDS, BLOCK };

class Visitor;
struct ConsistentContent;
struct AST;
struct Filler;

typedef std::shared_ptr<AST> ASTPtr;
typedef std::shared_ptr<Filler> FillerPtr;

struct AST {
  AST(NodeType type, const char* f_at)
    : type(type), f_at(f_at), startCol(UNKNOWN_COL), endCol(UNKNOWN_COL),
    numContentLines(UNKNOWN_COL), numFixedLines(UNKNOWN_COL), numTotalLines(UNKNOWN_COL) {}
  virtual void print() const = 0;
  virtual void accept(Visitor* v) = 0;
  virtual int getFixedLength() const = 0;
  virtual LiteralLength* getLiteralLength() = 0;

  virtual void convertLLSharesToLength();
  virtual void computeStartEndCols(int start, int end);
  virtual void flatten(ASTPtr self, ASTPtr parent, std::vector<ConsistentContent>* ccs,
    bool firstAfterBlockBoundary,
    std::vector<FillerPtr>* topFillersStack, std::vector<FillerPtr>* bottomFillersStack);

  virtual void computeNumContentLines();
  virtual void computeNumTotalLines(bool isRoot);
  virtual void computeBlockVerticalFillersShares();
  
  NodeType type;
  const char* f_at;   // position in the format string where this node is specified

  int startCol;     // starting column of this content
  int endCol;       // ending column of this content (1 past last)

  int numContentLines;    // number of lines of non-vertical-filler content (word lines for blocks, 1 otherwise)
  int numFixedLines;      // minimum number of lines of this content (word lines + fixed filler lines) for blocks, 1 otherwise
  int numTotalLines;      // number of lines including filler lines
};

// -------------------------------------------------------------------------------------------------

struct Filler : public AST {
  Filler(NodeType type, const char* f_at, const LiteralLength& length)
    : AST(type, f_at), length(length) {}
  int getFixedLength() const override { return length.shares ? UNKNOWN_COL : length.value; }
  LiteralLength* getLiteralLength() override { return &length; }
  virtual void printContent(FILE* stream) const = 0;
  virtual void printContent(char** bufAt) const = 0;

  LiteralLength length;
};


// -------------------------------------------------------------------------------------------------

struct StringLiteral : public Filler {
  StringLiteral(const char* f_at, const std::string& str)
    : Filler(STRING_LITERAL, f_at, LiteralLength((int)str.length(), false)), str(str) {}
  StringLiteral(const char* f_at, const char* src, int size)
    : Filler(STRING_LITERAL, f_at, LiteralLength(size, false)), str(src, size) {}
  void print() const override;
  void accept(Visitor* v) override;
  void printContent(FILE* stream) const override;
  void printContent(char** bufAt) const override;

  std::string str;
};

struct RepeatedCharLL : public Filler {
  RepeatedCharLL(const char* f_at, const LiteralLength& length, char c)
    : Filler(REPEATED_CHAR_LL, f_at, length), c(c) {}
  void print() const override;
  void accept(Visitor* v) override;
  void printContent(FILE* stream) const override;
  void printContent(char** bufAt) const override;

  char c;
};

// -------------------------------------------------------------------------------------------------

struct RepeatedCharFL : public AST {
  RepeatedCharFL(const char* f_at, const FunctionLength& length, char c)
    : AST(REPEATED_CHAR_FL, f_at), length(length), c(c) {}
  void print() const override;
  void accept(Visitor* v) override;
  int getFixedLength() const override { return UNKNOWN_COL; }
  LiteralLength* getLiteralLength() override { return NULL; }

  FillerPtr toRepeatedCharLL(int line) const;

  FunctionLength length;
  char c;
};

struct Words : public AST {
  Words(const char* f_at, const std::string& source, char wordSilhouette = '\0')
    : AST(WORDS, f_at), source(source), wordSilhouette(wordSilhouette) {}
  void print() const override;
  void accept(Visitor* v) override;
  int getFixedLength() const override { return UNKNOWN_COL; }
  LiteralLength* getLiteralLength() override { return NULL; }

  std::string source;
  std::vector<FillerPtr> interwordFillers;
  char wordSilhouette;        // use '\0' if unused
};

struct Block : public AST {
  Block(const char* f_at, const LiteralLength& length)
    : AST(BLOCK, f_at), length(length), wordsIndex(-1), hasFLChild(false) {}
  void print() const override;
  void accept(Visitor* v) override;
  int getFixedLength() const override { return length.shares ? UNKNOWN_COL : length.value; }
  LiteralLength* getLiteralLength() override { return &length; }

  void addChild(ASTPtr child);
  void addWords(ASTPtr words);
  bool hasWords() const;

  void convertLLSharesToLength() override;
  void computeStartEndCols(int start, int end) override;
  void flatten(ASTPtr self, ASTPtr parent, std::vector<ConsistentContent>* ccs,
    bool firstAfterBlockBoundary,
    std::vector<FillerPtr>* topFillersStack, std::vector<FillerPtr>* bottomFillersStack) override;

  void computeNumContentLines() override;
  void computeNumTotalLines(bool isRoot) override;
  void computeBlockVerticalFillersShares() override;

  LiteralLength length;
  std::vector<ASTPtr> children;
  int wordsIndex;   // use value < 0 if no greedy child
  bool hasFLChild;        // whether or not any children have function-length.
  std::vector<FillerPtr> topFillers;
  std::vector<FillerPtr> bottomFillers;
};

typedef std::shared_ptr<Block> BlockPtr;

// -------------------------------------------------------------------------------------------------
struct CCLine;

// If one child, then child must be consistent.
// If multiple children, then first and last children must be inconsistent
struct ConsistentContent {
  ConsistentContent(ASTPtr srcAst, bool childrenConsistent, int startCol, int endCol,
    const std::vector<FillerPtr> topFillers, const std::vector<FillerPtr> bottomFillers)
    : srcAst(srcAst), childrenConsistent(childrenConsistent),
    wordsIndex(UNKNOWN_COL), words(NULL),
    startCol(startCol), endCol(endCol), topFillers(topFillers), bottomFillers(bottomFillers),
    s_at(NULL), interwordFixedLength(UNKNOWN_COL),
    interwordHasShares(false) {}
  void print() const;

  void generateCCLine(int lineNum, CCLine* line);
  void generateCCLines();

  void generateLinesChars(int rootNumTotalLines);
  void printContentLine(FILE* stream, int lineNum, int rootNumTotalLines);
  void printContentLine(char** bufAt, int lineNum, int rootNumTotalLines);

  ASTPtr srcAst;
  bool childrenConsistent;  // true if all children startCol and endCol are known (line-independent)
  std::vector<ASTPtr> children;
  int wordsIndex;
  const Words* words;
  int startCol;
  int endCol;
  std::vector<FillerPtr> topFillers, bottomFillers;

  const char* s_at;
  int interwordFixedLength;
  bool interwordHasShares;
  std::vector<CCLine> lines;

  std::string topFillersChars, bottomFillersChars;
};


struct CCLine {
  void printContent(FILE* stream) const { for (const FillerPtr& c : contents) c->printContent(stream); }
  void printContent(char** bufAt) const { for (const FillerPtr& c : contents) c->printContent(bufAt); }
  std::vector<FillerPtr> contents;
};


#endif
