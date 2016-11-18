// Very simple: Compile with gcc redblack.c and then just run it!

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


///
/// Check that the value is nonzero and message and exit if it's not nonzero.
///

void ASSERT(int value, const char *message, ...)
{
	if (!value)
	{
		va_list arglist;
		va_start(arglist, message);
		printf("ASSERT FAILURE\n");
		vprintf(message, arglist);
		printf("\n");
		va_end(arglist);
		exit(1);
	}
}



///
/// Structs and Enums.
///

enum rb_color {
	rb_black = 0,
	rb_red
};

struct rb_node 
{
	long key;
	void *data;
	struct rb_node *left, *right, *parent;
	enum rb_color color;
	long num_children;
};




/// 
/// _rb_create_node
///
/// Just makes a new node given a parent, key and data. Nodes default to red.
///

struct rb_node *_rb_create_node(struct rb_node *parent, long key, void *data)
{
	struct rb_node *node = (struct rb_node *)malloc(sizeof(struct rb_node));
	
	node->parent = parent;
	node->left = node->right = NULL;
	node->key = key;
	node->data = data;
	node->color = rb_red;
	node->num_children = 0;

	return node;
}



///
/// _rb_find_node
///
/// Find a node under the given node, given its key. Return NULL if it's not found.
///

struct rb_node *_rb_find_node(struct rb_node *node, long key) 
{
	if (!node)
	{
		return NULL;
	}

	if (key < node->key) 
	{
		return _rb_find_node(node->left, key);
	}
	else if (key > node->key)
	{
		return _rb_find_node(node->right, key);
	}
	else // key == node->key
	{
		return node;
	}
}


///
/// _rb_find_smallest
///
/// Find the smallest node under the given one. Return NULL if it's not found.
///

struct rb_node *_rb_find_smallest(struct rb_node *node)
{
	if (node->left == NULL) 
	{
		return node;
	}
	return _rb_find_smallest(node->left);
}



/// 
/// _rb_clear
///
/// Clears a tree or subtree. Note that all pointers to the passed-in node should be 
/// discarded, since it will be freed.
///

void _rb_clear(struct rb_node *node)
{
	if (!node)
	{
		return;
	}

	_rb_clear(node->left);
	_rb_clear(node->right);
	free(node);
}





///
/// _rb_sibling
///
/// Return the child of the parent node which does not route to the current node.
/// Returns NULL if there is no parent.
///

struct rb_node *_rb_sibling(struct rb_node *node)
{
	struct rb_node *parent;

	if (!node)
	{
		return NULL;
	}

	parent = node->parent;
	if (!node->parent)
	{
		return NULL;
	}

	return (node == parent->left) ? parent->right : parent->left;
}





///
/// _rb_parent_child_pointer
///
/// Returns a pointer to the parent of this node's pointer to this child.
/// This assists with rotations and modifications by avoiding a bunch of "? :" syntax.
///

struct rb_node **_rb_parent_child_pointer(struct rb_node *node)
{
	if (!node || !node->parent)
	{
		return NULL;
	}

	if (node->parent->left == node)
	{
		return &node->parent->left;
	}
	else
	{
		return &node->parent->right;
	}
}




///
/// rb_create
///
/// Creates a new red-black tree. The resulting pointer is what needs to be 
/// passed into all the other functions.
///

struct rb_node **rb_create()
{
	struct rb_node **tree = (struct rb_node **)malloc(sizeof(struct rb_node *));
	*tree = NULL;
	return tree;
}




///
/// rb_destroy
///
/// Destroys an rb_tree. Frees all memory associated with it, but leaves
/// the values of nodes untouched (since they're opaque).
///

void rb_destroy(struct rb_node **tree)
{
	_rb_clear(*tree);
	free(*tree);
}




///
/// rb_print
///
/// Prints out the contents of the rb tree
///

void rb_print(struct rb_node **tree, int indent_level)
{
	struct rb_node *node = *tree;

	if (!node || indent_level > 10)	// Limit the depth to avoid infinite loops when there are bugs.
	{
		return;
	}

	for (int i = 0; i < indent_level; i++) {
		printf("   ");
	}
	printf("Key: %ld\tLeft: %ld\tRight: %ld\tParent: %ld\tColor: %s\tChildren: %ld\n", 
		node->key, node->left ? node->left->key : -1, 
		node->right ? node->right->key : -1, 
		node->parent ? node->parent->key : -1,
		node->color == rb_red ? "red" : "black",
		node->num_children);
	rb_print(&node->left, indent_level + 1);
	rb_print(&node->right, indent_level + 1);
}




///
/// _rb_validate_binary_tree
///
/// Checks that the binary tree is valid. That just means each node has a proper left and right child
/// and parent.

void _rb_validate_binary_tree(struct rb_node **tree)
{
	struct rb_node *node = *tree;
	if (!node)
	{
		return;
	}

	if (node->left)
	{
		ASSERT(node->left->key < node->key, "Wrongly placed child node");
		ASSERT(node->left->parent == node, "Child doesn't have me as a parent");
	}

	if (node->right)
	{
		ASSERT(node->right->key > node->key, "Wrongly placed child node");
		ASSERT(node->right->parent == node, "Child doesn't have me as a parent");
	}

	_rb_validate_binary_tree(&node->left);
	_rb_validate_binary_tree(&node->right);
}




///
/// rb_validate
///
/// Verifies that the tree follows all the rules and returns the black depth of the tree:
/// 1. The root is black
/// 2. If a node is red, both its children are black
/// 3. Every path from a given node to the leaf nodes contains the same # of black nodes
///

int rb_validate(struct rb_node **tree, struct rb_node *node)
{
	if (*tree == node)
	{
		ASSERT(node == NULL || node->color == rb_black, "Root node must be black");
	}

	if (!node)
	{
		return 0;
	}

	if (node->color == rb_red)
	{
		ASSERT(node->left == NULL || node->left->color == rb_black, "Left child of a red node must be black");
		ASSERT(node->right == NULL || node->right->color == rb_black, "Left child of a red node must be black");
	}

	{
		int left_depth = rb_validate(tree, node->left);
		int right_depth = rb_validate(tree, node->right);
		ASSERT(left_depth == right_depth, "Black depths must match for each node");

		return left_depth + (node->color == rb_black ? 1 : 0);
	}
}




/// 
/// _rb_insert_fixup
///
/// After an insert, fix the colors and do rotations as necessary.
/// NOTE: THIS IS WHERE YOU CAN TURN OFF THE RED-BLACK EASILY

void _rb_insert_fixup(struct rb_node **tree, struct rb_node *node)
{
	struct rb_node *parent = node->parent;

	ASSERT(node->color == rb_red, "Fixup can only happen on a red node");

	// Root node -> paint it black
	if (!parent)
	{
		node->color = rb_black;
		return;
	}

	// If its parent is black, no problem since no rule violations.
	if (parent->color == rb_black)
	{
		return;
	}
	else
	{
		struct rb_node *sibling;
		struct rb_node *grandparent = parent->parent;
		ASSERT(grandparent != NULL, "Grandparent not found when it should be.");

		// If the parent is red, rotations are required to preserve the red-black rules.
		sibling = _rb_sibling(parent);

		// If the sibling is not found, its color is black by definition.
		if (sibling && sibling->color == rb_red) 
		{
			sibling->color = rb_black;
			parent->color = rb_black;				
			grandparent->color = rb_red;

			_rb_insert_fixup(tree, grandparent);
		}
		else 
		{
			// This node is red but the sibling is black (which means it is colored black or NULL). 
			// Here we need to make sure the node is on the far left or far right of the tree from its
			// grandparent's perspective. If not, a rotation is needed to get it there.
			if (parent == grandparent->left && node == parent->right)
			{
				// Rotate left.
				grandparent->left = node;
				parent->right = node->left;				
				node->left = parent;

				// Update the parents.
				node->parent = grandparent;
				if (parent->right)
				{
					parent->right->parent = parent;
				}
				parent->parent = node;
				node = node->left;
			}
			else if (parent == grandparent->right && node == parent->left)
			{
				// Rotate right.
				grandparent->right = node;
				parent->left = node->right;
				node->right = parent;

				// Update the parents.				
				node->parent = grandparent;
				if (parent->left)
				{
					parent->left->parent = parent;					
				}
				parent->parent = node;
				node = node->right;
			}

			// Get back the parent and grandparent pointers since they may have changed in the
			// prior rotation.
			parent = node->parent;
			grandparent = parent->parent;
			if (grandparent)
			{

				// Finally, make the grandparent red, make the parent black and do a rotation to get everything
				// lined up.				
				struct rb_node **greatgrandparent_child = _rb_parent_child_pointer(grandparent);

				grandparent->color = rb_red;
				parent->color = rb_black;
				if (node == parent->left)
				{
					// Rotate right.
					grandparent->left = parent->right;
					if (grandparent->left)
					{
						grandparent->left->parent = grandparent;
					}

					parent->right = grandparent;
				}
				else
				{

					// Rotate left.
					grandparent->right = parent->left;
					if (grandparent->right)
					{
						grandparent->right->parent = grandparent;
					}

					parent->left = grandparent;

				}

				parent->parent = grandparent->parent;
				grandparent->parent = parent;


				// Complete the rotation (this is the same no matter the direction)
				if (greatgrandparent_child)
				{
					*greatgrandparent_child = parent;
				}
				else
				{
					// In this case, need to reparent the entire tree.
					*tree = parent;
				}

			}

		}

	}

}




///
/// rb_insert
///
/// Insert a new element into the tree. Uses recursion to traverse down the tree.
///

void rb_insert(struct rb_node **tree, struct rb_node **parent, long key, void *data) 
{
	struct rb_node *node = *parent;

	if (!node)
	{
		node = _rb_create_node(NULL, key, data);
		// The root must be colored black.
		node->color = rb_black;
		// Use *parent here since it needs to be saved into the passed-in pointer.
		*parent = node;
		return;
	}

	// Keys must be unique.
	ASSERT(key != node->key, "ERROR: Key already in tree: %ld", key);

	// Grab a pointer to the correct child depending on whether the key is
	// less than or greater than this node's key.
	struct rb_node **child = (key < node->key) ? &(node->left) : &(node->right);
	if (*child)
	{
		// Recursion on the subtree.
		rb_insert(tree, child, key, data);
	}
	else
	{
		// Create the node and do the red-black fixup.
		*child = _rb_create_node(node, key, data);

		// Update the parent nodes with how many children are there.
		for (node = (*child)->parent; node; node = node->parent)
		{
			node->num_children++;	
		}

		_rb_insert_fixup(tree, *child);
	}
}



///
/// _rb_left_rotate
///
/// Perform a left rotate around node.
/// Note that this can change what is the root of the tree. If so, that needs
/// to be updated.

void _rb_left_rotate(struct rb_node **tree, struct rb_node *node)
{
	struct rb_node *child;

	child = node->right;
	node->right = child->left;
	if (child->left)
	{
		child->left->parent = node;
	}

	child->parent = node->parent;
	if (node->parent)
	{
		*_rb_parent_child_pointer(node) = child;
	}
	else
	{
		*tree = child;
	}

	child->left = node;
	node->parent = child;
}




///
/// _rb_right_rotate
///
/// Perform a right rotate around node.
/// This is just like the left rotate above, but it's flipped.

void _rb_right_rotate(struct rb_node **tree, struct rb_node *node)
{
	struct rb_node *child;

	child = node->left;
	node->left = child->right;
	if (child->right)
	{
		child->right->parent = node;
	}

	child->parent = node->parent;
	if (node->parent)
	{
		*_rb_parent_child_pointer(node) = child;
	}
	else
	{
		*tree = child;
	}

	child->right = node;
	node->parent = child;
}




///
/// _rb_left_child
///
/// Just gives the left child.

struct rb_node *_rb_left_child(struct rb_node *node)
{
	return node->left;
}




///
/// _rb_right_child
///
/// Just gives the right child.

struct rb_node *_rb_right_child(struct rb_node *node)
{
	return node->right;
}




///
/// _rb_delete_fixup
///
/// Fix the tree after deleting the given node. It is guaranteed that node will have
/// zero or one children.

void _rb_delete_fixup(struct rb_node **tree, struct rb_node *node)
{
	struct rb_node *sibling;
	struct rb_node *(*child)(struct rb_node *);
	struct rb_node *(*opposite_child)(struct rb_node *);
	void (*rotate)(struct rb_node **, struct rb_node *);
	void (*rotate_opposite)(struct rb_node **, struct rb_node *);

	// Keep running until the node being fixed-up is the root, or the node being fixed-up is red.
	while (node != NULL && node->parent != NULL && node->color != rb_red)
	{

		// This just sets up the function pointers.
		if (node == node->parent->left)
		{
			sibling = node->parent->right;
			child = _rb_left_child;
			opposite_child = _rb_right_child;
			rotate = _rb_left_rotate;
			rotate_opposite = _rb_right_rotate;
		}
		else
		{
			sibling = node->parent->left;
			child = _rb_right_child;
			opposite_child = _rb_left_child;
			rotate = _rb_right_rotate;
			rotate_opposite = _rb_left_rotate;
		}


		// This is the actual algorithm.
		sibling = _rb_sibling(node);

		if (sibling && sibling->color == rb_red)
		{
			sibling->color = rb_black;
			node->parent->color = rb_red;
			rotate(tree, node->parent);
			sibling = _rb_sibling(node);
		}

		if ((!sibling->left || sibling->left->color == rb_black) && (!sibling->right || sibling->right->color == rb_black))
		{
			sibling->color = rb_red;
			node = node->parent;
		}
		else
		{
			if (!opposite_child(sibling) || opposite_child(sibling)->color == rb_black)
			{
				child(sibling)->color = rb_black;
				sibling->color = rb_red;
				rotate_opposite(tree, sibling);
				sibling = _rb_sibling(node);
			}
			sibling->color = node->parent->color;
			node->parent->color = rb_black;
			opposite_child(sibling)->color = rb_black;
			rotate(tree, node->parent);
			break;
		}
	}

	node->color = rb_black;
}




///
/// rb_delete
///
/// Remove an element from the tree.
///

void rb_delete(struct rb_node **tree, long key)
{

	struct rb_node *node = _rb_find_node(*tree, key);
	struct rb_node *victim, *victims_child;

	ASSERT(node != NULL, "rb_delete called on non-existent key.");

	// Goal configuration: We are deleting an element which has zero or one children, not two.
	// If the node to delete has two children, find its successor. Call this node the "victim" and copy its
	// data over the node we wanted to delete. Then delete the victim instead, which is guaranteed
	// to have zero or one children. 
	if (node->left == NULL || node->right == NULL)
	{
		victim = node;
	}
	else
	{
		// Find the successor to node.
		victim = _rb_find_smallest(node->right);
		node->key = victim->key;
		node->data = victim->data;
	}

	// Update the number of children for all the parents.
	for (node = victim->parent; node; node = node->parent)
	{
		node->num_children--;
	}

	if (victim->color == rb_black)
	{
		_rb_delete_fixup(tree, victim);
	}

	// Splice out the victim by repointing its child
	victims_child = (victim->left == NULL) ? victim->right : victim->left;
	if (victims_child) 
	{
		victims_child->parent = victim->parent;
	}

	if (victim->parent == NULL)	// victim was the root?
	{
		*tree = victims_child;
	}
	else
	{
		*_rb_parent_child_pointer(victim) = victims_child;
	}

	free(victim);

}

///
/// rb_lookup
/// 
/// Look up an element in the tree. If it's not found, return NULL.
///

void *rb_lookup(struct rb_node **tree, long key) 
{
	struct rb_node *node = _rb_find_node(*tree, key);
	return (node != NULL) ? node->data : NULL;
}



///
/// rb_count
/// 
/// Returns the number of elements in the tree.
///

long rb_count(struct rb_node **tree) 
{
	struct rb_node *node = *tree;
	if (!node) 
	{
		return 0;
	}
	return 1 + rb_count(&node->left) + rb_count(&node->right);
}



///
/// rb_maximum_depth
/// 
/// Figures out the deepest point in the tree and returns that depth.
///

long rb_maximum_depth(struct rb_node **tree)
{
	struct rb_node *node = *tree;

	if (node == NULL) 
	{
		return 0;
	}

	long left = rb_maximum_depth(&node->left);
	long right = rb_maximum_depth(&node->right);

	return (left > right) ? left + 1 : right + 1;
}



//
//
// UNIT TESTS
// 
//


void TEST_rb_simple()
{
	printf("START TEST_rb_simple\n");

	long i;
	struct rb_node **tree = rb_create();
	
	// Forward inserts.
	for (i = 0; i < 1000; i++)
	{
		rb_insert(tree, tree, i, (void *)i);
	}
	rb_validate(tree, *tree);
	rb_print(tree, 0);
	
	for (i = 0; i < 1000; i++) 
	{
		if ((long)rb_lookup(tree, i) != i)
		{
			printf("Failed on rb_lookup: %ld\n", i);
			exit(1);
		}
	}
	
	if (rb_count(tree) != 1000) 
	{
		printf("Failed on rb_count 1000\n");
		exit(1);
	}

	printf("Maximum depth for forward: %ld\n", rb_maximum_depth(tree));
	
	for (i = 0; i < 1000; i++) 
	{
		rb_delete(tree, i);
	}

	if (rb_count(tree) != 0) 
	{
		printf("Failed on rb_count 0\n");
		exit(1);
	}


	// Reverse inserts.
	for (i = 999; i >= 0; i--)
	{
		rb_insert(tree, tree, i, (void *)i);
	}
	rb_validate(tree, *tree);
	
	for (i = 999; i >= 0; i--) 
	{
		if ((long)rb_lookup(tree, i) != i)
		{
			printf("Failed on rb_lookup: %ld\n", i);
			exit(1);
		}
	}
	
	if (rb_count(tree) != 1000) 
	{
		printf("Failed on rb_count 1000\n");
		exit(1);
	}
	
	printf("Maximum depth for backward: %ld\n", rb_maximum_depth(tree));

	for (i = 999; i >= 0; i--) 
	{
		rb_delete(tree, i);
	}

	if (rb_count(tree) != 0) 
	{
		printf("Failed on rb_count 0\n");
		exit(1);
	}	

	// Randomish inserts. Make an array and shuffle it a bit.
	long array[1000];
	for (i = 0; i < 1000; i++)
	{
		array[i] = i;
	}

	for (i = 0; i < 10000; i++)
	{
		long elem1 = (i * 863) % 1000;
		long elem2 = (i * 427) % 1000;
		long swap = array[elem1];
		array[elem1] = array[elem2];
		array[elem2] = swap;
	}

	for (i = 0; i < 1000; i++)
	{
		rb_insert(tree, tree, array[i], (void *)array[i]);
	}
	
	for (i = 0; i < 1000; i++) 
	{
		if ((long)rb_lookup(tree, i) != i)
		{
			printf("Failed on rb_lookup: %ld\n", i);
			exit(1);
		}
	}
	
	if (rb_count(tree) != 1000) 
	{
		printf("Failed on rb_count 1000\n");
		exit(1);
	}

	printf("Maximum depth for randomish: %ld\n", rb_maximum_depth(tree));
	
	// This time delete the root again and again.
	for (i = 0; i < 1000; i++) 
	{
		rb_delete(tree, (*tree)->key);
	}

	if (rb_count(tree) != 0) 
	{
		printf("Failed on rb_count 0\n");
		exit(1);
	}

	rb_destroy(tree);

	printf("COMPLETED TEST_rb_simple\n");
}




int main(int argc, char **argv)
{
	TEST_rb_simple();
	return 0;
}

