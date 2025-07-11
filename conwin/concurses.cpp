#ifndef _WIN32

#include "console.h"
#include <ncurses.h>
#include <locale.h>
#include <unordered_map>
#include <vector>

class ConsoleImpl
{
    Rect console_rect;
    std::vector<std::vector<cchar_t>> frame_buffer;

    // Utility method to create a cchar_t from a wchar_t and attributes
    void create_cchar(cchar_t* dest, wchar_t ch, short pair_number) {
        wchar_t wch[2] = {ch, L'\0'};
        attr_t attr = COLOR_PAIR(pair_number);
        setcchar(dest, wch, attr, pair_number, nullptr);
    }
public:
    ConsoleImpl() {
        // Set up locale for Unicode support
        setlocale(LC_ALL, "");

        // Initialize ncurses with wide character support
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        nodelay(stdscr, TRUE);  // Non-blocking input
        curs_set(0);            // Hide cursor

        // Initialize color pairs
        // Pair 0 is default terminal colors
        init_pair(1, COLOR_BLACK, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_BLUE, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_CYAN, COLOR_BLACK);
        init_pair(8, COLOR_WHITE, COLOR_BLACK);

        // Background colors (pairs 9-16)
        init_pair(9, COLOR_WHITE, COLOR_BLACK);
        init_pair(10, COLOR_WHITE, COLOR_RED);
        init_pair(11, COLOR_WHITE, COLOR_GREEN);
        init_pair(12, COLOR_WHITE, COLOR_YELLOW);
        init_pair(13, COLOR_WHITE, COLOR_BLUE);
        init_pair(14, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(15, COLOR_WHITE, COLOR_CYAN);
        init_pair(16, COLOR_WHITE, COLOR_WHITE);

        update();
        clear();
    }

    ~ConsoleImpl() {
        endwin();
    }

    const Rect& get_rect() {
        return console_rect;
    }

    Key get_key(bool wait) {
        static const std::unordered_map<int, Key> keyMap = {
            {KEY_UP, Key::Up},
            {KEY_DOWN, Key::Down},
            {KEY_LEFT, Key::Left},
            {KEY_RIGHT, Key::Right},
            {'\n', Key::Enter},
            {27, Key::Escape},
            {KEY_PPAGE, Key::PageUp},
            {KEY_NPAGE, Key::PageDown},
            {KEY_HOME, Key::Home},
            {KEY_END, Key::End},
            {KEY_F(1), Key::F1},
            {KEY_F(2), Key::F2},
            {KEY_F(3), Key::F3},
            {KEY_F(4), Key::F4},
            {KEY_F(5), Key::F5},
            {KEY_F(6), Key::F6},
            {KEY_F(7), Key::F7},
            {KEY_F(8), Key::F8},
            {KEY_F(9), Key::F9},
            {KEY_F(10), Key::F10},
            {KEY_F(11), Key::F11},
            {KEY_F(12), Key::F12}
        };

        while (true)
        {
            int ch = getch();
            if (ch != ERR) {
                auto it = keyMap.find(ch);
                if (it != keyMap.end()) {
                    return it->second;
                }
            }

            if (!wait) {
                return Key::None;
            }

            if (update()) {
                clear();
                return Key::None;
            }

            napms(100);  // Sleep for 100ms
        }
    }

    bool update() {
        int new_width, new_height;
        getmaxyx(stdscr, new_height, new_width);

        if (new_width != console_rect.width || new_height != console_rect.height) {
            console_rect = Rect(0, 0, new_width, new_height);

            // Resize the frame buffer
            frame_buffer.resize(new_height);
            for (auto& row : frame_buffer) {
                row.resize(new_width);

                // Initialize with spaces
                cchar_t space;
                create_cchar(&space, L' ', 8);  // White on black
                std::fill(row.begin(), row.end(), space);
            }

            return true;
        }
        return false;
    }

    void clear() {
        // Clear the frame buffer with spaces
        for (auto& row : frame_buffer) {
            cchar_t space;
            create_cchar(&space, L' ', 8);  // White on black
            std::fill(row.begin(), row.end(), space);
        }

        // Clear the screen
        ::clear();
        refresh();
    }

    static void set_cursor(int x, int y) {
        move(y, x);
    }

    void set_character(int x, int y, wchar_t ch, const ColorPair& color) {
        if (x < 0 || x >= console_rect.width || y < 0 || y >= console_rect.height) {
            return;  // Silently ignore out-of-bounds
        }

        // Calculate color pair number
        short pair_number;

        // Create pairs for all foreground/background combinations
        if (color.background == Color::Black) {
            // Use first 8 pairs for foreground colors on black background
            switch (color.foreground) {
                case Color::Black: pair_number = 1; break;
                case Color::Red: pair_number = 2; break;
                case Color::Green: pair_number = 3; break;
                case Color::Yellow: pair_number = 4; break;
                case Color::Blue: pair_number = 5; break;
                case Color::Magenta: pair_number = 6; break;
                case Color::Cyan: pair_number = 7; break;
                case Color::White:
                default: pair_number = 8; break;  // Default to white on black
            }
        } else {
            // Use pairs 9-16 for white foreground on colored backgrounds
            switch (color.background) {
                case Color::Red: pair_number = 10; break;
                case Color::Green: pair_number = 11; break;
                case Color::Yellow: pair_number = 12; break;
                case Color::Blue: pair_number = 13; break;
                case Color::Magenta: pair_number = 14; break;
                case Color::Cyan: pair_number = 15; break;
                case Color::White: pair_number = 16; break;
                case Color::Black:
                default: pair_number = 9; break;  // Default to white on black
            }
        }

        // Set the character in the frame buffer
        create_cchar(&frame_buffer[y][x], ch, pair_number);
    }

    void draw_frame() {
        // Draw the frame buffer to the screen
        for (int y = 0; y < console_rect.height && y < (int)frame_buffer.size(); y++) {
            for (int x = 0; x < console_rect.width && x < (int)frame_buffer[y].size(); x++) {
                mvadd_wch(y, x, &frame_buffer[y][x]);
            }
        }
        refresh();
    }
};

// Implement Console class methods
Console::Console() : m_Impl(std::make_unique<ConsoleImpl>())
{
}

Console::~Console()
{
    m_Impl.reset();
}

const Rect& Console::get_rect() {
    return m_Impl->get_rect();
}

Key Console::get_key(bool wait) {
    return m_Impl->get_key(wait);
}

bool Console::update() {
    return m_Impl->update();
}

void Console::clear() {
    m_Impl->clear();
}

void Console::set_cursor(int x, int y) {
    ConsoleImpl::set_cursor(x, y);
}

void Console::set_character(int x, int y, wchar_t ch, const ColorPair& color) {
    m_Impl->set_character(x, y, ch, color);
}

void Console::draw_frame() {
    m_Impl->draw_frame();
}

#endif // ! _WIN32
