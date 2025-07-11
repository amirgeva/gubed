#ifdef _WIN32

#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "conwin.h"
#include "types.h"

#include "console.h"
#include <Windows.h>
#include <unordered_map>

// Static map to convert Windows virtual key codes to our Key enum
static const std::unordered_map<WORD, Key> virtualKeyToKeyMap = {
    {VK_UP, Key::Up},
    {VK_DOWN, Key::Down},
    {VK_LEFT, Key::Left},
    {VK_RIGHT, Key::Right},
    {VK_RETURN, Key::Enter},
    {VK_ESCAPE, Key::Escape},
    {VK_PRIOR, Key::PageUp},
    {VK_NEXT, Key::PageDown},
    {VK_HOME, Key::Home},
    {VK_END, Key::End},
    {VK_F1, Key::F1},
    {VK_F2, Key::F2},
    {VK_F3, Key::F3},
    {VK_F4, Key::F4},
    {VK_F5, Key::F5},
    {VK_F6, Key::F6},
    {VK_F7, Key::F7},
    {VK_F8, Key::F8},
    {VK_F9, Key::F9},
    {VK_F10, Key::F10},
    {VK_F11, Key::F11},
    {VK_F12, Key::F12}
};

class ConsoleImpl
{
	HANDLE					handle, input_handle;
	Rect					console_rect;
	std::vector<CHAR_INFO>	frame_buffer;
public:

	ConsoleImpl()
	{
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (handle == INVALID_HANDLE_VALUE)
		{
			throw std::runtime_error("Failed to get console handle");
		}
		input_handle = GetStdHandle(STD_INPUT_HANDLE);
		if (input_handle == INVALID_HANDLE_VALUE)
		{
			throw std::runtime_error("Failed to get console input handle");
		}
		update();
		set_cursor(0, 0);
		clear();
	}

	~ConsoleImpl()
	{

	}

	const Rect& get_rect()
	{
		return console_rect;
	}

	Key get_key(bool wait)
	{
		DWORD n;
		INPUT_RECORD input_record;
		do
		{
			DWORD pending;
			if (GetNumberOfConsoleInputEvents(input_handle, &pending) && pending > 0)
			{
				if (ReadConsoleInput(input_handle, &input_record, 1, &n) && n > 0)
				{
					if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
					{
						WORD vkCode = input_record.Event.KeyEvent.wVirtualKeyCode;
						
						 // Look up the key in our map
						auto it = virtualKeyToKeyMap.find(vkCode);
						if (it != virtualKeyToKeyMap.end())
						{
							return it->second;
						}
					}
				}
			}
			
			// If no key is available and wait is false, return None
			if (!wait)
				return Key::None;

			if (update())
			{
				clear();
				return Key::None;
			}
				
			// Otherwise, wait for input and continue the loop
			Sleep(100);
			
		} while (wait);
		
		// This line should never be reached since we return Key::None when !wait
		// above, but included for safety
		return Key::None;
	}

	WORD get_attribute(const ColorPair& cp)
	{
		WORD foreground = 0;
		WORD background = 0;

		// Map foreground color
		switch (cp.foreground)
		{
			case Color::Black:
				foreground = 0;
				break;
			case Color::Red:
				foreground = FOREGROUND_RED;
				break;
			case Color::Green:
				foreground = FOREGROUND_GREEN;
				break;
			case Color::Blue:
				foreground = FOREGROUND_BLUE;
				break;
			case Color::Yellow:
				foreground = FOREGROUND_RED | FOREGROUND_GREEN;
				break;
			case Color::Magenta:
				foreground = FOREGROUND_RED | FOREGROUND_BLUE;
				break;
			case Color::Cyan:
				foreground = FOREGROUND_GREEN | FOREGROUND_BLUE;
				break;
			case Color::White:
				foreground = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
				break;
		}

		// Add intensity to make colors brighter (except black)
		if (cp.foreground != Color::Black)
		{
			foreground |= FOREGROUND_INTENSITY;
		}

		// Map background color
		switch (cp.background)
		{
			case Color::Black:
				background = 0;
				break;
			case Color::Red:
				background = BACKGROUND_RED;
				break;
			case Color::Green:
				background = BACKGROUND_GREEN;
				break;
			case Color::Blue:
				background = BACKGROUND_BLUE;
				break;
			case Color::Yellow:
				background = BACKGROUND_RED | BACKGROUND_GREEN;
				break;
			case Color::Magenta:
				background = BACKGROUND_RED | BACKGROUND_BLUE;
				break;
			case Color::Cyan:
				background = BACKGROUND_GREEN | BACKGROUND_BLUE;
				break;
			case Color::White:
				background = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
				break;
		}

		// Add intensity to make colors brighter (except black)
		if (cp.background != Color::Black)
		{
			background |= BACKGROUND_INTENSITY;
		}

		return foreground | background;
	}


	bool update()
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(handle, &csbi))
		{
			int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			if (width!=console_rect.width || height!=console_rect.height)
			{
				console_rect = Rect(0, 0, width, height);
				frame_buffer.resize(width * height, CHAR_INFO{ ' ', csbi.wAttributes });
				return true;
			}
		}
		else
		{
			throw std::runtime_error("Failed to get console screen buffer info");
		}
		return false;
	}

	void clear()
	{
		update();
		frame_buffer.assign(frame_buffer.size(), CHAR_INFO{ ' ', 0x07 });
	}

	CHAR_INFO& operator()(const Point& p)
	{
		return operator()(p.x, p.y);
	}

	CHAR_INFO& operator()(int x, int y)
	{
		if (x < 0 || x >= console_rect.width || y < 0 || y >= console_rect.height)
			throw std::runtime_error("Coordinates out of bounds");
		return frame_buffer[y * console_rect.width + x];
	}

	void set_cursor(int x, int y)
	{
		COORD coord = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
		if (!SetConsoleCursorPosition(handle, coord))
		{
			throw std::runtime_error("Failed to set console cursor position");
		}
	}

	void set_character(int x, int y, wchar_t ch, const ColorPair& color)
	{
		if (x < 0 || x >= console_rect.width || y < 0 || y >= console_rect.height)
			throw std::runtime_error("Coordinates out of bounds");
		auto& c = operator()(x, y);
		c.Char.UnicodeChar = ch;
		c.Attributes = get_attribute(color);
	}

	void draw_frame()
	{
		COORD buffer_size = { SHORT(console_rect.width), SHORT(console_rect.height) };
		COORD buffer_coord = { 0, 0 };
		SMALL_RECT write_region = { 0, 0, SHORT(console_rect.width - 1), SHORT(console_rect.height - 1) };
		WriteConsoleOutputW(handle, frame_buffer.data(), buffer_size, buffer_coord, &write_region);
	}
};

// Implement Console class methods
Console::Console() : m_Impl(std::make_unique<ConsoleImpl>())
{}

Console::~Console()
{
// m_Impl is automatically destroyed
}

const Rect& Console::get_rect()
{
	return m_Impl->get_rect();
}

Key Console::get_key(bool wait)
{
	return m_Impl->get_key(wait);
}

bool Console::update()
{
	return m_Impl->update();
}

void Console::clear()
{
	m_Impl->clear();
}

void Console::set_cursor(int x, int y)
{
	m_Impl->set_cursor(x, y);
}

void Console::set_character(int x, int y, wchar_t ch, const ColorPair& color)
{
	m_Impl->set_character(x, y, ch, color);
}

void Console::draw_frame()
{
	m_Impl->draw_frame();
}


#endif // _WIN32
