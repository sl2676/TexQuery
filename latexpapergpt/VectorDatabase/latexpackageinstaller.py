import os
import re
import requests
from shutil import copyfile

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

def install_missing_packages(tex_file, latexml_directory):
    packages = extract_packages(tex_file)
    for package in packages:
        package_path = os.path.join(latexml_directory, 'lib', 'LaTeXML', 'Package', f'{package}.sty.ltxml')
        if not os.path.exists(package_path):
            print(f'Installing package: {package}')
            install_package(package, latexml_directory)

if __name__ == '__main__':
    tex_file = 'path/to/your/tex_file.tex'
    latexml_directory = 'path/to/your/latexml/installation'
    install_missing_packages(tex_file, latexml_directory)
