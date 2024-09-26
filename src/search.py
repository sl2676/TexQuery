import os
import json
import openai
import math
import re

from langchain.embeddings import OpenAIEmbeddings
from langchain.chat_models import ChatOpenAI
from langchain.schema import SystemMessage, HumanMessage
from pinecone import Pinecone, ServerlessSpec

openai.api_key = os.getenv("OPENAI_API_KEY") or 'your-openai-api-key'
pinecone_api_key = os.getenv("PINECONE_API_KEY") or 'your-pinecone-api-key'

pc = Pinecone(
    api_key=pinecone_api_key
)

def load_json(filename):
    """Load the JSON document."""
    with open(filename, 'r', encoding='utf-8') as file:
        try:
            return json.load(file)
        except json.JSONDecodeError as e:
            print(f"Error: Could not decode JSON from {filename}: {e}")
            return {}

def split_text(text, max_size):
    """Split text into chunks under the max_size limit."""
    words = text.split()
    chunks = []
    current_chunk = []

    for word in words:
        current_chunk.append(word)
        if len(' '.join(current_chunk).encode('utf-8')) > max_size:
            # Remove the last word to keep under limit
            current_chunk.pop()
            chunks.append(' '.join(current_chunk))
            current_chunk = [word]

    if current_chunk:
        chunks.append(' '.join(current_chunk))

    return chunks

def sanitize_index_name(name):
    """Sanitize index name to comply with Pinecone naming conventions."""
    name = name.lower()
    name = re.sub(r'[^a-z0-9-]+', '-', name)
    name = name.strip('-')
    if not name:
        name = 'default-index'
    return name

def store_embeddings(document, source_file):
    """Store structured vectors in Pinecone based on the document's components."""
    embeddings_model = OpenAIEmbeddings()

    index_name = sanitize_index_name(f"index-{source_file}")

    indexes = pc.list_indexes().names()
    if index_name not in indexes:
        pc.create_index(
            name=index_name,
            dimension=1536, 
            metric='cosine',
            spec=ServerlessSpec(
                cloud='aws',
                region='us-east-1'  
            )
        )

    index = pc.Index(index_name)

    vectors_to_upsert = []
    
    doc_title = document['document'].get('title', 'Title not listed.')
    doc_metadata = document['document'].get('metadata', {})
    doc_authors = doc_metadata.get('authors', [])

    authors = []
    affiliations = []
    for author in doc_authors:
        name = author.get('name', '')
        author_affiliations = author.get('affiliations', [])
        authors.append(name)
        affiliations.extend(author_affiliations)

    affiliations = list(set(affiliations))
    max_metadata_size = 40000  

    vector_count = 0
    for idx, section in enumerate(document['document'].get('content', [])):
        section_title = section.get('section', f'Section {idx+1}')
        section_type = section.get('type', '')
        section_references = section.get('references', [])
        figure_caption = section.get('figure_caption', '')
        image_file = section.get('image_file', '')

        references = [ref.get('label', '') for ref in section_references]

        extra_content = ""
        if figure_caption:
            extra_content += f"\nFigure Caption: {figure_caption}"
        if image_file:
            extra_content += f"\nImage File: {image_file}"

        full_content = section.get('content', '').strip() + extra_content

        content_chunks = split_text(full_content, max_metadata_size)

        for chunk_idx, chunk in enumerate(content_chunks):
            vector_count += 1
            section_id = f"{source_file}_section_{idx+1}_chunk_{chunk_idx+1}"

            metadata = {
                'source_file': source_file,  
                'document_title': doc_title,
                'authors': authors,
                'affiliations': affiliations,
                'section_title': section_title,
                'section_type': section_type,
                'references': references,
                'figure_caption': figure_caption,
                'image_file': image_file,
                'content': chunk
            }

            metadata_size = len(json.dumps(metadata).encode('utf-8'))
            if metadata_size > 40960:
                print(f"Warning: Metadata size for vector {section_id} exceeds limit after splitting.")
                continue  

            embedding = embeddings_model.embed_query(chunk)

            vectors_to_upsert.append({
                'id': section_id,
                'values': embedding,
                'metadata': metadata
            })

            if len(vectors_to_upsert) >= 100:
                index.upsert(vectors=vectors_to_upsert)
                vectors_to_upsert = []

    if vectors_to_upsert:
        index.upsert(vectors=vectors_to_upsert)

    print(f"Successfully stored {vector_count} vectors from {source_file} in Pinecone index '{index_name}'.")

def query_pinecone(query, index_name):
    """Query Pinecone for relevant content based on user query."""
    embeddings_model = OpenAIEmbeddings()
    query_embedding = embeddings_model.embed_query(query)

    index = pc.Index(index_name)

    response = index.query(
        vector=query_embedding,
        top_k=5,
        include_values=False,
        include_metadata=True
    )
    return response

def gpt_refined_query(query, pinecone_results):
    """Refine the query response using GPT."""
    contexts = []
    for match in pinecone_results['matches']:
        metadata = match['metadata']
        source_file = metadata.get('source_file', 'Unknown Source')
        section_title = metadata.get('section_title', 'No Title')
        content = metadata.get('content', 'No Content')
        authors = metadata.get('authors', [])
        affiliations = metadata.get('affiliations', [])
        references = metadata.get('references', [])

        context_parts = [
            f"### Source: {source_file}",
            f"Section Title: {section_title}",
            f"Content: {content}",
            f"Authors: {', '.join(authors)}",
            f"Affiliations: {', '.join(affiliations)}",
            f"References: {', '.join(references)}"
        ]
        context = "\n".join([part for part in context_parts if part])
        contexts.append(context)

    context_text = "\n\n".join(contexts)

    messages = [
        SystemMessage(content="You are an AI assistant helping to answer questions based on the provided context. Use the context to answer the question thoroughly, including any relevant affiliations and references."),
        HumanMessage(content=f"Context:\n{context_text}\n\nQuestion:\n{query}\n\nAnswer:")
    ]

    chat = ChatOpenAI(model_name='gpt-3.5-turbo', temperature=0.1)

    response = chat(messages)
    return response.content.strip()

def main():
    json_folder = "json_output"  

    if not os.path.exists(json_folder):
        print(f"Error: The folder '{json_folder}' does not exist.")
        return

    index_names = []
    for filename in os.listdir(json_folder):
        if filename.endswith('.json'):
            filepath = os.path.join(json_folder, filename)
            document = load_json(filepath)

            if not document:
                print(f"Error: No valid document to process in {filename}.")
                continue

            source_file = os.path.splitext(filename)[0]

            store_embeddings(document, source_file)

            index_name = sanitize_index_name(f"index-{source_file}")
            index_names.append(index_name)

    if not index_names:
        print("No indexes to query.")
        return

    print("Available indexes to query:")
    for idx_name in index_names:
        print(f"- {idx_name}")

    while True:
        user_query = input("Enter your query (or type 'quit' to exit): ")
        if user_query.lower() == 'quit':
            print("Exiting...")
            break

        index_name = input(f"Enter the index name to query (or type 'all' to query all indexes): ")

        if index_name.lower() == 'all':
            combined_contexts = []
            for idx_name in index_names:
                pinecone_results = query_pinecone(user_query, idx_name)
                combined_contexts.extend(pinecone_results['matches'])
            pinecone_results = {'matches': combined_contexts}
        elif index_name in index_names:
            pinecone_results = query_pinecone(user_query, index_name)
        else:
            print("Invalid index name.")
            continue

        refined_response = gpt_refined_query(user_query, pinecone_results)

        print("\nRefined GPT Response:\n", refined_response)
        print("\n-----------------------------\n")

if __name__ == "__main__":
    main()

