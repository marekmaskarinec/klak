#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef int kk_int;
typedef unsigned int kk_uint;
typedef double kk_float;
typedef char kk_char;
typedef char kk_bool;

typedef enum {
	kk_type_null,
	kk_type_int,
	kk_type_uint,
	kk_type_float,
	kk_type_gcobj,
	kk_type_char,

	kk_type_string,
} kk_type;

typedef struct {
	kk_type type;
	union {
		kk_int   int_val;
		kk_float float_val;
		void     *ptr_val;
		kk_uint  uint_val;
		kk_char  char_val;
	};
} kk_cell;

typedef struct {
	int refs;
	kk_type type;
	void *data;
} kk_gcobj;

typedef struct {
	int len;
	kk_cell *data;
} kk_array;

typedef struct _kk_node {
	struct _kk_node *next;
	kk_cell cell;
} kk_node;

typedef struct {
	char *name;
	kk_cell cell;
} kk_table_item;

typedef struct {
	int size;
	kk_table_item *items;
} kk_table;

kk_node *the_stack = NULL;
kk_cell tmp_cell = {0};
int kk_line = 0;
char *kk_file = NULL;
const char *type_strs[] = {
	"null", "int", "uint", "float", "gc object", "char", "string"
};

void kk_runtime_error(char *msg, ...) {
	fprintf(stderr, "\x1b[1m(%d): \x1b[31mruntime error: \x1b[0m", kk_line);

	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);

	fprintf(stderr, "\n");
	exit(1);
}

void kk_gcobj_inc(kk_gcobj *o) {
	o->refs++;
}

void kk_gcobj_free(kk_gcobj *o) {
	switch (o->type) {
	case kk_type_string:
		free(o->data);
		free(o);
		break;
	}
}

void kk_gcobj_dec(kk_gcobj *o) {
	o->refs--;

	if (!o->refs)
		kk_gcobj_free(o);
}

void kk_cell_copy(kk_cell *target, kk_cell *src) {
	if (target->type == kk_type_gcobj)
		kk_gcobj_dec((kk_gcobj *)target->ptr_val);

	memcpy(target, src, sizeof(kk_cell));

	if (src->type == kk_type_gcobj)
		kk_gcobj_inc((kk_gcobj *)src->ptr_val);
}

void kk_node_free(kk_node *node) {
	if (node->cell.type == kk_type_gcobj)
		kk_gcobj_dec((kk_gcobj *)node->cell.ptr_val);
}

void kk_list_push_front(kk_node **list, kk_cell data, int off) {
	if (data.type == kk_type_gcobj)
		kk_gcobj_inc((kk_gcobj *)data.ptr_val);

	kk_node *node = malloc(sizeof(kk_node));
	if (!node)
		kk_runtime_error("Failed to allocate a list node.");

	node->cell = data;

	if (!off) {
		node->next = *list;
		*list = node;
	} else {
		kk_node *curr = *list;
		for (int i=0; i < off - 1 && curr; i++)
			curr = curr->next;

		node->next = curr->next;
		curr->next = node;
	}
}

void kk_list_popn(kk_node **list, int n) {
	int i;
	for (i=0; *list && i < n; i++) {
		kk_node *to_free = *list;
		*list = (*list)->next;
		kk_node_free(to_free);
		free(to_free);
	}

	if (i != n)
		kk_runtime_error("Trying to pop %d elements from stack %i elements long.", n, i);
}

kk_cell kk_list_popget(kk_node **list) {
	if (*list == NULL)
		kk_runtime_error("Unexpected empty stack.");

	kk_node *to_free = *list;
	kk_cell out = (*list)->cell;
	*list = (*list)->next;
	kk_node_free(to_free);
	free(to_free);
	return out;
}

kk_bool kk_is_true(kk_cell cell) {
	switch (cell.type) {
	case kk_type_int:
		return cell.int_val;
	case kk_type_uint:
		return cell.uint_val;
	case kk_type_float:
		return cell.float_val;
	case kk_type_gcobj:
		return ((kk_gcobj *)cell.ptr_val)->data > 0;
	default:
		return 0;
	}

	return 0;
}

double kk_get_double_val(kk_cell cell) {
	switch (cell.type) {
	case kk_type_int:
		return cell.int_val;
	case kk_type_uint:
		return cell.uint_val;
	case kk_type_float:
		return cell.float_val;
	}

	return 0;
}

void kk_BUILTIN___SMALLER__(void) {
	kk_int a = kk_list_popget(&the_stack).int_val;
	kk_int b = kk_list_popget(&the_stack).int_val;

	printf("%d %d\n", a, b);

	tmp_cell.type = kk_type_int;
	tmp_cell.int_val = b < a;
	kk_list_push_front(&the_stack, tmp_cell, 0);
}

void kk_BUILTIN___EQUAL__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	kk_bool res = 0;

	switch (a.type) {
	case kk_type_null:
		res = b.type == kk_type_null;
		break;
	case kk_type_gcobj:
		switch(((kk_gcobj *)a.ptr_val)->type) {
		case kk_type_string:
			if (((kk_gcobj *)b.ptr_val)->type != kk_type_string)
				kk_runtime_error("Cannot compare string to %s.", type_strs[((kk_gcobj *)b.ptr_val)->type]);
			
			res = !strcmp((char *)((kk_gcobj *)a.ptr_val)->data, (char *)((kk_gcobj *)b.ptr_val)->data);
			break;
		default:
			break;
		}
		break;
	default:
		res = kk_get_double_val(a) == kk_get_double_val(b);
		break;
	}

	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = res }, 0);
}

int main() {
	{
		{
			kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 1 }, 0 );

		}

		kk_cell case_tmp_1  = kk_list_popget(&the_stack);
		
		{
			kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 1 }, 0 );

		}
		kk_list_push_front(&the_stack, case_tmp_1 , 0);
		kk_BUILTIN___EQUAL__();
		if (kk_is_true(kk_list_popget(&the_stack))) {
			printf("first case\n");
			kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 2 }, 0 );

		} else {

			{
				kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 2 }, 0 );

			}
			kk_list_push_front(&the_stack, case_tmp_1 , 0);
			kk_BUILTIN___EQUAL__();
			if (kk_is_true(kk_list_popget(&the_stack))) {
				printf("second case\n");
				kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 3 }, 0 );

			} else {
				printf("\n");
				kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_int, .int_val = 4 }, 0 );

			}
		}
	}

}

#ifdef TEST
int main() {
	int64_t tmp = 5;
	memcpy(tmp_cell, &tmp, sizeof(int64_t));
	printf("%d\n", (int64_t)(*tmp_cell));
	kk_list_push_front(&the_stack, tmp_cell, 0);
	printf("%d\n", (int64_t)(*the_stack->cell));
}
#endif
