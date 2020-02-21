/*
 * Musings on a alternative to scanf.
 */
/*
 * Returning a tuple is difficult as there is no way to tell at compile time
 * how many elements (or what type) might be needed.
 *
 * BUT...most of the time the format string would be known at compile time and,
 * in theory, could be parsed at compile time.
 *
 * std::string_view is very constexpr, BTW.
 *
 * Note that the length specifiers don't overlap with the format characters,
 * so the regex can be simplified:
 *
 * /(\*)?([0-9]+)?(..?)?([csdiuoxXnaAeEfFgGp]|\[.+\]))/
 *
 * There are four things a scanf pattern tells you...
 *
 *  1. The context for the data to be converted
 *  2. How to match the data
 *  3. How to convert the data from a string
 *  4. The data type
 *
 * For example: "(%4hx)" tells us...
 *
 *  1. The data appears inside parentheses
 *  2. We should match 1 to 4 hexadecimal characters
 *  3. The data should be parsed as hexadecimal
 *  4. The data will be stored in an unsigned short
 *
 * A regex can tell us 1 & 2.
 * The type of an out parameter can tell us 4.
 * But we still wouldn't know 3.
 *
 * Maybe use the scanf specifiers but w/o (or ignore) the size bits?
 */

/*
 * ? = {0,1}
 * * = {0,}
 * + = {1,}
 */
#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/variant.hpp>
#include <iostream>
#include <regex>
#include <string>
#include <cctype>

#if 0
template<typename T, typename... Ts>
std::ostream& operator<<(
        std::ostream& os, const boost::variant<typename T, Ts...>& v)
{
    std::visit([&os](auto&& arg) {
        os << arg;
    }, v);
    return os;
}

//Unicode compatibility would be...fun.

struct Conv_spec {
    bool                    suppress_assignment = false;
    boost::optional<int>    maximum_width;
    std::string             length_modifier;
    std::string             format;

    void dump() const
    {
        std::cout << '%';
        if (suppress_assignment) std::cout << '*';
        if (maximum_width) std::cout << *maximum_width;
        std::cout << length_modifier << format << '\n';
    }
};

struct Conv_spec_result {
    bool        valid = true;
    Conv_spec   conv_spec;
    size_t      chars_consumed;
};

template<typename New, typename... Rest>
auto prepend(New new_one, std::tuple<Rest...> rest)
{
    return std::tuple_cat(std::make_tuple(new_one), rest);
}

template<typename R>
R produce_tuple(const std::string& s)
{
    if (s.size() > 0) {
        return prepend(s[0], produce_tuple(s.substr(1)));
    } else {
        return std::make_tuple();
    }
}

#if 1
Conv_spec_result parse_conversion_specifier(boost::string_ref sv)
#else
Conv_spec_result parse_conversion_specifier(const std::string& sv)
#endif
{
    const std::string format(sv.begin(), sv.end());
    Conv_spec_result result;
    Conv_spec& cs { result.conv_spec };;
    //TODO: Should anchor at start of string?
    const std::regex rx {
        R"((\*)?([0-9]+)?(hh|h|l|ll|j|z|t|L)?([csdiuoxXnaAeEfFgGp]|\[.+\]))"
    };
    std::smatch match;
    bool ok { std::regex_match(format.begin(), format.end(), match, rx) };
    if (ok) {
        if (match[1].matched) cs.suppress_assignment = true;
        if (match[2].matched) cs.maximum_width = std::stoi(match[2].str());
        if (match[3].matched) cs.length_modifier = match[3].str();
        cs.format = match[4].str();
        result.chars_consumed = match.length();
    } else {
        result.valid = false;
    }
}

using Scan_result = boost::variant<char, wchar_t, std::string, std::wstring,
      signed char, unsigned char, int, unsigned, long, unsigned long,
      long long, unsigned long long, intmax_t, uintmax_t, size_t,
      ptrdiff_t, float, double, long double, void*>;

//Uses sscanf syntax.
//Returns a tuple of results.
//Regex for a conversion specifier:
// /%(\*)?([0-9]+)?(hh|h|l|ll|j|z|t|L)?([csdiuoxXnaAeEfFgGp]|\[.+\])/
//Let's return a vector of variants.
//Later we'll convert it to a tuple.
std::vector<Scan_result> scan(std::istream& in, boost::string_ref format)
{
    std::vector<Scan_result> results;
    for (size_t index{0}; index < format.size(); ++index) {
        if ('%' == format[index]) {
            Conv_spec_result csr {
                parse_conversion_specifier(format.substr(index + 1)) };
            if (csr.valid) {
                if ("d" == csr.conv_spec.format) {
                    int n;
                    in >> n;
                    results.push_back(n);
                    index += csr.chars_consumed;
                } else {
                    //TODO: Invalid conversion specifier. Fail.
                    //Return empty tuple.
                    return results;
                }
            } else {
                //TODO: Invalid conversion specifier. Fail. Return empty tuple.
                return results;
            }
        } else if (::isspace(format[index])) {
            auto next { in.peek() };
            while (::isspace(next)) {
                in.get();
                next = in.peek();
            }
        } else {
            auto c { in.get() };
            if (c != format[index]) {
                //Mismatch. Fail. Return empty tuple.
                return results;
            }
        }
    }
    return results;
}
#endif

#if 0
//Uses regex instead of scanf format.
//Converts captures into parameters.
template <typename... Args>
size_t rxscan(boost::string_ref scanee, std::regex pattern, Args...)
{
}
#endif

template <typename First, typename... Rest>
void xscan(
        std::istream& in, boost::string_ref format,
        First& first, Rest&... rest)
{
    while (not format.empty()) {
        if (::isspace(format[0])) {
            while (true) {
                auto next { in.peek() };
                if (::isspace(next)) {
                    in.get();
                } else {
                    format = format.substr(1);
                    break;
                }
            }
            format.substr(1);
        } else if ('{' == format[0]) {
            auto end { format.find('}') };
            in >> first;
            if constexpr (sizeof...(rest) > 0) {
                xscan(in, format.substr(end + 1), rest...);
            }
            break;
        } else {
            auto next { in.get() };
            if (format[0] != next) {
                //Whoops.
                return;
            }
            format = format.substr(1);
        }
    }
}

#if 0
void test(boost::string_ref sv)
{
    parse_conversion_specifier(sv).conv_spec.dump();
}
#endif

int main()
{
#if 0
    test("s");
    test("*s");
    test("32s");
    test("*32s");
    test("hi");
    test("*hi");
    test("10hi");
    test("*10hi");
    test("[abc]");
#endif
#if 0
    auto t { scan(std::istringstream{"10"}, "%d") };
    std::cout << std::get<0>(t) << '\n';
#endif
    std::istringstream in{"test (42): 3.14"};
    std::string str;
    int n;
    double f;
    xscan(in, "{} ({}): {}", str, n, f);
    std::cout << str << '(' << n << "): " << f << '\n';
}

