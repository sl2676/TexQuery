from typing import List, Optional, Any

def read_file(filename: str) -> str:
	with open(filename, 'r') as file:
		return file.read()

def write_to_file(filename: str, content: List[str]) -> None:
	with open(filename, 'w') as file:
		file.write('\n'.join(content))


