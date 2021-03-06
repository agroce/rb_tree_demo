#include<stdio.h>
#include<ctype.h>
#include <assert.h>
#include <time.h>
#include <stdint.h>

extern "C" {
#include"red_black_tree.h"
#include "container.h"
}

#define DEEPSTATE_TAKEOVER_RAND

#include <deepstate/DeepState.hpp>

using namespace deepstate;

#define META_REPS 5000
#define FUZZ_REPS 1000
#define TIMEOUT 60

/*  this file has functions to test a red-black tree of integers */

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

int FUZZ_RANGE;

int randomInt (void)
{
  return rand()%FUZZ_RANGE;
}

void *randomVoidP (void)
{
  uintptr_t p = 0;
  int i;
  for (i=0; i<sizeof(p); i++) {
    p <<= 8;
    p |= rand()%256;
  }
  return (void *)p;
}

int idx;

int nodups;

void InorderTreeVerify(rb_red_blk_tree* tree, rb_red_blk_node* x) {
  if (x != tree->nil) {
    struct elt_t e;
    InorderTreeVerify(tree,x->left);
    e = containerGet (idx);
    assert (e.val == *(int *)x->key);
    if (nodups) assert (e.info == x->info);
    idx = containerNext (idx);
    InorderTreeVerify(tree,x->right);
  }
}

void RBTreeVerify(rb_red_blk_tree* tree) {
  idx = containerStart();
  InorderTreeVerify(tree,tree->root->left);
  assert (idx == -1);
}

TEST(RBTree, JohnsFuzzer) {
  stk_stack* enumResult;
  int option=0;
  int newKey,newKey2;
  int* newInt;
  rb_red_blk_node* newNode;
  rb_red_blk_tree* tree;
  int i;
  int fuzz_reps;

  fuzz_reps = 1+rand()%FUZZ_REPS;

  tree=RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
  containerCreate ();
  nodups = rand()%2;
  if (rand()%2 == 0) {
    FUZZ_RANGE = 1 + rand()%fuzz_reps;
  } else {
    FUZZ_RANGE = 1 + rand()%RAND_MAX;
  }

  for (i=0; i<fuzz_reps; i++) {

    checkRep (tree);

  again:
    option = 1 + rand()%7;
    switch(option)
      {
      case 1:
	{
	  void *p;
	  newKey = randomInt();

	  if (nodups) {
	    if (containerFind (newKey)) goto again;
	  }

	  newInt=(int*) malloc(sizeof(int));
	  *newInt=newKey;
	  p = randomVoidP();
	  RBTreeInsert(tree,newInt,p);
	  containerInsert(newKey,p);
	}
	break;
	
      case 2:
	{
	  newKey = randomInt();
	  if ( ( newNode=RBExactQuery(tree,&newKey ) ) ) {
	    assert (containerFind (newKey));
	    RBDelete(tree,newNode);/*assignment*/
	    containerDelete (newKey);
	  } else {
	    assert (!containerFind (newKey));
	  }
	}
	break;

      case 3:
	{
	  newKey = randomInt();
	  if ( ( newNode = RBExactQuery(tree,&newKey) ) ) {/*assignment*/
	    assert (containerFind (newKey));
	  } else {
	    assert (!containerFind (newKey));
	  }
	}
	break;
      case 4:
	{
	  int res, key2;
	  newKey = randomInt();
	  res = containerPred(newKey, &key2);
	  if ( ( newNode = RBExactQuery(tree,&newKey) ) ) {/*assignment*/
	    newNode=TreePredecessor(tree,newNode);
	    if (nodups) {
	      if(tree->nil == newNode) {
		assert (res==NO_PRED_OR_SUCC);
	      } else {
		assert (res==FOUND);
		assert (*(int *)newNode->key == key2);
	      }
	    }
	  } else {
	    assert (res==KEY_NOT_FOUND);
	  } 
	}
	break;
      case 5:
	{
	  int res, key2;
	  newKey = randomInt();
	  res = containerSucc(newKey, &key2);
	  if ( (newNode = RBExactQuery(tree,&newKey) ) ) {
	    newNode=TreeSuccessor(tree,newNode);
	    if (nodups) {
	      if(tree->nil == newNode) {
		assert (res==NO_PRED_OR_SUCC);
	      } else {
		assert (res==FOUND);
		assert (*(int *)newNode->key == key2);	      
	      }
	    }
	  } else {
	    assert (!containerFind (newKey));
	    assert (res==KEY_NOT_FOUND);
	  }
	}
	break;
      case 6:
	{
	  int i;
	  newKey = randomInt();
	  newKey2 = randomInt();
	  i = containerStartVal (newKey,newKey2);
	  enumResult=RBEnumerate(tree,&newKey,&newKey2);	  
	  while ( (newNode = (rb_red_blk_node *)StackPop(enumResult)) ) {
	    struct elt_t e;
	    assert (i != -1);
	    e = containerGet(i);
	    assert (e.val == *(int *)newNode->key);
	    if (nodups) assert (e.info == newNode->info);
	    i = containerNextVal (newKey2, i);
	  }
	  assert (i==-1);
	  free(enumResult);
	}
	break;
      case 7:
	{
	  RBTreeVerify(tree);
	}
	break;
      default:
	assert (0);
      }
  }
  RBTreeVerify(tree);
  if (rand()%2 == 0) {
    while (1) {
      int val;
      int res = containerRandom (&val);
      if (!res) break;
      containerDelete (val);
      if (!(newNode=RBExactQuery(tree,&val))) assert (0);
      RBDelete(tree,newNode);
    }    
  }
  RBTreeDestroy(tree);
}
