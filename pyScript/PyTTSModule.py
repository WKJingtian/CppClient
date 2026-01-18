import pyttsx3 as pytts

def set_chinese_voice(engine):
    voices = engine.getProperty("voices")
    for voice in voices:
        if "zh-CN" in voice.languages:
            engine.setProperty("voice", voice.id)
            return
        if "Chinese" in voice.name or "Mandarin" in voice.name.title():
            engine.setProperty("voice", voice.id)
            return
    raise KeyError(f"No Chinese voice found among {voices}")

def set_english_voice(engine):
    voices = engine.getProperty("voices")
    for voice in voices:
        if "en" in voice.languages:
            engine.setProperty("voice", voice.id)
            return
        if "English" in voice.name or "English" in voice.name.title():
            engine.setProperty("voice", voice.id)
            return
    raise KeyError(f"No English voice found among {voices}")

def say_by_engine(engine, toSay, lang):
    utf8_string = toSay.decode('gb2312')
    if lang == 0:
        set_english_voice(engine)
    elif lang == 1:
        set_chinese_voice(engine)
    engine.say(utf8_string)
    engine.runAndWait()

def init_tts_engine():
    return pytts.init()
    
def debug_print(toPrint):
    print(toPrint)