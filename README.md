hikocsp - C++ Server Pages (CSP)
================================

hikocsp.exe usage
-----------------

### Synopsis
```
hikocsp --input=filename.csp --output=filename.hpp
```

  Argument           | Description
 :------------------ |:-----------------------
  \-\-input=<path>   | The filename of the template. recommended extension is .csp
  \-\-output=<path>  | The filename of the result. extension: .cpp, .hpp, .ixx
  
CSP Template format
-------------------

### Verbatim C++
By default the CSP-parser starts in verbatim C++. Verbatim C++ code will be
copied directly into the generated code.

### $output
The `$output` determines how the generated code writes text and placeholders
to the output.

If `$output` is the special value `co_yield`. Then text and placeholders are
yielded from a coroutine as `std::string`.

Otherwise `$output` will be a name of a non-const variable. `operator+=` is used
on this variable to append text and placeholders as a `std::string`.

It is recommended to use a parameter as output value, so that a `std::string`
allocation can be reused.

### $filter

### Text
A text block is started when `{{` is found in verbatim C++.
If there are more than two consecutive open braces `{` are found, then the last
two braces count as the terminator and the other open braces are still part of
verbatim C++.

The text block is terminated when `}}` is found. If more than two consecutive
close braces `}` are found then the first two braces count as the terminator
and the other close braces are part of verbatim C++.

The `$output` determines how the generated code writes the text block.

A text block may also contain:
 - placeholders,
 - C++ line,
 - escape,
  
### placeholder
There are two types of placeholders:
 - *Simple:* `${` expression ( `` ` `` filter )\* `}`
 - *Format:* `${` format-string ( `,` expression... )\* ( `` ` `` filter )\* `}`

The *simple-placeholder* formats an expression with default formatting
using and is syntactic sugar for the equivalent *format-placeholder*: `${"{}", ` expression ( `` ` `` filter )\* `}`

The *format-placeholder* formats one or more expression using a format-string for std::format().
The result of std::format() will be written to the `_out` std::string variable.

After formatting the text is passed through the filters specified in the
placeholder; or if no filter is specified the formatted text is filtered by
`_out_filter`.

The following filters are included:
 - `hi::sgml_escape`: For escaping SGML, HTML and XML text.
 - `hi::sgml_param_escape`: For escaping parameters in SGML, HTML and XML tags.
 - `hi::url_escape`: For escaping URLs.
 - `hi::null_escape`: Which can be used to bypass the `_out_filter`.

Something like the following code will be generated for a placeholder:

```cpp
_out = filter...(std::format(format-string, expression...));
```

An expression is terminated when one of the following characters appears outside
sub-expressions and string-literals: `,`, `` ` ``, `}`, `)`, `]`, `$` or `@`.

### C++ line
A single line of C++ can be inserted in a text-block. It starts with a `$` and ends in
the line-feed. This code is directly copied into the generated code.

A C++ line can not start with the following characters: `$`, `{`, `>`. If you
need to use these charaters then you may add a white-space before this character.

### escape
Use `$$` to escape a dollar, i.e. to append a dollar to the `_out` variable, 

A `$` at the end of the line will supress the line-feed and optional white-spaces.
This is a very useful feature to control the text being appended to `_out`.
Techniqually this command does not exist and is a side effect of an empty C++ line.

Example
-------

```
#include <string>
#include <format>
#include <vector>
#include "hikocsp/module.hpp"

[[nodiscard]] std::generator<std::string> generate_page(std::vector<int> list, int b) noexcept
{{{`hi::sgml_escape `hi::email_escape
    <html>
        <head><title>Page</title></head>
        <body>
        A list of values.
            <li>
            $for (auto value: list) {
                <ul><a href="value_page?${value `hi::url_escape}">${value} + ${b} = ${value + b}</a></ul>
            $}
            <li>
            escape double close bracket: ${"}}"}
            escape dollar: ${"$"}
        </body>
    </html> 
}}}

```

Syntax
------
  
```
# cpp-char may include '{{' inside C++ string-literals.
csp-file := ( cpp-char* | setting | text )*

setting := output-setting | filter-setting

output-setting := '$output ' ( 'co_yield' | cpp-char* ) ';'

filter-setting := '$filter ' cpp-char* ( ',' cpp-char* )* ';'

text := [^{]* '{{' ( text-char* | escape | placeholder | cpp-line )* '}}'

text-char := [^$]
escape := '$$'

# cpp-char may include commas ''' and close-braces '}' inside C++ string-literals and sub-expressions.
placeholder := '${' cpp-char* ( ',' cpp-char* )* ( '`' cpp-char* )* '}'

cpp-line := '$' cpp-char* '\n'

# Any character, however C++ string-literals and sub-expressions are being tracked.
cpp-char := .
```
  
  
