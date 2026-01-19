import os

file_path = "test/Res.txt"

with open(file_path, "r") as f:
    content = f.read().strip()

# Corrupt the last character
last_char = content[-1]
if last_char == '0':
    new_char = '1'
else:
    new_char = '0'

new_content = content[:-1] + new_char

with open(file_path, "w") as f:
    f.write(new_content)

print(f"Corrupted {file_path}: changed last char '{last_char}' to '{new_char}'")
