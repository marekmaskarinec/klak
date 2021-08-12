#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define GCOBJ(a) ((kk_gcobj *)(a).ptr_val)
#define CONS(a) ((kk_cons *)a->data)

typedef double kk_float;
typedef char kk_char;
typedef char kk_bool;

typedef enum {
	kk_type_null,
	kk_type_float,
	kk_type_gcobj,
	kk_type_char,

	kk_type_string,
	kk_type_cons,
} kk_type;

typedef struct {
	kk_type type;
	union {
		kk_float float_val;
		void     *ptr_val;
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
	kk_cell car;
	kk_cell cdr;
} kk_cons;

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
kk_bool tmp_res = 0;

int kk_line = 0;
char *kk_file = NULL;
const char *type_strs[] = {
	"null", "float", "gc object", "char", "string", "cons"
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

void kk_gcobj_inc(kk_cell *o) {
	if (o->type == kk_type_gcobj)
		((kk_gcobj *)(o->ptr_val))->refs++;
}

void kk_gcobj_free(kk_gcobj *o);
void kk_cell_free(kk_cell cell) {
	switch (cell.type) {
	case kk_type_gcobj:
		kk_gcobj_free(GCOBJ(cell));
		break;
	}
}

void kk_gcobj_free(kk_gcobj *o) {
	switch (o->type) {
	case kk_type_string:
		free(o->data);
		break;
	case kk_type_cons:
		kk_cell_free(CONS(o)->car);
		kk_cell_free(CONS(o)->cdr);
		free(o->data);
		break;
	}
	free(o);
}

void kk_gcobj_dec(kk_cell *c) {
	if (c->type != kk_type_gcobj)
		return;

	kk_gcobj *o = ((kk_gcobj *)c->ptr_val);

	o->refs--;

	if (!o->refs)
		kk_gcobj_free(o);
}

void kk_cell_copy(kk_cell *target, kk_cell *src) {
	if (target->type == kk_type_gcobj)
		kk_gcobj_dec(target);

	memcpy(target, src, sizeof(kk_cell));

	if (src->type == kk_type_gcobj)
		kk_gcobj_inc(src);
}

void kk_node_free(kk_node *node) { }

void kk_list_push_front(kk_node **list, kk_cell data, int off) {
	if (data.type == kk_type_gcobj)
		kk_gcobj_inc(&data);

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

int kk_list_len(kk_node *list) {
	int len = 0;
	for (; list; list = list->next)
		len++;

	return len;
}

kk_bool kk_is_true(kk_cell cell) {
	switch (cell.type) {
	case kk_type_char:
		return cell.char_val;
	case kk_type_float:
		return cell.float_val;
	case kk_type_gcobj:
		return ((kk_gcobj *)cell.ptr_val)->data > 0;
	default:
		return 0;
	}

	return 0;
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
				kk_runtime_error("=: Cannot compare string to %s.", type_strs[((kk_gcobj *)b.ptr_val)->type]);
			
			res = !strcmp((char *)((kk_gcobj *)a.ptr_val)->data, (char *)((kk_gcobj *)b.ptr_val)->data);
			break;

		default:
			kk_runtime_error("=: Cannot compare %s.", type_strs[((kk_gcobj *)a.ptr_val)->type]);
		}
  
		break;
	
	case kk_type_float:
		if (b.type != kk_type_float)
			kk_runtime_error("=: Cannot compare float to %s.", type_strs[b.type]);

		res = a.float_val == b.float_val;
		break;

	default:
		kk_runtime_error("=: Cannot compare %s.", type_strs[a.type]);
	}

	kk_gcobj_dec(&a);
	kk_gcobj_dec(&b);

	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = res }, 0);
}

void kk_BUILTIN___PLUS__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	switch (a.type) {
	case kk_type_gcobj:
		switch(((kk_gcobj *)a.ptr_val)->type) {
		case kk_type_string:
			if (((kk_gcobj *)b.ptr_val)->type != kk_type_string)
				kk_runtime_error("+: Cannot add string to %s.", type_strs[((kk_gcobj *)b.ptr_val)->type]);
			
			kk_gcobj *aobj = (kk_gcobj *)a.ptr_val;
			kk_gcobj *bobj = (kk_gcobj *)b.ptr_val;

			aobj->data = realloc(
				aobj->data,
				strlen((char *)aobj->data) + strlen((char *)bobj->data));

			kk_list_push_front(&the_stack, a, 0);
			kk_gcobj_dec(&b);
			break;

		default:
			kk_runtime_error("+: Cannot add %s.", type_strs[((kk_gcobj *)a.ptr_val)->type]);

		}
  
		break;
	
	case kk_type_float:
		if (b.type != kk_type_float)
			kk_runtime_error("+: Cannot add float to %s.", type_strs[b.type]);

		a.float_val += b.float_val;

		kk_list_push_front(&the_stack, a, 0);
		break;

	default:
		kk_runtime_error("+: Cannot add %s.", type_strs[a.type]);

	}
}

void kk_BUILTIN___MINUS__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		b.float_val -= a.float_val;
		kk_list_push_front(&the_stack, b, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot subtract %s from %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___MUL__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		b.float_val *= a.float_val;
		kk_list_push_front(&the_stack, b, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___DIV__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		if (a.float_val == 0)
			kk_runtime_error("Division by zero.");

		b.float_val /= a.float_val;
		kk_list_push_front(&the_stack, b, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___MOD__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		if (a.float_val == 0)
			kk_runtime_error("Division by zero.");

		b.float_val = (int)b.float_val % (int)a.float_val;
		kk_list_push_front(&the_stack, b, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___SMALLER__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = b.float_val < a.float_val }, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___BIGGER__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = b.float_val > a.float_val }, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___SMALLER____EQUAL__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = b.float_val <= a.float_val }, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___BIGGER____EQUAL__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = b.float_val >= a.float_val }, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___NOT____EQUAL__(void) {
	kk_cell a = kk_list_popget(&the_stack);
	kk_cell b = kk_list_popget(&the_stack);

	if (a.type == b.type && a.type == kk_type_float) {
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_char, .char_val = b.float_val != a.float_val }, 0);
	} else {
		kk_gcobj_dec(&a);
		kk_gcobj_dec(&b);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN_s__BIGGER__(void) {
	printf("<%d> ", kk_list_len(the_stack));

	for (kk_node *node=the_stack; node; node = node->next) {
		kk_cell cell = node->cell;

		switch (cell.type) {
		case kk_type_char:
			printf("%c(%d) ", cell.char_val, cell.char_val);
			break;
		case kk_type_float:
			printf("%f ", cell.float_val);
			break;
		case kk_type_gcobj:
			switch (((kk_gcobj *)cell.ptr_val)->type) {
			case kk_type_string:
				printf("\"%s\" ", (char *)((kk_gcobj *)cell.ptr_val)->data);
				break;
			default:
				printf("{gcobj %x %d} ", ((kk_gcobj *)cell.ptr_val)->data, ((kk_gcobj *)cell.ptr_val)->refs);
			}

			break;

		case kk_type_null:
			printf("null ");
			break;
		}
	}

	printf("\n");
}

void kk_BUILTIN_cons(void) {
	kk_gcobj *obj = malloc(sizeof(kk_gcobj));
	obj->refs = 0; obj->type = kk_type_cons;

	kk_cons *cons = malloc(sizeof(kk_cons));
	cons->car = kk_list_popget(&the_stack);
	cons->cdr = kk_list_popget(&the_stack);

	obj->data = cons;

	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_gcobj, .ptr_val = obj }, 0);
}

void USER_WORD_foo() {
	kk_line = 0 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 1 ;
	kk_BUILTIN___MINUS__();

	kk_line = 1 ;
	return;
}


int main() {
	kk_line = 0 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 0 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 2 }, 0 );

	kk_line = 0 ;
	{
		kk_gcobj *tmp = malloc(sizeof(kk_gcobj));
		if (!tmp) kk_runtime_error("Could not allocate a gc object.");
		tmp->type = kk_type_string; tmp->refs = 0; 
		tmp->data = malloc(15 );
		if (!tmp->data) kk_runtime_error("Could not allocate a string.");
		((char *)tmp->data)[14 ] = 0;
		strcpy(tmp->data, "this is a test");
		kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_gcobj, .ptr_val = tmp }, 0);
	}

	kk_line = 0 ;
	kk_BUILTIN_cons();

	kk_line = 0 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 0 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 0 ;
	kk_BUILTIN_s__BIGGER__();


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
