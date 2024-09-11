import sys
import requests
import os
import re
import time
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QLineEdit, QPushButton, QFileDialog, QProgressBar, QListWidget, QListWidgetItem, QComboBox, QSpinBox, QGridLayout, QMessageBox
from PyQt5.QtCore import QThread, pyqtSignal

# Set your folder path where you want to save the PDFs
output_folder = "pdf_downloads"

# Check if the folder exists, if not, create it
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

# Function to create a valid filename from the paper title
def create_filename(title):
    return re.sub(r'[\/:*?"<>|]', '', title).strip()

# Function to download a file
def download_file(url, title, output_folder, progress_callback, extension):
    response = requests.get(url, stream=True)
    total_size = int(response.headers.get("content-length", 0))
    chunk_size = 1024
    bytes_downloaded = 0

    os.makedirs(output_folder, exist_ok=True)
    file_path = os.path.join(output_folder, f"{title}{extension}")

    start_time = time.time()
    with open(file_path, "wb") as f:
        for data in response.iter_content(chunk_size):
            f.write(data)
            bytes_downloaded += len(data)

            if total_size != 0:
                progress = int(100 * bytes_downloaded / total_size)
            else:
                progress = 100
                
            elapsed_time = time.time() - start_time
            speed = (bytes_downloaded / (1024 * 1024)) / elapsed_time  # Calculate speed in MB/s
            progress_callback(progress, total_size, speed, title, extension)

# Function to scrape research papers and download them
def scrape_papers(search_query, output_folder, num_papers=10, format_choice="PDF", progress_callback=None):
    base_url = "https://arxiv.org"
    papers_downloaded = 0
    start = 0

    while papers_downloaded < num_papers:
        search_url = f"{base_url}/search/?query={search_query}&searchtype=all&size=200&start={start}"
        response = requests.get(search_url)
        soup = BeautifulSoup(response.content, 'html.parser')
        paper_links = soup.find_all('p', class_='list-title is-inline-block')
        paper_titles = soup.find_all('p', class_='title is-5 mathjax')

        if not paper_links or not paper_titles:
            break

        for i, (link, title) in enumerate(zip(paper_links, paper_titles)):
            if papers_downloaded >= num_papers:
                break

            paper_id = link.find('a')['href'].split('/')[-1]
            pdf_url = f"{base_url}/pdf/{paper_id}.pdf"
            latex_url = f"{base_url}/e-print/{paper_id}"
            title = create_filename(title.text)

            if format_choice == "PDF" or format_choice == "Both":
                download_file(pdf_url, title, output_folder, progress_callback, ".pdf")
            if format_choice == "LaTeX" or format_choice == "Both":
                latex_response = requests.head(latex_url)
                pdf_response = requests.head(pdf_url)
                if latex_response.headers.get("Content-Type") != pdf_response.headers.get("Content-Type"):
                    download_file(latex_url, title, output_folder, progress_callback, ".tar.gz")
            papers_downloaded += 1
        start += 200

class ScrapeThread(QThread):
    progress_signal = pyqtSignal(int, int, float, str, str)

    def __init__(self, search_query, output_folder, num_papers=5, format_choice="PDF"):
        super().__init__()
        self.search_query = search_query
        self.output_folder = output_folder
        self.num_papers = num_papers
        self.format_choice = format_choice

    def run(self):
        scrape_papers(self.search_query, self.output_folder, self.num_papers, self.format_choice, progress_callback=self.update_progress)

    def update_progress(self, progress, total_size, speed, title, extension):
        self.progress_signal.emit(progress, total_size, speed, title, extension)



class ScraperGUI(QWidget):
    def __init__(self):
        super().__init__()

        self.output_folder = "downloads"
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("Arxiv PDF Scraper")

        layout = QGridLayout()

        self.search_label = QLabel("Search Query:")
        layout.addWidget(self.search_label, 0, 0)

        self.search_input = QLineEdit()
        layout.addWidget(self.search_input, 0, 1)

        self.choose_folder_button = QPushButton("Choose Output Folder")
        layout.addWidget(self.choose_folder_button, 1, 0)

        self.output_folder_label = QLabel("Output Folder: None")
        layout.addWidget(self.output_folder_label, 1, 1)

        self.format_label = QLabel("File Format:")
        layout.addWidget(self.format_label, 2, 0)

        self.format_dropdown = QComboBox()
        self.format_dropdown.addItem("PDF")
        self.format_dropdown.addItem("LaTeX")
        self.format_dropdown.addItem("Both")
        layout.addWidget(self.format_dropdown, 2, 1)

        self.num_papers_label = QLabel("# of Papers:")
        layout.addWidget(self.num_papers_label, 3, 0)

        self.num_papers_spinbox = QSpinBox()
        self.num_papers_spinbox.setMinimum(1)
        self.num_papers_spinbox.setMaximum(5000)
        self.num_papers_spinbox.setValue(5)
        layout.addWidget(self.num_papers_spinbox, 3, 1)
        
        self.progress_info_label = QLabel("")
        layout.addWidget(self.progress_info_label, 8, 0, 1, 2)
        
        self.start_scraping_button = QPushButton("Start Scraping")
        layout.addWidget(self.start_scraping_button, 4, 0, 1, 2)

        self.progress_bar = QProgressBar()
        layout.addWidget(self.progress_bar, 5, 0, 1, 2)

        self.downloaded_files_label = QLabel("Downloaded Files:")
        layout.addWidget(self.downloaded_files_label, 6, 0)

        self.downloaded_files_list = QListWidget()
        layout.addWidget(self.downloaded_files_list, 7, 0, 1, 2)

        self.setLayout(layout)
        self.init_connections()
        
    def init_connections(self):
        self.choose_folder_button.clicked.connect(self.choose_folder)
        self.start_scraping_button.clicked.connect(self.start_scraping)

    def choose_folder(self):
        self.output_folder = QFileDialog.getExistingDirectory(self, "Select Output Folder")
        if self.output_folder:
            self.output_folder_label.setText(f"Output Folder: {self.output_folder}")
            self.downloaded_files_list.clear()

    def start_scraping(self):
        search_query = self.search_input.text()
        num_papers = self.num_papers_spinbox.value()
        format_choice = self.format_dropdown.currentText()

        if not search_query or not self.output_folder:
            QMessageBox.warning(self, "Error", "Please input a search query and choose an output folder.")
            return

        self.start_scraping_button.setEnabled(False)
        self.scrape_thread = ScrapeThread(search_query, self.output_folder, num_papers=num_papers, format_choice=format_choice)
        self.scrape_thread.progress_signal.connect(self.update_progress)
        self.scrape_thread.finished.connect(self.scraping_finished)
        self.scrape_thread.start()


    def update_progress(self, progress, total_size, speed, title, extension):
        self.progress_bar.setValue(progress)
        self.progress_info_label.setText(f"Downloading '{title}{extension}': {progress}% | File Size: {total_size // (1024 * 1024)} MB | Speed: {speed:.2f} MB/s")
        if progress == 100:
            item = QListWidgetItem(f"{title}{extension} ({total_size // (1024 * 1024)} MB)")
            self.downloaded_files_list.addItem(item)


    def scraping_finished(self):
        self.start_scraping_button.setEnabled(True)
        QMessageBox.information(self, "Scraping Completed", "Scraping has finished.")



app = QApplication(sys.argv)
scraper_gui = ScraperGUI()
scraper_gui.show()
sys.exit(app.exec_())
