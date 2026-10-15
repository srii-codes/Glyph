/*
 * glyph_optimizer.c
 * Glyph Compiler - Phase 3: Optimization Passes over TAC
 *
 * Pass 1 — Constant Folding:
 *   Before:  t0 = 5 + 3
 *   After:   t0 = 8          (the ADD instruction is replaced in-place)
 *
 * Pass 2 — Dead-Code Elimination:
 *   Code immediately following an unconditional GOTO or RETURN is
 *   unreachable.  Those instructions are unlinked from the list until
 *   the next LABEL (which may be a branch target elsewhere).
 *
 * Both passes are "single-sweep" — O(n) in the number of instructions.
 */

#include <stdio.h>
#include <stdlib.h>
#include "glyph_optimizer.h"

/* ------------------------------------------------------------------ */
/* Pass 1: Constant Folding                                             */
/* ------------------------------------------------------------------ */
static int fold_constants(TacInstr *head) {
    int folded = 0;
    for (TacInstr *i = head; i; i = i->next) {
        /* Only fold arithmetic on two compile-time constants */
        if (i->op1.kind != OPERAND_CONST || i->op2.kind != OPERAND_CONST)
            continue;

        long lv = i->op1.const_val;
        long rv = i->op2.const_val;
        long result;
        int  can_fold = 1;

        switch (i->op) {
            case TAC_ADD: result = lv + rv; break;
            case TAC_SUB: result = lv - rv; break;
            case TAC_MUL: result = lv * rv; break;
            case TAC_DIV:
                if (rv == 0) { can_fold = 0; break; } /* never fold div-by-zero */
                result = lv / rv; break;
            case TAC_EQ:  result = (lv == rv) ? 1 : 0; break;
            case TAC_LT:  result = (lv <  rv) ? 1 : 0; break;
            case TAC_GT:  result = (lv >  rv) ? 1 : 0; break;
            default:      can_fold = 0; break;
        }

        if (can_fold) {
            /* Rewrite in-place as a simple ASSIGN of the folded constant */
            i->op      = TAC_ASSIGN;
            i->op1     = i->result;      /* result operand stays the same */
            i->op1.kind      = OPERAND_CONST;
            i->op1.const_val = result;
            i->op2.kind      = OPERAND_NONE;
            folded++;
        }
    }
    return folded;
}

/* ------------------------------------------------------------------ */
/* Pass 2: Dead-Code Elimination                                        */
/* ------------------------------------------------------------------ */
static int eliminate_dead_code(TacInstr *head) {
    int removed = 0;
    for (TacInstr *i = head; i && i->next; i = i->next) {
        /* Keep dead-code removal inside its current function. */
        if (i->op == TAC_GOTO || i->op == TAC_RETURN) {
            while (i->next && i->next->op != TAC_LABEL &&
                   i->next->op != TAC_FUNC_END) {
                TacInstr *dead = i->next;
                i->next = dead->next;
                free(dead);
                removed++;
            }
        }
    }
    return removed;
}

/* ------------------------------------------------------------------ */
/* Public entry point — runs both passes, reports what was changed     */
/* ------------------------------------------------------------------ */
TacInstr *optimize(TacInstr *head) {
    printf("\n========= OPTIMIZATION PASSES =========\n");

    int f = fold_constants(head);
    printf("  Constant Folding  : %d instruction(s) folded\n", f);

    int d = eliminate_dead_code(head);
    printf("  Dead-Code Elim.   : %d instruction(s) removed\n", d);

    printf("=======================================\n\n");
    return head;   /* list is mutated in-place; return same head */
}
