#include "ast.h"
#include "visitor.h"

#include <stdio.h>
#include <algorithm>
#include <cctype>

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

void RepeatedCharLL::print() const {
  length.print();
  printf("'%c'", c);
}

// -------------------------------------------------------------------------------------------------

void RepeatedCharFL::print() const {
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
    printf(" ");
    filler->print();
  }
  printf(" }v{");
  for (const FillerPtr& filler : bottomFillers) {
    printf(" ");
    filler->print();
  }
  printf(" }");
}

void Block::addChild(ASTPtr child) {
  if (child->type == BLOCK) {
    if (hasWords()) {
      throw DSLException(child->f_at, "Parent block cannot contain both a child block and words.");
    }
  } else if (child->type == REPEATED_CHAR_FL) {
    hasFLChild = true;
  }
  children.push_back(std::move(child));
}
void Block::addWords(ASTPtr words) {
  if (hasWords()) {
    throw DSLException(words->f_at, "Cannot have multiple words blocks within a block.");
  }
  for (const ASTPtr& child : children) {
    if (child->type == BLOCK) {
      throw DSLException(child->f_at, "Parent block cannot contain both a child block and words.");
    }
  }
  wordsIndex = children.size();
  children.push_back(std::move(words));
}
bool Block::hasWords() const {
  return wordsIndex >= 0;
}

void ConsistentContent::print() const {
  printf("%d:%d [", startCol, endCol);
  for (const ASTPtr& child : children) {
    printf(" ");
    child->print();
  }
  printf(" ]");
  printf("^{");
  for (const FillerPtr& filler : topFillers) {
    printf(" ");
    filler->print();
  }
  printf(" }v{");
  for (const FillerPtr& filler : bottomFillers) {
    printf(" ");
    filler->print();
  }
  printf(" }");
}

// -------------------------------------------------------------------------------------------------

void StringLiteral::accept(Visitor* v) {
  v->visit(this);
}
void RepeatedCharLL::accept(Visitor* v) {
  v->visit(this);
}
void RepeatedCharFL::accept(Visitor* v) {
  v->visit(this);
}
void Words::accept(Visitor* v) {
  v->visit(this);
}
void Block::accept(Visitor* v) {
  v->visit(this);
  for (const ASTPtr& child : children) {
    child->accept(v);
  }
}

// -------------------------------------------------------------------------------------------------
static void putChars(FILE* stream, char c, int n) {
  for (int i = 0; i < n; ++i) {
    fputc(c, stream);
  }
}
static void putChars(char** bufAt, char c, int n) {
  for (int i = 0; i < n; ++i) {
    **bufAt = c;
    ++*bufAt;
  }
}

void StringLiteral::printContent(FILE* stream) const {
  fprintf(stream, "%s", str.c_str());
}
void StringLiteral::printContent(char** bufAt) const {
  *bufAt += str.copy(*bufAt, str.length()); // does not append '\0'
}

void RepeatedCharLL::printContent(FILE* stream) const {
  putChars(stream, c, length.value);
}
void RepeatedCharLL::printContent(char** bufAt) const {
  putChars(bufAt, c, length.value);
}

// -------------------------------------------------------------------------------------------------

static void llSharesToLength(int totalLength, const std::vector<LiteralLength*>& lls, const char* f_at) {
  int lengthRemaining = totalLength;
  int totalShareCount = 0;
  std::vector<LiteralLength*> shareLLs;
  for (LiteralLength* ll : lls) {
    if (ll->shares) {
      totalShareCount += ll->value;
      shareLLs.push_back(ll);
    } else {
      lengthRemaining -= ll->value;
    }
  }
  if (lengthRemaining < 0) {
    throw DSLException(f_at, "Sum of length of fixed-length content exceeds available length.");
  }
  if (totalShareCount == 0) {
    if (lengthRemaining > 0) {
      throw DSLException(f_at, "No share-length content to distribute remaining length to.");
    }
    // Distributing 0 length amongst 0 total shares is fine: all resulting share lengths are 0.
    for (LiteralLength* ll : lls) {
      if (ll->shares) {
        ll->value = 0;
        ll->shares = false;
      }
    }
    return;
  }
  // Initially, distribute from the total length so that each length is the floor of its target
  // value based on uniform shares.
  float avgShareLength = lengthRemaining / (float)totalShareCount;
  std::vector<float> deltas(shareLLs.size());
  for (int i = 0; i < shareLLs.size(); ++i) {
    float targetLength = shareLLs[i]->value * avgShareLength;
    int length = floorf(targetLength);
    shareLLs[i]->value = length;
    shareLLs[i]->shares = false;
    deltas[i] = targetLength - length;
    lengthRemaining -= length;
  }
  // lengthRemaining should be less than shareCounts.size(), but do this anyway in case of numerical
  // error
  while (lengthRemaining >= shareLLs.size()) {
    for (int i = 0; i < shareLLs.size(); ++i) {
      shareLLs[i]->value++;
      deltas[i] -= 1.f;
    }
    lengthRemaining -= shareLLs.size();
  }
  // Distribute remaining length to the lengths with the largest deltas.
  int n = deltas.size() - 1 - lengthRemaining;
  std::vector<float> deltasCopy(deltas);
  std::nth_element(deltasCopy.begin(), deltasCopy.begin() + n, deltasCopy.end());
  float deltaThreshold = deltasCopy[n];
  for (int i = 0; i < shareLLs.size() && lengthRemaining > 0; ++i) {
    if (deltas[i] > deltaThreshold) {
      shareLLs[i]->value++;
      --lengthRemaining;
    }
  }
  for (int i = 0; i < shareLLs.size() && lengthRemaining > 0; ++i) {
    if (deltas[i] == deltaThreshold) {
      shareLLs[i]->value++;
      --lengthRemaining;
    }
  }
  assert(lengthRemaining == 0);
}


void AST::convertLLSharesToLength() {
  // Parent block is expected to do the conversion. As for the root node, it's expected to be
  // fixed-length to begin with (expected to be verified by parser).
}

void Block::convertLLSharesToLength() {
  if (length.shares) {
    throw DSLException(f_at, "Block length is line-dependent.");
  }
  if (!hasWords() && !hasFLChild) {
    // None of the content varies line-by-line, so all children have consistent length and positions
    // Note this means that all children have literal length.
    std::vector<LiteralLength*> lls;
    for (const ASTPtr& child : children) {
      LiteralLength* ll = child->getLiteralLength();
      assert(ll != NULL);
      lls.push_back(ll);
    }
    llSharesToLength(length.value, lls, f_at);  // modifies the LiteralLength of all children to fixed lengths
    for (const ASTPtr& child : children) {
      assert(child->getFixedLength() != UNKNOWN_COL);
    }
  }
  for (const ASTPtr& child : children) {
    child->convertLLSharesToLength();
  }
}


void AST::computeStartEndCols(int start, int end) {
  startCol = start;
  endCol = end;
//printf("\n%s\n", f_at);
//printf("\t%d:%d, %d\n", startCol, endCol, getFixedLength());
}

void Block::computeStartEndCols(int start, int end) {
  startCol = start;
  endCol = end;
//printf("\n%s\n", f_at);
//printf("\t%d:%d, %d\n", startCol, endCol, getFixedLength());
  if (startCol == UNKNOWN_COL || endCol == UNKNOWN_COL) {
    throw DSLException(f_at, "Block bondaries are line-dependent.");
  }
  assert(endCol - startCol == length.value);

  // some content varies line-by-line, so only consecutive fixed-length children starting from
  // either end of this block have consistent starting positions.
  int i = 0;  // start index from left, iterate until a non-fixed-length child is found
  int iStartCol = startCol;
  {
    for (; i < children.size(); ++i) {
      const ASTPtr& child = children[i];
      int childEndCol = UNKNOWN_COL;
      int childNumCols = child->getFixedLength();
      if (childNumCols != UNKNOWN_COL) {
        childEndCol = iStartCol + childNumCols;
      } else {
        break;
      }
      child->computeStartEndCols(iStartCol, childEndCol);
      iStartCol = childEndCol;
    }
  }
  int jEndCol = endCol;
  if (i < children.size()) {
    // start index from right, iterate up to not including child i
    for (int j = children.size() - 1; j > i; --j) {
      const ASTPtr& child = children[j];
      int childStartCol = UNKNOWN_COL;
      int childNumCols = child->getFixedLength();
      if (childNumCols != UNKNOWN_COL && jEndCol != UNKNOWN_COL) {
        childStartCol = jEndCol - childNumCols;
      }
      child->computeStartEndCols(childStartCol, jEndCol);
      jEndCol = childStartCol;
    }
    children[i]->computeStartEndCols(iStartCol, jEndCol);
  }
}


void AST::flatten(ASTPtr self, ASTPtr parent, std::vector<ConsistentContent>* ccs, bool firstAfterBlockBoundary,
                  std::vector<FillerPtr>* topFillersStack, std::vector<FillerPtr>* bottomFillersStack) {
  bool startNewCC;
  bool newCCChildrenConsistent;
  
  if (firstAfterBlockBoundary) {
    startNewCC = true;
    newCCChildrenConsistent = (endCol != UNKNOWN_COL);
  } else {
    bool prevCCChildrenConsistent = ccs->back().childrenConsistent;
    if (startCol == UNKNOWN_COL) {
      startNewCC = false;
      assert(!prevCCChildrenConsistent);
    } else {
      newCCChildrenConsistent = (endCol != UNKNOWN_COL);
      startNewCC = (prevCCChildrenConsistent != newCCChildrenConsistent);
    }
  }
  // if we're starting a new CC, its start must be consistent
  assert(!startNewCC || startCol != UNKNOWN_COL);

  ConsistentContent* cc = NULL;
  if (startNewCC) {
    ccs->push_back(ConsistentContent(parent, newCCChildrenConsistent, startCol, UNKNOWN_COL,
                                     *topFillersStack, *bottomFillersStack));
    cc = &ccs->back();
  } else {
    assert(!ccs->empty());
    cc = &ccs->back();
  }
  if (type == WORDS) {
    cc->wordsIndex = cc->children.size();
    cc->words = static_cast<const Words*>(this);
    //cc->s_at = cc->words->source.c_str();
  }
  cc->children.push_back(self);
  cc->endCol = endCol;
}

void Block::flatten(ASTPtr self, ASTPtr parent, std::vector<ConsistentContent>* ccs, bool firstAfterBlockBoundary,
                    std::vector<FillerPtr>* topFillersStack, std::vector<FillerPtr>* bottomFillersStack) {
  topFillersStack->insert(topFillersStack->end(), topFillers.begin(), topFillers.end());
  bottomFillersStack->insert(bottomFillersStack->begin(), bottomFillers.begin(), bottomFillers.end());
  bool firstAfterBlockBegin = true;
  bool prevWasBlock = false;
  for (int i = 0; i < children.size(); ++i) {
    ASTPtr& child = children[i];
    bool isBlock = (child->type == BLOCK);
    bool firstAfterBlockEnd = (prevWasBlock && !isBlock);
    child->flatten(child, self, ccs, firstAfterBlockBegin || firstAfterBlockEnd, topFillersStack, bottomFillersStack);
    firstAfterBlockBegin = false;
    prevWasBlock = isBlock;
  }
  topFillersStack->resize(topFillersStack->size() - topFillers.size());
  bottomFillersStack->erase(bottomFillersStack->begin(), bottomFillersStack->begin() + bottomFillers.size());
}
 

FillerPtr RepeatedCharFL::toRepeatedCharLL(int line) const{
  assert(line >= 0);
  return FillerPtr(new RepeatedCharLL(f_at, length.toLiteralLength(line), c));
}

static bool isSpace(char c) {
  return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}

static const char* parseWhitespacesExceptNewline(const char* s_at) {
  while (isSpace(*s_at) && *s_at != '\n') {
    ++s_at;
  }
  return s_at;
}
static const char* parseUntilWhitespace(const char* s_at) {
  while (*s_at != '\0' && !isSpace(*s_at)) {
    ++s_at;
  }
  return s_at;
}

static void deepCopyFillers(const std::vector<FillerPtr>& src, std::vector<FillerPtr>* dst) {
  dst->clear();
  for (const FillerPtr& filler : src) {
    if (filler->type == REPEATED_CHAR_LL) {
      const RepeatedCharLL* rcLL = static_cast<const RepeatedCharLL*>(filler.get());
      dst->push_back(FillerPtr(new RepeatedCharLL(*rcLL)));
    } else {
      const StringLiteral* sl = static_cast<const StringLiteral*>(filler.get());
      dst->push_back(FillerPtr(new StringLiteral(*sl))); 
      // deep copy of StringLiteral not really needed for our purposes, but do it anyway in case
      //dst->push_back(filler);
    }
  }
}

static FillerPtr wordtoContent(const char* src, int size, char silhouette, const char* f_at) {
  if (silhouette != '\0') {
    return FillerPtr(new RepeatedCharLL(f_at, LiteralLength(size, false), silhouette));
  } else {
    return FillerPtr(new StringLiteral(f_at, src, size));
  }
}

static const char* wordsLineToContents(const Words& words, const char* s_at, int interwordMinLength,
                                       int lineMaxLength, std::vector<FillerPtr>* wordsContents) {
  assert(interwordMinLength >= 0);
  wordsContents->clear();
  int remainingLength = lineMaxLength;

  s_at = parseWhitespacesExceptNewline(s_at);
  /*if (*s_at == '\0') {
    return s_at;
  }*/
  // Add as many words and interword fillers as the maxWordsLength allows
  // Interword fillers only go between words on the same line.
  const char* firstWordEnd = parseUntilWhitespace(s_at);
  int firstWordLength = firstWordEnd - s_at;
  if (firstWordLength > lineMaxLength) {
    // First word is longer than max line length; push as much of the word as allowed, and pretend
    // the next word starts where we left off.
    wordsContents->push_back(wordtoContent(s_at, lineMaxLength, words.wordSilhouette, words.f_at));
    s_at += lineMaxLength;
    remainingLength = 0;
  } else {
    wordsContents->push_back(wordtoContent(s_at, firstWordLength, words.wordSilhouette, words.f_at));
    s_at = firstWordEnd;
    remainingLength -= firstWordLength;
    
    while (true) {
      s_at = parseWhitespacesExceptNewline(s_at);
      if (*s_at == '\0') {
        break;
      } else if (*s_at == '\n') {
        ++s_at;
        break;
      }
      // Check if there's enough room for interword fillers and another word.  If yes, insert 
      // interword fillers and the next word.  Otherwise, bail
      const char* wordEnd = parseUntilWhitespace(s_at);
      int wordLength = wordEnd - s_at;
      assert(wordLength > 0);
      if (interwordMinLength + wordLength <= remainingLength) {
        std::vector<FillerPtr> interwordsCopy;
        deepCopyFillers(words.interwordFillers, &interwordsCopy);
        wordsContents->insert(wordsContents->end(), interwordsCopy.begin(), interwordsCopy.end());
        wordsContents->push_back(wordtoContent(s_at, wordLength, words.wordSilhouette, words.f_at));
        s_at = wordEnd;
        remainingLength -= (interwordMinLength + wordLength);
      } else {
        break;
      }
    }
  }

  return s_at;
}

void ConsistentContent::generateCCLine(int lineNum, CCLine* line) {
  int totalLength = endCol - startCol;
  std::vector<FillerPtr>* lineContents = &line->contents;
  lineContents->clear();

  // add the non-word contents of this CC, with any function lengths evaluated to literal length
  for (const ASTPtr& child : children) {
    switch (child->type) {
    case STRING_LITERAL: {
      const StringLiteral* sl = static_cast<const StringLiteral*>(child.get());
      lineContents->push_back(FillerPtr(new StringLiteral(*sl)));
      // deep copy of StringLiteral not really needed for our purposes, but do it anyway in case
      //lineContents->push_back(std::static_pointer_cast<Filler>(child));
    } break;
    case REPEATED_CHAR_LL: {
      const RepeatedCharLL* rcLL = static_cast<const RepeatedCharLL*>(child.get());
      lineContents->push_back(FillerPtr(new RepeatedCharLL(*rcLL)));
    } break;
    case REPEATED_CHAR_FL: {
      const RepeatedCharFL* rcFL = static_cast<const RepeatedCharFL*>(child.get());
      lineContents->push_back(rcFL->toRepeatedCharLL(lineNum));
    } break;
    case WORDS:
      break;
    default:
      assert(false);
    }
  }
  // If this CC has no line-varying content, we've done all we needed to do
  if (childrenConsistent && words == NULL) {
    return;
  }

  // If this CC has words, insert additional content into lineContents at the wordsIndex that represent
  // the words.
  if (words != NULL) {
    int maxWordsLength = totalLength;
    for (const FillerPtr& lineContent : *lineContents) {
      if (!lineContent->length.shares) {
        maxWordsLength -= lineContent->length.value;
      }
    }
    if (maxWordsLength <= 0) {
      throw DSLException(words->f_at, "No length remaining for words.");
    }

    // Convert source text into contents (StringLiterals for words, Fillers for interwords).
    // Convert as much of the source as can fit in this line.
    std::vector<FillerPtr> wordsContents;
    s_at = wordsLineToContents(*words, s_at, interwordFixedLength, maxWordsLength, &wordsContents);
    // If the resulting wordsContents has any shares, then distribute any unused words length to them.
    // If the interword fillers have shares and more than 1 word from the source was put in wordsContent,
    // the wordsContents has shares.
    if (interwordHasShares && wordsContents.size() > 1) {
      std::vector<LiteralLength*> lls;
      for (const FillerPtr& filler : wordsContents) {
        lls.push_back(&filler->length);
      }
      llSharesToLength(maxWordsLength, lls, words->f_at);
    }

    // Insert the converted words contents into the line contents at the index where the Words child
    // is.
    lineContents->insert(lineContents->begin() + wordsIndex, wordsContents.begin(), wordsContents.end());
  }

  // Compute the share lengths of the line contents
  std::vector<LiteralLength*> lls;
  for (const FillerPtr& filler : *lineContents) {
    lls.push_back(&filler->length);
  }
  llSharesToLength(totalLength, lls, srcAst->f_at);
}

void ConsistentContent::generateCCLines() {
  lines.clear();
  if (words != NULL) {
    // initialize s_at to beginning of source; compute interwordHasShares and interwordFixedLength
    s_at = words->source.c_str();
    interwordFixedLength = 0;
    interwordHasShares = false;
    for (const FillerPtr& filler : words->interwordFillers) {
      if (!filler->length.shares) {
        interwordFixedLength += filler->length.value;
      } else {
        interwordHasShares = true;
      }
    }
    // do-while instead of while; if source is empty str, then a blank line is still inserted.
    // This ensures at least one CCLine is created.
    do {
      lines.push_back(CCLine());
      generateCCLine(lines.size() - 1, &lines.back());
     } while (*s_at != '\0');
     srcAst->numContentLines = lines.size();  // each block has at most one CC with Words
  } else {
    lines.push_back(CCLine());
    generateCCLine(UNKNOWN_COL, &lines.back());
  }
}

void AST::computeNumContentLines() {
  numContentLines = 1;
  numFixedLines = 1;
}

void Block::computeNumContentLines() {
  for (const ASTPtr& child : children) {
    child->computeNumContentLines();
  }
  if (hasWords()) {
    // If this block has Words, numContentLines should have been set by generateCCLines()
    assert(numContentLines != UNKNOWN_COL);
  } else {
    // num content lines of a block is the max of the fixed lengths of its children (i.e. the min
    // number of lines necessary to display all its children with vertical fillers)
    numContentLines = 0;
    for (const ASTPtr& child : children) {
      if (child->numFixedLines > numContentLines) {
        numContentLines = child->numFixedLines;
      }
    }
  }
  // Add up fixed-length content in vertical fillers to compute numFixedLines, which is the min
  // number of lines necessary to display this block with vertical fillers.
  numFixedLines = numContentLines;
  for (const FillerPtr& filler : topFillers) {
    if (!filler->length.shares) {
      numFixedLines += filler->length.value;
    }
  }
  for (const FillerPtr& filler : bottomFillers) {
    if (!filler->length.shares) {
      numFixedLines += filler->length.value;
    }
  }
}

void AST::computeNumTotalLines(bool isRoot) {
  if (isRoot) {
    numTotalLines = numFixedLines;
  }
}

void Block::computeNumTotalLines(bool isRoot) {
  if (isRoot) {
    numTotalLines = numFixedLines;
  }
  for (const ASTPtr& child : children) {
    child->numTotalLines = numContentLines;
    child->computeNumTotalLines(false);
  }
}

void AST::computeBlockVerticalFillersShares() {
}

void Block::computeBlockVerticalFillersShares() {
  assert(numContentLines != UNKNOWN_COL);
  assert(numFixedLines != UNKNOWN_COL);
  assert(numTotalLines != UNKNOWN_COL);
  std::vector<LiteralLength*> lls;
  for (const FillerPtr& filler : topFillers) {
    lls.push_back(&filler->length);
  }
  LiteralLength contentLines(numContentLines, false);
  lls.push_back(&contentLines);
  for (const FillerPtr& filler : bottomFillers) {
    lls.push_back(&filler->length);
  }
  llSharesToLength(numTotalLines, lls, f_at);

  for (const ASTPtr& child : children) {
    child->computeBlockVerticalFillersShares();
  }
}


static void verticalFillersToLinesChars(const std::vector<FillerPtr>& fillers, std::string* linesChars) {
  linesChars->clear();
  for (const FillerPtr& filler : fillers) {
    assert(!filler->length.shares);
    if (filler->type == REPEATED_CHAR_LL) {
      const RepeatedCharLL* rcLL = static_cast<const RepeatedCharLL*>(filler.get());
      int oldSize = linesChars->length();
      linesChars->resize(linesChars->length() + rcLL->length.value);
      memset(&(*linesChars)[oldSize], rcLL->c, rcLL->length.value);
    } else {
      const StringLiteral* sl = static_cast<const StringLiteral*>(filler.get());
      *linesChars += sl->str;
    }
  }
}

void ConsistentContent::generateLinesChars(int rootNumTotalLines) {
  verticalFillersToLinesChars(topFillers, &topFillersChars);
  verticalFillersToLinesChars(bottomFillers, &bottomFillersChars);
  assert(topFillersChars.length() + srcAst->numContentLines + bottomFillersChars.length() == rootNumTotalLines);
}



void ConsistentContent::printContentLine(FILE* stream, int lineNum, int rootNumTotalLines) {
  assert(0 <= lineNum && lineNum < rootNumTotalLines);
  if (lineNum < topFillersChars.length()) {
    putChars(stream, topFillersChars[lineNum], endCol - startCol);
  } else {
    lineNum -= topFillersChars.length();
    if (lineNum < srcAst->numContentLines) {
      if (words != NULL) {
        lines[lineNum].printContent(stream);
      } else {
        lines[0].printContent(stream);
      }
    } else {
      lineNum -= srcAst->numContentLines;
      putChars(stream, bottomFillersChars[lineNum], endCol - startCol);
    }
  }
}
void ConsistentContent::printContentLine(char** bufAt, int lineNum, int rootNumTotalLines) {
  assert(0 <= lineNum && lineNum < rootNumTotalLines);
  if (lineNum < topFillersChars.length()) {
    putChars(bufAt, topFillersChars[lineNum], endCol - startCol);
  } else {
    lineNum -= topFillersChars.length();
    if (lineNum < srcAst->numContentLines) {
      if (words != NULL) {
        lines[lineNum].printContent(bufAt);
      } else {
        lines[0].printContent(bufAt);
      }
    } else {
      lineNum -= srcAst->numContentLines;
      putChars(bufAt, bottomFillersChars[lineNum], endCol - startCol);
    }
  }
}
