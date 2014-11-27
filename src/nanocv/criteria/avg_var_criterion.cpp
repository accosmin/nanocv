#include "avg_var_criterion.h"
#include <cassert>

namespace ncv
{        
        avg_var_criterion_t::avg_var_criterion_t(const string_t& configuration)
                :       avg_criterion_t(configuration),
                        m_value2(0.0)
        {
        }

        void avg_var_criterion_t::reset()
        {
                avg_criterion_t::reset();

                m_value2 = 0.0;

                m_vgrad2.resize(psize());
                m_vgrad2.setZero();
        }

        criterion_t& avg_var_criterion_t::operator+=(const criterion_t& other)
        {
                avg_criterion_t::operator+=(other);
                
                const avg_var_criterion_t* vother = dynamic_cast<const avg_var_criterion_t*>(&other);
                assert(vother != nullptr);
                
                m_value2 += vother->m_value2;
                m_vgrad2 += vother->m_vgrad2;
                
                return *this;
        }

        void avg_var_criterion_t::accumulate(scalar_t value, scalar_t error)
        {
                avg_criterion_t::accumulate(value, error);
                
                m_value2 += value * value;
        }

        void avg_var_criterion_t::accumulate(const vector_t& vgrad, scalar_t value, scalar_t error)
        {
                avg_criterion_t::accumulate(vgrad, value, error);

                m_value2 += value * value;
                m_vgrad2 += value * vgrad;
        }
        
        scalar_t avg_var_criterion_t::value() const
        {
                return  avg_criterion_t::value() +
                        m_lambda * (count() * m_value2 - m_value * m_value) / (count() * count());
        }

        scalar_t avg_var_criterion_t::error() const
        {
                return  avg_criterion_t::error();
        }

        vector_t avg_var_criterion_t::vgrad() const
        {
                return  avg_criterion_t::vgrad() +
                        2.0 * m_lambda * (count() * m_vgrad2 - m_value * m_vgrad) / (count() * count());
        }

        bool avg_var_criterion_t::can_regularize() const
        {
                return true;
        }
}
	
