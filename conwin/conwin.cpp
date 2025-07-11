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

void Window::set_content(const std::vector<xstring>& content_lines)
{
	m_ContentLines = content_lines;
}

void Window::draw() const
{
	// Draw the window border
	// Top-left corner
	console.set_character(m_BorderRect.x, m_BorderRect.y, 0x250F, ColorPair());
	// Top-right corner
	console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y, 0x2513, ColorPair());
	// Bottom-left corner
	console.set_character(m_BorderRect.x, m_BorderRect.y + m_BorderRect.height - 1, 0x2517, ColorPair());
	// Bottom-right corner
	console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y + m_BorderRect.height - 1, 0x251B, ColorPair());

	// Draw top and bottom borders
	for (int ix = 1; ix < m_BorderRect.width - 1; ++ix)
	{
		// Top border
		console.set_character(m_BorderRect.x + ix, m_BorderRect.y, 0x2501, ColorPair());
		// Bottom border
		console.set_character(m_BorderRect.x + ix, m_BorderRect.y + m_BorderRect.height - 1, 0x2501, ColorPair());
	}

	// Draw left and right borders
	for (int iy = 1; iy < m_BorderRect.height - 1; ++iy)
	{
		// Left border
		console.set_character(m_BorderRect.x, m_BorderRect.y + iy, 0x2503, ColorPair());
		// Right border
		console.set_character(m_BorderRect.x + m_BorderRect.width - 1, m_BorderRect.y + iy, 0x2503, ColorPair());
	}

	if (!m_Title.empty())
	{
		int x = m_BorderRect.x + (m_BorderRect.width - m_Title.length()) / 2 - 2;
		int y = m_BorderRect.y;
		console.set_character(x++, y, 0x252B, ColorPair()); // Draw title connector
		console.set_character(x++, y, ' ', ColorPair()); // Draw title connector
		for (size_t i = 0; i < m_Title.length(); ++i)
		{
			console.set_character(x++, y, m_Title[i], ColorPair(Color::White, Color::Black)); // Draw title text
		}
		console.set_character(x++, y, ' ', ColorPair()); // Draw title connector
		console.set_character(x, y, 0x2523, ColorPair()); // Draw title connector
	}

	if (!m_StatusLine.empty())
	{
		int x = m_BorderRect.x + 2;
		int y = m_BorderRect.y + m_BorderRect.height - 1;
		console.set_character(x++, y, 0x252B, ColorPair()); // Draw title connector
		console.set_character(x++, y, ' ', ColorPair()); // Draw title connector
		for (size_t i = 0; i < m_StatusLine.length() && x < m_BorderRect.x + m_BorderRect.width - 2; ++i)
		{
			console.set_character(x++, y, m_StatusLine[i], ColorPair(Color::White, Color::Black));
		}
		console.set_character(x++, y, ' ', ColorPair()); // Draw title connector
		console.set_character(x, y, 0x2523, ColorPair()); // Draw title connector
	}

	// Draw the content
	for (size_t i = 0; i < m_ContentLines.size(); ++i)
	{
		int iy = int(i) - m_StartOffset;
		int y = m_ContentRect.y + iy;
		if (y >= m_ContentRect.y && y < m_ContentRect.bottom())
		{
			const auto& line = m_ContentLines[i];
			int n = int(line.length());

			// Get line color if it's highlighted
			ColorPair colorPair;
			auto it = m_HighlightLines.find(i);
			if (it != m_HighlightLines.end())
			{
				colorPair = it->second;
			}
			if (i==m_SelectedLine)
			{
				// If this line is selected, use a special color
				colorPair = ColorPair(Color::White, Color::Blue);
			}

			// Draw the content line
			for (int ix = 0; ix < m_ContentRect.width; ++ix)
			{
				if (ix < n)
				{
					// Draw character from the content line
					console.set_character(m_ContentRect.x + ix, y, line[ix], colorPair);
				}
				else
				{
					// Fill with spaces if line is shorter than content rect width
					console.set_character(m_ContentRect.x + ix, y, ' ', colorPair);
				}
			}
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

void Window::set_selected_line(int line)
{
	m_SelectedLine = line;
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

void Desktop::draw()
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
		w->draw();
	}
	console.draw_frame();
}

Key Desktop::get_key(bool wait)
{
	return console.get_key(wait);
}
