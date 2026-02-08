#include "pch.h"
#include "Utils/Console.h"

namespace
{
	constexpr const char* kPromptA = "> ";
}

Console& Console::Instance()
{
	static Console inst;
	return inst;
}

void Console::Start()
{
	Instance().StartInternal();
}

void Console::Stop()
{
	Instance().StopInternal();
}

std::ostream& Console::Out()
{
	return Instance().m_outStream;
}

std::ostream& Console::Err()
{
	return Instance().m_errStream;
}

bool Console::ReadLine(std::string& outLine)
{
	return Instance().ReadLineInternal(outLine);
}

Console::Console()
	: m_running(false),
	m_started(false),
	m_inHandle(INVALID_HANDLE_VALUE),
	m_outHandle(INVALID_HANDLE_VALUE),
	m_outputEvent(nullptr),
	m_stopEvent(nullptr),
	m_originalInMode(0),
	m_originalOutMode(0),
	m_defaultAttr(0),
	m_windowLeft(0),
	m_windowTop(0),
	m_screenCols(0),
	m_screenRows(0),
	m_inputRow(0),
	m_caretPos(0),
	m_prompt(L"> "),
	m_maxHistoryLines(1000),
	m_scrollOffset(0),
	m_followTail(true),
	m_outBuf(this, false),
	m_errBuf(this, true),
	m_outStream(&m_outBuf),
	m_errStream(&m_errBuf)
{
}

Console::~Console()
{
	StopInternal();
}

void Console::StartInternal()
{
	if (m_started.load())
		return;

	m_inHandle = GetStdHandle(STD_INPUT_HANDLE);
	m_outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (m_inHandle == INVALID_HANDLE_VALUE || m_outHandle == INVALID_HANDLE_VALUE)
		return;

	GetConsoleMode(m_inHandle, &m_originalInMode);
	GetConsoleMode(m_outHandle, &m_originalOutMode);

	DWORD inMode = m_originalInMode;
	inMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_QUICK_EDIT_MODE);
	inMode |= ENABLE_EXTENDED_FLAGS | ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT;
	inMode |= ENABLE_MOUSE_INPUT;
	SetConsoleMode(m_inHandle, inMode);

	UpdateWindowInfo();

	m_outputEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	m_running = true;
	m_started = true;
	m_uiThread = std::thread(&Console::UiThreadMain, this);
}

void Console::StopInternal()
{
	if (!m_started.load())
		return;

	m_running = false;
	m_commandCv.notify_all();

	if (m_stopEvent)
		SetEvent(m_stopEvent);
	if (m_outputEvent)
		SetEvent(m_outputEvent);

	if (m_uiThread.joinable())
		m_uiThread.join();

	m_started = false;

	if (m_outputEvent)
	{
		CloseHandle(m_outputEvent);
		m_outputEvent = nullptr;
	}
	if (m_stopEvent)
	{
		CloseHandle(m_stopEvent);
		m_stopEvent = nullptr;
	}

	if (m_inHandle != INVALID_HANDLE_VALUE)
		SetConsoleMode(m_inHandle, m_originalInMode);
	if (m_outHandle != INVALID_HANDLE_VALUE)
		SetConsoleMode(m_outHandle, m_originalOutMode);
}

bool Console::ReadLineInternal(std::string& outLine)
{
	std::unique_lock<std::mutex> lock(m_commandMutex);
	m_commandCv.wait(lock, [this]() {
		return !m_commandQueue.empty() || !m_running.load();
		});

	if (m_commandQueue.empty())
		return false;

	outLine = std::move(m_commandQueue.front());
	m_commandQueue.pop();
	return true;
}

void Console::QueueOutput(const std::string& text, bool isError)
{
	if (!m_started.load())
	{
		DWORD written = 0;
		HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
		if (out != INVALID_HANDLE_VALUE)
			WriteConsoleA(out, text.c_str(), (DWORD)text.size(), &written, nullptr);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_outputMutex);
		m_outputQueue.push(OutputChunk{ text, isError });
	}
	if (m_outputEvent)
		SetEvent(m_outputEvent);
}

void Console::QueueCommand(const std::wstring& line)
{
	std::string narrow = ToNarrow(line);
	QueueOutput(std::string(kPromptA) + narrow + "\n", false);

	{
		std::lock_guard<std::mutex> lock(m_commandMutex);
		m_commandQueue.push(std::move(narrow));
	}
	m_commandCv.notify_one();
}

void Console::UiThreadMain()
{
	RenderOutputRegion();
	RenderInputLine();

	HANDLE handles[3] = { m_inHandle, m_outputEvent, m_stopEvent };
	while (m_running.load())
	{
		bool stopRequested = false;
		DWORD wait = WaitForMultipleObjects(3, handles, FALSE, INFINITE);
		if (wait == WAIT_OBJECT_0)
		{
			INPUT_RECORD records[32];
			DWORD count = 0;
			if (ReadConsoleInput(m_inHandle, records, 32, &count))
			{
				for (DWORD i = 0; i < count; ++i)
				{
					if (records[i].EventType == KEY_EVENT)
					{
						HandleKeyEvent(records[i].Event.KeyEvent);
					}
					else if (records[i].EventType == WINDOW_BUFFER_SIZE_EVENT)
					{
						UpdateWindowInfo();
						RenderOutputRegion();
						RenderInputLine();
					}
					else if (records[i].EventType == MOUSE_EVENT)
					{
						const auto& mouse = records[i].Event.MouseEvent;
						if (mouse.dwEventFlags == MOUSE_WHEELED)
						{
							SHORT delta = (SHORT)HIWORD(mouse.dwButtonState);
							int steps = delta / WHEEL_DELTA;
							if (steps == 0)
								steps = (delta > 0) ? 1 : -1;
							AdjustScroll(steps * 3);
						}
					}
				}
			}
		}
		else if (wait == WAIT_OBJECT_0 + 1)
		{
			if (m_outputEvent)
				ResetEvent(m_outputEvent);
		}
		else if (wait == WAIT_OBJECT_0 + 2)
		{
			stopRequested = true;
		}

		DrainOutputQueue();
		if (stopRequested)
			break;
	}
}

void Console::HandleKeyEvent(const KEY_EVENT_RECORD& key)
{
	if (!key.bKeyDown)
		return;

	switch (key.wVirtualKeyCode)
	{
	case VK_BACK:
		if (m_caretPos > 0)
		{
			m_inputLine.erase(m_caretPos - 1, 1);
			--m_caretPos;
		}
		break;
	case VK_DELETE:
		if (m_caretPos < m_inputLine.size())
			m_inputLine.erase(m_caretPos, 1);
		break;
	case VK_LEFT:
		if (m_caretPos > 0)
			--m_caretPos;
		break;
	case VK_RIGHT:
		if (m_caretPos < m_inputLine.size())
			++m_caretPos;
		break;
	case VK_RETURN:
	{
		std::wstring line = m_inputLine;
		m_inputLine.clear();
		m_caretPos = 0;
		QueueCommand(line);
		break;
	}
	case VK_PRIOR:
		AdjustScroll(m_screenRows > 0 ? (m_screenRows - 1) : 10);
		return;
	case VK_NEXT:
		AdjustScroll(m_screenRows > 0 ? -(m_screenRows - 1) : -10);
		return;
	case VK_HOME:
		AdjustScroll(-(int)m_maxHistoryLines);
		break;
	case VK_END:
		ScrollToBottom();
		return;
	default:
		if (key.uChar.UnicodeChar >= 0x20)
		{
			m_inputLine.insert(m_caretPos, 1, key.uChar.UnicodeChar);
			++m_caretPos;
		}
		break;
	}

	RenderInputLine();
}

void Console::DrainOutputQueue()
{
	std::queue<OutputChunk> local;
	{
		std::lock_guard<std::mutex> lock(m_outputMutex);
		if (m_outputQueue.empty())
			return;
		std::swap(local, m_outputQueue);
	}

	size_t addedWrapped = 0;
	size_t removedWrapped = 0;

	while (!local.empty())
	{
		auto chunk = std::move(local.front());
		local.pop();

		std::wstring wide = ToWide(chunk.text);
		size_t start = 0;
		while (start <= wide.size())
		{
			size_t pos = wide.find(L'\n', start);
			std::wstring line;
			if (pos == std::wstring::npos)
			{
				line = wide.substr(start);
				start = wide.size() + 1;
			}
			else
			{
				line = wide.substr(start, pos - start);
				start = pos + 1;
			}
			if (!line.empty() && line.back() == L'\r')
				line.pop_back();
			AppendHistoryLine(line, addedWrapped, removedWrapped);
			if (pos == std::wstring::npos)
				break;
		}
	}

	if (!m_followTail)
	{
		if (addedWrapped > 0)
			m_scrollOffset += addedWrapped;
		if (removedWrapped > 0)
		{
			if (removedWrapped >= m_scrollOffset)
				m_scrollOffset = 0;
			else
				m_scrollOffset -= removedWrapped;
		}
	}
	if (m_scrollOffset == 0)
		m_followTail = true;
	RenderOutputRegion();
	RenderInputLine();
}

void Console::RenderOutputRegion()
{
	if (m_screenRows <= 1 || m_screenCols <= 0)
		return;

	short outputRows = (short)(m_screenRows - 1);
	size_t total = m_wrappedHistory.size();
	size_t maxOffset = 0;
	if (total > (size_t)outputRows)
		maxOffset = total - outputRows;

	if (m_followTail)
		m_scrollOffset = 0;
	if (m_scrollOffset > maxOffset)
		m_scrollOffset = maxOffset;

	size_t startIndex = 0;
	if (total > (size_t)outputRows)
		startIndex = total - outputRows - m_scrollOffset;

	for (short row = 0; row < outputRows; ++row)
	{
		short targetRow = (short)(m_windowTop + row);
		ClearLine(targetRow);

		size_t idx = startIndex + (size_t)row;
		if (idx >= total)
			continue;

		const std::wstring& line = m_wrappedHistory[idx];
		if (line.empty())
			continue;

		std::wstring trimmed = line;
		if (trimmed.size() > (size_t)m_screenCols)
			trimmed = trimmed.substr(0, m_screenCols);

		COORD pos = { m_windowLeft, targetRow };
		DWORD written = 0;
		WriteConsoleOutputCharacterW(m_outHandle, trimmed.c_str(), (DWORD)trimmed.size(), pos, &written);
	}
}

void Console::RenderInputLine()
{
	if (m_screenCols <= 0)
		return;

	ClearLine(m_inputRow);

	std::wstring full = m_prompt + m_inputLine;
	size_t totalLen = full.size();
	size_t start = 0;
	if (totalLen > (size_t)m_screenCols)
		start = totalLen - m_screenCols;
	std::wstring visible = full.substr(start, m_screenCols);

	COORD pos = { m_windowLeft, m_inputRow };
	DWORD written = 0;
	if (!visible.empty())
		WriteConsoleOutputCharacterW(m_outHandle, visible.c_str(), (DWORD)visible.size(), pos, &written);

	size_t caret = m_prompt.size() + m_caretPos;
	if (caret < start)
		caret = 0;
	else
		caret -= start;
	if (caret >= (size_t)m_screenCols)
		caret = (size_t)(m_screenCols - 1);

	COORD caretPos = { (SHORT)(m_windowLeft + caret), m_inputRow };
	SetConsoleCursorPosition(m_outHandle, caretPos);
}

void Console::UpdateWindowInfo()
{
	short oldCols = m_screenCols;
	CONSOLE_SCREEN_BUFFER_INFO info{};
	if (!GetConsoleScreenBufferInfo(m_outHandle, &info))
		return;

	m_defaultAttr = info.wAttributes;
	m_windowLeft = info.srWindow.Left;
	m_windowTop = info.srWindow.Top;
	m_screenCols = (short)(info.srWindow.Right - info.srWindow.Left + 1);
	m_screenRows = (short)(info.srWindow.Bottom - info.srWindow.Top + 1);
	m_inputRow = (short)(m_windowTop + m_screenRows - 1);

	if (oldCols != m_screenCols)
		RebuildWrappedHistory();
}

void Console::ClearLine(short row)
{
	if (m_screenCols <= 0)
		return;

	COORD pos = { m_windowLeft, row };
	DWORD written = 0;
	FillConsoleOutputCharacterW(m_outHandle, L' ', m_screenCols, pos, &written);
	FillConsoleOutputAttribute(m_outHandle, m_defaultAttr, m_screenCols, pos, &written);
}

void Console::AppendHistoryLine(const std::wstring& line, size_t& addedWrapped, size_t& removedWrapped)
{
	m_rawHistory.push_back(line);

	size_t wrapCount = 0;
	size_t width = m_screenCols > 0 ? (size_t)m_screenCols : 1;
	if (line.empty())
	{
		m_wrappedHistory.push_back(L"");
		wrapCount = 1;
	}
	else
	{
		size_t pos = 0;
		while (pos < line.size())
		{
			std::wstring segment = line.substr(pos, width);
			m_wrappedHistory.push_back(segment);
			pos += segment.size();
			wrapCount++;
		}
	}
	m_wrapCounts.push_back(wrapCount);
	addedWrapped += wrapCount;

	while (m_rawHistory.size() > m_maxHistoryLines && !m_wrapCounts.empty())
	{
		m_rawHistory.pop_front();
		size_t toRemove = m_wrapCounts.front();
		m_wrapCounts.pop_front();
		removedWrapped += toRemove;

		for (size_t i = 0; i < toRemove && !m_wrappedHistory.empty(); ++i)
			m_wrappedHistory.pop_front();
	}
}

void Console::AdjustScroll(int deltaLines)
{
	if (m_screenRows <= 1)
		return;

	short outputRows = (short)(m_screenRows - 1);
	size_t total = m_wrappedHistory.size();
	size_t maxOffset = 0;
	if (total > (size_t)outputRows)
		maxOffset = total - outputRows;

	if (deltaLines > 0)
	{
		m_scrollOffset = (m_scrollOffset + (size_t)deltaLines > maxOffset)
			? maxOffset
			: m_scrollOffset + (size_t)deltaLines;
	}
	else if (deltaLines < 0)
	{
		size_t down = (size_t)(-deltaLines);
		if (down > m_scrollOffset)
			m_scrollOffset = 0;
		else
			m_scrollOffset -= down;
	}
	m_followTail = (m_scrollOffset == 0);

	RenderOutputRegion();
	RenderInputLine();
}

void Console::ScrollToBottom()
{
	m_scrollOffset = 0;
	m_followTail = true;
	RenderOutputRegion();
	RenderInputLine();
}

void Console::RebuildWrappedHistory()
{
	m_wrappedHistory.clear();
	m_wrapCounts.clear();

	for (const auto& line : m_rawHistory)
	{
		size_t wrapCount = 0;
		size_t width = m_screenCols > 0 ? (size_t)m_screenCols : 1;
		if (line.empty())
		{
			m_wrappedHistory.push_back(L"");
			wrapCount = 1;
		}
		else
		{
			size_t pos = 0;
			while (pos < line.size())
			{
				std::wstring segment = line.substr(pos, width);
				m_wrappedHistory.push_back(segment);
				pos += segment.size();
				wrapCount++;
			}
		}
		m_wrapCounts.push_back(wrapCount);
	}
	if (m_followTail)
		m_scrollOffset = 0;
}

std::wstring Console::ToWide(const std::string& text) const
{
	if (text.empty())
		return L"";

	UINT cp = GetConsoleOutputCP();
	int size = MultiByteToWideChar(cp, 0, text.data(), (int)text.size(), nullptr, 0);
	if (size <= 0)
		return L"";

	std::wstring result(size, L'\0');
	MultiByteToWideChar(cp, 0, text.data(), (int)text.size(), result.data(), size);
	return result;
}

std::string Console::ToNarrow(const std::wstring& text) const
{
	if (text.empty())
		return "";

	UINT cp = GetConsoleCP();
	int size = WideCharToMultiByte(cp, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		return "";

	std::string result(size, '\0');
	WideCharToMultiByte(cp, 0, text.data(), (int)text.size(), result.data(), size, nullptr, nullptr);
	return result;
}

Console::ConsoleStreamBuf::ConsoleStreamBuf(Console* console, bool isError)
	: m_console(console),
	m_isError(isError)
{
}

int Console::ConsoleStreamBuf::overflow(int ch)
{
	if (ch == traits_type::eof())
		return traits_type::not_eof(ch);

	std::lock_guard<std::mutex> lock(m_mutex);
	m_buffer.push_back(static_cast<char>(ch));
	if (ch == '\n')
		FlushOnNewline();
	return ch;
}

std::streamsize Console::ConsoleStreamBuf::xsputn(const char* s, std::streamsize count)
{
	if (count <= 0)
		return 0;

	std::lock_guard<std::mutex> lock(m_mutex);
	m_buffer.append(s, (size_t)count);
	FlushOnNewline();
	return count;
}

int Console::ConsoleStreamBuf::sync()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	FlushBuffer();
	return 0;
}

void Console::ConsoleStreamBuf::FlushOnNewline()
{
	if (!m_console)
		return;

	size_t pos = 0;
	while ((pos = m_buffer.find('\n')) != std::string::npos)
	{
		std::string chunk = m_buffer.substr(0, pos + 1);
		m_console->QueueOutput(chunk, m_isError);
		m_buffer.erase(0, pos + 1);
	}
}

void Console::ConsoleStreamBuf::FlushBuffer()
{
	if (!m_console || m_buffer.empty())
		return;

	m_console->QueueOutput(m_buffer, m_isError);
	m_buffer.clear();
}
