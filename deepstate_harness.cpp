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
  
  rb_red_blk_tree* tree = RBTreeCreate(IntComp, IntDest, InfoDest, IntPrint, InfoPrint);  
  containerCreate();

  noDuplicates = DeepState_Bool();
  if (noDuplicates) {
    LOG(INFO) << "No duplicates allowed.";
  }
  
  for (int n = 0; n < LENGTH; n++) {
    OneOf(
	  [&] {
	    int key = DeepState_Int();
	    int* ip = (int*)malloc(sizeof(int));
	    *ip = key;
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
	    int key = DeepState_Int();
	    LOG(INFO) << n << ": FIND:" << key;
	    if ((node = RBExactQuery(tree, &key))) {
	      ASSERT(containerFind(key)) << "Expected to find " << key;
	    } else {
	      ASSERT(!containerFind(key)) << "Expected not to find " << key;
	    }
	  },
	  [&] {
	    int key = DeepState_Int();
	    LOG(INFO) << n << ": DELETE:" << key;
	    if ((node = RBExactQuery(tree, &key))) {
	      ASSERT(containerFind(key)) << "Expected to find " << key;
	      RBDelete(tree, node);
	      containerDelete(key);
	    } else {
	      ASSERT(!containerFind(key)) << "Expected not to find " << key;
	    }
	  },
	  [&] {
	    int key1 = DeepState_Int();	    
	    int res, key2;
	    LOG(INFO) << n << ": PRED:" << key1;
	    res = containerPred(key1, &key2);
	    if ((node = RBExactQuery(tree, &key1))) {
	      node = TreePredecessor(tree, node);
	      if (noDuplicates) {
		if(tree->nil == node) {
		  ASSERT(res==NO_PRED_OR_SUCC) << key1 << " should have no predecessor or successor!";
		} else {
		  ASSERT(res==FOUND) << "Expected to find " << key1;
		  ASSERT(*(int *)node->key == key2) << *(int *)node->key << " should equal " << key2;
		}
	      }
	    } else {
	      ASSERT(!containerFind(key1)) << "Expected not to find " << key1;
	      ASSERT(res==KEY_NOT_FOUND) << "Expected not to find " << key1;
	    }
	  },
	  [&] {
	    int key1 = DeepState_Int();	    
	    int res, key2;
	    LOG(INFO) << n << ": SUCC:" << key1;
	    res = containerSucc(key1, &key2);
	    if ((node = RBExactQuery(tree, &key1))) {
	      node = TreeSuccessor(tree, node);
	      if (noDuplicates) {
		if(tree->nil == node) {
		  ASSERT(res==NO_PRED_OR_SUCC) << key1 << " should have no predecessor or successor!";
		} else {
		  ASSERT(res==FOUND) << "Expected to find " << key1;
		  ASSERT(*(int *)node->key == key2) << *(int *)node->key << " should equal " << key2;
		}
	      }
	    } else {
	      ASSERT(!containerFind(key1)) << "Expected not to find " << key1;
	      ASSERT(res==KEY_NOT_FOUND) << "Expected not to find " << key1;
	    }
	  },
	  [&] {
	    int i;
	    int key1 = DeepState_Int();
	    int key2 = DeepState_Int();
	    i = containerStartVal(key1, key2);
	    stk_stack *enumResult = RBEnumerate(tree, &key1, &key2);	  
	    while ((node = (rb_red_blk_node *)StackPop(enumResult))) {
	      struct elt_t e;
	      ASSERT(i != -1) << "i should never be -1";
	      e = containerGet(i);
	      ASSERT(e.val == *(int *)node->key) << e.val << " should equal " *(int *)node->key;
	      if (noDuplicates)
		ASSERT(e.info == node->info) << e.info << " should equal" << node.info;
	      i = containerNextVal(key2, i);
	    }
	    ASSERT(i==-1) << "i should never be -1";
	    free(enumResult);
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
