#include "pch.h"
#include "AudioCenter.h"

AudioCenter& AudioCenter::Inst()
{
	static AudioCenter inst{};
	return inst;
}
AudioCenter::AudioCenter()
{
	m_audioPlayThread = std::thread(&AudioCenter::PlayVoiceMsg, this);
	m_mute = false;
	m_voiceMsgQueue = std::queue<voiceMsg>();
}
void AudioCenter::AddVoiceMsg(std::string msg, Language lang)
{
	std::unique_lock<std::mutex> lock(_mutex);
	m_voiceMsgQueue.push(voiceMsg{msg, lang});
}
void AudioCenter::PlayVoiceMsg()
{
	int options = 0;
	Py_Initialize();

	PyObject* sysPath = PySys_GetObject("path");
	PyList_Append(sysPath, PyUnicode_FromString("pyScript\\."));
	PyObject* pName = PyUnicode_DecodeFSDefault("PyTTSModule"); // Python script name without .py
	PyObject* pModule = PyImport_Import(pName);
	Py_DECREF(pName);
	if (pModule == NULL) std::cerr << "py module is null\n";

	PyObject* initFunc = PyObject_GetAttrString(pModule, "init_tts_engine");
	PyObject* sayFunc = PyObject_GetAttrString(pModule, "say_by_engine");
	PyObject* engine = PyObject_CallObject(initFunc, nullptr);

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		while (!m_voiceMsgQueue.empty())
		{
			voiceMsg vMsg{};
			int size = 0;
			{
				std::unique_lock<std::mutex> lock(_mutex);
				vMsg = m_voiceMsgQueue.front();
				m_voiceMsgQueue.pop();
			}

			if (m_mute) continue;

			PyObject* pArgs = PyTuple_Pack(3, engine,
				PyBytes_FromStringAndSize(vMsg.msg.c_str(), vMsg.msg.size()),
				Py_BuildValue("i", (int)vMsg.lang));
			PyObject_CallObject(sayFunc, pArgs);
			PyErr_Print();
			Py_DECREF(pArgs);
		}
	}

	Py_DECREF(sysPath);
	Py_DECREF(initFunc);
	Py_DECREF(sayFunc);
	Py_DECREF(engine);
	Py_DECREF(pModule);
}