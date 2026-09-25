import pytest

from automata import build_minimized_dfa
from lexer import lex


@pytest.fixture(scope="module")
def dfa():
    _, minimized = build_minimized_dfa()
    return minimized


@pytest.mark.parametrize(
    "text, expected",
    [
        ("", []),
        (" \t\r\n", []),
        ("0", [("INT", "0")]),
        ("_abc_123", [("IDENT", "_abc_123")]),
        (
            "function returns while if else assert assume invariant length",
            [
                ("FUNCTION", "function"),
                ("RETURNS", "returns"),
                ("WHILE", "while"),
                ("IF", "if"),
                ("ELSE", "else"),
                ("ASSERT", "assert"),
                ("ASSUME", "assume"),
                ("INVARIANT", "invariant"),
                ("LENGTH", "length"),
            ],
        ),
        (
            "()[]{},; + - * / == != <= >= < > =",
            [
                ("LPAREN", "("),
                ("RPAREN", ")"),
                ("LBRACKET", "["),
                ("RBRACKET", "]"),
                ("LBRACE", "{"),
                ("RBRACE", "}"),
                ("COMMA", ","),
                ("SEMICOLON", ";"),
                ("PLUS", "+"),
                ("MINUS", "-"),
                ("STAR", "*"),
                ("SLASH", "/"),
                ("EQ", "=="),
                ("NE", "!="),
                ("LE", "<="),
                ("GE", ">="),
                ("LT", "<"),
                ("GT", ">"),
                ("ASSIGN", "="),
            ],
        ),
        ("// hello\nfunction", [("FUNCTION", "function")]),
    ],
)
def test_valid_inputs(dfa, text, expected):
    assert lex(text, dfa) == expected


@pytest.mark.parametrize(
    "text",
    [
        "00",
        "01",
        "abcя",
        "α",
        "Ω",
        "π",
        "abcα",
        "@",
        "#",
        "$",
        "?",
        "abc@def",
        "function@x",
    ],
)
def test_invalid_inputs(dfa, text):
    with pytest.raises(ValueError):
        lex(text, dfa)
