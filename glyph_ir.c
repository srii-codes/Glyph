/*
 * glyph_ir.c
 * Glyph Compiler - Phase 3: Three-Address Code (TAC) Generator
 *
 * Walks the AST produced by the parser and emits a flat, ordered
 * linked-list of TAC instructions.  Control flow (IF/LOOP) is
 * handled with generated labels and conditional/unconditional jumps,
 * implementing the classic "backpatch" label strategy covered in
 * Syllabus Module 4.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glyph_ir.h"

/* ------------------------------------------------------------------ */
/* Internal state                                                       */
/* ------------------------------------------------------------------ */
static TacInstr *ir_head = NULL;   /* head of the instruction list   */
static TacInstr *ir_tail = NULL;   /* tail — for O(1) append         */
static int       temp_counter = 0; /* next available temp id          */
static int       label_counter= 0; /* next available label id         */

static const char *var_names[3]  = { "cat",  "dog",  "frog" };
static const char *func_names[3] = { "fox",  "lion", "bear" };

/* ------------------------------------------------------------------ */
/* Helper: build operand values                                         */
/* ------------------------------------------------------------------ */
static Operand make_const(long v) {
    Operand o; o.kind = OPERAND_CONST; o.const_val = v; o.id = 0; return o;
}
static Operand make_var(int id) {
    Operand o; o.kind = OPERAND_VAR;   o.id = id;  o.const_val = 0; return o;
}
static Operand make_temp(int id) {
    Operand o; o.kind = OPERAND_TEMP;  o.id = id;  o.const_val = 0; return o;
}
static Operand make_none(void) {
    Operand o; o.kind = OPERAND_NONE; o.id = 0; o.const_val = 0; return o;
}
static int new_temp(void)  { return temp_counter++; }
static int new_label(void) { return label_counter++; }

/* ------------------------------------------------------------------ */
/* Helper: append one instruction to the list                           */
/* ------------------------------------------------------------------ */
static TacInstr *emit(TacOp op, Operand res, Operand o1, Operand o2) {
    TacInstr *i = calloc(1, sizeof(TacInstr));
    i->op = op; i->result = res; i->op1 = o1; i->op2 = o2;
    i->label_id = -1; i->func_id = -1;
    if (!ir_head) { ir_head = ir_tail = i; }
    else          { ir_tail->next = i; ir_tail = i; }
    return i;
}

/* ------------------------------------------------------------------ */
/* Forward declaration                                                  */
/* ------------------------------------------------------------------ */
static Operand gen_expr(Node *n);
static void    gen_stmt(Node *n);

/* ------------------------------------------------------------------ */
/* Expression lowering — returns the operand holding the result        */
/* ------------------------------------------------------------------ */
static Operand gen_expr(Node *n) {
    if (!n) return make_none();

    switch (n->type) {
        case NODE_NUM:
            return make_const(n->num_value);

        case NODE_VAR:
            return make_var(n->id);

        case NODE_BINOP: {
            Operand l   = gen_expr(n->left);
            Operand r   = gen_expr(n->right);
            int     tmp = new_temp();
            TacOp   op;
            if      (n->op == '+') op = TAC_ADD;
            else if (n->op == '-') op = TAC_SUB;
            else if (n->op == '*') op = TAC_MUL;
            else                   op = TAC_DIV;
            emit(op, make_temp(tmp), l, r);
            return make_temp(tmp);
        }

        /* Relational operators — produce a 0/1 value in a temp */
        case NODE_EQ:
        case NODE_LT:
        case NODE_GT: {
            Operand l   = gen_expr(n->left);
            Operand r   = gen_expr(n->right);
            int     tmp = new_temp();
            TacOp   op  = (n->type == NODE_EQ) ? TAC_EQ
                        : (n->type == NODE_LT)  ? TAC_LT
                                                 : TAC_GT;
            emit(op, make_temp(tmp), l, r);
            return make_temp(tmp);
        }

        case NODE_FUNC_CALL: {
            Operand arg = gen_expr(n->expr);
            int     tmp = new_temp();
            TacInstr *ci = emit(TAC_CALL, make_temp(tmp), arg, make_none());
            ci->func_id = n->id;
            return make_temp(tmp);
        }

        default:
            return make_none();
    }
}

/* ------------------------------------------------------------------ */
/* Statement lowering                                                   */
/* ------------------------------------------------------------------ */
static void gen_stmt(Node *n) {
    if (!n) return;

    switch (n->type) {

        case NODE_ASSIGN: {
            Operand rhs = gen_expr(n->expr);
            emit(TAC_ASSIGN, make_var(n->id), rhs, make_none());
            break;
        }

        case NODE_PRINT: {
            Operand val = gen_expr(n->expr);
            emit(TAC_PRINT, make_none(), val, make_none());
            break;
        }

        case NODE_RETURN: {
            Operand val = gen_expr(n->expr);
            emit(TAC_RETURN, make_none(), val, make_none());
            break;
        }

        /*
         * IF node — classic "if-false jump" pattern:
         *
         *   <evaluate condition>
         *   IF_FALSE cond GOTO L_end
         *   <body>
         *   LABEL L_end
         */
        case NODE_IF: {
            Operand cond = gen_expr(n->expr);
            int     l_end = new_label();

            TacInstr *branch = emit(TAC_IF_FALSE, make_none(), cond, make_none());
            branch->label_id = l_end;

            /* body */
            Node *s = n->body ? n->body->body : NULL;
            while (s) { gen_stmt(s); s = s->next; }

            /* L_end: */
            TacInstr *lbl = emit(TAC_LABEL, make_none(), make_none(), make_none());
            lbl->label_id = l_end;
            break;
        }

        /*
         * LOOP node — while-loop pattern:
         *
         *   LABEL L_top
         *   <evaluate condition>
         *   IF_FALSE cond GOTO L_exit
         *   <body>
         *   GOTO L_top
         *   LABEL L_exit
         */
        case NODE_LOOP: {
            int l_top  = new_label();
            int l_exit = new_label();

            TacInstr *top_lbl = emit(TAC_LABEL, make_none(), make_none(), make_none());
            top_lbl->label_id = l_top;
            /*evaluation condition*/
            Operand cond = gen_expr(n->expr);

            TacInstr *branch = emit(TAC_IF_FALSE, make_none(), cond, make_none());
            branch->label_id = l_exit;

            /* body */
            Node *s = n->body ? n->body->body : NULL;
            while (s) { gen_stmt(s); s = s->next; }

            TacInstr *back = emit(TAC_GOTO, make_none(), make_none(), make_none());
            back->label_id = l_top;

            TacInstr *exit_lbl = emit(TAC_LABEL, make_none(), make_none(), make_none());
            exit_lbl->label_id = l_exit;
            break;
        }

        /*
         * FUNC_DEF — emit a FUNC_BEGIN/FUNC_END wrapper around the body.
         * The body's statements (including nested IF/LOOP/RETURN) are
         * lowered recursively, so recursion "just works" — the call
         * produces a TAC_CALL which references the same function label.
         */
        case NODE_FUNC_DEF: {
            TacInstr *fb = emit(TAC_FUNC_BEGIN, make_none(), make_none(), make_none());
            fb->func_id = n->id;
            fb->op1 = make_var(n->param_id);

            Node *s = n->body ? n->body->body : NULL;
            while (s) { gen_stmt(s); s = s->next; }

            TacInstr *fe = emit(TAC_FUNC_END, make_none(), make_none(), make_none());
            fe->func_id = n->id;
            break;
        }

        case NODE_BLOCK: {
            Node *s = n->body;
            while (s) { gen_stmt(s); s = s->next; }
            break;
        }

        default:
            /* Expression used as statement (e.g., standalone func call) */
            gen_expr(n);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */
TacInstr *generate_ir(Node *program) {
    ir_head = ir_tail = NULL;
    temp_counter = label_counter = 0;

    Node *s = program->body;
    while (s) { gen_stmt(s); s = s->next; }
    return ir_head;
}

/* ------------------------------------------------------------------ */
/* Pretty-printer (shown in terminal and in Phase 3 report)             */
/* ------------------------------------------------------------------ */
static void print_operand(Operand o) {
    switch (o.kind) {
        case OPERAND_CONST: printf("%ld", o.const_val); break;
        case OPERAND_VAR:   printf("%s",  var_names[o.id]); break;
        case OPERAND_TEMP:  printf("t%d", o.id); break;
        default:            break;
    }
}

void print_ir(TacInstr *head) {
    printf("\n========= THREE-ADDRESS CODE (IR) =========\n");
    for (TacInstr *i = head; i; i = i->next) {
        switch (i->op) {
            case TAC_LABEL:
                printf("L%d:\n", i->label_id);
                break;
            case TAC_FUNC_BEGIN:
                printf("\n--- BEGIN %s ---\n", func_names[i->func_id]);
                break;
            case TAC_FUNC_END:
                printf("--- END   %s ---\n\n", func_names[i->func_id]);
                break;
            case TAC_ASSIGN:
                printf("  "); print_operand(i->result);
                printf(" = "); print_operand(i->op1);
                printf("\n");
                break;
            case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: {
                char c = (i->op==TAC_ADD)?'+': (i->op==TAC_SUB)?'-':
                         (i->op==TAC_MUL)?'*':'/';
                printf("  "); print_operand(i->result);
                printf(" = "); print_operand(i->op1);
                printf(" %c ", c); print_operand(i->op2);
                printf("\n");
                break;
            }
            case TAC_EQ: case TAC_LT: case TAC_GT: {
                const char *rel = (i->op==TAC_EQ)?"==": (i->op==TAC_LT)?"<":">";
                printf("  "); print_operand(i->result);
                printf(" = "); print_operand(i->op1);
                printf(" %s ", rel); print_operand(i->op2);
                printf("\n");
                break;
            }
            case TAC_IF_FALSE:
                printf("  IF_FALSE "); print_operand(i->op1);
                printf(" GOTO L%d\n", i->label_id);
                break;
            case TAC_GOTO:
                printf("  GOTO L%d\n", i->label_id);
                break;
            case TAC_PRINT:
                printf("  PRINT "); print_operand(i->op1);
                printf("\n");
                break;
            case TAC_CALL:
                printf("  "); print_operand(i->result);
                printf(" = CALL %s(", func_names[i->func_id]);
                print_operand(i->op1); printf(")\n");
                break;
            case TAC_RETURN:
                printf("  RETURN "); print_operand(i->op1);
                printf("\n");
                break;
        }
    }
    printf("===========================================\n\n");
}
