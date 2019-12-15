RegEx engine by ya boi


Implemented features:
 - Literal character matching
 - '.'  (Any character except '\0')
 - '+'  Greedy quantifier (1 or more)
 - '+?' Lazy quantifier (1 or more)
 - '?'  Greedy quantifier (0 or 1)
 - '??' Lazy quantifier (0 or 1)
 - '\*' Greedy quantifier (0 or more)
 - '\*?' Lazy quantifier (0 or more)
 - '[a-z]' Character classes including ranges
 - '[^a-z]' Reverse character classes including ranges
 - '()' Capturing groups (No backreferencing though)
 - '|' Alternation
 - '^' Start of string
