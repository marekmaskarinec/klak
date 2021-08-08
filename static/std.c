#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

typedef long long int kk_int;
typedef char kk_char;

typedef enum {
	kk_type_null,
	kk_type_int,
	kk_type_float,
	kk_type_gcobj,
	kk_type_string,
} kk_type;

typedef struct {
	kk_type type;
	void *data;
} kk_cell;

typedef struct {
	int refs;
	kk_type type;
	void *data;
} kk_gcobj;

typedef struct {
	int32_t divident, divisor;
} kk_fraction;

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
		kk_gcobj_dec((kk_gcobj *)target->data);

	memcpy(target, src, sizeof(kk_cell));

	if (src->type == kk_type_gcobj)
		kk_gcobj_inc((kk_gcobj *)src->data);
}

void kk_node_free(kk_node *node) {
	if (node->cell.type == kk_type_gcobj)
		kk_gcobj_dec((kk_gcobj *)node->cell.data);
}

void kk_list_push_front(kk_node **list, kk_cell data, int off) {
	if (data.type == kk_type_gcobj)
		kk_gcobj_inc((kk_gcobj *)data.data);

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

void kk__BUILTIN___SMALLER__() {
	kk_int a = (kk_int)(kk_list_popget(&the_stack).data);
	kk_int b = (kk_int)(kk_list_popget(&the_stack).data);

	kk_int res = b < a;
	memcpy(tmp_cell.data, &res, sizeof(kk_int));
	kk_list_push_front(&the_stack, tmp_cell, 0);
}

int main() {
	kk_line = 0 ;
	{
		kk_int tmp = 1 ;
		memcpy(&tmp_cell.data, &tmp, sizeof(kk_int));
		tmp_cell.type = kk_type_int;
		kk_list_push_front(&the_stack, tmp_cell, 0 );
	}

	kk_line = 0 ;
	tmp_cell = kk_list_popget(&the_stack);
	if ((kk_int)(tmp_cell.data) != 0) {
		kk_line = 0 ;
		{
			kk_char *tmp = malloc(17 );
			if (!tmp) kk_runtime_error("Failed to allocate string.");
			tmp[16 ] = 0;
			strcpy(tmp, "pushing a string");
			kk_gcobj *tmpobj = malloc(sizeof(kk_gcobj));
			if (!tmpobj) kk_runtime_error("Failed to allocate gc object.");
			tmpobj->type = kk_type_string; tmpobj->refs = 0; tmpobj->data = tmp;
			tmp_cell.data = tmpobj;
			tmp_cell.type= kk_type_gcobj;
			kk_list_push_front(&the_stack, tmp_cell, 0 );
		}

		kk_line = 0 ;
		kk_cell __USER__VAR_var = {0};

		kk_line = 0 ;
		kk_cell_copy(&__USER__VAR_var, &the_stack->cell);
		kk_line = 0 ;
		kk_list_popn(&the_stack, 1 );

		kk_line = 0 ;
		if (__USER__VAR_var.type == kk_type_gcobj)
			kk_gcobj_dec((kk_gcobj *)__USER__VAR_var.data);
	kk_line = 0 ;
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
