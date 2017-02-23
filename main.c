#include "stdio.h"
#include "stdlib.h"

#include "garbage.h"

////////////////////////////////////////////////////////////////////////////////

struct Node {
	int value;
	struct Node* left;
	struct Node* right;
};

struct Node* Node_new(int value) {
	struct Node* n = NEW(struct Node);
	n->value = value;
	n->left = NULL;
	n->right = NULL;
	return n;
}

void Node_insert(struct Node* self, int value) {
	if (self->value == value) {
		return;
	}
	if (self->value < value) {
		if (self->right) {
			Node_insert(self->right, value);
		} else {
			self->right = Node_new(value);
		}
	} else {
		if (self->left) {
			Node_insert(self->left, value);
		} else {
			self->left = Node_new(value);
		}
	}
}

static void printDebug() {
	struct gc_statistics s = gc_get_statistics();
	printf("@ gc is managing %lld bytes across %lld allocations\n",
	(long long)s.bytes, (long long)s.allocations);
}

static void subroutine(int i) {
	struct Node* base = Node_new(5);
	Node_insert(base, 1);
	Node_insert(base, 2);
	Node_insert(base, 7);
	Node_insert(base, 6);
	Node_insert(base, 4);
	Node_insert(base, 3);
	printf("@ after creating tree:\n");
	printDebug();
	printf("@ after removing left\n");
	base->left = NULL;
	gc_collect();
	printDebug();
}

static char* useArray() {
	gc_setinfo("character-array");
	char* foo = NEW_ARRAY(char, 32);
	foo[5] = 'i';
	foo[6] = 'r';
	foo[7] = 'o';
	foo[8] = 'h';
	foo[9] = 'a';
	foo[10] = 0;

	printDebug();
	char* end = foo + 5;
	foo = NULL;
	gc_collect();
	printDebug();

	return end;
}

void* program(void* _) {
	printDebug();

	char* out = useArray();
	printf("@ output <%s>\n", out);

	//subroutine(1);
	//subroutine(2);
	//subroutine(3);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
	gc_initialize(program, NULL);

	return 0;
}
