with open('scripts/main.gd', 'r', encoding='utf-8') as f:
    text = f.read()
    start = text.find('func _run_ai_game_loop')
    end = text.find('func _append_history_state')
    print(text[start:end])
