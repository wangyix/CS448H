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
  if (child->type == REPEATED_CHAR_FL) {
    hasFLChild = true;
  }
  children.push_back(std::move(child));
}
void Block::addWords(ASTPtr words) {
  assert(!hasWords());
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

static void llSharesToLength(int totalLength, const std::vector<LiteralLength*>& lls,
                                    const char* f_at) {
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
    // Distributing 0 length amongst 0 total shares is fine: all resulting lengths are 0.
    for (LiteralLength* ll : lls) {
      ll->value = 0;
      ll->shares = false;
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
      deltas[i]--;
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

/*static void computeShareLengths(int totalLength, const std::vector<int> shareCounts,
                                std::vector<int>* lengths, const char* f_at) {
  assert(totalLength >= 0);
  int totalShareCount = 0;
  for (int count : shareCounts) {
    totalShareCount += count;
  }
  if (totalShareCount == 0) {
    if (totalLength > 0) {
      throw DSLException(f_at, "No share-length content to distribute remaining length to.");
    }
    // Distributing 0 length amongst 0 total shares is fine: all resulting lengths are 0.
    *lengths = std::vector<int>(shareCounts.size(), 0);
    return;
  }
  // Initially, distribute from the total length so that each length is the floor of its target
  // value based on uniform shares.
  float avgShareLength = totalLength / (float)totalShareCount;
  *lengths = std::vector<int>(shareCounts.size());
  std::vector<float> deltas(shareCounts.size());
  int lengthRemaining = totalLength;
  for (int i = 0; i < shareCounts.size(); ++i) {
    float targetLength = shareCounts[i] * avgShareLength;
    int length = floorf(targetLength);
    (*lengths)[i] = length;
    deltas[i] = targetLength - length;
    lengthRemaining -= length;
  }
  // lengthRemaining should be less than shareCounts.size(), but do this anyway in case of numerical
  // error
  while (lengthRemaining >= shareCounts.size()) {
    for (int i = 0; i < shareCounts.size(); ++i) {
      (*lengths)[i]++;
      deltas[i]--;
    }
    lengthRemaining -= shareCounts.size();
  }
  // Distribute remaining length to the lengths with the largest deltas.
  int n = deltas.size() - 1 - lengthRemaining;
  std::vector<float> deltasCopy(deltas);
  std::nth_element(deltasCopy.begin(), deltasCopy.begin() + n, deltasCopy.end());
  float deltaThreshold = deltasCopy[n];
  for (int i = 0; i < shareCounts.size() && lengthRemaining > 0; ++i) {
    if (deltas[i] > deltaThreshold) {
      (*lengths)[i]++;
      --lengthRemaining;
    }
  }
  for (int i = 0; i < shareCounts.size() && lengthRemaining > 0; ++i) {
    if (deltas[i] == deltaThreshold) {
      (*lengths)[i]++;
      --lengthRemaining;
    }
  }
  assert(lengthRemaining == 0);
}
*/


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


void AST::flatten(ASTPtr self, std::vector<ConsistentContent>* ccs, bool firstInParent,
                  std::vector<FillerPtr>* topFillersStack,
                  std::vector<FillerPtr>* bottomFillersStack) {
  bool startNewCC;
  bool newCCChildrenConsistent;
  
  bool ccsEmptyOrFillersChanged;
  ConsistentContent* cc = NULL;
  if (ccs->empty()) {
    ccsEmptyOrFillersChanged = true;
  } else {
    cc = &ccs->back();
    if (firstInParent) {
      ccsEmptyOrFillersChanged = (cc->topFillers != *topFillersStack || cc->bottomFillers != *bottomFillersStack);
    } else {
      ccsEmptyOrFillersChanged = false;
    }
  }

  if (ccsEmptyOrFillersChanged) {
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

  if (startNewCC) {
    ccs->push_back(ConsistentContent(newCCChildrenConsistent, startCol, UNKNOWN_COL));
    cc = &ccs->back();
    cc->topFillers = *topFillersStack;
    cc->bottomFillers = *bottomFillersStack;
  }
  if (type == WORDS) {
    cc->wordsIndex = cc->children.size();
    cc->words = static_cast<const Words*>(this);
    //cc->s_at = cc->words->source.c_str();
  }
  cc->children.push_back(self);
  cc->endCol = endCol;
}

void Block::flatten(ASTPtr self, std::vector<ConsistentContent>* ccs, bool firstInParent,
                    std::vector<FillerPtr>* topFillersStack,
                    std::vector<FillerPtr>* bottomFillersStack) {
  topFillersStack->insert(topFillersStack->end(), topFillers.begin(), topFillers.end());
  bottomFillersStack->insert(bottomFillersStack->end(), bottomFillers.begin(), bottomFillers.end());
  firstInParent = true;
  for (int i = 0; i < children.size(); ++i) {
    ASTPtr& child = children[i];
    child->flatten(child, ccs, firstInParent, topFillersStack, bottomFillersStack);
    firstInParent = false;
  }
  topFillersStack->erase(topFillersStack->end() - topFillers.size(), topFillersStack->end());
  bottomFillersStack->erase(bottomFillersStack->end() - bottomFillers.size(), bottomFillersStack->end());
}
 

FillerPtr RepeatedCharFL::toRepeatedCharLL(int line) const{
  assert(line >= 0);
  return FillerPtr(new RepeatedCharLL(f_at, length.toLiteralLength(line), c));
}

const char* parseWhitespaces(const char* s_at) {
  while (std::isspace(*s_at)) {
    ++s_at;
  }
  return s_at;
}
const char* parseUntilWhitespace(const char* s_at) {
  while (*s_at != '\0' && !std::isspace(*s_at)) {
    ++s_at;
  }
  return s_at;
}

int wordsToContents(const char** s_at,
                    const std::vector<FillerPtr>& interwordFillers, int interwordMinLength,
                    int lineMaxLength, std::vector<FillerPtr>* lineContents, const char* f_at) {
  assert(interwordMinLength >= 0);
  int remainingLength = lineMaxLength;

  *s_at = parseWhitespaces(*s_at);
  if (**s_at == '\0') {
    return remainingLength;
  }
  // Add as many words and interword fillers as the maxWordsLength allows
  // Interword fillers only go between words on the same line.
  const char* firstWordEnd = parseUntilWhitespace(*s_at);
  int firstWordLength = firstWordEnd - *s_at;
  assert(firstWordLength > 0);
  if (firstWordLength > lineMaxLength) {
    // First word is longer than max line length; push as much of the word as allowed, and pretend
    // the next word starts where we left off.
    lineContents->push_back(FillerPtr(new StringLiteral(f_at, *s_at, lineMaxLength)));
    *s_at += lineMaxLength;
    remainingLength = 0;
  } else {
    lineContents->push_back(FillerPtr(new StringLiteral(f_at, *s_at, firstWordLength)));
    *s_at = firstWordEnd;
    remainingLength -= firstWordLength;
    
    while (true) {
      *s_at = parseWhitespaces(*s_at);
      if (**s_at == '\0') {
        break;
      }
      // Check if there's enough room for interword fillers and another word.  If yes, insert 
      // interword fillers and the next word.  Otherwise, bail
      const char* wordEnd = parseUntilWhitespace(*s_at);
      int wordLength = wordEnd - *s_at;
      assert(wordLength > 0);
      if (interwordMinLength + wordLength <= remainingLength) {
        lineContents->insert(lineContents->end(), interwordFillers.begin(), interwordFillers.end());
        lineContents->push_back(FillerPtr(new StringLiteral(f_at, *s_at, wordLength)));
        *s_at = wordEnd;
        remainingLength -= (interwordMinLength + wordLength);
      } else {
        break;
      }
    }
  }

  return remainingLength;
}

void ConsistentContent::generateCCLine(int lineNum, CCLine* line) {
  int totalLength = endCol - startCol;
  std::vector<FillerPtr>* lineContents = &line->contents;
  lineContents->clear();

  // add the non-word contents of this CC, with any function lengths evaluated to literal length
  for (const ASTPtr& child : children) {
    switch (child->type) {
    case STRING_LITERAL:
      lineContents->push_back(std::static_pointer_cast<Filler>(child));
      break;
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
  if (childrenConsistent) {
    return;
  }
  // If this CC has words, insert additional content into lineContents at the wordsIndex that represent
  // the words.
  if (words != NULL) {
    interwordFixedLength = 0;
    for (const FillerPtr& filler : words->interwordFillers) {
      if (!filler->length.shares) {
        interwordFixedLength += filler->length.value;
      }
    }
    int maxWordsLength = totalLength;
    for (const FillerPtr& filler : *lineContents) {
      if (!filler->length.shares) {
        maxWordsLength -= filler->length.value;
      }
    }
    if (maxWordsLength <= 0) {
      throw DSLException(words->f_at, "No length remaining for words.");
    }
    std::vector<FillerPtr> wordsContents;
    wordsToContents(&s_at, words->interwordFillers, interwordFixedLength, maxWordsLength, &wordsContents, words->f_at);
    lineContents->insert(lineContents->begin() + wordsIndex, wordsContents.begin(), wordsContents.end());
  }
  // Compute the share lengths of the line contents
  std::vector<LiteralLength*> lls;
  for (const FillerPtr& filler : *lineContents) {
    lls.push_back(&filler->length);
  }
  llSharesToLength(totalLength, lls, children.front()->f_at);
}

void ConsistentContent::generateCCLines() {
  lines.clear();
  if (words != NULL) {
    s_at = words->source.c_str();
    while (*s_at != '\0') {
      lines.push_back(CCLine());
      generateCCLine(lines.size() - 1, &lines.back());
    }
  } else {
    lines.push_back(CCLine());
    generateCCLine(UNKNOWN_COL, &lines.back());
  }
}
