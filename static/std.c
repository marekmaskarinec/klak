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

void kk_gcobj_dec(kk_cell *c);
void kk_gcobj_free(kk_gcobj *o) {
	switch (o->type) {
	case kk_type_string:
		free(o->data);
		break;
	case kk_type_cons:
		kk_gcobj_dec(&CONS(o)->car);
		kk_gcobj_dec(&CONS(o)->cdr);
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

kk_type kk_cell_abstype(kk_cell cell) {
	if (cell.type == kk_type_gcobj)
		return GCOBJ(cell)->type;
	
	return cell.type;
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

			bobj->data = realloc(
				bobj->data,
				strlen((char *)aobj->data) + strlen((char *)bobj->data) + 1);

			strcat((char *)bobj->data, (char *)aobj->data);

			kk_list_push_front(&the_stack, b, 0);
			kk_gcobj_dec(&a);
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
	cons->cdr = kk_list_popget(&the_stack);
	cons->car = kk_list_popget(&the_stack);

	obj->data = cons;

	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_gcobj, .ptr_val = obj }, 0);
}

void kk_BUILTIN_car(void) {
	kk_cell cell = the_stack->cell;
	
	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot get car of a %s.", type_strs[type]);

	kk_list_push_front(&the_stack, CONS(GCOBJ(cell))->car, 0);
}

void kk_BUILTIN_cdr(void) {
	kk_cell cell = the_stack->cell;

	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot get cdr of a %s.", type_strs[type]);

	kk_list_push_front(&the_stack, CONS(GCOBJ(cell))->cdr, 0);
}

void kk_BUILTIN_uncons(void) {
	kk_cell cell = kk_list_popget(&the_stack);
	
	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot uncons a %s.", type_strs[type]);

	kk_list_push_front(&the_stack, CONS(GCOBJ(cell))->car, 0);
	kk_list_push_front(&the_stack, CONS(GCOBJ(cell))->cdr, 0);

	kk_gcobj_dec(&cell);
}

void kk_BUILTIN_dup(void) {
	if (!the_stack)
		kk_runtime_error("Trying to dup null stack.");

	kk_list_push_front(&the_stack, the_stack->cell, 0);
}

void kk_BUILTIN_swap(void) {
	if (!the_stack || !the_stack->next)
		kk_runtime_error("Cannot swap on a stack shorter than 2.");

	tmp_cell = the_stack->cell;
	the_stack->cell = the_stack->next->cell;
	the_stack->next->cell = tmp_cell;
}

void kk_BUILTIN_rot(void) {
	kk_cell first = kk_list_popget(&the_stack);
	kk_cell second = kk_list_popget(&the_stack);
	kk_cell third = kk_list_popget(&the_stack);

	kk_list_push_front(&the_stack, second, 0);
	kk_gcobj_dec(&second);

	kk_list_push_front(&the_stack, first, 0);
	kk_gcobj_dec(&first);

	kk_list_push_front(&the_stack, third, 0);
	kk_gcobj_dec(&third);
}

void kk_BUILTIN_tuck(void) {
	if (!the_stack || !the_stack->next)
		kk_runtime_error("Cannot tuck a stack shorter than 2.");

	kk_list_push_front(&the_stack, the_stack->cell, 2);
}

void kk_BUILTIN_over(void) {
	if (!the_stack || !the_stack->next)
		kk_runtime_error("Cannot over a stack shorter than 2.");

	kk_list_push_front(&the_stack, the_stack->next->cell, 0);
}


int main() {
	kk_line = 0 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 0 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 0 ;
	kk_BUILTIN_dup();

	kk_line = 0 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 0 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 0 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 0 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 0 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 2 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 2 }, 0 );

	kk_line = 2 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 2 ;
	kk_BUILTIN_swap();

	kk_line = 2 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 2 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 2 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 2 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 2 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 4 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 2 }, 0 );

	kk_line = 4 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 3 }, 0 );

	kk_line = 4 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 4 }, 0 );

	kk_line = 4 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 4 ;
	kk_BUILTIN_rot();

	kk_line = 4 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 4 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 4 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 4 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 4 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 4 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 4 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 6 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 2 }, 0 );

	kk_line = 6 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 3 }, 0 );

	kk_line = 6 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 6 ;
	kk_BUILTIN_tuck();

	kk_line = 6 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 6 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 6 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 6 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 6 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 6 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 6 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 1 }, 0 );

	kk_line = 8 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 2 }, 0 );

	kk_line = 8 ;
	kk_list_push_front(&the_stack, (kk_cell){ .type = kk_type_float, .float_val = 3 }, 0 );

	kk_line = 8 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 8 ;
	kk_BUILTIN_over();

	kk_line = 8 ;
	kk_BUILTIN_s__BIGGER__();

	kk_line = 8 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 8 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 8 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 8 ;
	kk_gcobj_dec(&the_stack->cell);
	kk_list_popn(&the_stack, 1 );

	kk_line = 8 ;
	kk_BUILTIN_s__BIGGER__();


}
