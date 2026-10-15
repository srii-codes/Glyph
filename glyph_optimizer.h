/*
 * glyph_optimizer.h
 * Glyph Compiler - Phase 3: IR Optimization Passes
 *
 * Declares the two optimization passes applied to the TAC list:
 *   1. Constant Folding: evaluates operations on two constants
 *      at compile-time and replaces the instruction with a
 *      single TAC_ASSIGN of the folded result.
 *   2. Dead-Code Elimination: removes unreachable instructions
 *      that appear between a GOTO/RETURN and the next LABEL.
 */

#ifndef GLYPH_OPTIMIZER_H
#define GLYPH_OPTIMIZER_H

#include "glyph_ir.h"

TacInstr *optimize(TacInstr *head);

#endif /* GLYPH_OPTIMIZER_H */
