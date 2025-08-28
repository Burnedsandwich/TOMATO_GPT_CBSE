# TOMATO_GPT_CBSE
it is made by M.vishwajith for a CBSE project


sudo apt update && sudo apt upgrade -y

mkdir ~/farm-assistant
cd ~/farm-assistant

python3 -m venv venv


source venv/bin/activate

pip install --upgrade pip
pip install numpy pickle-mixin sentence-transformers google-genai gTTS SpeechRecognition keyboard pyaudio


sudo apt install portaudio19-dev
pip install pyaudio
