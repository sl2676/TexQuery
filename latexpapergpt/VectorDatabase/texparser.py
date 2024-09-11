import os
import subprocess
import xml.etree.ElementTree as ET
import re
import requests

def extract_packages(tex_file):
    with open(tex_file, 'r') as f:
        content = f.read()
    packages = re.findall(r'\\usepackage(?:\[.*\])?{([^}]*)}', content)
    return [pkg.strip() for pkg_list in packages for pkg in pkg_list.split(',')]

def download_sty_file(package_name, download_path):
    url = f'https://ctan.org/tex-archive/macros/latex/contrib/{package_name}.zip'
    response = requests.get(url)
    if response.status_code == 200:
        with open(download_path, 'wb') as f:
            f.write(response.content)
        return True
    return False

def install_package(package_name, latexml_directory):
    download_path = os.path.join(latexml_directory, f'{package_name}.zip')
    if download_sty_file(package_name, download_path):
        print(f'Successfully downloaded {package_name}.zip')
        os.system(f'unzip {download_path} -d {latexml_directory}')
        os.remove(download_path)
    else:
        print(f'Failed to download {package_name}.zip')

def install_missing_packages(tex_file_path):
    # Use the extract_packages function from before to extract packages from the .tex file
    packages = extract_packages(tex_file_path)

    for package in packages:
        try:
            # Attempt to install the package using dnf
            subprocess.run(['sudo', 'dnf', 'install', '-y', f'tex({package}.sty)'], check=True)
            print(f"Installed package: {package}")
        except subprocess.CalledProcessError as e:
            print(f"Error installing package {package}: {e}")

def convert_tex_to_xml(input_path, output_path):
    try:
        subprocess.run(['latexml', input_path, '--destination=' + output_path], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error converting {input_path} to XML: {e}")

def parse_xml_files(input_path):
    xml_files = [file for file in os.listdir(input_path) if file.endswith('.xml')]

    parsed_data = []

    for file in xml_files:
        file_path = os.path.join(input_path, file)

        tree = ET.parse(file_path)
        root = tree.getroot()

        # Extract text, equations, and figures here using the XML structure

    return parsed_data

input_directory = '/home/jack/Documents/python/GalaxyResearch/pdf_downloads/testlatexunzipped'
latexml_directory = 'path/to/your/latexml/installation'  # Add the path to your LaTeXML installation directory here

subdirectories = [subdir for subdir in os.listdir(input_directory) if os.path.isdir(os.path.join(input_directory, subdir))]

all_parsed_data = []

for subdir in subdirectories:
    input_path = os.path.join(input_directory, subdir)
    tex_files = [file for file in os.listdir(input_path) if file.endswith('.tex')]

    for tex_file in tex_files:
        tex_file_path = os.path.join(input_path, tex_file)
        xml_file_path = os.path.join(input_path, os.path.splitext(tex_file)[0] + '.xml')

        install_missing_packages(tex_file_path)  # Install missing packages using dnf before converting
        convert_tex_to_xml(tex_file_path, xml_file_path)

    parsed_data = parse_xml_files(input_path)
    all_parsed_data.extend(parsed_data)
