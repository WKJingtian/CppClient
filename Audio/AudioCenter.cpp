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
	PyConfig config;
	PyConfig_InitIsolatedConfig(&config);
	PyConfig_SetString(&config, &config.executable, L".\\Python\\embed\\python.exe");
	PyConfig_SetString(&config, &config.home, L".\\Python\\embed");
	config.site_import = 1;

	// 显式设置搜索路径
	config.module_search_paths_set = 1;
	PyWideStringList_Append(&config.module_search_paths, L".\\Python\\embed");
	PyWideStringList_Append(&config.module_search_paths, L".\\Python\\embed\\python314.zip");          // 标准库
	PyWideStringList_Append(&config.module_search_paths, L".\\Python\\embed\\Lib\\site-packages");     // 你的第三方库

	auto status = Py_InitializeFromConfig(&config);
	PyConfig_Clear(&config);

	if (PyStatus_Exception(status)) {
		Console::Err() << "embeded python init failed\n";
		PyErr_Print();
		Py_ExitStatusException(status);
	}

	PyObject* sysPath = PySys_GetObject("path");
	PyList_Append(sysPath, PyUnicode_FromString("pyScript\\."));
	PyObject* pName = PyUnicode_DecodeFSDefault("PyTTSModule"); // Python script name without .py
	PyObject* pModule = PyImport_Import(pName);
	if (pModule == NULL)
	{
		Console::Err() << "py module is null\n";
		PyErr_Print();
	}
	Py_DECREF(pName);

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
