// Copyright (c) 2013 Craig Henderson
// Part of the Data Processing Library
// https://github.com/cdmh/dataproc

namespace cdmh {
namespace data_processing {
namespace detail {

template<typename T>
inline
unsigned const bit_count(T n)
{
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    return (((n + (n >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

inline
std::pair<string_view, type_mask_t>
read_field(char const *&begin, char const *end)
{
    assert(begin != end);

    // we'll trim spaces before any quotes, but not within quotes
    ltrim(begin, end);

    bool in_quotes  = false;
    if (*begin == '\"')
    {
        in_quotes = true;
        ++begin;
    }

    auto it = begin;
    type_mask_t incl_type_mask = string_type;
    type_mask_t excl_type_mask = 0;

    // special case for unary operators
    if (!in_quotes  &&  (*it == '-'  ||  *it == '+'))
    {
        incl_type_mask |= double_type | integer_type;
        ++it;
    }

    bool seen_period = false;
    bool seen_space  = false;
    for (;
        it!=end
        &&  (
            (in_quotes  &&  *it != '\"')
        ||  (in_quotes  &&  *it == '\"'  &&  it+1!=end  &&  *(it+1) == '\"')   // inside quotes, "" escaped quotes
        ||  (!in_quotes  &&  *it != ','  &&  *it != '\r'  &&  *it != '\n')     // outside quotes, terminate on comma or CR or LF
        );
        ++it)
    {
        if (in_quotes)
        {
            if (*it == '\"')
            {
                assert(it+1 != end);
                assert(*(it+1) == '\"');
                ++it;
            }
        }
        else
        {
            if (*it >= '0'  &&  *it <= '9')
            {
                if (seen_space)
                    excl_type_mask |= double_type | integer_type;
                else
                    incl_type_mask |= double_type | integer_type;
            }
            else if (*it == '.')
            {
                if (seen_space)
                    excl_type_mask |= double_type | integer_type;
                else if (seen_period)
                    excl_type_mask |= double_type;
                else
                {
                    incl_type_mask |= double_type;
                    excl_type_mask |= integer_type;
                    seen_period = true;
                }
            }
            else if (*it != '\r'  &&  *it != '\n')
            {
                assert(!in_quotes);
                if (isspace(*it))
                    seen_space = true;
                else
                    excl_type_mask |= double_type | integer_type;
            }
        }
    }

    if (!in_quotes)
        rtrim(begin, it);

    if (begin == it)
        return std::make_pair(string_view(begin, it), null_type);

    // precedences
    incl_type_mask &= ~excl_type_mask;
    if (incl_type_mask & integer_type)   // prefer integer to double and string
        incl_type_mask &= ~(double_type | string_type);
    if (incl_type_mask & double_type)    // prefer double to string
        incl_type_mask &= ~string_type;

    assert(detail::bit_count(incl_type_mask) == 1);
    auto result = std::make_pair(string_view(begin, it), incl_type_mask);

    // update returning 'begin' iterator to the start next field
    begin = it;
    if (in_quotes)
    {
        if (*begin != '\"')
            DebugBreak();
        assert(*begin == '\"');
        ++begin;
    }
    return result;
}

}   // namespace detail
}   // namespace data_processing
}   // namespace cdmh
