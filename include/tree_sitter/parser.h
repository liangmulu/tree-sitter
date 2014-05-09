#ifndef TREE_SITTER_PARSER_H_
#define TREE_SITTER_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

//#define TS_DEBUG_PARSE
//#define TS_DEBUG_LEX

#include "tree_sitter/runtime.h"
#include "tree_sitter/parser/lexer.h"
#include "tree_sitter/parser/stack.h"
#include "tree_sitter/parser/lr_parser.h"

#define SYMBOL_NAMES \
static const char *ts_symbol_names[]

SYMBOL_NAMES;

#define HIDDEN_SYMBOLS \
static const int hidden_symbol_flags[SYMBOL_COUNT]

#define UBIQUITOUS_SYMBOLS \
static const int ubiquitous_symbol_flags[SYMBOL_COUNT]

#define LEX_STATES \
static ts_state_id ts_lex_states[STATE_COUNT]

#define LEX_FN() \
static ts_tree * ts_lex(ts_lexer *lexer, ts_state_id lex_state)

#define START_LEXER() \
char lookahead; \
next_state: \
lookahead = ts_lexer_lookahead_char(lexer); \
DEBUG_LEX("CHAR '%c'", lookahead);

#define START_TOKEN() \
ts_lexer_start_token(lexer);

#define ADVANCE(state_index) \
{ DEBUG_LEX("ADVANCE %d", state_index); ts_lexer_advance(lexer); lex_state = state_index; goto next_state; }

#define ACCEPT_TOKEN(symbol) \
{ DEBUG_LEX("TOKEN %s", ts_symbol_names[symbol]); return ts_lexer_build_node(lexer, symbol); }

#define LEX_ERROR() \
{ return ts_lexer_build_node(lexer, ts_builtin_sym_error); }

#define LEX_PANIC() \
{ DEBUG_LEX("LEX ERROR: unexpected state %d", lex_state); return NULL; }

#define PARSE_TABLE \
static const ts_parse_action ts_parse_actions[STATE_COUNT][SYMBOL_COUNT]

#define SHIFT(to_state_value) \
{ .type = ts_parse_action_type_shift, .data = { .to_state = to_state_value } }

#define REDUCE(symbol_val, child_count_val) \
{ .type = ts_parse_action_type_reduce, .data = { .symbol = symbol_val, .child_count = child_count_val } }

#define ACCEPT_INPUT() \
{ .type = ts_parse_action_type_accept }

#ifdef TS_DEBUG_LEX
#include <stdio.h>
#define DEBUG_LEX(...) fprintf(stderr, "\n" __VA_ARGS__)
#else
#define DEBUG_LEX(...)
#endif

#ifdef TS_DEBUG_PARSE
#include <stdio.h>
#define DEBUG_PARSE(...) fprintf(stderr, "\n" __VA_ARGS__)
#else
#define DEBUG_PARSE(...)
#endif

static const ts_tree *
ts_parse(void *data, ts_input input, ts_input_edit *edit) {
    ts_lr_parser *parser = (ts_lr_parser *)data;
    ts_lr_parser_initialize(parser, input, edit);

    int done = 0;
    while (!done) {
        ts_parse_action action = ts_lr_parser_next_action(parser);
        DEBUG_PARSE("LOOKAHEAD %s", ts_symbol_names[ts_tree_symbol(parser->lookahead)]);
        switch (action.type) {
            case ts_parse_action_type_shift:
                DEBUG_PARSE("SHIFT %d", action.data.to_state);
                ts_lr_parser_shift(parser, action.data.to_state);
                break;
            case ts_parse_action_type_reduce:
                DEBUG_PARSE("REDUCE %s %d", ts_symbol_names[action.data.symbol], action.data.child_count);
                ts_lr_parser_reduce(parser, action.data.symbol, action.data.child_count);
                break;
            case ts_parse_action_type_accept:
                DEBUG_PARSE("ACCEPT");
                done = 1;
                break;
            case ts_parse_action_type_error:
                DEBUG_PARSE("ERROR");
                done = !ts_lr_parser_handle_error(parser);
                break;
        }
    }

    return ts_lr_parser_tree_root(parser);
}

#define EXPORT_PARSER(constructor_name) \
ts_parser constructor_name() { \
    return (ts_parser) { \
        .parse_fn = ts_parse, \
        .free_fn = NULL, \
        .symbol_names = ts_symbol_names, \
        .data = ts_lr_parser_make( \
            SYMBOL_COUNT, \
            (const ts_parse_action *)ts_parse_actions, \
            ts_lex_states, \
            ts_lex, \
            hidden_symbol_flags, \
            ubiquitous_symbol_flags \
        ), \
    }; \
}

#ifdef __cplusplus
}
#endif

#endif  // TREE_SITTER_PARSER_H_
