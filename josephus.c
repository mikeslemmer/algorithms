#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef unsigned char BOOL;
#define FALSE 0
#define TRUE 1


long num_splits = 0;
long num_steps = 0;

/* Tree implementation */

#define USE_TREE

/* End tree implementation */



/* SIMPLE HASH TABLE IMPLEMENTATION */
#ifdef USE_HASH_TABLE 

#define HASH_BUCKET_COUNT 113

struct HashElem {
	long key;
	void *data;
	struct HashElem *next;
};
struct HashElem hash_buckets[HASH_BUCKET_COUNT];

void hash_init() {
	memset(hash_buckets, 0, sizeof(struct HashElem) * HASH_BUCKET_COUNT);
}

long hash_function(long key) {
	return key % HASH_BUCKET_COUNT;
}

void hash_insert(long key, void *data) {
	struct HashElem *elem;
	long hash_key = hash_function(key);

	for (elem = &hash_buckets[hash_key]; elem->next != NULL; elem = elem->next);
	elem->key = key;
	elem->data = data;
	elem->next = (struct HashElem *)malloc(sizeof(struct HashElem));
	memset(elem->next, 0, sizeof(struct HashElem));
};

void *hash_lookup(long key) {
	long hash_key = hash_function(key);
	for (struct HashElem *elem = &hash_buckets[hash_key]; elem != NULL; elem = elem->next) {
		if (elem->key == key) {
			return elem->data;
		}
	}
	return NULL;
}

void hash_remove(long key) {
	long hash_key = hash_function(key);
	struct HashElem *next;

	for (struct HashElem *elem = &hash_buckets[hash_key]; elem->next != NULL; elem = elem->next) {
		if (elem->key == key) {
			next = elem->next;
			memcpy(elem, next, sizeof(struct HashElem));
			free(next);
			return;
		}
	}
}

#endif // USE_HASH_TABLE


/* END HASH TABLE */

struct RunElem;
struct ParentElem;
struct ParentElem *root_elem = NULL;
long num_run_elems = 0;
long num_parent_elems = 0;

struct ParentElem {
	long count;
	union {
		struct RunElem *run_elem;
		struct ParentElem *parent_elem;	
	} left_child;

	union {
		struct RunElem *run_elem;
		struct ParentElem *parent_elem;
	} right_child;

	BOOL left_is_run;
	BOOL right_is_run;

	struct ParentElem *parent;
};



struct RunElem {
	long count;
	long start;
	struct RunElem *next;
	struct RunElem *prev;
	struct ParentElem *parent;
};


void dump(struct RunElem *first) {
    long i;
    const long first_start = first->start;
    for (struct RunElem *elem = first; ; elem = elem->next) {
        printf("{ %lx, next: %lx, prev: %lx } ", (unsigned long)elem, (unsigned long)elem->next, (unsigned long)elem->prev);

        printf("[");
        for (i = 0; i < elem->count - 1; i++) {
            printf("%ld, ", elem->start + i);
        }
        if (elem->next->start == first_start) {
            printf("%ld]", elem->start + elem->count - 1);
            break;
        } else {
            printf("%ld], ", elem->start + elem->count - 1);
        }

    }

    printf("\n");
}


struct RunElem *create_run_elem() {
    num_run_elems++;
	struct RunElem *elem = (struct RunElem *)malloc(sizeof(struct RunElem));
    memset(elem, 0, sizeof(struct RunElem));
	return elem;
}

struct ParentElem *create_parent_elem() {
    num_parent_elems++;
	struct ParentElem *elem = (struct ParentElem *)malloc(sizeof(struct ParentElem));
	memset(elem, 0, sizeof(struct ParentElem));
	return elem;
}

/* updateElem
 * 
 * Simply updates an element with the provided data.
 * @param elem The elem
 * @param start 
 * @param count
 * @param prev
 * @param next
 * @return The elem
 */
struct RunElem *updateElem(struct RunElem *elem, long start, long count, struct RunElem *prev, struct RunElem *next) {
	elem->start = start;
	elem->count = count; 
	elem->prev = prev;
	elem->next = next;
    return elem;
}


/* pushBoundariesUp
 * 
 * Informs parents, grandparents, etc of the updated boundaries of the stuff under them.
 *
 * @param elem The bottommost parent_elem that needs a fixup.
 */
void pushBoundariesUp(struct ParentElem *elem) {
	if (!elem) {
		return;
	}
	struct RunElem *run_elem;
	struct ParentElem *parent_elem;
    long count = 0;
	
	if (elem->left_is_run) {
		run_elem = elem->left_child.run_elem;
		if (run_elem) {
            count += run_elem->count;
		}
	} else {
		parent_elem = elem->left_child.parent_elem;
		if (parent_elem) {
            count += parent_elem->count;
		}
	}
	
	if (elem->right_is_run) {
		run_elem = elem->right_child.run_elem;
		if (run_elem) {
            count += run_elem->count;
		}
	} else {
		parent_elem = elem->right_child.parent_elem;
		if (parent_elem) {
			count = parent_elem->count;
		}
	}

    if (count) {
        elem->count = count;
    }

    if (elem->parent) {
	    pushBoundariesUp(elem->parent);
    } else {
        if (root_elem && root_elem != elem) {
            printf("WELL THATS BAD\n");
        }
        root_elem = elem;
    }
}




/* findPersonToKill
 *
 * Given the linked list of ranges of people, identify the one to kill next.
 * @param starting_elem Is the starting elem for doing it.
 * @param people_to_skip is the number of people to skip over. Zero indicates that the person is the first in this run.
 *                       1 indicates the second person. A number too high means need to go to the next run and so on.
 * @out_param out_person_to_kill The zero based element number in this array where the to-be killed person is.
 * @return The element containing the person to kill.
 */
struct RunElem *findPersonToKill(struct RunElem *start_elem, long people_to_skip, long *out_index_to_kill) {
    while (people_to_skip >= start_elem->count) {
		num_steps++;
        people_to_skip -= start_elem->count;
        start_elem = start_elem->next;
    }
    *out_index_to_kill = people_to_skip;
    return start_elem;
}


/* Kills the given person. Does a split as needed, and updates the start pointer as
 * needed.
 * @param elem The element where the kill needs to happen.
 * @param elem A new element to use if needed.
 * @param index The index of the person who needs to die.
 * @param out_who_killed Returns who was killed if anyone.
 * @return Where the start pointer now needs to point.
 */
struct RunElem *doKill(struct RunElem *elem, long index, long *out_who_killed) {
    if (index < 0 || index >= elem->count) {
        return NULL;
    }
    struct RunElem *toRet;

    *out_who_killed = elem->start + index;
    if (index == 0) {
        if (elem->count == 1) {
            // The elem needs to be deleted. Unlink it from the linked list.
            toRet = elem->next;
            if (elem->parent) {
                if (elem->parent->left_child.run_elem == elem) {
                    elem->parent->left_child.run_elem = NULL;
                } else if (elem->parent->right_child.run_elem == elem) {
                    elem->parent->right_child.run_elem = NULL;
                } else {
                    printf("Oops, run_elem has parent that has disowned it!\n");
                    exit(1);
                }
                
            }
            elem->prev->next = elem->next;
            elem->next->prev = elem->prev;
            updateElem(elem, -1, -1, NULL, NULL);

        } else { 
            // Remove the first element in the list. Leave the rest and update the bookkeeping.
            // The next is just this one again since only the first entry got used.
            updateElem(elem, elem->start + 1, elem->count - 1, elem->prev, elem->next);
            toRet = elem;
        }
    } else if (index == elem->count - 1) {
        // Remove the last element in the list. The next one is the following in the list.
        updateElem(elem, elem->start, elem->count - 1, elem->prev, elem->next);
        toRet = elem->next;
    } else {
		struct RunElem *new_elem = create_run_elem();
		num_splits++;
        // Split into two elements.
        updateElem(new_elem, elem->start + index + 1, elem->count - index - 1, elem, elem->next);
        updateElem(elem, elem->start, index, elem->prev, new_elem);
        // These lines are needed for corner cases when you are splitting from having a single
        // elem to a second element.
        new_elem->prev->next = new_elem;
        new_elem->next->prev = new_elem;

		// On a split, need to create a new parent.
		struct ParentElem *parent_elem = create_parent_elem();
		parent_elem->left_child.run_elem = elem;
		parent_elem->left_is_run = TRUE;
		parent_elem->right_child.run_elem = new_elem;
		parent_elem->right_is_run = TRUE;
		parent_elem->count = elem->count + new_elem->count;

		new_elem->parent = parent_elem;
		if (elem->parent) {
			if (elem->parent->left_child.run_elem == elem) {
				elem->parent->left_child.parent_elem = parent_elem;
				elem->parent->left_is_run = FALSE;
			} else if (elem->parent->right_child.run_elem == elem) {
				elem->parent->right_child.parent_elem = parent_elem;
				elem->parent->right_is_run = FALSE;
			} else {
				printf("Error: Tree isn't hooked up properly.\n");
				exit(1);
			}
            parent_elem->parent = elem->parent;
		}
		elem->parent = parent_elem;


        toRet = new_elem;
    }

	pushBoundariesUp(toRet->parent);
	return toRet;
}

/*
 * Runs algorithm
 *
 * Keep an array of "runs" of people who are still alive. Then when someone dies,
 * split one of the elements (realizing there are corner cases where you wouldn't do a split).
 * Then, I believe you can race through people much faster when doing the m%n stuff.
 * struct elem {
 *   long count;
 *   long start;
 * };
 * e.g.
 * elem: { count: 20, start: 7 } - means guys 7 through 26 (i think) are in the run.
 *
 * Timings when compiled -O3
 *
 * Timing as for the naive. Much faster!
 * time ./a.out 200000 30 > /dev/null
 * real	0m0.112s
 * user	0m0.072s
 * sys	0m0.012s
 * 
 */

void runs_algorithm(long n, long m) {

    // Allocate everything.
	long i;
    long num_left;
	long *kill_order = (long *)malloc(n * sizeof(long *));
    long kill_index;
    long who_killed;
    
    
    // Attach the first elem to itself in a simple loop..
    struct RunElem *elem = create_run_elem();

    updateElem(elem, 1, n, elem, elem);

    for (num_left = n; num_left > 0; num_left--) {
        // Pass in m - 1 because we are doing zero-based arithmetic.
        elem = findPersonToKill(elem, (m - 1) % num_left, &kill_index);
        elem = doKill(elem, kill_index, &who_killed);
        if (elem == NULL) {
            printf("Error\n");
            exit(1);
        }
        kill_order[who_killed - 1] = n - num_left + 1;
        if (num_left <= n - 10) {
            exit(1);
        }
        printf("tree root count: %ld\n", root_elem->count);
    }

	for (i = 0; i < n; i++) {
		printf("%ld", kill_order[i]);
		if (i != n - 1) {
			printf(",");
		}
	}
	printf("\n");
	printf("num splits: %ld\n", num_splits);
	printf("num traversal steps: %ld\n", num_steps);
    printf("num parents: %ld\n", num_parent_elems);
    printf("num runs: %ld\n", num_run_elems);

    struct ParentElem *p = root_elem;
    for (i = 1;; i++) {
        if (!p->left_is_run) {
            p = p->left_child.parent_elem;
        } else if (!p->right_is_run) {
            p = p->right_child.parent_elem;
        } else {
            break;
        }
    }
    printf("approx tree height: %ld\n", i);
    printf("tree root count: %ld\n", root_elem->count);

	printf("\n");
}




/* 
 * stacks_algorithm
 *
 *
 */

#define STACKS_BLOCK_SIZE 16
#define NUM_STACK_LEVELS 8 

struct stack {
    long start;
    long size; // How many could fit in here.
    long count; // How many are in here.
};


/*
 * power
 *
 * Super simple integer exponent function
 */

long power(long a, long b) {
    long ret = 1;
    for (int i = 0; i < b; i++) {
        ret *= a;
    }
    return ret;
}



long stack_level_size(long level) {
    return power(STACKS_BLOCK_SIZE, level + 1);
}

struct stack **create_stacks(long n, long *stack_counts) {
    long i, j;

    // Create NUM_STACK_LEVELS levels of stacks (can be extended easily).
    struct stack **stacks = (struct stack **)malloc(sizeof(struct stack *) * NUM_STACK_LEVELS);
    for (i = 0; i < NUM_STACK_LEVELS; i++) {
        const long size = stack_level_size(i);
        const long count = n / size + (n % size > 0 ? 1 : 0);
        stack_counts[i] = count;
        stacks[i] = (struct stack *)malloc(sizeof(struct stack) * count);
        for (j = 0; j < count; j++) {
            long start = j * size;
            struct stack *stack = &stacks[i][j];
            stack->start = j * size;
            if (start + size > n) {
                long ragged_size = n - start;
                stack->size = ragged_size;
                stack->count = ragged_size;
            } else {
                stack->size = size;
                stack->count = size;
            }
        }
    }

    return stacks;
}

struct stack *find_stack(struct stack **stacks, long id, long level) {
    if (level < 0 || level >= NUM_STACK_LEVELS) {
        return NULL;
    }
    long stack_size = stack_level_size(level);

    return &(stacks[level][id / stack_size]);
}


void update_stacks(struct stack **stacks, long id) {
    for (long i = 0; i < NUM_STACK_LEVELS; i++) {
        struct stack *stack = find_stack(stacks, id, i);
        stack->count--;
    }
}

void dump_stacks(struct stack **stacks, long *stack_counts) {
    long i, j;

    // Dump routine
    for (i = 0; i < NUM_STACK_LEVELS; i++) {
        printf("Stack %ld: ", i);
        for (j = 0; j < stack_counts[i]; j++) {
            printf("[%ld, %ld, %ld]", stacks[i][j].start, stacks[i][j].size, stacks[i][j].count);
            if (j != stack_counts[i] - 1) {
                printf(",");
            }
        }
        printf("\n");
    }
}


void stacks_algorithm(long n, long m) {

    long *execution_order = (long *)malloc(n * sizeof(long));

    // Create an array represent who's alive. Note this could be a bitvector.
    unsigned char *array = (unsigned char *)malloc(n);
    memset(array, 1, n);

    // Create stacks to allow faster traversal.
    long *stack_counts = (long *)malloc(sizeof(long *) * NUM_STACK_LEVELS);
    struct stack **stacks = create_stacks(n, stack_counts);


    // Now kill people. First don't use stacks at all.
    long i, pos, last_killed = 0;
    for (i = 0; i < n; i++) {
        long skipped = 0;
        for (pos = last_killed; ; pos = (pos + 1) % n) {
            if (array[pos]) {
                skipped++;
            }
            if (skipped >= m) {
                break;
            }
        }
        last_killed = pos;
        array[last_killed] = 0;
        update_stacks(stacks, last_killed);
        execution_order[pos] = i + 1;
    }

    for (i = 0; i < n; i++) {
        printf("%ld", execution_order[i]);
        if (i != n - 1) {
            printf(",");
        }
    }
    printf("\n");
    dump_stacks(stacks, stack_counts);
}






/*
 * naive_algorithm 
 *
 * Allocate an array and kill people. As you kill people, update the num_alive
 * variable and shrink the array by moving the remaining elements over.
 *
 * Correct result for 10 7: 4,3,5,2,8,6,1,9,10,7
 * 
 * Timings when compiled -O3
 *
 * time ./a.out 200000 30 > /dev/null
 * real   0m1.655s
 * 
 * With memmove (hah slower):
 * time ./a.out 200000 30 > /dev/null
 * real   0m1.681s
 *
 */
void naive_algorithm(long n, long m) {
    long i;
    long num_alive = n;
    long pos;

    long *array = (long *)malloc(n * sizeof(long));
    long *execution_order = (long *)malloc(n * sizeof(long));

    for (i = 0; i < n; i++) {
        array[i] = i + 1;
    }

    for (pos = m - 1; num_alive > 0; pos += m) {
        if (pos >= num_alive) {
            pos %= num_alive;
        }

        execution_order[array[pos] - 1] = n - num_alive + 1;
        num_alive--;
/*
        for (i = pos; i < num_alive; i++) {
            array[i] = array[i + 1];
        }
*/

        memmove(&array[pos], &array[pos + 1], (num_alive - pos) * sizeof(long));


        /* Because we just killed him! */
        pos--;
    }

    for (i = 0; i < n; i++) {
        printf("%ld", execution_order[i]);
        if (i != n - 1) {
            printf(",");
        }
    }
    printf("\n");
    
    free(execution_order);
    free(array);
}


/*
 * Index Algorithm
 *
 */

// This should be based on 'm' I'm pretty sure.
 #define INDEX_BLOCK_SIZE 256

void index_algorithm(int n, int m) {

    long *execution_order = (long *)malloc(n * sizeof(long));

    // Create an array represent who's alive. Note this could be a true bitvector.
    unsigned char *bitvector = (unsigned char *)malloc(n);
    memset(bitvector, 1, n);

    long index_length = (n / INDEX_BLOCK_SIZE) + 1;
    long *index = (long *)malloc(index_length * sizeof(long));

    long i;
    for (i = 0; i < index_length - 1; i++) {
        index[i] = INDEX_BLOCK_SIZE;
    }

    // Last index block has the ragged edge in it.
    index[index_length - 1] = n % INDEX_BLOCK_SIZE;


    // Now kill people.
    long pos, last_killed = 0;
    for (i = 0; i < n; i++) {
        long skipped = 0;
        for (pos = last_killed; ; pos = (pos + 1) % n) {
            //printf("pos: %ld\n", pos);
            // See if we can use the index.
            if (pos % INDEX_BLOCK_SIZE == 0) {
                long index_id = pos / INDEX_BLOCK_SIZE;
                if (skipped + index[index_id] < m) {
                    //printf("Skipping ahead... %ld\n", pos);
                    skipped += index[index_id];
                    pos = (index_id + 1) * INDEX_BLOCK_SIZE - 1; // minus 1 since it will have to +1 next.
                    //printf("Skipped ahead... %ld\n", pos);
                    continue;
                }

            }


            if (bitvector[pos]) {
                skipped++;
            }
            if (skipped >= m) {
                break;
            }
        }
        //printf("KILLED: %ld\n", pos);
        last_killed = pos;

        // Update the index.
        index[pos / INDEX_BLOCK_SIZE]--;

        bitvector[last_killed] = 0;
        execution_order[pos] = i + 1;
    }

    for (i = 0; i < n; i++) {
        printf("%ld", execution_order[i]);
        if (i != n - 1) {
            printf(",");
        }
    }
    printf("\n");

}


/*
 *
 * TESTS
 *
 *
 */



void TEST_ASSERT(long value, const char *string) {
    if (!value) {
        printf("ASSERT FAILURE: %s\n", string);
        exit(1);
    }
}



void TEST_findPersonToKill() {
    struct RunElem elem1, elem2, elem3, elem4;
    updateElem(&elem1, 10, 5, &elem4, &elem2);
    updateElem(&elem2, 20, 5, &elem1, &elem3);
    updateElem(&elem3, 30, 5, &elem2, &elem4);
    updateElem(&elem4, 40, 5, &elem3, &elem1);
    
    struct RunElem *out;
    long outIndex;

    printf("Starting TEST_findPersonToKill()\n");
    out = findPersonToKill(&elem1, 3, &outIndex);
    TEST_ASSERT(out == &elem1, "1");
    TEST_ASSERT(outIndex == 3, "2");
    out = findPersonToKill(&elem4, 8, &outIndex);
    TEST_ASSERT(out == &elem1, "3");
    TEST_ASSERT(outIndex == 3, "4");
    out = findPersonToKill(&elem1, 18, &outIndex);
    TEST_ASSERT(out == &elem4, "5");
    TEST_ASSERT(outIndex == 3, "6");
    out = findPersonToKill(&elem1, 20, &outIndex);
    TEST_ASSERT(out == &elem1, "7");
    TEST_ASSERT(outIndex == 0, "8");
    out = findPersonToKill(&elem1, 55, &outIndex);
    TEST_ASSERT(out == &elem4, "9");
    TEST_ASSERT(outIndex == 0, "10");
    
    printf("TEST_findPersonToKill succeeded\n");

}


void TEST_doKill() {
    struct RunElem elem1, elem2, elem3, elem4;
    updateElem(&elem1, 10, 5, &elem4, &elem2);
    updateElem(&elem2, 20, 5, &elem1, &elem3);
    updateElem(&elem3, 30, 5, &elem2, &elem4);
    updateElem(&elem4, 40, 5, &elem3, &elem1);

    struct RunElem *out;

    printf("Starting TEST_doKill()\n");
    long who_killed;
    out = doKill(&elem1, 0, &who_killed);
    TEST_ASSERT(elem1.prev == &elem4, "1");
    TEST_ASSERT(elem1.next == &elem2, "2");
    TEST_ASSERT(elem1.start == 11, "3");
    TEST_ASSERT(elem1.count == 4, "4");
    TEST_ASSERT(out == &elem1, "5");
    TEST_ASSERT(who_killed == 10, "5.1");
   
    out = doKill(&elem1, 3, &who_killed);
    TEST_ASSERT(elem1.prev == &elem4, "6");
    TEST_ASSERT(elem1.next == &elem2, "7");
    TEST_ASSERT(elem1.start == 11, "8");
    TEST_ASSERT(elem1.count == 3, "9");
    TEST_ASSERT(out == &elem2, "10");
    TEST_ASSERT(who_killed == 14, "10.1");

    out = doKill(&elem2, 2, &who_killed);
    TEST_ASSERT(out == elem2.next, "11");
    TEST_ASSERT(elem2.prev == &elem1, "12");
    TEST_ASSERT(out->next == &elem3, "13");
    TEST_ASSERT(elem2.start == 20, "14");
    TEST_ASSERT(out->start == 23, "15");
    TEST_ASSERT(elem2.count == 2, "16");
    TEST_ASSERT(out->count == 2, "17");
    TEST_ASSERT(who_killed == 22, "17.1");
   
    out = doKill(&elem3, 0, &who_killed);
    out = doKill(&elem3, 0, &who_killed);
    out = doKill(&elem3, 0, &who_killed);
    out = doKill(&elem3, 0, &who_killed);
    out = doKill(&elem3, 0, &who_killed);
    TEST_ASSERT(elem2.next->next == &elem4, "20");
    TEST_ASSERT(elem2.next->next->next == &elem1, "22");
    TEST_ASSERT(elem2.next->next->next->next == &elem2, "23");
    TEST_ASSERT(elem4.prev->prev == &elem2, "21");
    TEST_ASSERT(elem4.prev->prev->prev == &elem1, "24");
    TEST_ASSERT(elem4.prev->prev->prev->prev == &elem4, "25");
   
    
    printf("TEST_doKill() Succeeded\n");

}


void TEST_hashTable() {
#ifdef USE_HASH_TABLE

    printf("Starting TEST_hashTable()\n");
	hash_init();
	hash_insert(0, (void*)57);
	hash_insert(HASH_BUCKET_COUNT, (void*)100);
	hash_insert(HASH_BUCKET_COUNT * 2, (void*)200);
	hash_insert(HASH_BUCKET_COUNT * 3, (void*)300);
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT * 2) == 200, "1");
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT * 3) == 300, "2");
	TEST_ASSERT((long)hash_lookup(0) == 57, "3");
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT) == 100, "4");
	hash_remove(HASH_BUCKET_COUNT * 3);
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT * 2) == 200, "5");
	TEST_ASSERT(hash_lookup(HASH_BUCKET_COUNT * 3) == NULL, "6");
	TEST_ASSERT((long)hash_lookup(0) == 57, "7");
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT) == 100, "8");
	hash_remove(0);
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT * 2) == 200, "9");
	TEST_ASSERT(hash_lookup(HASH_BUCKET_COUNT * 3) == NULL, "10");
	TEST_ASSERT(hash_lookup(0) == NULL, "11");
	TEST_ASSERT((long)hash_lookup(HASH_BUCKET_COUNT) == 100, "12");
	hash_remove(HASH_BUCKET_COUNT);
	hash_remove(HASH_BUCKET_COUNT * 2);
	TEST_ASSERT(hash_lookup(HASH_BUCKET_COUNT * 2) == NULL, "13");
	TEST_ASSERT(hash_lookup(HASH_BUCKET_COUNT * 3) == NULL, "14");
	TEST_ASSERT(hash_lookup(0) == NULL, "15");
	TEST_ASSERT(hash_lookup(HASH_BUCKET_COUNT) == NULL, "16");
	TEST_ASSERT(hash_function(0) == hash_function(HASH_BUCKET_COUNT) && hash_function(HASH_BUCKET_COUNT * 3) == hash_function(HASH_BUCKET_COUNT * 2), "17");

    printf("TEST_hashTable() Succeeded\n");
#endif
}

void TEST() {
    TEST_findPersonToKill();
    TEST_doKill();
#ifdef USE_HASH_TABLE
	TEST_hashTable();
#endif
}



/*
 *
 * main 
 *
 *
 */


int main(int argc, char **argv) {
    long n, m;

    if (argc > 1 && strnstr(argv[1], "test", 4) == argv[1]) {
        TEST();
        return 0;
    }

    if (argc != 3) {
        printf("Usage: %s n m\n", argv[0]);
        return 1;
    }

    n = atoi(argv[1]);
    m = atoi(argv[2]);

    if (n <= 0 || m <= 0) {
        printf("Both n and m must be positive integers.");
        return 1;
    }
 
 
    //naive_algorithm(n, m);  
	//runs_algorithm(n, m); 
    //stacks_algorithm(n, m);
    index_algorithm(n, m);
    
    return 0;
}
