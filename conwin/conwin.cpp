#include "conwin.h"
#include "console.h"

static Console console;

Window::Window(const Rect& border_rect)
	: m_BorderRect(border_rect)
	, m_ContentRect(border_rect.x + 1, border_rect.y + 1, border_rect.width - 2, border_rect.height - 2)
{}

Window::Window(const Pointd& size_percent)
	: m_SizePercent(size_percent)
{}


void Window::set_title(const xstring& title)
{
	m_Title = title;
}

void Window::set_status_line(const xstring& status_line)
{
	m_StatusLine = status_line;
}

const Rect& Window::get_border_rect() const { return m_BorderRect; }

const Rect& Window::get_content_rect() const { return m_ContentRect; }

void Window::set_line_foreground_color(size_t line, const Color& color)
{
	auto it = m_HighlightLines.find(line);
	if (it != m_HighlightLines.end())
	{
		it->second.foreground = color;
	}
	else
	{
		m_HighlightLines[line] = ColorPair(color, Color::Black);
	}
}

void Window::set_line_background_color(size_t line, const Color& color)
{
	auto it = m_HighlightLines.find(line);
	if (it != m_HighlightLines.end())
	{
		it->second.background = color;
	}
	else
	{
		m_HighlightLines[line] = ColorPair(Color::White, color);
	}
}

const ColorPair& Window::get_line_color(size_t line) const
{
	auto it = m_HighlightLines.find(line);
	if (it != m_HighlightLines.end())
		return it->second;
	static const ColorPair cp;
	return cp; // Return default color pair if not found
}

void Window::clear_line_colors()
{
	m_HighlightLines.clear();
}

static xstring normalize_line(const xstring& line)
{
	xstring normalized = line;
	normalized.trim_right();
	normalized.replace("\t", "    "); // Replace tabs with spaces
	return normalized;
}

void Window::set_content(const std::vector<xstring>& content_lines)
{
	m_ContentLines.assign(content_lines.size(), "");
	std::transform(content_lines.begin(), content_lines.end(), m_ContentLines.begin(),
				   [](const xstring& line) { return normalize_line(line); });
}

const std::vector<xstring>& Window::get_content() const
{
	return m_ContentLines;
}

void Window::append_content(const xstring& line)
{
	m_ContentLines.push_back(line);
}

struct Border
{
	wchar_t top_left = 0x250F;
	wchar_t top_right = 0x2513;
	wchar_t bottom_left = 0x2517;
	wchar_t bottom_right = 0x251B;
	wchar_t horizontal = 0x2501;
	wchar_t vertical = 0x2503;
	wchar_t left_limiter = 0x252B;
	wchar_t right_limiter = 0x2523;
};

void Window::draw(bool active) const
{
	const Border border;
	const ColorPair border_color = active ? ColorPair(Color::White, Color::Cyan) : ColorPair(Color::White, Color::Black);
	// Draw the window border
	// Top-left corner
	console.set_character(m_BorderRect.x, m_BorderRect.y, 
						  border.top_left, border_color);
	// Top-right corner
	console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y, 
						  border.top_right, border_color);
	// Bottom-left corner
	console.set_character(m_BorderRect.x, m_BorderRect.y + m_BorderRect.height - 1, 
						  border.bottom_left, border_color);
	// Bottom-right corner
	console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y + m_BorderRect.height - 1,
						  border.bottom_right, border_color);

	// Draw top and bottom borders
	for (int ix = 1; ix < m_BorderRect.width - 1; ++ix)
	{
		// Top border
		console.set_character(m_BorderRect.x + ix, m_BorderRect.y, border.horizontal, border_color);
		// Bottom border
		console.set_character(m_BorderRect.x + ix, m_BorderRect.y + m_BorderRect.height - 1, border.horizontal, border_color);
	}

	// Draw left and right borders
	for (int iy = 1; iy < m_BorderRect.height - 1; ++iy)
	{
		// Left border
		console.set_character(m_BorderRect.x, m_BorderRect.y + iy, 
							  border.vertical, border_color);
		// Right border
		console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y + iy,
							  border.vertical, border_color);
	}

	if (!m_Title.empty())
	{
		int x = m_BorderRect.x + (m_BorderRect.width - m_Title.length()) / 2 - 2;
		int y = m_BorderRect.y;
		console.set_character(x++, y, border.left_limiter, border_color); // Draw title connector
		console.set_character(x++, y, ' ', border_color); // Draw title connector
		for (size_t i = 0; i < m_Title.length(); ++i)
		{
			console.set_character(x++, y, m_Title[i], ColorPair(Color::White, Color::Black)); // Draw title text
		}
		console.set_character(x++, y, ' ', border_color); // Draw title connector
		console.set_character(x, y, border.right_limiter, border_color); // Draw title connector
	}

	if (!m_StatusLine.empty())
	{
		int x = m_BorderRect.x + 2;
		int y = m_BorderRect.y + m_BorderRect.height - 1;
		console.set_character(x++, y, 0x252B, border_color); // Draw title connector
		console.set_character(x++, y, ' ', border_color); // Draw title connector
		for (size_t i = 0; i < m_StatusLine.length() && x < m_BorderRect.x + m_BorderRect.width - 2; ++i)
		{
			console.set_character(x++, y, m_StatusLine[i], ColorPair(Color::White, Color::Black));
		}
		console.set_character(x++, y, ' ', border_color); // Draw title connector
		console.set_character(x, y, 0x2523, border_color); // Draw title connector
	}

	// Draw the content
	int y = m_ContentRect.y;
	for (size_t i = 0; i < m_ContentLines.size(); ++i)
	{
		int iy = int(i) - m_StartOffset;
		y = m_ContentRect.y + iy;
		if (y >= m_ContentRect.y && y < m_ContentRect.bottom())
		{
			const auto& line = m_ContentLines[i];
			int n = int(line.length());

			// Get line color if it's highlighted
			ColorPair color;
			auto it = m_HighlightLines.find(i);
			if (it != m_HighlightLines.end())
			{
				color = it->second;
			}
			if (i==m_HighlightLine)
			{
				// If this line is selected, use a special color
				color = ColorPair(Color::White, Color::Blue);
			}

			// Draw the content line
			for (int ix = 0; ix < m_ContentRect.width; ++ix)
			{
				if (ix < n)
				{
					// Draw character from the content line
					console.set_character(m_ContentRect.x + ix, y, line[ix], color);
				}
				else
				{
					// Fill with spaces if line is shorter than content rect width
					console.set_character(m_ContentRect.x + ix, y, ' ', color);
				}
			}
		}
	}
	ColorPair blank_color(Color::White, Color::Black);
	while(++y < m_ContentRect.bottom())
	{
		for (int ix = 0; ix < m_ContentRect.width; ++ix)
		{
			console.set_character(m_ContentRect.x + ix, y, ' ', blank_color);
		}
	}
}

const Pointd& Window::get_size_percent() const 
{
	return m_SizePercent;
}

void Window::set_size_percent(const Pointd& size_percent)
{
	m_SizePercent = size_percent;
}

void Window::set_rect(const Rect& rect)
{
	m_BorderRect = rect;
	m_ContentRect = Rect(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2);
}

int Window::get_highlight_line() const
{
	return m_HighlightLine;
}

void Window::set_highlight_line(int line)
{
	m_HighlightLine = line;
	ensure_visible(m_HighlightLine);
}

void Window::change_highlight_line(int delta)
{
	m_HighlightLine += delta;
	if (m_HighlightLine < 0)
		m_HighlightLine = 0;
	if (m_HighlightLine >= int(m_ContentLines.size()))
		m_HighlightLine = int(m_ContentLines.size()) - 1;
	ensure_visible(m_HighlightLine);
}

void Window::ensure_visible(int line)
{
	if (line < 0 || line >= int(m_ContentLines.size()))
		return;
	
	// Calculate the center position that would put the line in the middle
	int center_offset = line - m_ContentRect.height / 2;
	
	// Make sure we don't scroll past the beginning
	if (center_offset < 0)
		center_offset = 0;
	
	// Make sure we don't scroll past the end
	int max_offset = int(m_ContentLines.size()) - m_ContentRect.height;
	if (center_offset > max_offset)
		center_offset = max_offset > 0 ? max_offset : 0;
	
	// Set the new offset
	m_StartOffset = center_offset;
}

void Desktop::clear()
{
	console.clear();
	m_Windows.clear();
	console.draw_frame();
}

void Desktop::draw(WindowPtr active_window)
{
	if (console.update())
		console.clear();
	Point p(0, 0);
	for (auto w : m_Windows)
	{
		Pointd size_percent = w->get_size_percent();
		if (size_percent.x > 0 && size_percent.y > 0)
		{
			int width = int(console.get_rect().width * size_percent.x / 100.0);
			int height = int(console.get_rect().height * size_percent.y / 100.0);
			Rect rect(p.x, p.y, width, height);
			w->set_rect(rect);
			p.y += height;
		}
		w->draw(w == active_window);
	}
	if (!m_StatusLine.empty())
	{
		int x = (get_rect().width - m_StatusLine.size() - 4) / 2;
		if (x < 1) x = 1;
		xstring text = m_StatusLine;
		if (text.length() > get_rect().width - 4)
			text = text.substr(0, get_rect().width - 4);
		Border border;
		int y = get_rect().height - 1;
		ColorPair color;
		console.set_character(x++, y, border.left_limiter, color);
		console.set_character(x++, y, ' ', color);
		for (size_t i = 0; i < text.length(); ++i)
			console.set_character(x++, y, text[i], color);
		console.set_character(x++, y, ' ', color);
		console.set_character(x++, y, border.right_limiter, color);
	}
	console.draw_frame();
}

Key Desktop::get_key(bool wait)
{
	return console.get_key(wait);
}

Rect Desktop::get_rect()
{
	return console.get_rect();
}

void Desktop::add_window(const WindowPtr& window)
{
	m_Windows.push_back(window);
}

const std::vector<WindowPtr>& Desktop::get_windows() const
{ 
	return m_Windows;
}

size_t Desktop::size() const
{
	return m_Windows.size();
}

WindowPtr Desktop::get_window(size_t index) const
{
	if (index < m_Windows.size())
		return m_Windows[index];
	return nullptr;
}

void Desktop::set_status_line(const xstring& status_line)
{ 
	m_StatusLine = status_line;
}
