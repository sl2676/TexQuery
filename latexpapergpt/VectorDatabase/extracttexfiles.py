import os
import shutil

def is_main_tex_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            if r'\begin{document}' in content and r'\end{document}' in content:
                return True
    except UnicodeDecodeError:
        print(f'Skipped non-UTF-8 encoded file: {file_path}')
        return False

    return False

def remove_specific_files(src_dir, filenames_to_remove):
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if file in filenames_to_remove:
                file_path = os.path.join(root, file)
                os.remove(file_path)
                print(f'Removed {file_path}')

def extract_tex_files(src_dir, dest_dir):
    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if file.endswith('.tex'):
                src_file = os.path.join(root, file)
                if is_main_tex_file(src_file):
                    dest_file = os.path.join(dest_dir, file)
                    
                    # Handle duplicate filenames by appending a number
                    counter = 1
                    while os.path.exists(dest_file):
                        name, ext = os.path.splitext(file)
                        dest_file = os.path.join(dest_dir, f'{name}_{counter}{ext}')
                        counter += 1

                    shutil.copy(src_file, dest_file)
                    print(f'Copied {src_file} to {dest_file}')

if __name__ == '__main__':
    source_directory = '/Users/seanly/lsearch/latexpapergpt/pdf_downloads/testlatexunzipped'
    destination_directory = '/Users/seanly/lsearch/latexpapergpt/pdf_downloads/testtexfiles'
    files_to_remove = ['mnras_guide.tex', 'mnras_template.tex', 'preamble.tex','preamble_optional.tex']

    remove_specific_files(source_directory, files_to_remove)
    extract_tex_files(source_directory, destination_directory)
