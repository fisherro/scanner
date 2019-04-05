# scan

Musings on a C++ version of scanf.

## sscanr

    int sscanr(std::string\_view input, const std::regex& re, ...);

I wrote that as if it were a C-style variadic function, but it is really a C++ variadic template function.

This function matches input against re. For each submatch, it uses operator>> to parse the submatch and assign its value to the next argument.

It returns the number of arguments that it assigned values to.

For example:

    std::regex re{"([^:]+) *: *([0-9]+)"};
    std::string name;
    int value{0};
    int count = sscanr("total: 15", re, name, value);
    std::cout << count << '\n';
    std::cout << name << '\n';
    std::cout << value << '\n';

...would output...

    2
    total
    15

## scanr

Ideally there would be a scanr function like this...

    int scanr(std::istream& input, const std::regex& re, ...);

...but std::regex\_match doesn't play nice with std::istream. _Sigh_

This can be worked around, but I haven't done it yet.

## scan

This is a work-in-progress.

    int scan(std::istream& input, std::string\_view format, ...);

Again...not a C variadic function but a C++ variadic function template.

This function aims to be mostly like scanf (or, I suppose, fscanf), but...

* It takes a std::istream& instead of a FILE\* for the input
* It takes a std::string\_view instead of a const char\* for the format
* It knows the types of the additional arguments
* It ignores the "length modifier" part of any conversion specifiers since it knows the type of the corresponding argument

There is also this overload...

    int scan(std::string\_view format, ...);

...which reads from std::cin.

## sscan

    int sscan(std::string\_view input, std::string\_view format, ...);

Like scan, but reads from a std::string\_view instead of a std::istream&.

