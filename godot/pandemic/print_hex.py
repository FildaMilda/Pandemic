with open('scripts/main.gd', 'rb') as f:
    lines = f.read().split(b'\n')
for i, l in enumerate(lines[424:428]):
    print(f'Line {i+425}:', l.decode('utf-8'))
    print('Hex:', l.hex(' '))
