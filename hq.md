HQ(1) - General Commands Manual

# NAME

**hq** - reads and parses html file based on CSS selectors

# SYNOPSIS

**hq**
\[**-a**&nbsp;*attr\_name\[,attr\_name]*]
\[**-c**]
\[**-d**]
\[**-f**&nbsp;*htmlfile*]
\[**-h**]
\[**-p**]
\[**-t**]
*CSSselector*

# DESCRIPTION

**hq**
will read and parse an html file and then display the output based on the specified CSS selector.

**-a**

> **hq**
> will output the specified attribute for all matching elements. Multiple attributes can be
> specified in a comma separated list.  If a matching element does not have any specified
> attributes, it will be skipped.

**-c**

> **hq**
> will output comments from any matching selectors.  If
> **-d**
> is used, it will output everything except the comments.

**-d**

> **hq**
> will invert the selection, effectly deleting the matching elements from the output.

**-f**

> **hq**
> will specify the HTML file to parse.

**-h**

> **hq**
> will output the usage banner.

**-d**

> is used it will output everything execpt the text elements.

**-p**

> **hq**
> will attempt to output all matching elements in a pretty formatted way with proper indention.

**-t**

> **hq**
> will output text elements from any matching selectors. If
> **-d**
> is used, it will output everything except the text elements.

# CSS SELECTOR

**hq**
will accept most CSS selector formats including: HTML Element; Class and Id.
It will accept subselecctors matching attributes of a CSS element.  Selector
options the would match the DOM state (often preceeded by : or ::) are not
accepted. It is recommended to contain the CSS selector in single quotes to
protect it from shell interpretation.  The format for an element selector is:

	[element][.class|#id][attributes]

If
*element*
is not specified it is assumed to be
*\*&zwnj;*
(all elements).
*Class*
(preceeded by a period)
and
*id*
(preceeded by a
"#"
) can be added to further limit the selection.
*Attributes*
are enclosed in square
"\["
and
"]"
brackets.  Attributs can be specified to further limit selection. See
*ATTRIBUTES*
for options.

\*

> Match all elements.

element

> Match a specific element.

.class

> Match any element with specified Class.

\#id

> Match any element with Specified ID.

element.class

> Match a specific element that contains the specific class.

element#id

> Match a specific element that contains the specific id.

Elements can be further subselected by specifing multiple elements and an
element selection operator.

element1, element2

> Match all elements that match element1 and element2.

element1 SPC element2

> Match all elements of element2 inside element1.

element1 &gt; element2

> Match all elements of element2 with a direct parent of element1.

element1 + element2

> Match the first element of element2 that is placed immediatly after element1.

element1 ~ element2

> Match every element2 that is preceeded by element1.

# ATTRIBUTES

Attributes can be selected by presence or their value.

attribute

> Select the element if the attribute is present in the element regardless of
> value.

attribute = value

> Select the element if the attribute matches the value.  Values should be
> enclosed within quotes
> '"'

attribute ~= value

> Select the elements with the attribute containing the word
> "value".

attribute |= value

> Select the elements with the value equal to
> "value"
> or starting with
> "value-"

attribute ^= value

> Select the elements where the attribute begins with
> "value".

attribute $= value

> Select the elements where the attribute ends with
> "value".

attribute \*= value

> Select the elements where the attribute contains the substring
> "value".

# EXIT STATUS

**hq**
will exist with a return value of 0 on successful parse and display.
On failure it will exit with a non-zero value.

# EXAMPLES

Extract meta tags

	hq -f htmlfile 'meta'

Extract meta tags with the attribute
"name"
equal to
"description"

	hq -f htmlfile 'meta[name="description"]'

Extract text

	hq -f htmlfile -t ''

Extract all tags

	hq -f htmlfile '*'

Extract just the javascript

	hq -f htmlfile 'javascript'

Extract text and elements with the class of 'article'

	hq -f htmlfile '.article'

Extract just text from class 'article'

	hq -f htmlfile -t '.article'

Extract all DIV and P elements

	hq -f htmlfile 'div, p'

Extract all P inside DIV elements

	hq -f htmlfile 'div p'

Extract all P where the parent is a DIV element

	hq -f htmlfile 'div > p'

Extract first P that is placed immediately after DIV element

	hq -f htmlfile 'div + p'

Extract every P element that is preceded by a DIV element

	hq -f htmlfile 'div ~ p'

# AUTHORS

Michael Graves

# CAVEATS

**hq**
is not an HTML linting application. It will not fix broken or badly formated
HTML documents.

OpenBSD 7.4 - February 26, 2024
