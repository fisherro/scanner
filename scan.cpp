#include <regex>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

/*
 * for_each_arg
 *
 * Apply the function f to each of the additional arguments.
 * If f returns false, break out early.
 */
template <typename Func, typename First, typename... Rest>
void for_each_arg(Func f, First& first, Rest&... rest)
{
    bool keep_going = f(first);
    if (not keep_going) return;
    if constexpr (sizeof...(rest) > 0) {
        for_each_arg(f, rest...);
    }
}

/*
 * scanr (to be written)
 *
 * Uses a regex to scan a stream. Any capture groups are read into the
 * additional arguments using the read operator (>>).
 */
template <typename... Args>
int scanr(std::istream&, const std::regex&, Args&... args)
{
    //Standard regex and iostreams don't play nice together. u_u
    return 0;
}

template <typename... Args>
int scanr(const std::regex& re, Args&... args)
{
    return scanr(std::cin, re, args...);
}

/*
 * sscanr
 *
 * Like scanr but reads from a std::string_view instead of a std::istream&.
 */
template <typename... Args>
int sscanr(std::string_view in, const std::regex& re, Args&... args)
{
    using svmatch = std::match_results<std::string_view::const_iterator>;
    svmatch matches;
    bool matched { std::regex_match(in.begin(), in.end(), matches, re) };
    if (not matched) {
        return 0;
    }

    size_t i{1};
    auto each{[&](auto& arg){
        if (i >= matches.size()) return false;
        std::istringstream in{matches[i].str()};
        in >> arg;
        ++i;
        return true;
    }};
    for_each_arg(each, args...);
    return i - 1;
}

//Internal helper function
//
//Conversions cdiosuxXaAeEfFgG all handled by generic code.
//  Special cases for iduoxX generate I/O manipulators.
//
//[set] will require special code
//
//What about p? There is an op>> for void*
//
//n will need special code too
//
//After handling all formats, will need to handle...
//  assignment suppression
//  length limitation
//
//Does std::setw only apply to reading strings?
//  Looks like it. We'll have to do it the hard way. Sigh.
//  We really need arbitrary putback. Sigh.
//
//Implement a generic conversion specifier?
template <typename First, typename... Rest>
int _scan(std::istream& in,  int count, std::string_view format, First& first, Rest&... rest)
{
    while (true) {
        char fc { format[0] }; // fc = format char
        if ('%' == fc) {
            //For now, only implement %d
            format = format.substr(1);
            fc = format[0];

            if ('*' == fc) {
                //Hmm...
            }

            std::string_view easy{"cdiosuxXaAeEfFgG"};
            if (easy.npos != easy.find(fc)) {
                if ('i' == fc) in >> std::setbase(0);
                if (('d' == fc) or ('u' == fc)) in >> std::dec;
                if ('o' == fc) in >> std::oct;
                if (('x' == fc) or ('X' == fc)) in >> std::hex;
                in >> first;
                ++count;
                if constexpr (sizeof...(rest) > 0) {
                    return _scan(in, count, format.substr(1), rest...);
                }
            } else {
                //Bad format
                //TODO: Throw exception?
                return count;
            }
        } else if (::isspace(fc)) {
            //TODO: Better way?
            //Should this match zero whitespace characters?
            //Empirical evidence says: Yes!
            while (true) {
                auto next { in.peek() };
                if (::isspace(next)) in.get();
                else break;
            }
        } else {
            auto ic { in.get() }; // ic = input char
            if (!in) {
                //TODO: Check for error?
                return count;
            }
            if (fc != ic) {
                //Format mismatch
                //TODO: Throw exception?
                return count;
            }
        }
        format = format.substr(1);
    }
}

/*
 * scan
 *
 * Like scanf except:
 *
 *  - If first parameter is an std::istream&, it reads from that stream.
 *    Otherwise it reads from std::cin.
 *
 *  - Takes a std::string_view format string instead of a const char*.
 *
 *  - Takes references instead of pointers
 *
 *  - Ignores the "length modifier" part of any conversion specifier
 *    (It uses the type of the corresponding argument.)
 */
template <typename... Args>
int scan(std::istream& in, std::string_view format, Args&... args)
{
    return _scan(in, 0, format, args...);
}

template <typename First, typename... Rest>
int scan(std::string_view format, First& first, Rest&... rest)
{
    return scan(std::cin, format, first, rest...);
}

/*
 * sscan
 *
 * Like scan but reads from a string_view instead of std::cin.
 */
template <typename First, typename... Rest>
int sscan(
        std::string_view in, std::string_view format,
        First& first, Rest&... rest)
{
    return scan(std::istringstream{in}, format, first, rest...);
}

void test_sscanr()
{
    char c{'x'};
    std::string s;
    int n{-1};
    double f{-1.5};

    std::regex re{
        "char: *(.); *string: *([^;]+); *int: *([0-9]+); *float: *(.+)"
    };

    char match_me[] {
        "char: A; string: Hello; int: 15; float: 3.14"
    };

    int count = sscanr(match_me, re, c, s, n, f);

    std::cout << count << ": "
        << c << ", "
        << s << ", "
        << n << ", "
        << f << '\n';
}

void test_scan()
{
    std::istringstream in{
        "char: A; string: Hello ; int: -15; unsigned: 15; float: 3.14"
    };

    char c{'!'};
    std::string s;
    int n{-1};
    unsigned m{0};
    double f{0};

    //TODO: Need %[^;] support to do this right!
    const std::string format {
        "char: %c; string: %s ; int: %d; unsigned: %u; float: %f"
    };

    int count = scan(in, format, c, s, n, m, f);
    std::cout << count << ": "
        << c << ", "
        << s << ", "
        << n << ", "
        << m << ", "
        << f << '\n';
}

void test_scan_ints()
{
    std::istringstream in{
        "0xff -42 42 42 42 fF"
    };
    const char pattern[]{
        "%i %d %u %o %x %X"
    };
    int a{}, b{};
    unsigned c{}, d{}, e{}, f{};
    int count { scan(in, pattern, a, b, c, d, e, f) };
    std::cout << count << ':'
        << a << ','
        << b << ','
        << c << ','
        << d << ','
        << e << ','
        << f << '\n';
}

int main()
{
    test_sscanr();
    test_scan();
    test_scan_ints();
}

