with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines[140:160]):
    print(f'{i+141}: {l.strip()}')
