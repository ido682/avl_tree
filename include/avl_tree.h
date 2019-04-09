#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

#include <stddef.h> /* size_t */

typedef int (*avl_action_func_t)(void *data, void *param);
typedef int (*avl_cmp_func_t)(const void *external_data,
							  const void *internal_data,
							  void *param);
typedef int (*avl_is_match_func_t)(const void *external_data,
								   const void *internal_data,
								   void *param);
typedef struct avl_tree avl_tree_t;


/*******************************************************************/

avl_tree_t *AVLTreeCreate(avl_cmp_func_t func, void *param);
/*                   
Returns NULL on failure.
*/

void AVLTreeDestroy(avl_tree_t *avl_tree);

int AVLTreeInsert(avl_tree_t *avl_tree, void *data);
/*
A reference to the data has to be passed.
An already-existing data mustn't be inserted (undefined behaviour).
Returns 0 on seccuess, 1 on failure.
*/

size_t AVLTreeHeight(const avl_tree_t *tree);

size_t AVLTreeCount(const avl_tree_t *tree);

int AVLTreeIsEmpty(const avl_tree_t *tree);

void *AVLTreeFind(const avl_tree_t *tree, const void *data_to_match);
/*                
Returned NULL if not found.
*/

int AVLTreeForEach(avl_tree_t *tree, avl_action_func_t action, void *param);
/*
"action" is invoked on each element in tree, until a non-zero value
is returned.
Returnes the first non-zero returned value, or 0 if all action funcs
returned 0.
action funcs invoked in-order.
*/

void AVLTreeRemove(avl_tree_t *tree, const void *data_to_remove);


#endif /*__AVL_TREE_H__*/

