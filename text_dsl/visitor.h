#ifndef VISITOR_H
#define VISITOR_H

class StringLiteral;
class RepeatedCharLL;
class RepeatedCharFL;
class Words;
class Block;

class Visitor {
public:
  virtual void visit(StringLiteral* sl) = 0;
  virtual void visit(RepeatedCharLL* rcll) = 0;
  virtual void visit(RepeatedCharFL* rcfl) = 0;
  virtual void visit(Words* w) = 0;
  virtual void visit(Block* b) = 0;
};

/*class ComputeConsistentPosVisitor : public Visitor {
public:
  ComputeConsistentPosVisitor();
  void visit(StringLiteral* sl);
  void visit(RepeatedCharLL* rcll);
  void visit(RepeatedCharFL* rcfl);
  void visit(Words* w);
  void visit(Block* b);

  int startColHint;
  int numColsHint;
};*/

#endif
