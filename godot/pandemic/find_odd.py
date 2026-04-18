with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.readlines()

for i, line in enumerate(lines):
    clean_line = line
    # Strip comments naively (find first # not wrapped in quotes? Too hard)
    # Actually, Godot ignores everything after # unless # is IN a string.
    in_string = False
    for j, c in enumerate(line):
        if c == '\"':
            in_string = not in_string
        elif c == '#' and not in_string:
            clean_line = line[:j]
            break
            
    if clean_line.count('\"') % 2 != 0:
        print(f'{i+1}: {clean_line}')
