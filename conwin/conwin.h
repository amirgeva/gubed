#pragma once

#include <memory>
#include <unordered_map>
#include <types.h>
#include <xstring.h>

enum class Color
{
	Black,
	Red,
	Green,
	Blue,
	Yellow,
	Magenta,
	Cyan,
	White
};

enum class Key
{
	None = 0,
	Up,
	Down,
	Left,
	Right,
	Enter,
	Escape,
	PageUp,
	PageDown,
	Home,
	End,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12
};

struct ColorPair
{
	Color foreground;
	Color background;
	ColorPair(Color fg = Color::White, Color bg = Color::Black)
		: foreground(fg), background(bg) {}
};

class Window
{
	Rect										m_BorderRect;
	Rect										m_ContentRect;
	std::unordered_map<size_t, ColorPair>		m_HighlightLines;
	std::vector<xstring>						m_ContentLines;
	int											m_StartOffset = 0;
	Pointd										m_SizePercent=Pointd(0,0);
	xstring										m_Title, m_StatusLine;
	int											m_SelectedLine = -1;
public:
	Window(const Rect& border_rect);
	Window(const Pointd& size_percent);

	void				set_title(const xstring& title);
	void				set_status_line(const xstring& status_line);
	void				set_size_percent(const Pointd& size_percent);
	const Pointd&		get_size_percent() const;
	void				set_content(const std::vector<xstring>& content_lines);
	void				set_rect(const Rect& rect);
	const Rect&			get_border_rect() const;
	const Rect&			get_content_rect() const;
	void				clear_line_colors();
	void				set_line_foreground_color(size_t line, const Color& color);
	void				set_line_background_color(size_t line, const Color& color);
	const ColorPair&	get_line_color(size_t line) const;
	void				set_selected_line(int line);
	void				ensure_visible(int line);
	void				draw() const;
};

using WindowPtr = std::shared_ptr<Window>;

class Desktop
{
	std::vector<WindowPtr> m_Windows;
public:
	void add_window(const WindowPtr& window) { m_Windows.push_back(window); }
	const std::vector<WindowPtr>& get_windows() const { return m_Windows; }
	WindowPtr get_window(size_t index) const 
	{
		if (index < m_Windows.size())
			return m_Windows[index];
		return nullptr;
	}
	Key get_key(bool wait = true);
	void clear();
	void draw();
};
