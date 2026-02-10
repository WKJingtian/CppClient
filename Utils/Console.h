#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <ostream>
#include <queue>
#include <streambuf>
#include <string>
#include <thread>

#include <windows.h>

class Console
{
public:
	static Console& Instance();
	static void Start();
	static void Stop();

	static std::ostream& Out();
	static std::ostream& Err();
	static bool ReadLine(std::string& outLine);

private:
	Console();
	~Console();

	Console(const Console&) = delete;
	Console& operator=(const Console&) = delete;

	void StartInternal();
	void StopInternal();
	bool ReadLineInternal(std::string& outLine);

	void QueueOutput(const std::string& text, bool isError);
	void QueueCommand(const std::wstring& line);

	void UiThreadMain();
	void HandleKeyEvent(const KEY_EVENT_RECORD& key);
	void DrainOutputQueue();
	void RenderOutputRegion();
	void AppendHistoryLine(const std::wstring& line, size_t& addedWrapped, size_t& removedWrapped);
	void AdjustScroll(int deltaLines);
	void ScrollToBottom();
	void RenderInputLine();
	void UpdateWindowInfo();
	void ClearLine(short row);
	void RebuildWrappedHistory();

	std::wstring ToWide(const std::string& text) const;
	std::string ToNarrow(const std::wstring& text) const;

	struct OutputChunk
	{
		std::string text;
		bool isError = false;
	};

	class ConsoleStreamBuf final : public std::streambuf
	{
	public:
		ConsoleStreamBuf(Console* console, bool isError);

	protected:
		int overflow(int ch) override;
		std::streamsize xsputn(const char* s, std::streamsize count) override;
		int sync() override;

	private:
		void FlushOnNewline();
		void FlushBuffer();

		std::mutex m_mutex;
		Console* m_console = nullptr;
		bool m_isError = false;
		std::string m_buffer;
	};

	std::mutex m_outputMutex;
	std::queue<OutputChunk> m_outputQueue;
	std::mutex m_commandMutex;
	std::condition_variable m_commandCv;
	std::queue<std::string> m_commandQueue;

	std::atomic<bool> m_running;
	std::atomic<bool> m_started;
	std::thread m_uiThread;

	HANDLE m_inHandle;
	HANDLE m_outHandle;
	HANDLE m_outputEvent;
	HANDLE m_stopEvent;
	DWORD m_originalInMode;
	DWORD m_originalOutMode;
	WORD m_defaultAttr;

	short m_windowLeft;
	short m_windowTop;
	short m_screenCols;
	short m_screenRows;
	short m_inputRow;

	std::wstring m_inputLine;
	size_t m_caretPos;
	std::wstring m_prompt;

	std::deque<std::wstring> m_rawHistory;
	std::deque<size_t> m_wrapCounts;
	std::deque<std::wstring> m_wrappedHistory;
	size_t m_maxHistoryLines;
	size_t m_scrollOffset;
	bool m_followTail;

	std::deque<std::wstring> m_cmdHistory;
	size_t m_cmdHistoryIndex;
	std::wstring m_cmdDraft;
	size_t m_maxCmdHistory;

	ConsoleStreamBuf m_outBuf;
	ConsoleStreamBuf m_errBuf;
	std::ostream m_outStream;
	std::ostream m_errStream;
};
