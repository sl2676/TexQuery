import PyPDF2
import os
import tempfile
import pytesseract
from pdf2image import convert_from_path
from sympy import sympify, latex

# Set the path to the Tesseract executable
pytesseract.pytesseract.tesseract_cmd = r'/usr/bin/tesseract'

def pdf_to_text(pdf_path):
    with open(pdf_path, 'rb') as pdf_file:
        pdf_reader = PyPDF2.PdfReader(pdf_file)
        num_pages = len(pdf_reader.pages)
        text = ''

        for i in range(num_pages):
            page = pdf_reader.pages[i]
            text += page.extract_text()

    return text

def pdf_to_image(pdf_path):
    images = convert_from_path(pdf_path, dpi=300)
    return images

def ocr_equations(image):
    text = pytesseract.image_to_string(image, config='--psm 6 --oem 3 -c tessedit_char_whitelist=0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-=()[]{}^*/_')
    return text

def convert_equations_to_latex(equations):
    latex_equations = []

    for equation in equations:
        try:
            sympy_eq = sympify(equation)
            latex_equations.append(latex(sympy_eq))
        except:
            print(f"Could not convert equation: {equation}")

    return latex_equations

def main():
    pdf_path = '/home/jack/Documents/python/GalaxyResearch/pdf_downloads/2211.05107.pdf'
    text = pdf_to_text(pdf_path)
    images = pdf_to_image(pdf_path)

    equations = []

    for image in images:
        equations.append(ocr_equations(image))

    latex_equations = convert_equations_to_latex(equations)
    
    with open("output_latex.txt", "w") as output_file:
        for latex_eq in latex_equations:
            output_file.write(latex_eq + "\n")

if __name__ == "__main__":
    main()
