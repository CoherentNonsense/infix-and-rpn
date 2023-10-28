#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESET "\e[0m"
#define ERROR_MSG "\e[1m"
#define ERROR_TEXT "\e[0;31m"
#define ERROR_HINT "\e[1;93m"
#define RESULT "\e[1;34m"

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
    TokenType_Unexpected,
    TokenType_LParen,
    TokenType_RParen,
} TokenType;

typedef struct {
    TokenType type;
    size_t position;
    double value;
} Token;


Token tok_next(Context* context) {
    char current_char = context->source[context->position];
    // skip whitespace
    while (current_char == ' ') { context->position += 1; current_char = context->source[context->position]; }

    Token token = {0};
    token.position = context->position;

    switch (current_char) {
        case '\0':
        case EOF:  token.type = TokenType_Eof; break;
        case '\n': token.type = TokenType_Newline; break;
        case '+':  token.type = TokenType_Add; break;
        case '-':  token.type = TokenType_Sub; break;
        case '*':  token.type = TokenType_Mul; break;
        case '/':  token.type = TokenType_Div; break;
        case '(':  token.type = TokenType_LParen; break;
        case ')':  token.type = TokenType_RParen; break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': case '.':
               token.type = TokenType_Number;
               char* number_source_end;
               token.value = strtof(context->source + context->position, &number_source_end);
               context->position = number_source_end - context->source - 1;
               break;

        default: token.type = TokenType_Unexpected; break;
    }

    context->position += 1;
    return token;
}

Token tok_peek(Context* context) {
    size_t position = context->position;
    Token token = tok_next(context);
    context->position = position;

    return token;
}


// ============================================================================
// Parser =====================================================================
// ============================================================================
typedef enum {
    ParseResultType_Success,
    ParseResultType_Error,
} ParseResultType;

typedef struct {
    ParseResultType type;
    double value;
} ParseResult;


#define TOK_NEXT(token, context) \
    Token token = tok_next(context); \
    do { \
        if (token.type == TokenType_Unexpected) { \
            parse_error(context, "Unexpected character", "Only +, -, *, /, and numbers are allowed", &token); \
            return (ParseResult){ ParseResultType_Error }; \
        } \
    } while(0)

#define TOK_PEEK(token, context) \
    Token token = tok_peek(context); \
    do { \
        if (token.type == TokenType_Unexpected) { \
            parse_error(context, "Unexpected character", "Only +, -, *, /, and numbers are allowed", &token); \
            return (ParseResult){ ParseResultType_Error }; \
        } \
    } while(0)

void parse_error(Context* context, const char* message, const char* hint, const Token* token) {
    fprintf(stderr, ERROR_TEXT "\n[error] " RESET ERROR_MSG "%s\n  " RESET, message);

    for (int i = 0;; i++) {
    if (context->source[i] == '\0') { break; }
        if (i == token->position) { fprintf(stderr, ERROR_HINT); }
        fputc(context->source[i], stderr);
        if (i == token->position) { fprintf(stderr, RESET); }
    }

    fprintf(stderr, "\n  ");

    for (int i = 0; i < context->position + 2; i++) {
    if (i == token->position) { break; }
        fputc(' ', stderr);
    }

    fprintf(stderr, ERROR_HINT "^ %s\n\n" RESET, hint);
}

// expect a number to be next
ParseResult parse_primary(Context* context);
ParseResult parse_expression(Context* context, int precedence);
ParseResult parse_rpn(Context* context, double first) {
    // lazy stack
    double stack[100];
    size_t top = 0;

    stack[0] = first;

    while (1) {
        TOK_PEEK(next, context);

        switch (next.type) {
            case TokenType_LParen:
            case TokenType_Number:
            stack[++top] = parse_primary(context).value; break;
            case TokenType_Add: tok_next(context); stack[top - 1] += stack[top]; top--; break;
            case TokenType_Sub: tok_next(context); stack[top - 1] -= stack[top]; top--; break;
            case TokenType_Mul: tok_next(context); stack[top - 1] *= stack[top]; top--; break;
            case TokenType_Div: tok_next(context); stack[top - 1] /= stack[top]; top--; break;
            default:
            parse_error(context, "RPN stack must be emptied", "Expected number or operator here", &next);
            return (ParseResult){ ParseResultType_Error };
        }

        // RPN is finished when the stack has only one item and the next token is an operator
        if (top == 0) {
            TOK_PEEK(maybe_op, context);
            if (maybe_op.type != TokenType_Number) {
                break;
            }
        }
    }

    return (ParseResult){ParseResultType_Success, stack[0]};
}

ParseResult parse_primary(Context* context) {
    // look at top two tokens to determine notation
    TOK_NEXT(first, context);

    // paren group
    if (first.type == TokenType_LParen) {
        ParseResult inner_result = parse_expression(context, 0);
        if (inner_result.type == ParseResultType_Error) {
            return inner_result;
        }

        // closing paren
        Token rparen = tok_next(context);
        if (rparen.type != TokenType_RParen) {
            parse_error(context,  "Missing parenthesis", "Expected ')' here", &rparen);
            return (ParseResult){ ParseResultType_Error };
        }

        return inner_result;
    }

    // ) doesn't make sense
    if (first.type == TokenType_RParen) {
        parse_error(context, "Unmatched parenthesis", "Did you mean to add this?", &first);
        return (ParseResult){ ParseResultType_Error };
    }

    // first token should be a number
    if (first.type != TokenType_Number) {
        parse_error(context, "Expression must start with a number", "This should be a number", &first);
        return (ParseResult){ ParseResultType_Error };
    }

    // return number
    return (ParseResult){ ParseResultType_Success, first.value };
}

ParseResult parse_expression(Context* context, int precedence) {
    ParseResult left_result = parse_primary(context);
    if (left_result.type == ParseResultType_Error) { return left_result; }

    double left = left_result.value;

    while (1) {
    // get current operator precedence
    TOK_PEEK(op, context);

    // if two numbers in a row, we switchn to rpn
    if (op.type == TokenType_LParen || op.type == TokenType_Number) {
        ParseResult rpn_result = parse_rpn(context, left);
        if (rpn_result.type == ParseResultType_Error) { return rpn_result; }
        left = rpn_result.value;
        break;
    }

    int current_precedence;
    switch (op.type) {
        case TokenType_Add:
        case TokenType_Sub: current_precedence = 10; break;
        case TokenType_Mul:
        case TokenType_Div: current_precedence = 20; break;
        case TokenType_RParen:
        case TokenType_Eof: current_precedence = -1; break;
        default: break;
    }

    if (current_precedence == -1) {
        break;
    }

    if (current_precedence < precedence) { break; }

    // consume operator
    tok_next(context);

    ParseResult right_result = parse_expression(context,  current_precedence + 1);
    if (right_result.type == ParseResultType_Error) { return right_result; }
        switch (op.type) {
            case TokenType_Add: left += right_result.value; break;
            case TokenType_Sub: left -= right_result.value; break;
            case TokenType_Mul: left *= right_result.value; break;
            case TokenType_Div: left /= right_result.value; break;
            default: fprintf(stderr, "something ewnt wrong (2) %d\n", op.type); exit(1);
        }
    }

    return (ParseResult){ ParseResultType_Success, left };
}

ParseResult parse_statement(Context* context) {
    ParseResult result = parse_expression(context, 0);
    if (result.type == ParseResultType_Error) { return result; }
        TOK_PEEK(last, context);
        if (last.type == TokenType_RParen) {
            parse_error(context, "Unmatched parenthesis", "Did you mean to add this?", &last);
        return (ParseResult){ ParseResultType_Error };
    }

    return result;
}

int main(void) {

    // init
    Context context;
    context.source = malloc(sizeof(char) * 100);

    while (1) {
        printf("%% ");
        fgets(context.source, 100, stdin);
        context.source[strcspn(context.source, "\r\n")] = '\0';
    
        context.position = 0;

        ParseResult result = parse_statement(&context);

        if (result.type == ParseResultType_Success) {
            printf("= " RESULT "%lf\n" RESET,  result.value);
        }
    }

    // deinit
    free(context.source);

    return EXIT_SUCCESS;
}
