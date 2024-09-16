# TexQuery

## Introduction

LaTeX documents, widely used in academia and technical writing, present unique challenges for parsing due to their complex hierarchical structure and extensive cross-referencing. Traditional parsers often fall short in capturing the depth and relationships within these documents. This project was developed to address these limitations by providing a robust, efficient way to process LaTeX files.

The goal of this parser is to:

- Extract both **hierarchical** and **non-hierarchical** relationships from LaTeX documents.
- Generate **context-aware** chunks of content enriched with metadata.
- Output data in a structured format like **JSON**, which can be used by machine learning models for more precise document analysis.

By leveraging an **Abstract Syntax Tree (AST)** for document structure, a **Directed Acyclic Graph (DAG)** for managing cross-references, and a **Finite State Machine (FSM)** to handle parsing states, this parser is designed to overcome the limitations of conventional LaTeX parsers, providing an advanced tool for document processing and AI integration.

## Overview

This repository contains a LaTeX document parser designed to generate structured, metadata-rich representations of LaTeX documents by leveraging the combined strengths of an **Abstract Syntax Tree (AST)**, **Directed Acyclic Graph (DAG)**, and **Finite State Machine (FSM)**. The parser efficiently processes hierarchical document structures and cross-referenced content, producing JSON output for machine learning models and document analysis.

- **AST (Abstract Syntax Tree):** Captures the hierarchical structure of LaTeX documents (e.g., sections, subsections) and organizes the content into a structured, tree-like format.
- **DAG (Directed Acyclic Graph):** Handles non-hierarchical relationships, such as cross-references, ensuring efficient linking without redundancy.
- **FSM (Finite State Machine):** Manages parsing states, allowing for smooth transitions between different LaTeX environments like sections, math modes, and figures.

The combined use of these components ensures robust and context-aware parsing, suitable for generating content with detailed metadata in JSON format.

## Usage
1. **Clone the repository:**

   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```
   
2. **Execution:**
   ```bash
   ./parser <input-file.tex>
   ```
3. **Run Python Script:**
   ```bash
   python search.py
   ```
4. **Query for response:**
   ```bash
    python search.py
   
    Enter your query (or type 'quit' to exit): What is this paper about?
   
    Refined GPT Response:
    GPT Response
   ```
### Example of JSON Output
```json
{
    "document": {
        "content": [
            {
                "content": "Circle Point (Small Caps)",
                "references": [],
                "section": "Lists",
                "type": "equation"
            },
            {
                "content": "",
                "references": [],
                "section": "Equations",
                "type": "text"
            },
            {
                "content": "[Binomial Theorem]\nFor any nonnegative integer $n$, we have\n$$(1+x)^n = _i=0^n n ix^i$$\n",
                "references": [],
                "section": "Binomial Theorem",
                "type": "text"
            },
            {
                "content": "The Taylor series expansion for the function $e^x$ is given by\ne^x = 1 + x + 2+ 6+ = _n0n!",
                "references": [],
                "section": "Taylor Series",
                "type": "text"
            },
            {
                "content": "For any sets $A$, $B$ and $C$, we have\n$$ (AB)-(C-A) = A (B-C)$$\n(AB)-(C-A) &=& (AB) (C-A)^c&=& (AB) (C A^c)^c &=& (AB) (C^c A) &=& A (BC^c) &=& A (B-C)\n",
                "references": [],
                "section": "Sets",
                "type": "text"
            },
            {
                "content": "l||c|rleft justified & center & right justified 1 & 3.14159 & 5 2.4678 & 3 &  1234 3.4678 & 6.14159 & 1239\n",
                "references": [],
                "section": "Tables",
                "type": "text"
            },
            {
                "content": "(100,100)(0,0)\n1pt(20,70)(20,70)*10(80,70)(80,70)*10(40,40)(1,2)10(60,40)(-1,2)10(40,40)(1,0)20(50,20)(80,10)[b](0,90)(4,0)10(1,3)4(100,90)(-4,0)10(-1,3)4",
                "references": [],
                "section": "A Picture",
                "type": "text"
            }
        ],
        "metadata": {
            "authors": ["David P. Little"]
        },
        "title": "Sample \\LaTeX ~File"
    }
}
```
