#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

// If we are compiling on Windows compile these functions
#ifdef _WIN32
#include <string.h>

static chat buffer[2048];

// Fale readlione function
char* readline(char* prompt)
{
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

// Fale add_history function
void add_history(char* unused) {}

// Otherwise indlude the editline headers
#else 
#include <editline/readline.h>
#include <histedit.h>
#endif

// Create enumeration of possible aval types
enum { AVAL_NUM, AVAL_ERR };

// Create enumaration of possible error types
enum { AERR_DIV_ZERO, AERR_BAD_OP, AERR_BAD_NUM };

typedef struct {
	int type;
	long num;
	int err;
} aval;

aval aval_num(long x)
{
	aval v;
	v.type = AVAL_NUM;
	v.num = x;
	return v;
}

aval aval_err(int x)
{
	aval v;
	v.type = AVAL_ERR;
	v.err = x;
	return v;
}

void aval_print(aval v)
{
	switch (v.type) {
		// In the case the type a number, print it
		// Then break out of the switch
		case AVAL_NUM:
			printf("%li", v.num);
			break;
		// In the case the type is an error
		case AVAL_ERR:
			// Check what type of error it is and print
			if (v.err == AERR_DIV_ZERO) {
				printf("Error: Division by zero!");
			}
			if (v.err == AERR_BAD_OP) {
				printf("Error: Invalid operator!");
			}
			if (v.err == AERR_BAD_NUM) {
				printf("Error: Invalid number");
			}
			break;
	}
}

void aval_println(aval v)
{
	aval_print(v);
	putchar('\n');
}

long min(long x, long y)
{
	if (x > y) 	return y;
	else 		return x;
}

long max(long x, long y)
{
	if (x > y) 	return x;
	else 		return y;
}

aval eval_op(aval x, char* op, aval y) {
	// If either value is an error, return it
	if (x.type == AVAL_ERR) { return x; }
	if (y.type == AVAL_ERR) { return y; }

	// Otherwise do maths on the number values
	if (strcmp(op, "+") == 0) { return aval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return aval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return aval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) { 
		return y.num == 0
			? aval_err(AERR_DIV_ZERO)
			: aval_num(x.num / y.num);
	}
	

	if (strcmp(op, "%") == 0) { return aval_num(x.num % y.num); }
	if (strcmp(op, "^") == 0) { return aval_num(pow(x.num, y.num)); }
	if (strcmp(op, "min") == 0) { return aval_num(min(x.num, y.num)); }
	if (strcmp(op, "max") == 0) { return aval_num(max(x.num, y.num)); }
	return aval_err(AERR_BAD_OP);
}

aval eval(mpc_ast_t* t)
{
	// If tagged as number, return it directly
	if (strstr(t->tag, "number")) {
		// Check is there is some error in conversion
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? aval_num(x) : aval_err(AERR_BAD_NUM);
	}


	// The operator is always second child
	char* op = t->children[1]->contents;

	// We store the third child in x
	aval x = eval(t->children[2]);

	// Iterate the remaining children and combining
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	if (strcmp(op, "-") == 0) { x = aval_num(x.num * -1); }

	return x;
}

int main(int argc, char** argv)
{
	// Create some parsers
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* ArcLisp = mpc_new("arclisp");

	// Define them with the following language
	mpca_lang(MPCA_LANG_DEFAULT,
		"													\
			number : /-?[0-9]+/ ;							\
			operator : '+' | '-' | '*' | '/' |				\
					   '%' | '^' | \"min\" | \"max\" ;		\
			expr : <number> | '(' <operator> <expr>+ ')' ;	\
			arclisp : /^/ <operator> <expr>+ /$/ ;			\
		",
		Number, Operator, Expr, ArcLisp);

	puts("ArcLisp Version 0.0.0.0.3");
	puts("This is FREE software");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		// Now in either case readline will be corerctly defined
		char* input = readline("arclisp> ");
		add_history(input);

		// Attempt to parse the user input
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, ArcLisp, &r)) {
			// On success print the AST
			aval result = eval(r.output);
			aval_println(result);
			mpc_ast_delete(r.output);
		} else {
			// Otherwise print the error
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	// Undefine and delete our parsers
	mpc_cleanup(4, Number, Operator, Expr, ArcLisp);

	return 0;
}