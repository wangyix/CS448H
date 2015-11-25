#include "ast.h"
#include "visitor.h"

#include <stdio.h>
#include <algorithm>

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
    for (const FillerPtrShared& filler : interwordFillers) {
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
  for (const FillerPtrShared& filler : topFillers) {
    filler->print();
  }
  printf("}v{");
  for (const FillerPtrShared& filler : bottomFillers) {
    filler->print();
  }
  printf("}");
}

void Block::addChild(ASTPtr child) {
  if (child->type == REPEATED_CHAR_FL) {
    hasFLChild = true;
  }
  children.push_back(std::move(child));
}
void Block::addGreedyChild(ASTPtr words) {
  assert(!hasGreedyChild());
  greedyChildIndex = children.size();
  children.push_back(std::move(words));
}
bool Block::hasGreedyChild() const {
  return greedyChildIndex >= 0;
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
  if (!hasGreedyChild() && !hasFLChild) {
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
      child->convertLLSharesToLength();
    }
  }
}


void AST::computeStartCol(int start) {
  startCol = start;
printf("\n%s\n", f_at);
printf("\tstartCol = %d, numCols = %d\n", startCol, getFixedLength());
}

void Block::computeStartCol(int start) {
  startCol = start;
  if (startCol == UNKNOWN_COL) {
    throw DSLException(f_at, "Block start position is line-dependent.");
  }
printf("\n%s\n", f_at);
printf("\tstartCol = %d, numCols = %d\n", startCol, getFixedLength());

  // some content varies line-by-line, so only fixed-length children that are preceded solely
  // by other fixed-length children (in the same block) can have their startCol computed.
  int childStartCol = startCol;
  for (const ASTPtr& child : children) {
    child->computeStartCol(childStartCol);
    // As sson as one child with non-fixed-length is found, childStartCol will be set to UNKNOWN_COL
    // for all subsequent children
    int childNumCols = child->getFixedLength();
    if (childStartCol != UNKNOWN_COL) {
      if (childNumCols != UNKNOWN_COL) {
        childStartCol += childNumCols;
      } else {
        childStartCol = UNKNOWN_COL;
      }
    }
  }
}


