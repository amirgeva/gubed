#pragma once

#include <memory>
#include "types.h"
#include "conwin.h"

class Console
{
	std::unique_ptr<class ConsoleImpl> m_Impl;
public:
	Console();
	~Console();
	const Rect& get_rect();
	Key get_key(bool wait);
	// Query window size and update frame buffer size
	bool update();
	void clear();
	void set_cursor(int x, int y);
	void set_character(int x, int y, wchar_t ch, const ColorPair& color);
	void draw_frame();
};
