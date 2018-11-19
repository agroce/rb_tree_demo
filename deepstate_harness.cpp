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

int* intP() {
  symbolic_int x;
  int *p = (int*)malloc(sizeof(int));
  *p = x;
  return p;
}

void *voidP() {
  uintptr_t p = 0;
  int i;
  for (i=0; i<sizeof(p); i++) {
    p <<= 8;
    p |= DeepState_Char();
  }
  return (void *)p;
}

TEST(RBTree, GeneralFuzzer) {
  int n;
  
  rb_red_blk_node* newNode;
  rb_red_blk_tree* tree;
  tree=RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
  containerCreate();
  
  for (int n = 0; n < LENGTH; n++) {
    OneOf(
	  [&] {
	    int* ip = intP();
	    void* vp = voidP();
	    RBTreeInsert(tree, ip, vp);
	    containerInsert(*ip, vp);
	  });
  }
}
