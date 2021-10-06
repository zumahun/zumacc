#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// token kind
typedef enum {
	TK_RESERVED, 	//Symbol
	TK_NUM,		//Number
	TK_EOF,		//END
} TokenKind;

typedef struct Token Token;

// token types
struct Token {
	TokenKind kind; 	//token kind
	Token *next;		//next token
	int val;		//if kind is TK_NUM, val
	char *str;		// Token string
};

char *user_input;
Token *token;

void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, "");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

bool consume(char op)
{
	if(token->kind != TK_RESERVED || token->str[0] != op)
		return false;
	token = token->next;
	return true;
}

void expect(char op)
{
	if (token->kind != TK_RESERVED || token->str[0] != op)
		error_at(token->str, "expected '%c'", op);
	token = token->next;
}
int expect_number()
{
	if (token->kind != TK_NUM)
		error_at(token->str,"expected a number");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof()
{
	return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind, Token *cur, char *str)
{
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

Token *tokenize() 
{
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) 
	{
		// space skip
		if (isspace(*p))
		{
			p++;
			continue;
		}

		if (strchr("+-*/()", *p))
		{
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if (isdigit(*p))
		{
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(p, "invalid token");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

typedef enum
{
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node
{
	NodeKind kind; // Node kind
	Node *lhs;	// Left-hand side
	Node *rhs;	// Right-hand side
	int val;	// Used if kind == ND_NUM
};

Node *new_node(NodeKind kind)
{
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(NodeKind  kind, Node *lhs, Node *rhs) 
{
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_num(int val)
{
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

Node *expr();
Node *mul();
Node *primary();

// expr = mul ("+" mul | "-" mul)*
Node *expr()
{
	Node *node = mul();

	for (;;) 
	{
		if (consume('+'))
			node = new_binary(ND_ADD, node, mul());
		else if (consume('-'))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

// mul = primary ("*" primary | "/" primary)*
Node *mul()
{
	Node *node = primary();

	for (;;)
	{
		if (consume('*'))
			node = new_binary(ND_MUL, node, primary());
		else if (consume('/'))
			node = new_binary(ND_DIV, node, primary());
		else
			return node;
	}
}

// primary = "(" expr ")" | num
Node *primary() 
{
	if (consume('('))
	{
		Node *node = expr();
		expect(')');
		return node;
	}

	return new_num(expect_number());
}

void gen(Node *node)
{
	if (node->kind == ND_NUM)
	{
		printf("	push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("	pop rdi\n");
	printf("	pop rax\n");

	switch (node->kind)
	{
		case ND_ADD:
			printf("	add rax, rdi\n");
			break;
		case ND_SUB:
			printf("	sub rax, rdi\n");
			break;
		case ND_MUL:
			printf("	imul rax, rdi\n");
			break;
		case ND_DIV:
			printf("	cqo\n");
			printf("	idiv rdi\n");
			break;
	}

	printf("	push rax\n");
}

int main(int argc, char **argv) {
	if (argc != 2)
		error("%s: invalid number of arguments", argv[0]);


	// Tokenize and parse.
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");


	// Traverse the AST to emit assembly
	gen(node);

	printf("	pop rax\n");
	printf("	ret\n");
	return 0;

}

