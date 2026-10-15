/*
 * main.c
 * Glyph Compiler — Full Phase 3 Pipeline Driver
 *
 * Pipeline:
 *   Source (.glf)
 *     -> Lexer        (glyph_lexer)
 *     -> Parser       (glyph_parser)      => AST
 *     -> Interpreter  (glyph_interpreter) => Direct execution + output
 *     -> IR Generator (glyph_ir)          => Three-Address Code
 *     -> Optimizer    (glyph_optimizer)   => Constant Folding + Dead-Code Elim
 *     -> Code Gen     (glyph_codegen)     => output.py (runnable Python 3)
 */

#include <stdio.h>
#include <stdlib.h>
#include "glyph_lexer.h"
#include "glyph_parser.h"
#include "glyph_interpreter.h"
#include "glyph_ir.h"
#include "glyph_optimizer.h"
#include "glyph_codegen.h"

int main(int argc, char **argv) {
    const char *in_file = argc > 1 ? argv[1] : "inputtt.glf";
    const char *out_file = argc > 2 ? argv[2] : "output.py";
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [source.glf] [output.py]\n", argv[0]);
        return 1;
    }

    /* ---- 1. Read source file into memory ---- */
    FILE *f = fopen(in_file, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open %s\n", in_file);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: Could not seek in %s\n", in_file);
        fclose(f);
        return 1;
    }
    long fsize = ftell(f);
    if (fsize < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Could not read %s\n", in_file);
        fclose(f);
        return 1;
    }
    char *buffer = malloc((size_t)fsize + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate source buffer\n");
        fclose(f);
        return 1;
    }
    size_t bytes_read = fread(buffer, 1, (size_t)fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Error: Could not read %s\n", in_file);
        free(buffer);
        fclose(f);
        return 1;
    }
    buffer[fsize] = 0;
    fclose(f);

    printf("============================================\n");
    printf("  GLYPH COMPILER\n");
    printf("  Source: %s\n", in_file);
    printf("============================================\n\n");

    /* ---- 2. Lexical Analysis ---- */
    printf("[1] Lexical Analysis ... done\n");
    lex_source(buffer);

    /* ---- 3. Parsing -> AST ---- */
    printf("[2] Parsing -> AST ... done\n");
    Node *ast = parse_program();

    /* ---- 4. Interpretation (direct execution) ---- */
    printf("[3] Interpreter Output:\n");
    printf("--------------------------------------------\n");
    printf("\t\t");
    interpret(ast);
    printf("--------------------------------------------\n\n");

    /* ---- 5. IR Generation (Three-Address Code) ---- */
    printf("[4] Generating Intermediate Representation ...\n");
    TacInstr *ir = generate_ir(ast);
    print_ir(ir);

    /* ---- 6. Optimization passes ---- */
    printf("[5] Running Optimization Passes ...\n");
    ir = optimize(ir);

    /* ---- 7. Optimized IR (after optimization) ---- */
    printf("[6] Optimized IR:\n");
    print_ir(ir);

    /* ---- 8. Python Code Generation ---- */
    printf("[7] Generating Python Target Code ...\n");
    codegen_python(ir, out_file);

    free(buffer);
    return 0;
}
