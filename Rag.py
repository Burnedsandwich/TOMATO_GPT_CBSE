# ===============================
# 1. Import libraries
# ===============================
import os
import numpy as np
import pickle
from sentence_transformers import SentenceTransformer
from google import genai


# ===============================
# 2. Setup Gemini Client
# ===============================
client = genai.Client(api_key="AIzaSyDUNgnzgJHRNKOIClEVw72n2zbZTtN4MqI")

# ===============================
# 3. Load Embeddings (already created)
# ===============================
embedding_file = r"C:\Users\vishw\Downloads\tomato_embeddings.pkl"

with open(embedding_file, "rb") as f:
    embeddings, chunks_store = pickle.load(f)

print(f"✅ Loaded precomputed embeddings! Total chunks: {len(embeddings)}")

# ===============================
# 4. Load same embedding model for query encoding
# ===============================
embed_model = SentenceTransformer("all-MiniLM-L6-v2")

def get_embedding(text):
    return embed_model.encode(text, convert_to_numpy=True)

# ===============================
# 5. Cosine Similarity Search
# ===============================
def cosine_similarity(a, b):
    return np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b))

def search(query, top_k=3):
    q_emb = get_embedding(query)
    sims = [cosine_similarity(q_emb, emb) for emb in embeddings]
    top_idx = np.argsort(sims)[-top_k:][::-1]
    return [(chunks_store[i][0], chunks_store[i][1], sims[i]) for i in top_idx]

# ===============================
# 6. Ask a Question with RAG
# ===============================
def ask(query):
    results = search(query, top_k=2)
    context = "\n\n".join([r[1] for r in results])

    prompt = f"""
    You are a farm assistant.
    Here are the relevant document sections pls answer them in an easy way for not educated farmers:

    {context}

    question: {query}

    Please write the answer in simple Tamil, in a way that the farmer can understand.
    """

    response = client.models.generate_content(
        model="gemini-1.5-flash",
        contents=prompt,
    )
    return response.text

# ===============================
# 7. Example Run
# ===============================
query = "What are the methods of natural fertilization for tomatoes?"
print("🤖 Gemini Answer:\n", ask(query))
