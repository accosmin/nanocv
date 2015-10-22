#include "mstream.h"
#include <algorithm>

namespace cortex
{
        static bool isendl(char c)
        {
                return (c == '\n') || (c == '\r');
        }

        bool mstream_t::read(char* bytes, std::streamsize num_bytes)
        {
                if (tellg() + num_bytes > size())
                {
                        num_bytes = size() - tellg();
                }

                if (num_bytes > 0)
                {
                        std::copy(m_data + m_tellg, m_data + (m_tellg + num_bytes), bytes);

                        m_tellg += num_bytes;
                        m_gcount = num_bytes;
                        return true;
                }
                else
                {
                        return false;
                }
        }

        bool mstream_t::getline(std::string& line)
        {
                char c;

                for ( ; m_tellg < m_size && isendl(c = m_data[tellg()]); m_tellg ++)
                {
                }

                line.clear();
                for ( ; m_tellg < m_size && !isendl(c = m_data[tellg()]); m_tellg ++)
                {
                        line.push_back(c);
                }

                return !line.empty();
        }

        bool mstream_t::skip(std::streamsize num_bytes)
        {
                if (tellg() + num_bytes <= size())
                {
                        m_tellg += num_bytes;
                        m_gcount = 0;
                        return true;
                }
                else
                {
                        return false;
                }
        }

        std::streamsize mstream_t::gcount() const
        {
                return m_gcount;
        }

        std::streamsize mstream_t::tellg() const
        {
                return m_tellg;        
        }

        std::streamsize mstream_t::size() const
        {
                return m_size;
        }

        bool mstream_t::eof() const
        {
                return tellg() == size();
        }

        bool mstream_t::good() const
        {
                return !eof();
        }

        mstream_t::operator bool() const
        {
                return !eof();
        }

        const char* mstream_t::data() const
        {
                return m_data;
        }
}
