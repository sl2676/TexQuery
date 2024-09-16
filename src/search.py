import os
import pinecone
import openai
import json
from langchain.embeddings.openai import OpenAIEmbeddings  # Correct Langchain import for embeddings
from langchain.llms import OpenAI  # Ensure you have installed langchain properly

# Set OpenAI and Pinecone API keys from environment variables
openai.api_key = os.getenv("OPENAI_API_KEY")

# Initialize Pinecone
pinecone.init(api_key=os.getenv("PINECONE_API_KEY"), environment="us-east1-gcp")  # Replace 'us-east1-gcp' with your Pinecone environment
index_name = "document"

# Check if the index exists, and create it if not
if index_name not in pinecone.list_indexes():
    pinecone.create_index(index_name, dimension=1536, metric="euclidean")

# Get the index instance
index = pinecone.Index(index_name)

def load_json(filename):
    """Load the full JSON file and return the entire document."""
    with open(filename, 'r') as file:
        try:
            return json.load(file)  # Load the entire JSON document
        except json.JSONDecodeError:
            print(f"Error: Could not decode JSON from {filename}")
            return {}

def prepare_embedding_content(chunk):
    """Concatenate all relevant fields of a chunk into a single string for embedding."""
    # Prepare content string for embedding
    content_str = chunk.get('content', '')
    return content_str

def store_embeddings(document):
    """Store each chunk's embeddings and metadata into Pinecone."""
    embeddings_model = OpenAIEmbeddings()

    # Extract document-level metadata such as title and authors
    doc_title = document.get("title", "Unknown Title")
    metadata = document.get("metadata", {})
    authors = metadata.get("authors", [])
    
    # Prepare author information for metadata
    authors_info = [
        {
            "name": author.get("name", "Unknown Author"),
            "affiliations": author.get("affiliations", [])
        }
        for author in authors
    ]

    # Loop through each chunk in the document's content
    for i, chunk in enumerate(document.get("content", [])):
        # Prepare the content for embedding
        chunk_content = prepare_embedding_content(chunk)

        # Embed the content of the chunk
        embedding = embeddings_model.embed_documents([chunk_content])[0]  # Single embedding
        
        # Generate a unique vector ID for each chunk
        vector_id = f"chunk-{i+1}"
        
        # Prepare metadata for this chunk
        chunk_metadata = {
            "title": doc_title,
            "section": chunk.get("section", "Unknown Section"),
            "type": chunk.get("type", "Unknown Type"),
            "references": chunk.get("references", []),
            "authors": authors_info
        }
        
        # Upsert the vector into Pinecone with the content, references, and metadata
        index.upsert([(vector_id, embedding, chunk_metadata)])

def query_pinecone(query):
    embeddings_model = OpenAIEmbeddings()  
    query_embedding = embeddings_model.embed_query(query)  
    
    result = index.query(queries=[query_embedding], top_k=5, include_metadata=True)
    
    return result

def gpt_refined_query(query, pinecone_results):
    # Safeguard in case 'content' is not available in the metadata
    context = "\n".join([result['metadata'].get('content', 'No content available') for result in pinecone_results["matches"]])

    prompt_template = """Based on the following content:
    {context}
    Answer the following question: {query}"""

    formatted_prompt = prompt_template.format(context=context, query=query)

    # Use OpenAI for generating the refined response
    response = openai.Completion.create(
        engine="text-davinci-003",  # or another GPT-3 engine like `text-davinci-003`
        prompt=formatted_prompt,
        max_tokens=150,
        temperature=0.1
    )

    return response.choices[0].text.strip()

def main():
    json_filename = "document.json"
    document = load_json(json_filename)

    # Check if the document is valid
    if not document:
        print("Error: No valid document to process.")
        return

    # Store the embeddings into Pinecone
    store_embeddings(document)

    while True:
        user_query = input("Enter your query (or type 'quit' to exit): ")

        if user_query.lower() == 'quit':
            print("Exiting...")
            break

        pinecone_results = query_pinecone(user_query)

        refined_response = gpt_refined_query(user_query, pinecone_results)

        print("\nRefined GPT Response:\n", refined_response)
        print("\n-----------------------------\n")

if __name__ == "__main__":
    main()

