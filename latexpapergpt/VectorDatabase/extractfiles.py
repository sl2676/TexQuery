import os
import tarfile

def extract_files(archive_path, output_path):
    with tarfile.open(archive_path, 'r:gz') as tar:
        tar.extractall(output_path)

archive_directory = '/Users/seanly/lsearch/latexpapergpt/pdf_downloads/test_latex'
output_directory = '/Users/seanly/lsearch/latexpapergpt/pdf_downloads/testlatexunzipped'

# List all tar.gz files in the archive directory
tar_gz_files = [file for file in os.listdir(archive_directory) if file.endswith('.tar.gz')]

# Iterate through all tar.gz files and extract them to their respective folders
for file in tar_gz_files:
    file_name = os.path.splitext(os.path.splitext(file)[0])[0]  # Remove .tar.gz extension
    output_path = os.path.join(output_directory, file_name)
    
    # Create output folder if it doesn't exist
    if not os.path.exists(output_path):
        os.makedirs(output_path)
    
    archive_path = os.path.join(archive_directory, file)
    extract_files(archive_path, output_path)
