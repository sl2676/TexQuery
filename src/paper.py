import openai
import os
import pinecone
import json
import numpy as np
from pinecone import Pinecone, ServerlessSpec

openai.api_key = os.getenv("OPENAI_API_KEY")
pc = Pinecone(api_key=os.getenv("PINECONE_API_KEY"))

index_name = "latex-chunks"
if index_name not in pc.list_indexes().names():
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

def normalize_vector(vector):
	norm = np.linalg.norm(vector)
	if norm == 0:
		return vector
	return vector / norm
	
def clamp_vector(vector, min_value=-0.99, max_value=0.99):
	return np.clip(vector, min_value, max_value)

def check_vector_validity(vector):
	if not np.all(np.isfinite(vector)):
		raise ValueError("The vector contains NaN or infinite values.")
	return vector

def get_gpt_embedding(text):
	response = openai.embeddings.create(
		input=text,
		model="text-embedding-ada-002"
	)
	embedding = response.data[0].embedding
	normalized_embedding = normalize_vector(embedding)

	return normalized_embedding

def store_in_pinecone(chunk_id, tag, content, embedding):
	embedding = [float(val) for val in embedding]
	index.upsert(vectors=[(chunk_id, embedding, {'tag': tag, 'content': content})])

def process_json_and_store_embeddings(json_file):
	with open(json_file, 'r') as f:
		data = json.load(f)
	for i, chunk in enumerate(data):
		tag = chunk["tag"]
		content = chunk["content"]

		print(f"Processing chunk {i}: Tag - {tag}")

		embedding = get_gpt_embedding(content)
		store_in_pinecone(str(i), tag, content, embedding)
	print("All chunks have been processed and stored in Pinecone.")

def search_pinecone(query):
	query_embedding = get_gpt_embedding(query)

	if len(query_embedding) != 1536:
		raise ValueError(f"Embedding has invalid dimension: {len(query_embedding)}. Expected 1536 dimensions.")

	print(f"Original Query Embedding: {query_embedding[:10]}...") 

	query_embedding = np.clip(query_embedding, -0.8, 0.8).tolist()
	query_embedding = [round(float(x), 6) for x in query_embedding]

	print(f"Clamped and Rounded Query Embedding: {query_embedding[:10]}...")

	if not np.all(np.isfinite(query_embedding)):
		raise ValueError("Query vector contains invalid values (NaN or infinity).")

	print(f"Final Query Embedding to be Sent: {query_embedding[:10]}...")

	results = index.query(queries=[query_embedding], top_k=5)
	print("\nSearch Results:")

	for match in results['matches']:
		score = match['score']
		tag = match['metadata']['tag']
		content = match['metadata']['content']
		print(f"Score: {score}, Tag: {tag}, Content: {content[:200]}...")

if __name__ == "__main__":
	json_file_path = 'latex_chunks.json'
	process_json_and_store_embeddings(json_file_path)
	query = "Explain LaTeX environments"
	search_pinecone(query)

