with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()
    for i, l in enumerate(lines):
        if '#' in l:
            comment = l[l.index('#'):]
            if '\"' in comment:
                print(f'Line {i+1} has quote in comment: {comment}')
