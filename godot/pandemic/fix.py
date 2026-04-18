import re
with open('d:/Projects/Pandemic/godot/pandemic/scripts/main.gd', 'r', encoding='utf-8') as f:
    text = f.read()

pattern = re.compile(
    r'(var act_str.*?.*?var t_info = step_clone.execute_action\(action\))',
    re.DOTALL
)

new_str = '''var act_str = ""
if step_clone.has_method("get_action_string"):
act_str = step_clone.get_action_string(action)
else:
act_str = "Action " + str(action)

print("[DEBUG] AI Executing Action: ", act_str)
var t_info = step_clone.execute_action(action)'''

text = pattern.sub(new_str, text, count=1)

with open('d:/Projects/Pandemic/godot/pandemic/scripts/main.gd', 'w', encoding='utf-8') as f:
    f.write(text)

print("Updated script")
