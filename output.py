def _glyph_div(left, right):
    quotient = abs(left) // abs(right)
    return -quotient if (left < 0) != (right < 0) else quotient

def fox(cat):
    pc = 0
    while pc < 3:
        if pc == 0:
            t0 = cat + 5
            pc += 1
        elif pc == 1:
            cat = t0
            pc += 1
        elif pc == 2:
            return cat
        else:
            break

def _glyph_main():
    pc = 0
    while pc < 2:
        if pc == 0:
            t1 = fox(10)
            pc += 1
        elif pc == 1:
            print(t1)
            pc += 1
        else:
            break

_glyph_main()
