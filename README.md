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

### verbatim C++
By default the CSP-parser starts in verbatim mode. Verbatim C++ code will be copied
directly into the generated code.

The verbatim C++ code is terminated when `$<` is found outside of string-literals.

### text
A text block is started after the `$<` which ends the varbatim C++. The text block
is terminated when `$>` is found, after which verbatim C++ starts again.

The generated code will append the text the `_out` std::string variable.

A text block may also contain:
 - placeholders,
 - C++ line,
 - dollar escape,
 - new-line escape.
  
### placeholder
There are two types of placeholders:
 - *Simple:* `${` expression `}`
 - *Format:* `${` format-string ( `,` expression... )\* `}`

The *simple-placeholder* formats an expression with default formatting
using and is syntactic sugar for the equivilant *format-placeholder*: `${"{}", ` expression `}`

The *format-placeholder* formats one or more expression using a format-string for std::format().
The result of std::format() will be written to the `_out` std::string variable.

The following code will be generated for a placeholder:

```cpp
_out += std::format(format-string, expression...);
```

The expressions may be any valid C++ expression. The CSP-parser will therefor ignore the closing
brace '}' if it is contained within a string-literal, or inside a sub-expression.

### C++ line
A single line of C++ can be inserted in a text-block. It starts with a `$` and ends in
the line-feed. This code is directly copied into the generated code.

A C++ line can not start with the following characters: `$`, `{`, `>`. If you
need to use these charaters then you may add a white-space before this character.

### escape
Use `$$` to escape a dollar, i.e. to append a dollar to the `_out` variable, 

### new-line escape
A `$` at the end of the line will supress the line-feed and optional white-spaces.

This is a very useful feature to control the text being appended to `_out`.

Techniqually this command does not exist and is a side effect of an empty C++ line.
    
Syntax
------
  
```
template := verbatim ( '$<' text '$>' verbatim )* ( '$<' text )?

text := '$<' command* '$>'
command := text | placeholder | cpp-line | escape
  
text := [^$]*
placeholder := '${' expression ( ',' expression )* '}'
cpp-line := '$' [^{>$] verbatim '\n'
escape := '$$'

# cpp-expression is a C++ expression.
# A C++ expression may be terminated when the resulting expression is valid.
expression := ...

# cpp-verbatim is a piece of C++ code.
# Verbatim C++ code may be terminated at any time, except inside a quoted string.
verbatim := ...
```
  
  
