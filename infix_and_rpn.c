#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char* source;
    size_t position;
} Context;

// ============================================================================
// Lexer ======================================================================
// ============================================================================
typedef enum {
    TokenType_Number,
    TokenType_Add,
    TokenType_Sub,
    TokenType_Mul,
    TokenType_Div,
    TokenType_Newline,
    TokenType_Eof,
} TokenType;

typedef struct {
    TokenType type;
    long value;
} Token;

Token tok_next(Context* context) {
    char current_char = context->source[context->position];
    // skip whitespace
    while (current_char == ' ') { context->position += 1; }

    // get number
    if ('0' <= current_char && current_char <= '9') {
	context->position += 1;
	return (Token){ TokenType_Number };
    }

    context->position += 1;
    switch (current_char) {
	case '\0':
	case EOF: return (Token){ TokenType_Eof };
	case '\n': return (Token){ TokenType_Newline };
	case '+': return (Token){ TokenType_Add };
	case '-': return (Token){ TokenType_Sub };
	case '*': return (Token){ TokenType_Mul };
	case '/': return (Token){ TokenType_Div };
	default:
	    fprintf(stderr, "Lexing Error: Unknown character: %d\n", current_char);
	    exit(0);
    }
}


// ============================================================================
// Parser =====================================================================
// ============================================================================
typedef struct Expr Expr;

typedef enum {
    OpType_Add,
    OpType_Sub,
    OpType_Mul,
    OpType_Div,
} OpType;

typedef enum {
    OpPrec_Term,
    OpPrec_Factor,
} OpPrec;

typedef enum {
    ExprType_Infix,
    ExprType_Rpn,
    ExprType_Number,
} ExprType;

typedef struct {
    ExprType type;
    Expr* left;
    Expr* right;
} ExprOp;

typedef struct {
    long value;
} ExprNum;

typedef struct Expr {
    union {
	ExprOp operator;
	ExprNum number;
    };
} Expr;


ExprNum* parse_number(Context* context) {
    return NULL;
}

Expr* parse_rpn(Context* context) {
    return NULL;
}

Expr* parse_expression_prec(Context* context, int precedence) {
    return NULL;
}

void parse_expression(Context* context) {
    // look at top two tokens to determine notation
    parse_expression_prec(context, 0);
}

int main(void) {
    Context context;

    // init
    context.source = malloc(sizeof(char) * 100);

    FILE* file = fopen("infix_and_rpn.txt", "r");

    fgets(context.source, 100, file);
    printf("%s", context.source);

    // parse
    parse_expression(&context);

    // deinit
    free(context.source);

    return EXIT_SUCCESS;
}
