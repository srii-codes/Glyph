/*
 * glyph_ir.h
 * Glyph Compiler - Phase 3: Intermediate Representation (Three-Address Code)
 *
 * Defines the TAC (Three-Address Code) data structures and the function
 * that lowers the AST into a flat, linear list of TAC instructions.
 * Each instruction has the form: result = operand1 op operand2
 */

#ifndef GLYPH_IR_H
#define GLYPH_IR_H

#include "glyph_ast.h"

/* -------------------------------------------------------------------
 * Operand: an operand in a TAC instruction can be a literal integer,
 * a named variable (var_id 0-2), or a compiler-generated temporary.
 * ------------------------------------------------------------------- */
typedef enum {
    OPERAND_NONE,
    OPERAND_CONST,   /* A literal integer value, e.g. 5          */
    OPERAND_VAR,     /* A named variable: cat(0), dog(1), frog(2) */
    OPERAND_TEMP     /* A compiler temp: t0, t1, t2, ...          */
} OperandKind;

typedef struct {
    OperandKind kind;
    long  const_val;   /* used when kind == OPERAND_CONST */
    int   id;          /* var id (0-2) or temp id when applicable */
} Operand;

/* -------------------------------------------------------------------
 * TAC Instruction opcodes
 * ------------------------------------------------------------------- */
typedef enum {
    TAC_ASSIGN,     /* result = operand1                          */
    TAC_ADD,        /* result = op1 + op2                         */
    TAC_SUB,        /* result = op1 - op2                         */
    TAC_MUL,        /* result = op1 * op2                         */
    TAC_DIV,        /* result = op1 / op2                         */
    TAC_EQ,         /* result = (op1 == op2) ? 1 : 0              */
    TAC_LT,         /* result = (op1 <  op2) ? 1 : 0              */
    TAC_GT,         /* result = (op1 >  op2) ? 1 : 0              */
    TAC_IF_FALSE,   /* if result == 0 jump to label               */
    TAC_GOTO,       /* unconditional jump to label                */
    TAC_LABEL,      /* defines a label target                     */
    TAC_PRINT,      /* print operand1                             */
    TAC_CALL,       /* result = call func_id(op1)                 */
    TAC_RETURN,     /* return operand1                            */
    TAC_FUNC_BEGIN, /* marks the start of a function definition   */
    TAC_FUNC_END    /* marks the end of a function definition     */
} TacOp;

/* -------------------------------------------------------------------
 * A single TAC instruction
 * ------------------------------------------------------------------- */
typedef struct TacInstr {
    TacOp    op;
    Operand  result;
    Operand  op1;
    Operand  op2;
    int      label_id;     /* for LABEL, GOTO, IF_FALSE targets   */
    int      func_id;      /* for CALL, FUNC_BEGIN, FUNC_END       */
    struct TacInstr *next;
} TacInstr;

/* -------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------- */
TacInstr *generate_ir(Node *program);
void      print_ir(TacInstr *head);

#endif /* GLYPH_IR_H */
