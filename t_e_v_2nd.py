import speech_recognition as sr
import keyboard
import threading
from gtts import gTTS
import os
from google import genai

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

        # Gemini AI interaction
        client = genai.Client(api_key="UH NUH")
        prompt = f"""
You are a helpful assistant who responds humbly and respectfully, as if explaining to a farmer who may not be educated. Reply in Tamil.

User Command: {text}
"""
        response = client.models.generate_content(
            model="gemini-1.5-flash",
            contents=prompt,
        )
        reply = response.text
        print("🤖 Gemini Reply:", reply)

        # Speak reply in Tamil
        tts = gTTS(text=reply, lang='ta')
        tts.save("response.mp3")
        os.system("start response.mp3")  # Windows; use "afplay" on Mac, "xdg-open" on Linux

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

print("⌨️ Press SPACE to start/stop listening. Press ESC to exit.")
keyboard.add_hotkey("space", toggle_listen)
keyboard.wait("esc")
print("👋 Exiting program.")

