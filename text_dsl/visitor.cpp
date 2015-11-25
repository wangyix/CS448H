#include "visitor.h"
#include "ast.h"

#include <vector>


/*ComputeConsistentPosVisitor::ComputeConsistentPosVisitor()
  : startColHint(UNKNOWN_COL), numColsHint(UNKNOWN_COL) {}

void ComputeConsistentPosVisitor::visit(StringLiteral* sl) {
  assert(sl->numCols == UNKNOWN_COL);
  sl->numCols = sl->length.value;
}
void ComputeConsistentPosVisitor::visit(RepeatedCharLL* rcll) {
  assert(rcll->numCols == UNKNOWN_COL);
  if (rcll->length.shares) {
    rcll->numCols = numColsHint;
  } else {
    rcll->numCols = rcll->length.value;
  }
}
void ComputeConsistentPosVisitor::visit(RepeatedCharFL* rcfl) {
  assert(rcfl->numCols == UNKNOWN_COL);
}
void ComputeConsistentPosVisitor::visit(Words* w) {
  
}



void ComputeConsistentPosVisitor::visit(Block* b) {
  
}
*/
