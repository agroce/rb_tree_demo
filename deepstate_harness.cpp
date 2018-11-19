#include <deepstate/DeepState.hpp>

using namespace deepstate;

extern "C" {
#include"red_black_tree.h"
#include "container.h"
}

#define LENGTH 100

void IntDest(void* a) {
  free((int*)a);
}

int IntComp(const void* a,const void* b) {
  if( *(int*)a > *(int*)b) return(1);
  if( *(int*)a < *(int*)b) return(-1);
  return(0);
}

void IntPrint(const void* a) {
  printf("%i",*(int*)a);
}

void InfoPrint(void* a) {
  ;
}

void InfoDest(void *a){
  ;
}

TEST(RBTree, GeneralFuzzer) {
  rb_red_blk_node* newNode;
  rb_red_blk_tree* tree;
  tree=RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
  containerCreate();
}
