#include "ast.h"
#include "visitor.h"

#include <stdio.h>

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
    filler->print();
  }
  printf("}v{");
  for (const FillerPtr& filler : bottomFillers) {
    filler->print();
  }
  printf("}");
}

void Block::addChild(ASTPtr child) {
  children.push_back(std::move(child));
  if (child->type == REPEATED_CHAR_FL) {
    hasFLChild = true;
  }
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

static void computeShareLengths(int totalLength, const std::vector<int> shares, std::vector<int>* lengths) {
  assert(totalLength >= 0);
  // initially, evenly distribute from the total length to each share until there isn't enough
  // to add 1 more to every share
  int totalShares = 0;
  for (int s : shares) {
    totalShares += s;
  }
  float avgShare = totalLength / (float)totalShares;
  int avgShareFloor = std::floorf(avgShare);
  *lengths = std::vector<int>(shares.size());
  for (int i = 0; i < shares.size(); ++i) {
    (*lengths)[i] = shares[i] * avgShareFloor;
  }
  // Each remaining unit of length is distributed randomly to the groups, with each group's
  // probability of being selected for a unit being proportional to its number of shares.
  int lengthRemaining = totalLength % totalShares;
  for (; lengthRemaining > 0; --lengthRemaining) {
    float r = (std::rand() / (float)RAND_MAX) * totalShares;
    int runningTotalShares = 0;
    int i;
    for (i = 0; i < shares.size() - 1; ++i) {
      runningTotalShares += shares[i];
      if ((float)runningTotalShares >= r) {
        break;
      }
    }
    (*lengths)[i]++;
  }
}

void AST::computeConsistentPos(int startColHint, int numColsHint) {
  startCol = startColHint;
  int fixedLength = lengthPtr->getFixedLength();
  numCols = (fixedLength == UNKNOWN_COL) ? numColsHint : fixedLength;
}

void Block::computeConsistentPos(int startColHint, int numColsHint) {
  startCol = startColHint;
  numCols = length.shares ? numColsHint : length.value;
  if (startCol == UNKNOWN_COL || numCols == UNKNOWN_COL) {
    throw DSLException(f_at, "Block boundaries are line-dependent.");
  }

  if (!hasGreedyChild() && !hasFLChild) {
    // None of the content varies line-by-line, so all children have consistent length and positions
    // Note this means that all children have literal length.
    int sharesCols = numCols;
    std::vector<int> shares;
    for (const ASTPtr& child : children) {
      LiteralLength* ll = static_cast<LiteralLength*>(child->lengthPtr);
      if (ll->shares) {
        shares.push_back(ll->value);
      } else {
        sharesCols -= ll->value;
      }
    }
    // compute the width of each share-length child
    if (sharesCols < 0) {
      throw DSLException(f_at, "Block length too small to fit its contents.");
    }
    std::vector<int> evaluatedLengths;
    computeShareLengths(sharesCols, shares, &evaluatedLengths);
    int i = 0;
    int childStartCol = startCol;
    for (const ASTPtr& child : children) {
      LiteralLength* ll = static_cast<LiteralLength*>(child->lengthPtr);
      int childNumCols = ll->shares ? evaluatedLengths[i++] : ll->value;
      child->computeConsistentPos(childStartCol, childNumCols);
      childStartCol += childNumCols;
    }
  } else {
    // some content varies line-by-line, so only fixed-length children that are preceded solely
    // by other fixed-length children are consistent.
    int childStartCol = startCol;
    for (const ASTPtr& child : children) {
      child->computeConsistentPos(childStartCol, UNKNOWN_COL);  // only fixed-length children will set their numCols
      int childNumCols = child->numCols;
      // As sson as one child with non-fixed-length is found, childStartCol will be set to UNKNOWN_COL
      // for all subsequent children
      if (childStartCol != UNKNOWN_COL) {
        if (childNumCols == UNKNOWN_COL) {
          childStartCol = UNKNOWN_COL;
        } else {
          childStartCol += childNumCols;
        }
      } 
    }
  }
}
