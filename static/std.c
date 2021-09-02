#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define GCOBJ(a) ((kk_gcobj *)(a).ptr_val)
#define CONS(a) ((kk_cons *)a->ptr_val)
#define PUSH(t, f, v) (*++stack = (kk_cell){ .type = kk_type_##t, . f##_val = v })
#define POP() (*stack--)
#define STACKLEN() (stack - stack_storage)

typedef double kk_float;
typedef char kk_char;
typedef char kk_bool;

typedef enum {
	kk_type_null,
	kk_type_float,
	kk_type_char,
	kk_type_gcobj,

	kk_type_string,
	kk_type_cons,
	kk_type_array,
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
	short refs;
	kk_type type;
	void *ptr_val;
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

kk_cell stack_storage[1<<16] = {0};
kk_cell *stack = stack_storage;
kk_cell tmp_cell = {0};
kk_bool tmp_res = 0;

int kk_line = 0;
char kk_file[2048] = {0};
const char *type_strs[] = {
	"null", "float", "gc object", "char", "string", "cons", "array"
};

void kk_runtime_error(char *msg, ...) {
	fprintf(stderr, "\x1b[1m(%s: %d): \x1b[31mruntime error: \x1b[0m", kk_file, kk_line);

	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);

	fprintf(stderr, "\n");
	exit(1);
}

void kk_gcobj_inc(kk_cell *o) {
	if (o->type == kk_type_gcobj) {
		((kk_gcobj *)(o->ptr_val))->refs++;
	}
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
		free(o->ptr_val);
		break;
	case kk_type_cons:
		kk_gcobj_dec(&CONS(o)->car);
		kk_gcobj_dec(&CONS(o)->cdr);
		free(o->ptr_val);
		break;
	case kk_type_array:
		free(((kk_array *)o->ptr_val)->data);
		free(o->ptr_val);
		break;
	}

	free(o);
}

void kk_gcobj_dec(kk_cell *c) {
	if (c->type != kk_type_gcobj)
		return;

	kk_gcobj *o = ((kk_gcobj *)c->ptr_val);

	o->refs--;

	if (o->refs <= 0)
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

void kk_cell_put(kk_cell cell, int debug) {
	switch (kk_cell_abstype(cell)) {
	case kk_type_char:
		if (debug)
			printf("#%c/'%d", cell.char_val, cell.char_val);
		else
			printf("%c", cell.char_val);
		break;
	case kk_type_float:
		printf("%f", cell.float_val);
		break;
	case kk_type_string:
		if (debug)
			printf("\"%s\"", (char *)(GCOBJ(cell)->ptr_val));
		else
			printf("%s", (char *)(GCOBJ(cell)->ptr_val));
		break;
	case kk_type_cons:
		printf("( ");
		kk_cell_put(CONS(GCOBJ(cell))->car, debug);
		printf(" . ");
		kk_cell_put(CONS(GCOBJ(cell))->cdr, debug);
		printf(" )");
		break;
	case kk_type_array:
		printf("[ ");

		kk_array *arr = ((kk_array *)GCOBJ(cell)->ptr_val);

		for (int i=0; i < arr->len; i++) {
			kk_cell_put(arr->data[i], debug);
			printf(" ");
		}

		printf("]");
		break;
	case kk_type_null:
		printf("null");
		break;
	}
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
		//kk_node_free(to_free);
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
	//kk_node_free(to_free);
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
		return ((kk_gcobj *)cell.ptr_val)->ptr_val > 0;
	default:
		return 0;
	}

	return 0;
}

void kk_BUILTIN___EQUAL__(void) {
	kk_cell a = POP();
	kk_cell b = POP();

	kk_bool res = 0;
	switch (a.type) {
	case kk_type_null:
		res = b.type == kk_type_null;
		break;

	case kk_type_gcobj:
		switch(((kk_gcobj *)a.ptr_val)->type) {
		case kk_type_string:
			if (kk_cell_abstype(b) != kk_type_string)
				kk_runtime_error(
					"Cannot compare string to %s.",
					type_strs[kk_cell_abstype(b)]);
			
			res = !strcmp(
				(char *)((kk_gcobj *)a.ptr_val)->ptr_val,
				(char *)((kk_gcobj *)b.ptr_val)->ptr_val);
			break;

		default:
			kk_runtime_error("Cannot compare %s.", type_strs[((kk_gcobj *)a.ptr_val)->type]);
		}
  
		break;
	
	case kk_type_float:
		if (b.type != kk_type_float)
			kk_runtime_error("Cannot compare float to %s.", type_strs[kk_cell_abstype(b)]);

		res = a.float_val == b.float_val;
		break;

	default:
		kk_runtime_error("Cannot compare %s.", type_strs[kk_cell_abstype(a)]);
	}

	kk_gcobj_dec(&a);
	kk_gcobj_dec(&b);

	PUSH(char, char, res);
}

void kk_BUILTIN___DIV____EQUAL__(void) {
	kk_BUILTIN___EQUAL__();
	stack->char_val = !stack->char_val;
}

void kk_BUILTIN___PLUS__(void) {
	kk_cell a = POP();

	switch (a.type) {
	case kk_type_gcobj:;
		kk_cell b = POP();

		switch(((kk_gcobj *)a.ptr_val)->type) {
		case kk_type_string:
			if (((kk_gcobj *)b.ptr_val)->type != kk_type_string)
				kk_runtime_error("Cannot add string to %s.", type_strs[((kk_gcobj *)b.ptr_val)->type]);
			
			kk_gcobj *aobj = (kk_gcobj *)a.ptr_val;
			kk_gcobj *bobj = (kk_gcobj *)b.ptr_val;

			bobj->ptr_val = realloc(
				bobj->ptr_val,
				strlen((char *)aobj->ptr_val) + strlen((char *)bobj->ptr_val) + 1);

			strcat((char *)bobj->ptr_val, (char *)aobj->ptr_val);

			*++stack = b;
			kk_gcobj_dec(&a);
			kk_gcobj_dec(&b);
			break;

		case kk_type_array:
			if (kk_cell_abstype(b) != kk_type_array)
				kk_runtime_error("Cannot add array to %s.", type_strs[kk_cell_abstype(b)]);

			kk_array *aa = (kk_array *)(((kk_gcobj *)(a.ptr_val))->ptr_val);
			kk_array *ba = (kk_array *)(GCOBJ(b)->ptr_val);

			ba->len += aa->len;

			ba->data = realloc(ba->data, ba->len * sizeof(kk_cell));
			memcpy(ba->data + (ba->len - aa->len),
				aa->data, aa->len * sizeof(kk_cell));

			*++stack = b;
			kk_gcobj_dec(&a);
			kk_gcobj_dec(&b);

			break;

		default:
			kk_runtime_error("Cannot add %s.", type_strs[((kk_gcobj *)a.ptr_val)->type]);

		}
  
		break;
	
	case kk_type_float:
		if (!stack)
			kk_runtime_error("The stack is empty.");
		if (stack->type != kk_type_float)
			kk_runtime_error("Cannot add float to %s.", type_strs[b.type]);

		stack->float_val += a.float_val;
		break;

	default:
		kk_runtime_error("Cannot add %s.", type_strs[a.type]);

	}
}

void kk_BUILTIN___MINUS__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == kk_type_float && stack->type == kk_type_float) {
		stack->float_val -= a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot subtract %s from %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___MUL__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		stack->float_val *= a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___DIV__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		if (a.float_val == 0)
			kk_runtime_error("Division by zero.");

		stack->float_val /= a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___MOD__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		if (a.float_val == 0)
			kk_runtime_error("Division by zero.");

		stack->float_val = (int)stack->float_val % (int)a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___SMALLER__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		stack->float_val = stack->float_val < a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___BIGGER__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		stack->float_val = stack->float_val > a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___SMALLER____EQUAL__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		stack->float_val = stack->float_val <= a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN___BIGGER____EQUAL__(void) {
	kk_cell a = POP();
	if (!stack)
		kk_runtime_error("The stack is empty.");

	if (a.type == stack->type && a.type == kk_type_float) {
		stack->float_val = stack->float_val >= a.float_val;
	} else {
		kk_gcobj_dec(&a);

		kk_runtime_error("Cannot multiply %s with %s.", type_strs[a.type]);
	}
}

void kk_BUILTIN_s__BIGGER__(void) {
	printf("<%d> ", STACKLEN() + 1);

	for (kk_cell *p = stack_storage + 1; p <= stack; p++) {
		kk_cell_put(*p, 1);
		printf(" ");
	}

	printf("\n");
}

void kk_BUILTIN_cons(void) {
	if (STACKLEN() < 2)
		kk_runtime_error("Not enough values on the stack to cons.");

	kk_gcobj *obj = malloc(sizeof(kk_gcobj));
	obj->refs = 1; obj->type = kk_type_cons;

	kk_cons *cons = malloc(sizeof(kk_cons));
	cons->cdr = POP();
	cons->car = POP();

	obj->ptr_val = cons;

	PUSH(gcobj, ptr, obj);
}

void kk_BUILTIN_car(void) {
	kk_cell cell = *stack;
	
	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot get car of a %s.", type_strs[type]);

	*++stack = CONS(GCOBJ(cell))->car;
	kk_gcobj_inc(stack-1);
}

void kk_BUILTIN_cdr(void) {
	kk_cell cell = *stack;

	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot get cdr of a %s.", type_strs[type]);

	*++stack = CONS(GCOBJ(cell))->cdr;
	kk_gcobj_inc(stack-1);
}

void kk_BUILTIN_uncons(void) {
	kk_cell cell = POP();
	
	kk_type type = kk_cell_abstype(cell);
	if (type != kk_type_cons)
		kk_runtime_error("Cannot uncons a %s.", type_strs[type]);

	*++stack = CONS(GCOBJ(cell))->car;
	//kk_gcobj_inc(stack-1);
	*++stack = CONS(GCOBJ(cell))->cdr;
	//kk_gcobj_inc(stack-1);

	kk_gcobj_dec(&cell);
}

void kk_BUILTIN_dup(void) {
	if (stack == stack_storage)
		kk_runtime_error("Trying to dup null stack.");

	stack[1] = *stack;
	kk_gcobj_inc(++stack);
}

void kk_BUILTIN_swap(void) {
	if (STACKLEN() < 2)
		kk_runtime_error("Cannot swap on a stack shorter than 2.");

	tmp_cell = *stack;
	*stack = stack[-1];
	stack[-1] = tmp_cell;
}

void kk_BUILTIN_rot(void) {
	kk_cell first = POP();
	kk_cell second = POP();
	kk_cell third = POP();

	*++stack = second;
	*++stack = first;
	*++stack = third;
}

void kk_BUILTIN_tuck(void) {
	if (STACKLEN() < 2)
		kk_runtime_error("Cannot tuck a stack shorter than 2.");

	stack[1] = *stack;
	*stack = stack[-1];
	stack[-1] = stack[1];
}

void kk_BUILTIN_over(void) {
	if (STACKLEN() < 2)
		kk_runtime_error("Cannot over a stack shorter than 2.");

	stack[1] = stack[-1];
	stack++;
}

void kk_BUILTIN_mka(void) {
	kk_cell lencell = POP();

	if (lencell.type != kk_type_float)
		kk_runtime_error("Cannot use %s as length.", type_strs[lencell.type]);

	kk_array *arr = malloc(sizeof(kk_array));
	arr->data = calloc(sizeof(kk_cell), lencell.float_val);
	arr->len = lencell.float_val;

	kk_gcobj *o = malloc(sizeof(kk_gcobj));
	o->refs = 1; o->type = kk_type_array;
	o->ptr_val = arr;
	PUSH(gcobj, ptr, arr);
}

void kk_BUILTIN_get(void) {
	kk_cell icell = POP();

	if (icell.type != kk_type_float)
		kk_runtime_error("Cannot use %s as an index.", type_strs[icell.type]);

	int index = icell.float_val;

	switch (kk_cell_abstype(*stack)) {
	case kk_type_array:;
		kk_array *arr = (kk_array *)GCOBJ(*stack)->ptr_val;

		if (index > arr->len)
			kk_runtime_error("Index %d out of range %d.", index, arr->len);

		*++stack = arr->data[index];
		break;

	case kk_type_string:;
		char *s = (char *)GCOBJ(*stack)->ptr_val;

		int len = strlen(s);
		if (index > len)
			kk_runtime_error("Index %d out of range %d.", index, len);

		PUSH(char, char, s[index]);
		break;

	case kk_type_cons:;
		kk_cons *list = (kk_cons *)GCOBJ(*stack)->ptr_val;

		for (int i=0; i < index && list; i++) {
			kk_type type = list->cdr.type;

			if (type == kk_type_null)
				kk_runtime_error("Index %d out of range %d.", index, i);

			if (type != kk_type_cons)
				kk_runtime_error("Trying to index not a list.");

			list = (kk_cons *)(list->cdr.ptr_val);
		}

		*++stack = list->car;
		break;

	default:
		kk_runtime_error("Cannot iterate over %s.", type_strs[stack->type]);

	}
}

void kk_BUILTIN_set(void) {
	kk_cell val = POP();
	kk_cell icell = POP();

	if (icell.type != kk_type_float)
		kk_runtime_error("Cannot use %s as an index.", type_strs[icell.type]);

	int index = icell.float_val;

	switch (kk_cell_abstype(*stack)) {
	case kk_type_array:;
		kk_array *arr = (kk_array *)GCOBJ(*stack)->ptr_val;

		if (index > arr->len)
			kk_runtime_error("Index %d out of range %d.", index, arr->len);

		kk_gcobj_dec(&arr->data[index]);
		arr->data[index] = val;
		break;

	case kk_type_string:;
		if (val.type != kk_type_char)
			kk_runtime_error("Trying to set string value with %s.", type_strs[val.type]);

		char *s = (char *)GCOBJ(*stack)->ptr_val;

		int len = strlen(s);
		if (index > len)
			kk_runtime_error("Index %d out of range %d.", index, len);

		PUSH(char, char, s[index]);
		break;

	case kk_type_cons:;
		kk_cons *list = (kk_cons *)GCOBJ(*stack)->ptr_val;

		for (int i=0; i < index && list; i++) {
			kk_type type = list->cdr.type;

			if (type == kk_type_null)
				kk_runtime_error("Index %d out of range %d.", index, i);

			if (type != kk_type_cons)
				kk_runtime_error("Trying to index not a list.");

			list = (kk_cons *)(list->cdr.ptr_val);
		}

		kk_gcobj_dec(&list->car);
		list->car = val;
		break;

	default:
		kk_runtime_error("Cannot iterate over %s.", type_strs[stack->type]);

	}
}

void kk_BUILTIN_put(void) {
	kk_cell cell = POP();

	kk_cell_put(cell, 0);
	printf("\n");

	kk_gcobj_dec(&cell);
}

void kk_BUILTIN_len(void) {
	int res;

	switch (kk_cell_abstype(*stack)) {
	case kk_type_string:
		res = strlen((char *)GCOBJ(*stack)->ptr_val);
		break;
	case kk_type_array:
		res = ((kk_array *)GCOBJ(*stack)->ptr_val)->len;
		break;
	case kk_type_cons:
		res = 0;

		for (
			kk_cons *node = (kk_cons *)GCOBJ(*stack)->ptr_val;;
			node = (kk_cons *)GCOBJ(node->cdr)->ptr_val) {
				res++;

				if (node->cdr.type == kk_type_null)
					break;

				if (kk_cell_abstype(node->cdr) != kk_type_cons)
					kk_runtime_error("Cannot get length of non list cons.");
			}

		break;
	default:
		kk_runtime_error("Cannot get length of %s.", type_strs[stack->type]);
	}

	PUSH(float, float, res);
}

void kk_BUILTIN_nip(void) {
	kk_BUILTIN_swap();
	tmp_cell = POP();
	kk_gcobj_dec(&tmp_cell);
}

void kk_BUILTIN_num(void) {
	kk_cell cell = POP();

	kk_float res;
	switch (kk_cell_abstype(cell)) {
	case kk_type_string:
		res = atof((char *)GCOBJ(cell)->ptr_val);
		break;
	case kk_type_char:
		res = cell.char_val;
		break;
	case kk_type_null:
		res = 0;
		break;
	case kk_type_float:
		res = cell.float_val; // it should not error on float.
		break;
	default:
		kk_runtime_error("Cannot convert %s to num.", type_strs[kk_cell_abstype(cell)]);
	}

	kk_gcobj_dec(&cell);
	PUSH(float, float, res);
}

void kk_BUILTIN_char(void) {
	kk_cell cell = POP();

	kk_char res;
	switch (cell.type) {
	case kk_type_char:
		res = cell.char_val;
		break;
	case kk_type_null:
		res = 0;
		break;
	case kk_type_float:
		res = cell.float_val;
		break;
	default:
		kk_runtime_error("Cannot convert %s to char.", type_strs[kk_cell_abstype(cell)]);
	}

	kk_gcobj_dec(&cell);
	PUSH(char, char, res);
}

void kk_BUILTIN_stoa(void) {
	int len = 0;
	for (kk_cell *p=stack; p != stack_storage && p->type; p--)
		len++;

	kk_array *arr = malloc(sizeof(kk_array));
	arr->len = len;
	arr->data = malloc(sizeof(kk_cell) * len);

	for (int i=0; i < len; i++)
		arr->data[i] = POP();

	POP();
	kk_gcobj *o = malloc(sizeof(kk_gcobj));
	o->type = kk_type_array;
	o->refs = 1;
	o->ptr_val = arr;

	PUSH(gcobj, ptr, o);
}

void kk_BUILTIN_atos(void) {
	kk_cell cell = POP();
	if (kk_cell_abstype(cell) != kk_type_array)
		kk_runtime_error("Cannot atos a %s.", type_strs[kk_cell_abstype(cell)]);

	kk_array *arr = (kk_array *)cell.ptr_val;
	for (int i=arr->len-1; i >= 0; i--) {
		*++stack = arr->data[i];
	}

	kk_gcobj_dec(&cell);
}

void kk_BUILTIN_l__BIGGER__(void) {
	kk_cell val = POP();
	kk_type type = stack->type;
	if (type != kk_type_cons)
		kk_runtime_error("Cannot push to %s.", type_strs[type]);

	kk_cons *cons = malloc(sizeof(kk_cons));
	cons->car = val;

	cons->cdr = *stack;
	kk_gcobj *o = malloc(sizeof(kk_gcobj));
	o->type = kk_type_cons;
	o->refs = 1;

	stack->ptr_val = o;
}

void kk_BUILTIN_read(void) {
	char *buf = malloc(256);
	int len = 0;
	int cap = 256;

	char c;
	while ((c = getchar()) != '\n') {
		buf[len++] = c;
		if (len >= cap) {
			buf = realloc(buf, cap + 256);
			cap += 256;
		}
	}
	buf[len] = 0;

	kk_gcobj *o = malloc(sizeof(kk_gcobj));
	o->type = kk_type_string;
	o->refs = 1;
	o->ptr_val = buf;
	PUSH(gcobj, ptr, o);
}

void kk_BUILTIN_abs(void) {
	if (stack->type != kk_type_float)
		kk_runtime_error("Cannot abs a %s.", type_strs[stack->type]);

	if (stack->float_val < 0)
		stack->float_val *= -1;
}

void kk_BUILTIN_and(void) {
	kk_cell a = POP();
	kk_cell b = POP();	

	kk_bool res = kk_is_true(a) && kk_is_true(b);
	PUSH(char, char, res);
	kk_gcobj_dec(&a);
	kk_gcobj_dec(&b);
}

