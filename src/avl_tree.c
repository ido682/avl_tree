#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>

#include "avl_tree.h"

#define BADCOFEE (void *)0xBadC0fee /* invalid address */

typedef struct node node_t;
typedef struct insert_output insert_output_t;

typedef enum CHILD_ENUMS
{
	LEFT = 0, 
	RIGHT = 1, 
	CHILD_ENUM_COUNT = 2
} child_t;

struct node
{
	size_t height;
	node_t *children[CHILD_ENUM_COUNT];
	void *data;
};

struct avl_tree
{
	node_t stub;
	avl_cmp_func_t cmp_func;
	void *param;
};


static node_t *GetChild(const node_t *node, child_t side);
static void *GetData(const node_t *node);
static int HasChild(const node_t *node, child_t side);
static node_t *GetRoot(const avl_tree_t *tree);
void DestroyRecursive(node_t *node);
node_t *InsertRecursive(node_t *root_node, node_t *new_node,
						avl_cmp_func_t cmp_func, void *param);
static node_t *GetStub(avl_tree_t *tree);
node_t *CreateNode(void *data);
int ForEachRecursive(node_t *root_node, avl_action_func_t action_func,
					 void *param);
size_t CountRecursive(const node_t *root_node);
node_t *FindRecursive(node_t *root_node, const void *data,
					  avl_cmp_func_t cmp_func, void *param);
size_t CalculateHeight(node_t *root_node);
size_t Max(size_t a, size_t b);
node_t *RemoveRecursive(node_t *root_node, const void *data,
						avl_cmp_func_t cmp_func, void *param);
node_t *Balance(node_t *node);
static node_t *DiveAndRemoveRecursive(node_t *root_node, child_t side,
									  node_t *node_to_change);
static node_t *RemoveWithTwoChildren(node_t *node);
static node_t *RemoveWithOneChild(node_t *node);
size_t GetHeight(const node_t *node);
node_t *RotateOneSide(node_t *grandparent, child_t side);
node_t *RotateLeft(node_t *grandparent);
node_t *RotateRight(node_t *grandparent);
node_t *RotateLeftRight(node_t *grandparent);
node_t *RotateRightLeft(node_t *grandparent);
ssize_t CalculateBalance(const node_t *node);

/****************************************************************************/


avl_tree_t *AVLTreeCreate(avl_cmp_func_t cmp_func, void *param)
{
	avl_tree_t *tree = NULL;
	node_t stub = {0};

	assert(cmp_func != NULL);
	
	tree = malloc(sizeof(avl_tree_t));
	if (NULL == tree)
	{
		return (NULL);
	}

	stub.height = 0;
	stub.children[LEFT] = NULL;
	stub.children[RIGHT] = BADCOFEE;
	stub.data = BADCOFEE;

	tree->stub = stub;
	tree->cmp_func = cmp_func;
	tree->param = param;

	return (tree);
}

/************/
void AVLTreeDestroy(avl_tree_t *tree)
{
	assert(tree != NULL);

	DestroyRecursive(GetRoot(tree));

	free(tree);
	tree = NULL;
}

/************/
void DestroyRecursive(node_t *node)
{
	if (NULL == node)
	{
		return;
	}

	DestroyRecursive(GetChild(node, LEFT));
	DestroyRecursive(GetChild(node, RIGHT));

	free(node);
	node = NULL;
}

/************/
int AVLTreeIsEmpty(const avl_tree_t *tree)
{
	return(NULL == GetRoot(tree));
}

/************/
size_t AVLTreeHeight(const avl_tree_t *tree)
{
	return (GetHeight(GetRoot(tree)));
}

/************/
int AVLTreeInsert(avl_tree_t *tree, void *data)
{
	node_t *new_node = NULL;
	
	assert(tree != NULL);
	assert(data != NULL);

	new_node = CreateNode(data);
	if (NULL == new_node)
	{
		return (1);
	}

	if (AVLTreeIsEmpty(tree))
	{
		GetStub(tree)->children[LEFT] = new_node;
		return (0);
	}

	GetStub(tree)->children[LEFT] =
		InsertRecursive(GetRoot(tree), new_node, tree->cmp_func, tree->param);

	return (0);
}

/************/
node_t *InsertRecursive(node_t *root_node, node_t *new_node,
						avl_cmp_func_t cmp_func, void *param)
{
	int cmp_result = 0;
	child_t side = LEFT;
	node_t *child = NULL;
	
	if (NULL == root_node)
	{
		return (new_node);
	}

	cmp_result = cmp_func(GetData(root_node), GetData(new_node), param);
	/* Inserting an existing node is prohibited */
	assert(cmp_result != 0); 

	/* RIGHT if true, LEFT if false */
	side = (cmp_result < 0); 
	child = GetChild(root_node, side);

	root_node->children[side] =
		InsertRecursive(child, new_node, cmp_func, param);

	return (Balance(root_node));
}

/************/
int AVLTreeForEach(avl_tree_t *tree,
				   avl_action_func_t action_func,
				   void *param)
{
	assert(tree != NULL);
	assert(action_func != NULL);

	return (ForEachRecursive(GetRoot(tree), action_func, param));
}

/************/
int ForEachRecursive(node_t *root_node,
					 avl_action_func_t action_func,
					 void *param)
{
	int ret_val = 0;
	node_t *left_child = NULL;
	node_t *right_child = NULL;

	if (NULL == root_node)
	{
		return (0);
	}

	left_child = GetChild(root_node, LEFT);
	right_child = GetChild(root_node, RIGHT);

	ret_val = ForEachRecursive(left_child, action_func, param);
	if (ret_val != 0)
	{
		return (ret_val);
	}

	ret_val = action_func(GetData(root_node), param);
	if (ret_val != 0)
	{
		return (ret_val);
	}

	ret_val = ForEachRecursive(right_child, action_func, param);
	if (ret_val != 0)
	{
		return (ret_val);
	}

	return (0);
}

/************/
size_t AVLTreeCount(const avl_tree_t *tree)
{
	assert(tree != NULL);

	return (CountRecursive(GetRoot(tree)));
}

/************/
size_t CountRecursive(const node_t *root_node)
{
	size_t nodes_ctr = 0;
	node_t *left_child = NULL;
	node_t *right_child = NULL;

	if (NULL == root_node)
	{
		return (0);
	}

	left_child = GetChild(root_node, LEFT);
	right_child = GetChild(root_node, RIGHT);

	++nodes_ctr;
	nodes_ctr += CountRecursive(left_child);
	nodes_ctr += CountRecursive(right_child);

	return (nodes_ctr);
}

/************/
void *AVLTreeFind(const avl_tree_t *tree, const void *data_to_match)
{
	node_t *found_node = NULL;
	void *found_data = NULL;
	
	assert(tree != NULL);
	assert(data_to_match != NULL);

	found_node = FindRecursive(GetRoot(tree), data_to_match,
							   tree->cmp_func, tree->param);

	if (found_node != NULL)
	{
		found_data = GetData(found_node);
	}

	return (found_data);
}

/************/
node_t *FindRecursive(node_t *root_node, const void *data,
					  avl_cmp_func_t cmp_func, void *param)
{
	int cmp_result = 0;
	child_t side = LEFT;
	node_t *child = NULL;
	
	if (NULL == root_node)
	{
		return (NULL);
	}

	cmp_result = cmp_func(data, GetData(root_node), param);

	/* An exact match */
	if (0 == cmp_result) 
	{
		return (root_node);
	}

	/* RIGHT if true, LEFT if false */
	side = (cmp_result > 0); 
	child = GetChild(root_node, side);

	return (FindRecursive(child, data, cmp_func, param));
}

/************/
void AVLTreeRemove (avl_tree_t *tree, const void *data_to_remove)
{
	assert(tree != NULL);
	assert(data_to_remove != NULL);

	GetStub(tree)->children[LEFT] =
		RemoveRecursive(GetRoot(tree), data_to_remove,
						tree->cmp_func, tree->param);
}

/************/
node_t *RemoveRecursive(node_t *root_node, const void *data, avl_cmp_func_t cmp_func, void *param)
{
	int cmp_result = 0;
	child_t side = LEFT;
	node_t *child = NULL;
	
	if (NULL == root_node)
	{
		return (root_node);
	}

	cmp_result = cmp_func(data, GetData(root_node), param);

	/* An exact match, remove is comitted */
	if (0 == cmp_result) 
	{
		if (HasChild(root_node, LEFT) && HasChild(root_node, RIGHT))
		{
			return (RemoveWithTwoChildren(root_node));
		}
		else
		{
			/* Works also for a leaf (no children at all) */
			return (RemoveWithOneChild(root_node)); 
		}
	}

	/* RIGHT if true, LEFT if false */
	side = (cmp_result > 0); 
	child = GetChild(root_node, side);

	root_node->children[side] =
		RemoveRecursive(child, data, cmp_func, param);

	return (Balance(root_node));
}

/************/
static node_t *RemoveWithOneChild(node_t *node)
{
    node_t *child = NULL;

	assert(node != NULL);

	if (HasChild(node, LEFT))
	{
		child = GetChild(node, LEFT);
	}
	else
	{
		/* Works also for a leaf (no children at all) */
		child = GetChild(node, RIGHT); 
	}

    free(node);
	node = NULL;

    return (child);
}

/************/
static node_t *RemoveWithTwoChildren(node_t *node)
{
    node_t *left_child = NULL;

	assert(node != NULL);

	left_child = GetChild(node, LEFT);
	/* the function DiveAndRemoveRecursive:
	The previous node's data replaces the current node's data
	(the node is passed to the function for this reason),
	than the previous node is removed */
	node->children[LEFT] = DiveAndRemoveRecursive(left_child, RIGHT, node);

	return (Balance(node));
} 

/************/
static node_t *DiveAndRemoveRecursive(node_t *root_node,
									  child_t side,
									  node_t *node_to_change)
{
    node_t *child = NULL;
	
	assert(root_node != NULL);

	child = GetChild(root_node, side);
	
	/* last node on the required side is found - its data replaces
	the data of node should be removed, and the found node is removed */
	if (NULL == child) 
	{
		/* node_to_change is the node had to be removed originally */
		node_to_change->data = GetData(root_node);
		/* necessarily has one child at most */
		return (RemoveWithOneChild(root_node)); 
	}

	root_node->children[side] =
		DiveAndRemoveRecursive(child, side, node_to_change);

	return (Balance(root_node));
}




/********************  BALANCING FUNCTIONS  *********************/

node_t *Balance(node_t *node)
{
	ssize_t difference = 0;
	node_t *left_child = NULL;
	node_t *right_child = NULL;
	node_t *node_to_return = node;

	assert(node != NULL);

	/* "height" field in node is updated */
	CalculateHeight(node); 

	left_child = GetChild(node, LEFT);
	right_child = GetChild(node, RIGHT);

	difference = CalculateBalance(node);
	
	/* left subtree is higher by more than one level */
	if (difference > 1) 
	{
		difference  = CalculateBalance(left_child);
		/* left subtree of left subtree is higher or equal */
		if (difference >= 0)
		{
			node_to_return = RotateRight(node);
		}
		/* right subtree of left subtree is higher */
		else
		{
			node_to_return = RotateLeftRight(node);
		}
	}
	/* right subtree is higher by more than one level */
	else if (difference < -1) 
	{
		difference = CalculateBalance(right_child);

		/* right subtree of right subtree is higher or equal */
		if (difference <= 0)
		{
			node_to_return = RotateLeft(node);
		}
		/* left subtree of right subtree is higher */
		else
		{
			node_to_return = RotateRightLeft(node);
		}
	}

	return (node_to_return);
}

/************/
ssize_t CalculateBalance(const node_t *node)
{
	node_t *left_child = NULL;
	node_t *right_child = NULL;

	assert(node != NULL);
	
	left_child = GetChild(node, LEFT);
	right_child = GetChild(node, RIGHT);

	return (GetHeight(left_child) - GetHeight(right_child));
}

/************/
size_t CalculateHeight(node_t *root_node)
{
	size_t height = 0;
	node_t *left_child = NULL;
	node_t *right_child = NULL;

	if (NULL == root_node)
	{
		return (0);
	}

	left_child = GetChild(root_node, LEFT);
	right_child = GetChild(root_node, RIGHT);

	height = Max(GetHeight(left_child), GetHeight(right_child)) + 1;

	root_node->height = height;

	return (height);
}

/************/
size_t GetHeight(const node_t *node)
{
	if (NULL == node)
	{
		return (0);
	}
	
	return (node->height);
}

/************/
node_t *RotateOneSide(node_t *grandparent, child_t side)
{
	node_t *parent = GetChild(grandparent, !side);

	grandparent->children[!side] = parent->children[side];
	parent->children[side] = grandparent;
	CalculateHeight(grandparent);
	CalculateHeight(parent);

	return (parent);
}

/************/
node_t *RotateLeft(node_t *grandparent)
{
	return (RotateOneSide(grandparent, LEFT));
}

/************/
node_t *RotateRight(node_t *grandparent)
{
	return (RotateOneSide(grandparent, RIGHT));
}

/************/
node_t *RotateLeftRight(node_t *grandparent)
{
	node_t *parent = GetChild(grandparent, LEFT);
	grandparent->children[LEFT] = RotateLeft(parent);
	return (RotateRight(grandparent));
}

/************/
node_t *RotateRightLeft(node_t *grandparent)
{
	node_t *parent = GetChild(grandparent, RIGHT);
	grandparent->children[RIGHT] = RotateRight(parent);
	return (RotateLeft(grandparent));
}




/********************  MORE FUNCTIONS  *********************/

static node_t *GetChild(const node_t *node, child_t side)
{
    return(node->children[side]);
}

/************/
static void *GetData(const node_t *node)
{
    return(node->data);
}

/************/
node_t *CreateNode(void *data)
{
	node_t *new_node = malloc(sizeof(node_t));
	
	if (NULL == new_node)
	{
		return (NULL);
	}

	new_node->data = data;
	new_node->children[LEFT] = NULL;
	new_node->children[RIGHT] = NULL;
	new_node->height = 1;

	return (new_node);
}

/************/
static node_t *GetRoot(const avl_tree_t *tree)
{
    return (tree->stub.children[LEFT]);
}

/************/
static node_t *GetStub(avl_tree_t *tree)
{
    return (&tree->stub);
}

/************/
static int HasChild(const node_t *node, child_t side)
{
    return(GetChild(node, side) != NULL);
}

/************/
size_t Max(size_t a, size_t b)
{
	return ((a >= b) ? (a) : (b));
}
