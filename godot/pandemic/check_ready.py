with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines[50:80]):
    print(f'{i+51}: {l.strip()}')
