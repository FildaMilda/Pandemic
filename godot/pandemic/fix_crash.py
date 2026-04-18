with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    text = f.read()

new_text = text.replace('state_clone.free()', 'state_clone.call_deferred("free")')

with open('scripts/main.gd', 'w', encoding='utf-8') as f:
    f.write(new_text)

print('Updated free to call_deferred')
