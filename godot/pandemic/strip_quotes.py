import os

with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    text = f.read()

# Replace any literal backslash double-quote
text = text.replace(chr(92) + '\"', '\"')

with open('scripts/main.gd', 'w', encoding='utf-8') as f:
    f.write(text)

print('Removed backslashes successfully!')
