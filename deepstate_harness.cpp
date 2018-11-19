#include <deepstate/DeepState.hpp>

using namespace deepstate;

extern "C" {
#include"red_black_tree.h"
#include "container.h"
}

#define LENGTH 10

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

bool noDuplicates;

int idx;

void InorderTreeVerify(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  if (x != tree->nil) {
    struct elt_t e;
    InorderTreeVerify(tree,x->left);
    e = containerGet (idx);
    ASSERT(e.val == *(int *)x->key) << e.val << " should equal " << *(int *)x->key;
    if (noDuplicates)
      ASSERT(e.info == x->info) << e.info << " should equal " << x->info;
    idx = containerNext (idx);
    InorderTreeVerify(tree,x->right);
  }
}

void RBTreeVerify(rb_red_blk_tree* tree) {
  idx = containerStart();
  InorderTreeVerify(tree,tree->root->left);
  ASSERT(idx == -1) << "idx should be -1!";
}


TEST(RBTree, GeneralFuzzer) {
  rb_red_blk_node* node;
  rb_red_blk_tree* tree;
  tree = RBTreeCreate(IntComp, IntDest, InfoDest, IntPrint, InfoPrint);
  containerCreate();

  noDuplicates = DeepState_Bool();
  if (noDuplicates) {
    LOG(INFO) << "No duplicates allowed.";
  }
  
  for (int n = 0; n < LENGTH; n++) {
    OneOf(
	  [&] {
	    int* ip = intP();
	    if (!noDuplicates || !containerFind(*ip)) {
	      void* vp = voidP();
	      LOG(INFO) << n << ": INSERT:" << *ip << " " << vp;
	      RBTreeInsert(tree, ip, vp);
	      containerInsert(*ip, vp);
	    } else {
	      LOG(INFO) << n << ": AVOIDING DUPLICATE INSERT:" << *ip;
	      free(ip);	      
	    }
	  },
	  [&] {
	    int* ip = intP();
	    LOG(INFO) << n << ": FIND:" << *ip;
	    if ((node = RBExactQuery(tree, ip))) {
	      ASSERT(containerFind(*ip)) << "Expected to find " << *ip;
	      free(ip);
	    } else {
	      ASSERT(!containerFind(*ip)) << "Expected not to find " << *ip;
	      free(ip);
	    }
	  },
	  [&] {
	    int* ip = intP();
	    LOG(INFO) << n << ": DELETE:" << *ip;
	    if ((node = RBExactQuery(tree, ip))) {
	      ASSERT(containerFind(*ip)) << "Expected to find " << *ip;
	      RBDelete(tree, node);
	      containerDelete(*ip);
	    } else {
	      ASSERT(!containerFind(*ip)) << "Expected not to find " << *ip;
	      free(ip);
	    }
	  },
	  [&] {
	    int res, key;
	    int *ip = intP();
	    LOG(INFO) << n << ": PRED:" << *ip;
	    res = containerPred(*ip, &key);
	    if ((node = RBExactQuery(tree,ip))) {
	      node=TreePredecessor(tree,node);
	      if (noDuplicates) {
		if(tree->nil == node) {
		  ASSERT(res==NO_PRED_OR_SUCC) << *ip << " should have no predecessor or successor!";
		} else {
		  ASSERT(res==FOUND) << "Expected to find " << *ip;
		  ASSERT(*(int *)node->key == key) << *(int *)node->key << " should equal " << key;
		}
	      }
	    } else {
	      ASSERT(!containerFind(*ip)) << "Expected not to find " << *ip;
	      ASSERT(res==KEY_NOT_FOUND) << "Expected not to find " << *ip;
	    }
	    free(ip);
	  },
	  [&] {
	    int res, key;
	    int *ip = intP();
	    LOG(INFO) << n << ": SUCC:" << *ip;
	    res = containerSucc(*ip, &key);
	    if ((node = RBExactQuery(tree,ip))) {
	      node=TreeSuccessor(tree,node);
	      if (noDuplicates) {
		if(tree->nil == node) {
		  ASSERT(res==NO_PRED_OR_SUCC) << *ip << " should have no predecessor or successor!";
		} else {
		  ASSERT(res==FOUND) << "Expected to find " << *ip;
		  ASSERT(*(int *)node->key == key) << *(int *)node->key << " should equal " << key;
		}
	      }
	    } else {
	      ASSERT(!containerFind(*ip)) << "Expected not to find " << *ip;
	      ASSERT(res==KEY_NOT_FOUND) << "Expected not to find " << *ip;
	    }
	    free(ip);
	  }
	  );
    LOG(INFO) << "checkRep...";
    checkRep(tree); 
    LOG(INFO) << "RBTreeVerify...";   
    RBTreeVerify(tree);
  }
  LOG(INFO) << "Destroying the tree...";
  RBTreeDestroy(tree);
}
