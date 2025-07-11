#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

  template<class E, class T = std::char_traits<E>, class A = std::allocator<E> >
  class basic_xstring : public std::basic_string<E, T, A>
  {
    typedef E                           value_type;
    typedef A                           allocator_type;
    typedef std::basic_string<E, T, A>    parent;
    typedef basic_xstring<E, T, A>        self;
    typedef typename parent::size_type  size_type;
    using parent::npos;
  public:
    basic_xstring(const value_type* ptr, size_type n, const allocator_type& al = allocator_type())
      : parent(ptr, n, al) {}
    basic_xstring(const value_type* ptr, const allocator_type& al = allocator_type())
      : parent(ptr, al) {}
    basic_xstring(const basic_xstring& s, size_type off = 0, size_type n = npos, const allocator_type& al = allocator_type())
      : parent(s, off, n, al) {}
    basic_xstring(size_type n, value_type c, const allocator_type& al = allocator_type())
      : parent(n, c, al) {}
    explicit basic_xstring(const allocator_type& al = allocator_type())
      : parent(al) {}
    basic_xstring(const parent& p) : parent(p) {}
    template<class U>
    explicit basic_xstring(const U& u)
    {
      std::basic_ostringstream<E, T, A> os;
      os << u;
      *this = os.str();
    }

    using parent::find;
    using parent::length;
    using parent::append;
    using parent::substr;

    operator const value_type* () const { return parent::c_str(); }

    template<typename U>
    U as() const
    {
        U res;
        std::istringstream is(*this);
        is >> res;
        return res;
    }

    int as_int() const
    {
      return atoi(parent::c_str());
    }

    double as_double() const
    {
      return atof(parent::c_str());
    }

    self& trim_left()
    {
      int p = int(parent::find_first_not_of(" \t\n\r"));
      if (p > 0) *this = parent::substr(p);
      return *this;
    }

    self& trim_right()
    {
      int p = int(parent::find_last_not_of(" \t\n\r"));
      if (p >= 0)
        *this = parent::substr(0, p + 1);
      return *this;
    }

    self& trim()
    {
      trim_left();
      trim_right();
      return *this;
    }

    bool read_line_skip_empty(std::istream& is)
    {
      while (true)
      {
        if (!read_line(is)) return false;
        if (!trim().empty()) break;
      }
      return true;
    }

    bool read_line(std::istream& is)
    {
      *this = "";
      if (is.eof()) return false;
      char buffer[65536];
      bool rc = false;
      while (true)
      {
        buffer[0] = 0;
        is.getline(buffer, 65535);
        bool found_eol = !is.fail();
        rc |= found_eol;
        std::streamsize act = is.gcount();
        buffer[act] = 0;
        if (act > 0)
        {
          parent s = buffer;
          *this += s;
        }
        if (is.eof() || found_eol) break;
        is.clear();
      }
      return rc;
    }

    template<class U>
    self& operator<< (const U& u)
    {
      int p = int(find("{}"));
      if (p >= 0) parent::replace(p, 2, self(u));
      else append(self(u));
      return *this;
    }

    using parent::replace;

    self& replace(const self& from, const self& to)
    {
      int p = -1;
      while ((p = int(find(from))) >= 0)
        parent::replace(p, from.length(), to);
      return *this;
    }

    bool startswith(const self& s) const
    {
      if (length() < s.length()) return false;
      return substr(0, s.length()) == s;
    }

    bool endswith(const self& s) const
    {
      if (length() < s.length()) return false;
      return substr(length() - s.length()) == s;
    }
  };

  template<class T>
  class basic_string_tokenizer
  {
	  typedef basic_string_tokenizer<T> self;
    typedef basic_xstring<T> str;
    typedef std::vector<str> seq;
    seq      m_Tokens;
    unsigned m_Current;
  public:
    basic_string_tokenizer(const str& s, const str& delim = " \t")
      : m_Current(0)
    {
      int p = -1;
      int len = int(s.length());
      while (true)
      {
        p = int(s.find_first_not_of(delim, p + 1));
        if (p < 0) break;
        int e = int(s.find_first_of(delim, p + 1));
        if (e < 0) e = len;
        m_Tokens.push_back(s.substr(p, e - p));
        p = e;
      }
    }

    unsigned size() const { return unsigned(m_Tokens.size()); }
    bool     has_more_tokens() const { return m_Current < m_Tokens.size(); }
    str      get_next_token()
    {
      if (m_Current < m_Tokens.size())
        return m_Tokens[m_Current++];
      return str();
    }

	template<typename U>
	self& operator>> (U& u)
	{
		std::istringstream is(get_next_token());
		is >> u;
		return *this;
	}

  };


  namespace std {
    template<class E>
    struct hash<basic_xstring<E>> : public hash<basic_string<E>>
    {
    };
  }


  typedef basic_xstring<char> xstring;
  typedef basic_string_tokenizer<char> xstring_tokenizer;


  namespace xstring_utils {

    inline void make_lower(xstring& s)
    {
      xstring::iterator b = s.begin(), e = s.end();
      for (; b != e; ++b)
      {
        char& c = *b;
        if (c >= 'A' && c <= 'Z') c += 32;
      }
    }

    inline xstring to_lower(const xstring& s)
    {
      xstring res = s;
      make_lower(res);
      return res;
    }

    inline void make_upper(xstring& s)
    {
      xstring::iterator b = s.begin(), e = s.end();
      for (; b != e; ++b)
      {
        char& c = *b;
        if (c >= 'a' && c <= 'z') c -= 32;
      }
    }

    inline xstring to_upper(const xstring& s)
    {
      xstring res = s;
      make_upper(res);
      return res;
    }


    template<class T>
    inline xstring pad(const T& t, unsigned len = 0, char fill = ' ', bool pre_fill = true)
    {
      xstring res(t);
      if (res.length() < len)
      {
        xstring fill_str = xstring(len - res.length(), fill);
        if (pre_fill) res = fill_str + res;
        else          res += fill_str;
      }
      return res;
    }

  } // namespace xstring_utils

