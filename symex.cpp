#include <deepstate/DeepState.hpp>

using namespace deepstate;

extern "C" {
#include"red_black_tree.h"
#include "container.h"
}

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

void *voidP() {
  uintptr_t p = 0;
  int i;
  for (i=0; i<sizeof(p); i++) {
    p <<= 8;
    p |= DeepState_Char();
  }
  return (void *)p;
}

TEST(RBTree, TinySymex) {
  rb_red_blk_node* node;
  
  rb_red_blk_tree* tree = RBTreeCreate(IntComp, IntDest, InfoDest, IntPrint, InfoPrint);  
  containerCreate();

  int key = DeepState_Int();
  int* ip = (int*)malloc(sizeof(int));
  *ip = key;
  void* vp = voidP();
  RBTreeInsert(tree, ip, vp);

  key = DeepState_Int();
  ip = (int*)malloc(sizeof(int));
  *ip = key;
  vp = voidP();
  RBTreeInsert(tree, ip, vp);
}
