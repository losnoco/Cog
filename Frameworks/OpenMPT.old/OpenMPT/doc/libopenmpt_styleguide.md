
Coding conventions
------------------


### libopenmpt

**Note:**
**This applies to `libopenmpt/` and `openmpt123/` directories only.**
**Use OpenMPT style otherwise.**

The code generally tries to follow these conventions, but they are not
strictly enforced and there are valid reasons to diverge from these
conventions. Using common sense is recommended.

 -  In general, the most important thing is to keep style consistent with
    directly surrounding code.
 -  Use C++ std types when possible, prefer `std::size_t` and `std::int32_t`
    over `long` or `int`. Do not use C99 std types (e.g. no pure `int32_t`)
 -  Qualify namespaces explicitly, do not use `using`.
    Members of `namespace openmpt` can be named without full namespace
    qualification.
 -  Prefer the C++ version in `namespace std` if the same functionality is
    provided by the C standard library as well. Also, include the C++
    version of C standard library headers (e.g. use `<cstdio>` instead of
    `<stdio.h>`.
 -  Do not use ANY locale-dependant C functions. For locale-dependant C++
    functionaly (especially iostream), always imbue the
    `std::locale::classic()` locale.
 -  Prefer kernel_style_names over CamelCaseNames.
 -  If a folder (or one of its parent folders) contains .clang-format,
    use clang-format v3.5 for indenting C++ and C files, otherwise:
     -  `{` are placed at the end of the opening line.
     -  Enclose even single statements in curly braces.
     -  Avoid placing single statements on the same line as the `if`.
     -  Opening parentheses are separated from keywords with a space.
     -  Opening parentheses are not separated from function names.
     -  Place spaces around operators and inside parentheses.
     -  Align `:` and `,` when inheriting or initializing members in a
        constructor.
     -  The pointer `*` is separated from both the type and the variable name.
     -  Use tabs for identation, spaces for formatting.
        Tabs should only appear at the very beginning of a line.
        Do not assume any particular width of the TAB character. If width is
        important for formatting reasons, use spaces.
     -  Use empty lines at will.
 -  API documentation is done with doxygen.
    Use general C doxygen for the C API.
    Use QT-style doxygen for the C++ API.

#### libopenmpt indentation example

~~~~{.cpp}
namespace openmpt {

// This is totally meaningless code and just illustrates indentation.

class foo
	: public base
	, public otherbase
{

private:

	std::int32_t x;
	std::int16_t y;

public:

	foo()
		: x(0)
		, y(-1)
	{
		return;
	}

	int bar() const;

}; // class foo

int foo::bar() const {

	for ( int i = 0; i < 23; ++i ) {
		switch ( x ) {
			case 2:
				something( y );
				break;
			default:
				something( ( y - 1 ) * 2 );
				break;
		}
	}
	if ( x == 12 ) {
		return -1;
	} else if ( x == 42 ) {
		return 1;
	}
	return 42;

}

} // namespace openmpt
~~~~

