with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    lines = f.readlines()
for i, l in enumerate(lines[120:145]):
    print(f'{i+121}: {l.strip()}')
