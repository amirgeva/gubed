#include "ui.h"
#include <fstream>
#include <filesystem>
#include <regex>
#include <conwin.h>
#include <json.hpp>

using json = nlohmann::json;
using RectMap = std::map<std::string, Rect>;

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

static xstring unite_lines(const std::vector<xstring>& lines)
{
	xstring code;
	for (const auto& line : lines)
	{
		code += line + "\n";
	}
	return code;
}

struct Node
{
	enum class Kind { Vertical, Horizontal, Rect };

	Kind kind;
	int  percentage{};        // 0-100 or 0 (=auto) for exactly one child in a layout
	std::string id;           // for leaf rectangles (optional)
	std::vector<Node> children;

	bool is_layout() const noexcept
	{
		return kind == Kind::Vertical || kind == Kind::Horizontal;
	}
};

static Node parse_node(const json& json_node)
{
	if (!json_node.contains("type")) throw std::runtime_error("Node missing 'type'");
	std::string t = json_node.at("type").get<std::string>();
	Node node;
	if (t == "vertical")      node.kind = Node::Kind::Vertical;
	else if (t == "horizontal") node.kind = Node::Kind::Horizontal;
	else if (t == "rect")       node.kind = Node::Kind::Rect;
	else throw std::runtime_error("Unknown type: " + t);

	if (!json_node.contains("percentage")) throw std::runtime_error("Node missing 'percentage'");
	node.percentage = json_node.at("percentage").get<int>();
	if (node.percentage < 0 || node.percentage > 100)
		throw std::runtime_error("Percentage must be between 0 and 100");

	if (node.kind == Node::Kind::Rect)
	{
		// Leaf may carry an identifier to make printed output clearer.
		if (json_node.contains("id")) node.id = json_node.at("id").get<std::string>();
		else                    node.id = "rect";
	}
	else
	{
	 // Recursive parse of children
		if (!json_node.contains("children")) throw std::runtime_error("Layout node missing 'children'");
		for (const auto& cj : json_node.at("children")) node.children.emplace_back(parse_node(cj));
		if (node.children.empty())
			throw std::runtime_error("Layout node must have at least one child");
	}
	return node;
}

static void resolve_percentages(Node& node)
{
	if (!node.is_layout()) return;           // leaves are fine

	int zeroCount = 0;
	int nonZeroSum = 0;
	for (auto& c : node.children)
	{
		if (c.percentage == 0)
			++zeroCount;
		else
			nonZeroSum += c.percentage;
	}

	if (zeroCount > 1)
		throw std::runtime_error("Layout node has more than one child with percentage 0");

	if (zeroCount == 0)
	{
		if (nonZeroSum != 100)
			throw std::runtime_error("Percentages must sum to 100 (found " + std::to_string(nonZeroSum) + ")");
	}
	else
	{ // exactly one automatic child
		if (nonZeroSum > 100)
			throw std::runtime_error("Non-zero percentages exceed 100 (" + std::to_string(nonZeroSum) + ")");
		int remainder = 100 - nonZeroSum;
		// Assign remainder to the zero-percentage child
		for (auto& c : node.children)
		{
			if (c.percentage == 0)
			{
				c.percentage = remainder;
				break;
			}
		}
	}

	// Recurse
	for (auto& c : node.children)
		resolve_percentages(c);
}

static void compute_layout(const Node& n, const Rect& r,
						   RectMap& out)
{
	if (!n.is_layout())
	{
		// Leaf: store rectangle
		out[n.id]=r;
		return;
	}

	const bool isVertical = (n.kind == Node::Kind::Vertical);
	int major = isVertical ? r.height : r.width;

	// First pass: compute ideal sizes in pixels (integer division) and track remainder
	int accumulated = 0;
	int childCount = static_cast<int>(n.children.size());
	std::vector<int> sizes(childCount);
	for (int i = 0; i < childCount; ++i)
	{
		sizes[i] = major * n.children[i].percentage / 100;
		accumulated += sizes[i];
	}
	int remainder = major - accumulated; // pixels left due to truncation
	if (remainder > 0) sizes.back() += remainder; // give leftover to last child

	// Second pass: assign rectangles
	int offset = 0;
	for (int i = 0; i < childCount; ++i)
	{
		Rect cr = r;
		if (isVertical)
		{
			cr.y += offset;
			cr.height = sizes[i];
		}
		else
		{
			cr.x += offset;
			cr.width = sizes[i];
		}
		offset += sizes[i];
		compute_layout(n.children[i], cr, out);
	}
}


class UserInterface : public IUserInterface
{
	std::unordered_map<xstring, std::set<int>>		m_Breakpoints;
	xstring											m_CurrentModule;
	enum ActivePane { CODE, VARS }					m_ActivePane = CODE;
	std::vector<xstring>							m_CurrentCode;
	std::vector<xstring>							m_CurrentVars;
	Desktop											m_Desktop;
	size_t											m_ActiveWindowIndex = 0;
	WindowPtr										m_CodeWindow;
	WindowPtr										m_VarsWindow;
	WindowPtr										m_OutputWindow;
	WindowPtr										m_ProjectWindow;

	WindowPtr get_active_window()
	{
		return m_Desktop.get_window(m_ActiveWindowIndex);
	}

	void swap_active_pane()
	{
		m_ActiveWindowIndex = (m_ActiveWindowIndex + 1) % m_Desktop.size();
	}

	void set_active_window(WindowPtr w)
	{
		while (m_Desktop.get_window(m_ActiveWindowIndex) != w)
		{
			swap_active_pane();
		}
	}

	std::vector<xstring> load_module_list()
	{
		std::vector<xstring> modules;
		try {
			for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
				if (entry.is_regular_file()) {
					std::filesystem::path path = entry.path();
					if (path.extension() == ".wren") {
						// Convert stem to string and add to modules list
						modules.emplace_back(path.stem().string());
					}
				}
			}
		} catch (const std::filesystem::filesystem_error& e) {
			// Handle potential errors silently, just return empty list
		}

		return modules;
	}

	std::string load_layout_json()
	{
		const char* env_vars[] = { "HOME", "USERPROFILE" };
		for (const char* env_var : env_vars)
		{
			const char* home = std::getenv(env_var);
			if (home)
			{
				std::filesystem::path dir = home;
				std::filesystem::path layout_file = (dir / ".gubed") / "layout.json";
				if (std::filesystem::exists(layout_file) && std::filesystem::is_regular_file(layout_file))
				{
					return unite_lines(load_file(layout_file.string()));
				}
			}
		}
		const std::string default_layout = R"({
  "type": "vertical",
  "percentage": 100,
  "children": [
    {
      "percentage": 60,
      "type": "horizontal",
      "children": [
        { "type": "rect", "percentage": 30, "id": "Project" },
        { "type": "rect", "percentage": 70, "id": "Code" }
      ]
    },
    {
      "percentage": 0,
      "type": "horizontal",
      "children": [
		{ "type": "rect", "percentage": 50, "id": "Vars" },
		{ "type": "rect", "percentage": 50, "id": "Output" }
	  ]
    }
  ]
})";
		return default_layout;
	}

	void load_layout(const Rect& desktop_rect, RectMap& window_rects)
	{
		std::string loaded_layout = load_layout_json();
		json root_node_json;
		std::istringstream is(loaded_layout);
		is >> root_node_json;
		Node root_node = parse_node(root_node_json);
		resolve_percentages(root_node);
		compute_layout(root_node, desktop_rect, window_rects);
	}
public:
	UserInterface()
	{
		RectMap layout_rects;
		load_layout(m_Desktop.get_rect(), layout_rects);
		std::map<std::string, WindowPtr> windows_map;
		for (const auto& [name, rect] : layout_rects)
		{
			auto window = std::make_shared<Window>(rect);
			window->set_title(name);
			windows_map[name] = window;
			m_Desktop.add_window(window);
		}
		m_CodeWindow = windows_map["Code"];
		m_VarsWindow = windows_map["Vars"];
		m_OutputWindow = windows_map["Output"];
		m_ProjectWindow = windows_map["Project"];
		m_ProjectWindow->set_content(load_module_list());
		m_Desktop.set_status_line("F5 Continue | F6 Next Pane | F9 Breakpoint | F10 Step | Esc Quit");
	}

	~UserInterface()
	{
	}

	virtual void load_module(const xstring& module_name) override
	{
		if (module_name != m_CurrentModule)
		{
			m_CurrentModule = module_name;
			m_CurrentCode = load_file(module_name+".wren");
			if (m_CodeWindow)
			{
				m_CodeWindow->set_content(m_CurrentCode);
				m_CodeWindow->set_title(module_name);
			}
		}
	}

	void set_colors()
	{
		if (m_CodeWindow)
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
		}
	}

	virtual void highlight_line(size_t line_index) override
	{
		if (m_CodeWindow)
		{
			m_CodeWindow->set_highlight_line(line_index);
			set_colors();
		}
	}

	virtual void set_variables(const std::string& variables) override
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
		if (m_VarsWindow)
			m_VarsWindow->set_content(m_CurrentVars);
	}

	virtual bool is_breakpoint(const xstring& module_name, size_t line_index) override
	{
		auto it = m_Breakpoints.find(module_name);
		if (it == m_Breakpoints.end()) return false;
		return it->second.find(line_index) != it->second.end();
	}

	virtual void print(const char* text) override
	{
		if (m_OutputWindow)
		{
			m_OutputWindow->append_content(text);
			m_OutputWindow->ensure_visible(m_OutputWindow->get_content().size() - 1);
		}
	}

	void toggle_breakpoint()
	{
		int line = m_CodeWindow->get_highlight_line();
		if (line < 0) return;
		auto it = m_Breakpoints.find(m_CurrentModule);
		if (it == m_Breakpoints.end())
		{
			m_Breakpoints[m_CurrentModule] = { line };
		}
		else
		{
			auto& bps = it->second;
			if (bps.find(line) != bps.end())
			{
				bps.erase(line);
			}
			else
			{
				bps.insert(line);
			}
		}
		set_colors();
	}

	Action ui_loop() override
	{
		Action res = NONE;
		while (res==NONE)
		{
			auto current_window = get_active_window();
			m_Desktop.draw(current_window);
			Key key = m_Desktop.get_key();
			switch (key)
			{
				case Key::F10: res = STEP; break;
				case Key::F5: res = CONTINUE; break;
				case Key::F6: swap_active_pane(); break;
				case Key::Up: current_window->change_highlight_line(-1); break;
				case Key::Down: current_window->change_highlight_line(1); break;
				case Key::F9: toggle_breakpoint(); break;
				case Key::Enter:
				{
					if (current_window == m_ProjectWindow)
					{
						xstring selected_module = current_window->get_highlight_line() < m_ProjectWindow->get_content().size()
							? m_ProjectWindow->get_content()[current_window->get_highlight_line()] : "";
						if (!selected_module.empty())
						{
							load_module(selected_module);
							m_CodeWindow->set_highlight_line(0);
							set_colors();
							set_active_window(m_CodeWindow);
						}
					}
				} break;
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