import os
import pinecone
from pinecone import Pinecone, ServerlessSpec
import openai
import json
from langchain_openai import OpenAIEmbeddings, OpenAI 

openai.api_key = os.getenv("OPENAI_API_KEY")
pc = Pinecone(api_key=os.getenv("PINECONE_API_KEY"))

index_name = "latex-chunks"
if index_name not in pc.list_indexes().names():
    pc.create_index(
        name=index_name, 
        dimension=1536,  
        metric='euclidean',
        spec=ServerlessSpec(
            cloud='aws',
            region='us-east-1'  
        )
    )
index = pc.Index(index_name)

def load_json_chunks(filename):
    with open(filename, 'r') as file:
        return json.load(file)


def store_embeddings(json_chunks):
    embeddings_model = OpenAIEmbeddings()
    
    for i, chunk in enumerate(json_chunks):
        content = chunk["content"]
        embedding = embeddings_model.embed_documents([content])  
        
        embedding = embedding[0]  

        vector_id = f"chunk-{i}"
        
        index.upsert([(vector_id, embedding, {"content": content})])


def query_pinecone(query):
    embeddings_model = OpenAIEmbeddings()  
    query_embedding = embeddings_model.embed_query(query)  
    
    result = index.query(vector=query_embedding, top_k=5, include_metadata=True)
    
    return result

def gpt_refined_query(query, pinecone_results):
    context = "\n".join([result['metadata']['content'] for result in pinecone_results["matches"]])

    prompt_template = """Based on the following LaTeX chunks:
    {context}
    Answer the following question: {query}"""

    formatted_prompt = prompt_template.format(context=context, query=query)

    llm = OpenAI(temperature=0.1)  

    response = llm(formatted_prompt)

    return response

def main():
    json_filename = "latex_chunks.json"
    latex_chunks = load_json_chunks(json_filename)

    store_embeddings(latex_chunks)

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

