# ===============================
# 1. Imports
# ===============================
import os
import numpy as np
import pickle
import threading
import speech_recognition as sr
import keyboard
from gtts import gTTS
from sentence_transformers import SentenceTransformer
from google import genai

# ===============================
# 2. Setup Gemini Client
# ===============================
client = genai.Client(api_key="AIzaSyDUNgnzgJHRNKOIClEVw72n2zbZTtN4MqI")

# ===============================
# 3. Load Precomputed Embeddings
# ===============================
embedding_file = r"C:\Users\vishw\Downloads\tomato_embeddings.pkl"

with open(embedding_file, "rb") as f:
    embeddings, chunks_store = pickle.load(f)

print(f"✅ Loaded precomputed embeddings! Total chunks: {len(embeddings)}")

# ===============================
# 4. Load Embedding Model
# ===============================
embed_model = SentenceTransformer("all-MiniLM-L6-v2")

def get_embedding(text):
    return embed_model.encode(text, convert_to_numpy=True)

# ===============================
# 5. Similarity Search
# ===============================
def cosine_similarity(a, b):
    return np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b))

def search(query, top_k=3):
    q_emb = get_embedding(query)
    sims = [cosine_similarity(q_emb, emb) for emb in embeddings]
    top_idx = np.argsort(sims)[-top_k:][::-1]
    return [(chunks_store[i][0], chunks_store[i][1], sims[i]) for i in top_idx]

# ===============================
# 6. Ask (RAG + Gemini)
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
# 7. Speech Recognition + TTS
# ===============================
recognizer = sr.Recognizer()
mic = sr.Microphone()

is_listening = False
audio_data = None

def listen_and_process():
    global is_listening, audio_data
    with mic as source:
        recognizer.adjust_for_ambient_noise(source)
        print("🎙️ Listening... Press SPACE again to stop.")
        audio_data = recognizer.listen(source)
    print("🛑 Listening stopped.")
    is_listening = False
    process_audio()

def process_audio():
    global audio_data
    try:
        print("🧠 Recognizing Tamil speech...")
        text = recognizer.recognize_google(audio_data, language="ta-IN")
        print("📝 You said:", text)

        # 🔎 Run through RAG
        reply = ask(text)
        print("🤖 Gemini RAG Reply:", reply)

        # 🗣️ Speak in Tamil
        tts = gTTS(text=reply, lang='ta')
        tts.save("response.mp3")
        os.system("start response.mp3")  # Windows; "afplay" (Mac), "xdg-open" (Linux)

    except sr.UnknownValueError:
        print("❌ Could not understand audio.")
    except sr.RequestError as e:
        print(f"⚠️ API error: {e}")

def toggle_listen():
    global is_listening
    if not is_listening:
        is_listening = True
        threading.Thread(target=listen_and_process).start()
    else:
        print("Already listening... Press SPACE again to stop recording.")

# ===============================
# 8. Hotkey Control
# ===============================
print("⌨️ Press SPACE to start/stop listening. Press ESC to exit.")
keyboard.add_hotkey("space", toggle_listen)
keyboard.wait("esc")
print("👋 Exiting program.")
