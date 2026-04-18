with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    text = f.read()
    print('Total double quotes:', text.count('\"'))
