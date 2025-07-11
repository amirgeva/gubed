#include "ui.h"
#include <fstream>
#include <regex>
#include <conwin.h>


static bool g_UIActive = true;
void disable_ui()
{
	g_UIActive = false;
}


static std::vector<xstring> load_file(const std::string& filename)
{
	std::ifstream f(filename);
	if (f.fail()) return {};
	std::string line;
	std::vector<xstring> res;
	while (std::getline(f, line))
	{
		res.emplace_back(line);
	}
	return res;
}

class UserInterface : public IUserInterface
{
	std::unordered_map<xstring, std::set<int>>		m_Breakpoints;
	xstring											m_CurrentModule;
	enum ActivePane { CODE, VARS }					m_ActivePane = CODE;
	std::vector<xstring>							m_CurrentCode;
	std::vector<xstring>							m_CurrentVars;
	int												m_HighlightIndex = -1;
	int												m_CurrentLine = -1;

	WindowPtr										m_CodeWindow;
	WindowPtr										m_VarsWindow;
	Desktop											m_Desktop;
public:
	UserInterface()
	{
		m_CodeWindow = std::make_shared<Window>(Pointd(100, 60));
		m_CodeWindow->set_title("Code");
		m_VarsWindow = std::make_shared<Window>(Pointd(100, 40));
		m_VarsWindow->set_title("Variables");
		m_VarsWindow->set_status_line("F3 Open | F5 Continue | F6 Next Pane | F9 Breakpoint | F10 Next | Esc Quit");
		m_Desktop.add_window(m_CodeWindow);
		m_Desktop.add_window(m_VarsWindow);
	}

	~UserInterface()
	{
	}

	virtual void LoadModule(const xstring& module_name) override
	{
		if (module_name != m_CurrentModule)
		{
			m_CurrentModule = module_name;
			m_CurrentCode = load_file(module_name+".wren");
			m_CodeWindow->set_content(m_CurrentCode);
			m_CodeWindow->set_title(module_name);
		}
	}

	void SetColors()
	{
		m_CodeWindow->clear_line_colors();
		auto it = m_Breakpoints.find(m_CurrentModule);
		if (it != m_Breakpoints.end())
		{
			for (const auto& bp : it->second)
			{
				m_CodeWindow->set_line_foreground_color(bp, Color::White);
				m_CodeWindow->set_line_background_color(bp, Color::Red);
			}
		}
		if (m_CurrentLine >= 0 && m_CurrentLine < m_CurrentCode.size())
		{
			m_CodeWindow->set_line_foreground_color(m_CurrentLine, Color::White);
			m_CodeWindow->set_line_background_color(m_CurrentLine, Color::Green);
		}
	}

	virtual void HighlightLine(size_t line_index) override
	{
		m_HighlightIndex = line_index;
		m_CurrentLine = line_index;
		SetColors();
	}

	virtual void SetVariables(const std::string& variables) override
	{
		m_CurrentVars.clear();
		xstring_tokenizer st(variables, "|");
		std::regex pattern(R"((\w+)=([^|]*))");
		std::smatch res;
		while (st.has_more_tokens())
		{
			std::string s = st.get_next_token();
			if (std::regex_match(s, res, pattern))
			{
				std::string name = res[1], value = res[2];
				m_CurrentVars.push_back(name + "\t\t\t" + value);
			}
		}
		m_VarsWindow->set_content(m_CurrentVars);
	}

	virtual bool IsBreakpoint(const xstring& module_name, size_t line_index) override
	{
		auto it = m_Breakpoints.find(module_name);
		if (it == m_Breakpoints.end()) return false;
		return it->second.find(line_index) != it->second.end();
	}

	void ToggleBreakpoint()
	{
		auto it = m_Breakpoints.find(m_CurrentModule);
		if (it == m_Breakpoints.end())
		{
			m_Breakpoints[m_CurrentModule] = { int(m_HighlightIndex) };
		}
		else
		{
			auto& bps = it->second;
			if (bps.find(m_HighlightIndex) != bps.end())
			{
				bps.erase(m_HighlightIndex);
			}
			else
			{
				bps.insert(m_HighlightIndex);
			}
		}
		SetColors();
	}

	Action UILoop() override
	{
		//m_Status.setText("F3 Open | F5 Continue | F6 Next Pane | F9 Breakpoint | F10 Next");
		//m_Code.setText(highlight_code(m_CurrentCode, m_HighlightIndex));
		Action res = NONE;
		while (res==NONE)
		{
			m_CodeWindow->set_selected_line(m_HighlightIndex);
			m_Desktop.draw();
			Key key = m_Desktop.get_key();
			switch (key)
			{
				case Key::F10: res = STEP; break;
				case Key::F5: res = CONTINUE; break;
				case Key::Up: m_HighlightIndex = std::max(m_HighlightIndex-1, 0); break;
				case Key::Down: m_HighlightIndex = std::min(m_HighlightIndex + 1, int(m_CurrentCode.size() - 1)); break;
				case Key::F9: ToggleBreakpoint(); break;
				case Key::Escape: res = QUIT; m_Desktop.clear(); break;
			}
		}
		return res;
	}

};


std::shared_ptr<IUserInterface> IUserInterface::Create()
{
	return std::make_shared<UserInterface>();
}