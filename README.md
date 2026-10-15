# Glyph

A compiler for a small imperative programming language whose token vocabulary is composed entirely of Unicode emoji, rather than ASCII keywords.

Glyph is built as a compiler-theory exploration, not a practical end-user tool — in the tradition of esoteric languages like Brainfuck and APL. Its core technical focus: most compilers (and tools like Lex/Flex) assume single-byte ASCII input. Glyph's lexer has to correctly decode multi-byte UTF-8 sequences and handle Unicode grapheme clusters, which is a genuinely different systems problem than parsing plain text.


## Why emoji?

Not a gimmick, its a constraint. Building a lexer that's correct under variable-width, multi-byte Unicode input (rather than assumed single-byte ASCII) is the actual technical challenge this project explores. 

## Pipeline

Emoji Source (UTF-8)
→ UTF-8 Decoder / Grapheme Grouping
→ Lexer (Token Stream)
→ Parser (AST)
→ Semantic Analyzer (Annotated AST + Symbol Table)
→ IR Generator (Three-Address Code)
→ Optimizer (Optimized IR)
→ Code Generator
→ Runnable Python/C Output


## Language Specification


| Emoji | Codepoint | Token | Meaning  |
| :---: | :--- | :--- | :--- |
| 🖨 | `U+1F5A8` | `PRINT` | Output statement |
| ➕ | `U+2795` | `PLUS` | Addition operator |
| ➖ | `U+2796` | `MINUS` | Subtraction operator |
| ✖ | `U+2716` | `MUL` | Multiplication operator |
| ➗ | `U+2797` | `DIV` | Division operator |
| 🟰 | `U+1F7F0` | `EQ` | Equality comparison operator |
| 📉 | `U+1F4C9` | `LT` | Less-than comparison operator |
| 📈 | `U+1F4C8` | `GT` | Greater-than comparison operator |
| 🔁 | `U+1F501` | `LOOP` | While loop construct |
| ❓ | `U+2753` | `IF` | Conditional branching construct |
| 📥 | `U+1F4E5` | `ASSIGN` | Variable assignment operator |
| 🐱 | `U+1F431` | `VAR (cat)` | Variable identifier (ID 0) |
| 🐶 | `U+1F436` | `VAR (dog)` | Variable identifier (ID 1) |
| 🐸 | `U+1F438` | `VAR (frog)` | Variable identifier (ID 2) |
| 🏭 | `U+1F3ED` | `DEF` | Begins a function definition block |
| 🔚 | `U+1F51A` | `END` | Closes a block (`DEF`, `IF`, `LOOP`) |
| 🔙 | `U+1F519` | `RETURN` | Returns a value from a function |
| 🦊 | `U+1F98A` | `FUNC (fox)` | Function-name identifier (ID 0) |
| 🦁 | `U+1F981` | `FUNC (lion)` | Function-name identifier (ID 1) |
| 🐻 | `U+1F43B` | `FUNC (bear)` | Function-name identifier (ID 2) |



## Example

The default `input.glf` demonstrates a recursive factorial function:

```text
🏭 🦊 🐱
	❓ 🐱 📉 🔢2
		🔙 🔢1
	🔚
	🔙 🐱 ✖ 🦊 🐱 ➖ 🔢1
🔚
🖨 🦊 🔢5
```

Both the interpreter and generated Python print `120`.



## Build and run

```bash
gcc main.c glyph_lexer.c glyph_parser.c glyph_ast.c glyph_interpreter.c glyph_ir.c glyph_optimizer.c glyph_codegen.c -o main
./main
python output.py
```

By default, the compiler reads `input.glf` and writes `output.py` in the current directory. Optional arguments select other paths: `./main source.glf generated.py`.

## Known Technical Challenges

- **UTF-8 / ZWJ lexing** — some emoji are multiple Unicode codepoints joined by zero-width joiners into one visual character. The current token set avoids these cases; full grapheme-cluster handling is a stretch goal.
- **Column tracking** — a 4-byte emoji is one visible character, not four. Error reporting needs to count decoded characters, not raw bytes.
- **Nested-loop backpatching** — generating correct jump targets for nested `🔁`/`❓` in three-address code is flagged as the highest-risk part of Phase 2.
- **Scope trade-off** — no functions/recursion (and therefore no activation records), by deliberate choice, to prioritize depth on IR generation and optimization instead of partial coverage everywhere.


## Tech Stack

- **Language:** C
- **Lexer/Parser:** hand-written (Flex/Bison evaluated only as a stretch goal, given Flex's byte-oriented limitations)
- **Target output:** Python or C source

## Background

- Lex/Flex are byte-oriented by design and don't natively guarantee correct multi-byte/ZWJ handling — this motivates Glyph's custom lexer.
- Esoteric languages (Brainfuck, Whitespace) prove non-conventional token sets can form complete languages, but typically over single-byte ASCII.
- [Unicode Standard Annex #29](https://unicode.org/reports/tr29/) defines grapheme cluster boundaries — the formal basis this project applies.
