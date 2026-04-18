with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines):
    indent = l[:len(l) - len(l.lstrip())]
    if ' ' in indent and '\t' in indent:
        print(f'Line {i+1} has mixed tabs/spaces: {repr(indent)}')
