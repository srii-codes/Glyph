/*
 * glyph_codegen.h
 * Glyph Compiler - Phase 3: Python Code Generator
 *
 * Translates the optimized TAC instruction list into valid Python 3
 * source code and writes it to a .py output file.
 */

#ifndef GLYPH_CODEGEN_H
#define GLYPH_CODEGEN_H

#include "glyph_ir.h"

void codegen_python(TacInstr *head, const char *out_filename);

#endif /* GLYPH_CODEGEN_H */
