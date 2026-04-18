import re

with open('d:/Projects/Pandemic/src/Bindings/Godot/GodotInterface.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

old_clone = '''PandemicGame* godot::PandemicGame::clone()
{
    PandemicGame* cloned_node = memnew(PandemicGame);
    cloned_node->game = this->game;
    cloned_node->rng = this->rng;
    return cloned_node;
}'''

new_clone = '''PandemicGame* godot::PandemicGame::clone()
{
    // Important: Do not copy the MacroMCTS instance. It is huge and has preallocated memory 
    // that may trigger placement delete panics if Godot manages its lifetime. 
    PandemicGame* cloned_node = memnew(PandemicGame);
    cloned_node->game = this->game;
    cloned_node->rng = this->rng;
    return cloned_node;
}'''

text = text.replace(old_clone, new_clone)
with open('d:/Projects/Pandemic/src/Bindings/Godot/GodotInterface.cpp', 'w', encoding='utf-8') as f:
    f.write(text)

print('Updated GodotInterface.cpp')
