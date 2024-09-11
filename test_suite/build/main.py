import sys
from soup_parse import latex_to_text
from texml_parse import latex_to_xml
from langchain_parser import langchain_ts 
file_path = sys.argv[1]
def main():
	latex_to_text(file_path, "output.txt")
	latex_to_xml(file_path, "output.xml")
	langchain_ts()
if __name__ == "__main__":
	main()
