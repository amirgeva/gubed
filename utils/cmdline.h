#pragma once

#include "xstring.h"
#include <memory>
#include <map>
#include <vector>


  class CommandLineOption
  {
    bool    m_TakesParam;
    xstring m_Description;
  public:
    CommandLineOption() : m_TakesParam(false) {}
    virtual ~CommandLineOption() {}
    virtual void process(const xstring& param) = 0;

    void set_takes_param(bool state) { m_TakesParam = state; }
    void set_description(const xstring& d) { m_Description = d; }
    const xstring& get_description() const { return m_Description; }
    bool takes_param() const { return m_TakesParam; }
  };

#define COMMAND_LINE_OPTION(name,takes_param,description)\
class Opt_##name : public CommandLineOption { public:\
Opt_##name() { set_description(description); CommandLine::instance()->register_option(#name,takes_param,this); }\
virtual void process(const xstring& param) override; } g_Opt_##name;\
void Opt_##name::process(const xstring& param)

#define COMMAND_LINE_OPTION_BOOL(name,var_name,default_value,description)\
bool var_name=default_value;\
COMMAND_LINE_OPTION(name,false,description){\
var_name=!default_value;}

  class CommandLine
  {
  public:
    static CommandLine* instance()
    {
      static std::unique_ptr<CommandLine> ptr(new CommandLine);
      return ptr.get();
    }

    void register_option(const xstring& name, bool takes_param, CommandLineOption* opt)
    {
      opt->set_takes_param(takes_param);
      m_Options[name] = opt;
    }

    bool process(int argc, char* argv[], const xstring& usage, unsigned minp, unsigned maxp)
    {
      m_Name = argv[0];
      m_Usage = usage;
      try
      {
        for (int i = 1; i < argc; ++i)
        {
          xstring arg = argv[i];
          if (arg[0U] == '-')
          {
            arg = arg.substr(1);
            opt_map::iterator it = m_Options.find(arg);
            if (it == m_Options.end()) throw xstring("Invalid command line option -" + arg);
            CommandLineOption* opt = it->second;
            xstring param;
            if (opt->takes_param())
            {
              if (i == (argc - 1)) throw xstring("Missing parameter for option -" + arg);
              param = argv[++i];
            }
            opt->process(param);
          }
          else
            m_Params.push_back(arg);
        }
        if (m_Params.size() < minp || m_Params.size() > maxp) throw xstring("Invalid number of parameters");
      }
      catch (const xstring& err)
      {
        if (!err.empty())
          std::cerr << err << std::endl;
        print_usage();
        return false;
      }
      return true;
    }

    unsigned get_parameter_count() const { return unsigned(m_Params.size()); }

  private:
    class HelpCommandOption : public CommandLineOption
    {
    public:
      HelpCommandOption() { set_description("Display command line help"); }
      virtual void process(const xstring& param) override
      {
        throw xstring();
      }
    };


    friend struct std::default_delete<CommandLine>;
    CommandLine() {}
    ~CommandLine() { register_option("h", false, &m_HelpOption); }
    CommandLine(const CommandLine&) {}
    CommandLine& operator= (const CommandLine&) { return *this; }

    typedef std::map<xstring, CommandLineOption*> opt_map;
    typedef std::vector<xstring> str_vec;
    HelpCommandOption m_HelpOption;
    opt_map m_Options;
    str_vec m_Params;
    int     m_Current=0;
    xstring m_Name;
    xstring m_Usage;
  public:
    typedef str_vec::const_iterator const_iterator;
    const_iterator begin() const { return m_Params.begin(); }
    const_iterator end()   const { return m_Params.end(); }

    void print_usage()
    {
      std::cerr << "Usage: " << m_Name << " [options] " << m_Usage << std::endl;
      for (auto it = m_Options.begin(); it != m_Options.end(); ++it)
      {
        std::cerr << "    -" << it->first << "\t" << it->second->get_description() << std::endl;
      }
    }

    xstring get(unsigned index) const
    {
      if (index >= get_parameter_count()) return "";
      const_iterator it = begin();
      if (index > 0) std::advance(it, index);
      return *it;
    }

    template<class U>
    CommandLine& operator>>(U& u) { std::istringstream is(get(m_Current++)); is >> u; return *this; }

  };

  template<>
  inline CommandLine& CommandLine::operator>>(xstring& s) { s = get(m_Current++); return *this; }



#define PROCESS_COMMAND_LINE_P(usage,minp,maxp)\
CommandLine* cmd=CommandLine::instance();  if (!cmd->process(argc,argv,usage,minp,maxp)) return 1

#define PROCESS_COMMAND_LINE(usage)\
CommandLine* cmd=CommandLine::instance();  if (!cmd->process(argc,argv,usage,0,99999)) return 1

