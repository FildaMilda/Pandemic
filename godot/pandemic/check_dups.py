with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.read().splitlines()
funcs = set()
for i, l in enumerate(lines):
    if l.startswith('func '):
        fn = l.split('(')[0]
        if fn in funcs:
            print('DUPLICATE FUNCTION:', fn, 'at line', i+1)
        funcs.add(fn)
