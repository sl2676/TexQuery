# TexQuery ~ LaTeX Research Paper Search Engine

## Overview
This project implements a powerful search engine for LaTeX-formatted research papers.
It leverages a suffix tree for efficient pattern matching using regex and builds a finite state machine (FSM) from the regex patterns. 
The engine is designed to chunk large research papers into manageable data segments, 
enabling high-performance search and retrieval of relevant information.

The search engine utilizes the GPT embedding API to deliver more accurate and precise responses, 
offering users highly relevant sections of research papers. This tool is especially suited for academic and research environments,
with potential to expand into additional file formats beyond LaTeX.

## Features
 - Suffix Tree-Based Search: Optimized pattern searching using suffix trees for efficient regex matching within large LaTeX documents.
 - Finite State Machine (FSM): Converts regex into an FSM to efficiently match patterns across research papers.
 - Data Chunking: Automatically breaks down LaTeX research papers into smaller data chunks to allow faster and more precise search results.
 - GPT Embedding API Integration: Uses the GPT embedding API to analyze and return accurate, contextually relevant responses from the paper.
 - LaTeX Format Support: Native support for LaTeX-formatted research papers, with future plans to expand to other formats (PDF, DOCX, etc.).
   
WIP
## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/sl2676/TexQuery.git
    cd TexQuery
    ```

2. Install the required dependencies:
    ```bash
    pip install -r requirements.txt
    ```

3. Set up your GPT API key:
    ```bash
    export OPENAI_API_KEY="your-api-key"
    ```
4. Set up your Pinecone API key:
    ```base
    export PINECONE_API_KEY="your-api-key"
    ```
   

---
WIP
## Usage

1. **Indexing LaTeX Research Papers**:
    - Place your LaTeX files in the `papers/` directory.
    - Run the indexing script to chunk and prepare the papers for searching:
      ```bash
      WIP
      ```

2. **Prompt Response**:
    - Use the prompt functionality to find accurate and precise response:
      ```bash
      WIP
      ```

3. **Analyzing Results**:
    - The search will return the matched chunks of the paper along with GPT-processed summaries for accurate responses.


