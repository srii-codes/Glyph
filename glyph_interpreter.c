
#include <stdio.h>
#include <stdlib.h>
#include "glyph_interpreter.h"

/* The Call Stack & Activation Records */
#define MAX_CALL_DEPTH 100
typedef struct {
    long vars[3];
    int assigned[3];
} ActivationRecord;

static ActivationRecord stack[MAX_CALL_DEPTH];
static int sp = -1; /* Stack Pointer */

/* Global Function Table */
static Node *functions[3] = {NULL};

static int exec_block(Node *block, long *ret_val);

static long eval_expr(Node *n) {
    if (n->type == NODE_NUM) return n->num_value;
    if (n->type == NODE_VAR) {
        if (!stack[sp].assigned[n->id]) {
            fprintf(stderr, "Runtime Error: Variable %d used before assignment in this scope.\n", n->id); exit(1);
        }
        return stack[sp].vars[n->id];
    }
    if (n->type == NODE_FUNC_CALL) {
        if (!functions[n->id]) { fprintf(stderr, "Runtime Error: Function %d not defined.\n", n->id); exit(1); }
        long arg = eval_expr(n->expr);

        /* Push new Activation Record onto the Call Stack */
        sp++;
        if(sp >= MAX_CALL_DEPTH) { fprintf(stderr, "Stack Overflow!\n"); exit(1); }

        for(int i=0; i<3; i++) stack[sp].assigned[i] = 0; /* Clear local scope */

        /* Pass the argument to the function's requested parameter variable */
        stack[sp].vars[functions[n->id]->param_id] = arg;
        stack[sp].assigned[functions[n->id]->param_id] = 1;

        long ret = 0;
        exec_block(functions[n->id]->body, &ret);

        sp--; /* Pop Activation Record off the stack */
        return ret;
    }
    if (n->type == NODE_BINOP) {
        long l = eval_expr(n->left);
        long r = eval_expr(n->right);
        if (n->op == '+') return l + r;
        if (n->op == '-') return l - r;
        if (n->op == '*') return l * r;
        if (n->op == '/') return l / r;
    }
    if (n->type == NODE_EQ) { return eval_expr(n->left) == eval_expr(n->right); }
    if (n->type == NODE_LT) { return eval_expr(n->left) < eval_expr(n->right); }
    if (n->type == NODE_GT) { return eval_expr(n->left) > eval_expr(n->right); }
    return 0;
}

static int exec_block(Node *block, long *ret_val) {
    Node *curr = block->body;
    while (curr) {
        if (curr->type == NODE_PRINT) { printf("%ld\n", eval_expr(curr->expr)); }
        else if (curr->type == NODE_ASSIGN) {
            stack[sp].vars[curr->id] = eval_expr(curr->expr);
            stack[sp].assigned[curr->id] = 1;
        }
        else if (curr->type == NODE_IF) {
            if (eval_expr(curr->expr)) {
                if (exec_block(curr->body, ret_val)) return 1; /* Propagate return */
            }
        }
        else if (curr->type == NODE_LOOP) {
            while (eval_expr(curr->expr)) {
                if (exec_block(curr->body, ret_val)) return 1;
            }
        }
        else if (curr->type == NODE_RETURN) {
            *ret_val = eval_expr(curr->expr);
            return 1; /* Tell caller we hit a return */
        }
        curr = curr->next;
    }
    return 0;
}

void interpret(Node *program) {
    /* Pass 1: Hoist all function definitions to the global table */
    Node *curr = program->body;
    while (curr) {
        if (curr->type == NODE_FUNC_DEF) { functions[curr->id] = curr; }
        curr = curr->next;
    }

    /* Setup Global Scope (Activation Record 0) */
    sp = 0;
    for(int i=0; i<3; i++) stack[sp].assigned[i] = 0;

    /* Pass 2: Execute global statements one at a time.
       NOTE: We must save curr->next BEFORE dispatching, because exec_block
       for a LOOP node internally re-evaluates the condition repeatedly and
       must NOT have its next pointer clobbered. */
    long dummy_ret = 0;
    curr = program->body;
    while (curr) {
        Node *next_stmt = curr->next; /* save next BEFORE any mutation */
        if (curr->type != NODE_FUNC_DEF) {
            Node temp_block = { .type = NODE_BLOCK, .body = curr };
            curr->next = NULL; /* isolate this single statement */
            exec_block(&temp_block, &dummy_ret);
            curr->next = next_stmt; /* restore after execution */
        }
        curr = next_stmt;
    }
}
