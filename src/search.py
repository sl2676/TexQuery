import os
import json
import openai
import math
import re
import pyttsx3

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
            current_chunk.pop()
            chunks.append(' '.join(current_chunk))
            current_chunk = [word]

    if current_chunk:
        chunks.append(' '.join(current_chunk))

    return chunks

def delete_all_indexes():
    """Delete all existing indexes in Pinecone."""
    indexes = pc.list_indexes().names()
    
    for index_name in indexes:
        print(f"Deleting index '{index_name}'...")
        pc.delete_index(index_name)

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

    doc_title_page = document['document'].get('title', 'Title not listed.')
    index_name = sanitize_index_name(f"index-{source_file}")
    #index_name = sanitize_index_name(f"index-{doc_title_page[0:10]}")
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

        content = section.get('content', '')
        if isinstance(content, list):
            content_strings = []
            for item in content:
                if isinstance(item, dict):
                    if item.get('type') == 'inline_math':
                        content_strings.append(f"${item.get('content', '')}$")
                    else:
                        content_strings.append(json.dumps(item))
                else:
                    content_strings.append(item)
            content = ''.join(content_strings)
        elif not isinstance(content, str):
            content = str(content)

        full_content = content.strip() + extra_content

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
    if not query.strip():
        raise ValueError("Query cannot be empty")

    query_embedding = embeddings_model.embed_query(query)

    index = pc.Index(index_name)

    response = index.query(
        vector=query_embedding,
        top_k=5,
        include_values=False,
        include_metadata=True
    )
    return response

def gpt_refined_query(query, pinecone_results, temperature=0.1):
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

    chat = ChatOpenAI(model_name='gpt-3.5-turbo', temperature=temperature)

    response = chat(messages)
    return response.content.strip()

def text_to_speech(text):
    """Convert the text to speech using pyttsx3."""
    engine = pyttsx3.init()
    engine.setProperty('rate', 200)  
    engine.say(text)
    engine.runAndWait()

def main():
    json_folder = "json_output"  

    if not os.path.exists(json_folder):
        print(f"Error: The folder '{json_folder}' does not exist.")
        return

    delete_all_indexes()

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

    current_index_name = None
    tts_enabled = False
    temperature = 0.1

    print("\nAvailable commands during query:")
    print("- Type 'quit' to exit.")
    print("- Type 'change index' to select a different index.")
    print("- Type 'toggle tts' to enable/disable text-to-speech.")
    print("- Type 'set temperature' to adjust the temperature (creativity) of the response.")
    print("- Type 'help' to display this message again.\n")

    while True:
        print(f"Current settings: Index='{current_index_name}', Text-to-Speech={'On' if tts_enabled else 'Off'}, Temperature={temperature}")
        user_query = input("Enter your query (or type a command): ").strip()
        if user_query.lower() == 'quit':
            print("Exiting...")
            break
        elif user_query.lower() == 'change index':
            current_index_name = None
            print("Index selection reset.")
            continue
        elif user_query.lower() == 'toggle tts':
            tts_enabled = not tts_enabled
            print(f"Text-to-Speech is now {'enabled' if tts_enabled else 'disabled'}.")
            continue
        elif user_query.lower() == 'set temperature':
            temp_input = input(f"Enter the desired temperature (current: {temperature}): ").strip()
            try:
                new_temperature = float(temp_input)
                if 0.0 <= new_temperature <= 1.0:
                    temperature = new_temperature
                    print(f"Temperature set to {temperature}.")
                else:
                    print("Temperature must be between 0.0 and 1.0.")
            except ValueError:
                print("Invalid input. Please enter a number between 0.0 and 1.0.")
            continue
        elif user_query.lower() == 'help':
            print("\nAvailable commands during query:")
            print("- Type 'quit' to exit.")
            print("- Type 'change index' to select a different index.")
            print("- Type 'toggle tts' to enable/disable text-to-speech.")
            print("- Type 'set temperature' to adjust the temperature (creativity) of the response.")
            print("- Type 'help' to display this message again.\n")
            continue

        while not current_index_name:
            index_input = input(f"Enter the index name to query (or type 'all' to query all indexes): ").strip()
            if index_input.lower() == 'all':
                current_index_name = 'all'
            elif index_input in index_names:
                current_index_name = index_input
            else:
                print("Invalid index name.")
                print("Available indexes to query:")
                for idx_name in index_names:
                    print(f"- {idx_name}")

        if current_index_name == 'all':
            combined_contexts = []
            for idx_name in index_names:
                pinecone_results = query_pinecone(user_query, idx_name)
                combined_contexts.extend(pinecone_results['matches'])
            pinecone_results = {'matches': combined_contexts}
        else:
            pinecone_results = query_pinecone(user_query, current_index_name)

        refined_response = gpt_refined_query(user_query, pinecone_results, temperature)

        print("\nRefined GPT Response:\n", refined_response)
        print("\n-----------------------------\n")
        if tts_enabled:
            text_to_speech(refined_response)

if __name__ == "__main__":
    main()

