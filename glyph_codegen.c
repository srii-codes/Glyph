/*
 * glyph_codegen.c
 * Glyph Compiler - Phase 3: Python 3 Target Code Generator
 *
 * Walks the optimized TAC list and emits syntactically valid Python 3.
 * Strategy:
 *   - Function bodies (FUNC_BEGIN ... FUNC_END) are emitted as Python
 *     "def" blocks with proper indentation.
 *   - Labels become Python label-comment markers; jumps are converted
 *     to a structured goto-free equivalent using a "pc" (program counter)
 *     loop inside each function — clean and runnable Python.
 *   - Global statements (outside any function) are emitted at module level.
 *   - The output file is written to disk as "output.py" and can be run
 *     directly with: python output.py
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glyph_codegen.h"

static const char *var_names[3]  = { "cat",  "dog",  "frog" };
static const char *func_names[3] = { "fox",  "lion", "bear" };

/* ------------------------------------------------------------------ */
/* Helper: write one operand as a Python expression string             */
/* ------------------------------------------------------------------ */
static void write_operand(FILE *f, Operand o) {
    switch (o.kind) {
        case OPERAND_CONST: fprintf(f, "%ld", o.const_val);   break;
        case OPERAND_VAR:   fprintf(f, "%s",  var_names[o.id]);break;
        case OPERAND_TEMP:  fprintf(f, "t%d", o.id);          break;
        default:            fprintf(f, "None");                break;
    }
}

/* ------------------------------------------------------------------ */
/* Helper: write the result operand name                               */
/* ------------------------------------------------------------------ */
static void write_result(FILE *f, Operand o) {
    switch (o.kind) {
        case OPERAND_VAR:  fprintf(f, "%s",  var_names[o.id]); break;
        case OPERAND_TEMP: fprintf(f, "t%d", o.id);            break;
        default:                                                break;
    }
}

static size_t label_position(TacInstr **code, size_t count, int label_id) {
    for (size_t i = 0; i < count; i++) {
        if (code[i]->op == TAC_LABEL && code[i]->label_id == label_id) return i;
    }
    return count;
}

static void emit_execution_block(FILE *f, TacInstr **code, size_t count,
                                 const char *indent) {
    if (count == 0) {
        fprintf(f, "%spass\n", indent);
        return;
    }

    const char *case_indent = indent[0] ? "        " : "    ";
    const char *body_indent = indent[0] ? "            " : "        ";
    const char *nested_indent = indent[0] ? "                " : "            ";

    fprintf(f, "%spc = 0\n%swhile pc < %zu:\n", indent, indent, count);
    for (size_t pc = 0; pc < count; pc++) {
        fprintf(f, "%s%sif pc == %zu:\n", case_indent, pc ? "el" : "", pc);
        TacInstr *i = code[pc];
        switch (i->op) {
            case TAC_ASSIGN:
                fprintf(f, "%s", body_indent); write_result(f, i->result);
                fprintf(f, " = "); write_operand(f, i->op1);
                fprintf(f, "\n%spc += 1\n", body_indent);
                break;
            case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: {
                fprintf(f, "%s", body_indent); write_result(f, i->result);
                if (i->op == TAC_DIV) {
                    fprintf(f, " = _glyph_div("); write_operand(f, i->op1);
                    fprintf(f, ", "); write_operand(f, i->op2);
                    fprintf(f, ")\n%spc += 1\n", body_indent);
                    break;
                }
                char op = i->op == TAC_ADD ? '+' : i->op == TAC_SUB ? '-' : '*';
                fprintf(f, " = "); write_operand(f, i->op1);
                fprintf(f, " %c ", op); write_operand(f, i->op2);
                fprintf(f, "\n%spc += 1\n", body_indent);
                break;
            }
            case TAC_EQ: case TAC_LT: case TAC_GT: {
                const char *op = i->op == TAC_EQ ? "==" : i->op == TAC_LT ? "<" : ">";
                fprintf(f, "%s", body_indent); write_result(f, i->result);
                fprintf(f, " = int("); write_operand(f, i->op1);
                fprintf(f, " %s ", op); write_operand(f, i->op2);
                fprintf(f, ")\n%spc += 1\n", body_indent);
                break;
            }
            case TAC_IF_FALSE:
                fprintf(f, "%sif not ", body_indent); write_operand(f, i->op1);
                fprintf(f, ":\n%spc = %zu\n%selse:\n%spc += 1\n",
                        nested_indent, label_position(code, count, i->label_id),
                        body_indent, nested_indent);
                break;
            case TAC_GOTO:
                fprintf(f, "%spc = %zu\n", body_indent,
                        label_position(code, count, i->label_id));
                break;
            case TAC_LABEL:
                fprintf(f, "%spc += 1\n", body_indent);
                break;
            case TAC_PRINT:
                fprintf(f, "%sprint(", body_indent); write_operand(f, i->op1);
                fprintf(f, ")\n%spc += 1\n", body_indent);
                break;
            case TAC_CALL:
                fprintf(f, "%s", body_indent); write_result(f, i->result);
                fprintf(f, " = %s(", func_names[i->func_id]);
                write_operand(f, i->op1);
                fprintf(f, ")\n%spc += 1\n", body_indent);
                break;
            case TAC_RETURN:
                fprintf(f, "%sreturn ", body_indent); write_operand(f, i->op1);
                fprintf(f, "\n");
                break;
            default:
                fprintf(f, "%spc += 1\n", body_indent);
                break;
        }
    }
    fprintf(f, "%s    else:\n%s        break\n", indent, indent);
}

void codegen_python(TacInstr *head, const char *out_filename) {
    FILE *f = fopen(out_filename, "w");
    if (!f) {
        fprintf(stderr, "Codegen error: Cannot open %s for writing\n", out_filename);
        return;
    }

    fprintf(f, "# Generated by the Glyph Compiler (Phase 3)\n\n");
    fprintf(f, "def _glyph_div(left, right):\n");
    fprintf(f, "    quotient = abs(left) // abs(right)\n");
    fprintf(f, "    return -quotient if (left < 0) != (right < 0) else quotient\n\n");
    TacInstr *i = head;
    while (i) {
        if (i->op != TAC_FUNC_BEGIN) { i = i->next; continue; }
        TacInstr *end = i->next;
        size_t count = 0;
        while (end && end->op != TAC_FUNC_END) { count++; end = end->next; }
        TacInstr **body = count ? malloc(count * sizeof(*body)) : NULL;
        TacInstr *current = i->next;
        for (size_t j = 0; j < count; j++, current = current->next) body[j] = current;

        int param_id = i->op1.kind == OPERAND_VAR ? i->op1.id : 0;
        fprintf(f, "def %s(%s):\n", func_names[i->func_id], var_names[param_id]);
        emit_execution_block(f, body, count, "    ");
        fprintf(f, "\n");
        free(body);
        i = end ? end->next : NULL;
    }

    size_t global_count = 0;
    int in_func = 0;
    for (i = head; i; i = i->next) {
        if (i->op == TAC_FUNC_BEGIN) in_func = 1;
        else if (i->op == TAC_FUNC_END) in_func = 0;
        else if (!in_func) global_count++;
    }
    if (global_count) {
        TacInstr **global_code = malloc(global_count * sizeof(*global_code));
        size_t index = 0;
        in_func = 0;
        for (i = head; i; i = i->next) {
            if (i->op == TAC_FUNC_BEGIN) in_func = 1;
            else if (i->op == TAC_FUNC_END) in_func = 0;
            else if (!in_func) global_code[index++] = i;
        }
        fprintf(f, "def _glyph_main():\n");
        emit_execution_block(f, global_code, global_count, "    ");
        fprintf(f, "\n_glyph_main()\n");
        free(global_code);
    }

    fclose(f);
    printf("Code generation complete -> %s\n", out_filename);
}
