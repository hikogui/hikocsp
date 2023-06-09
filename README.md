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
  
### `$<` Text block `$>`
A CSP template starts in C++ verbatim-mode, it switches to text-mode when the
parser encounters `$<` outside of a quoted-string. The parser switches back
to C++ verbatim-mode when it encounters `$>`.
  
All characters in C++ verbatim-mode is directly placed in the generated code.
  
The text-mode contains text, but also place-holders and short pieces of C++ code.
Code will be generated for text and place-holders to write to the `_out` variable.
The `_out` variable is of type `std::string` (or equivilant).
  
### `${` expression `}`
The expression place-holder is for writing the result of a C++ expression to `_out`.
The resulting generated code of an expression place-holder is as follows:
  
```cpp
_out += std::format("{}", expression);
```

The expression must be a valid and complete C++ expression.
_This rule exists to determine when a `}` can terminate the place-holder._
  
### `$(` format-arguments `)`
The format place-holder is for formatting and writing the result of one or more C++ expression.
The resulting generated code of a format place-holder is as follows:
  
```cpp
_out += std::format(format-arguments);
```

The expression must be a valid and complete C++ expression.
_This rule exists to determine when a `)` can terminate the place-holder._

  
### `$` C++ line `\n`
Short pieces of verbatim C++ code can be added to the generated code using this syntax.
  
The first character of the C++ line can't be `{`, `(`, `$` or `>`. 
  
Syntax
------
  
```
template := ( cpp-verbatim | block )*
 
block := '$<' command* '$>'
command := text | expression | format | cpp-line | dollar-escape | newline-escape
  
expression := '${' cpp-expression '}'
format := '$(' cpp-arguments ')'
cpp-line := '$' cpp-verbatim '\n'
dollar-escape := '$$'
newline-escape := '$' [ \t\r] [\n\f\v]
text := [^$]*

# cpp-expression is a C++ expression.
# A C++ expression may be terminated when the resulting expression is valid.
cpp-expression := ...

cpp-arguments := cpp-expression ( ',' cpp-expression )*

# cpp-verbatim is a piece of C++ code.
# Verbatim C++ code may be terminated at any time, except inside a quoted string.
cpp-verbatim := ...
```
  
  
