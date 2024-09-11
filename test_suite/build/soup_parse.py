from TexSoup import TexSoup
from parse_util import read_file
def latex_to_text(latex_file, output_file):
	try:
		file_content = read_file(latex_file)
		file_content.replace('\n', '')	
		file_contents = '\n'.join(line for line in file_content.splitlines() if line != '')
		soup = TexSoup(file_contents)
		print(soup.section.string)
		
	except FileNotFoundError:
		print("File not found. Please provide a valid file name")

