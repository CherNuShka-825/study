from typing import Any

from automata import DFA


def lex(text: str, dfa: DFA) -> list[tuple[str, str]]:
    tokens: list[tuple[str, str]] = []
    pos = 0

    while pos < len(text):
        state = dfa["start"]
        i = pos
        last_accept: tuple[int, dict[str, Any]] | None = None

        while i < len(text):
            code = ord(text[i])

            if code >= 128:
                state = dfa["trap"]
            else:
                state = dfa["transitions"][state][code]

            if state == dfa["trap"]:
                break

            i += 1

            if state in dfa["accepting"]:
                last_accept = (i, dfa["accepting"][state])

        if last_accept is None:
            raise ValueError(
                "lexical error at position " + str(pos) + ": " + repr(text[pos])
            )

        end, info = last_accept
        lexeme = text[pos:end]

        if (
            info["name"] == "INT"
            and lexeme == "0"
            and end < len(text)
            and text[end].isdigit()
        ):
            raise ValueError("integer with leading zero at position " + str(pos))

        if not info["skip"]:
            tokens.append((info["name"], lexeme))

        pos = end

    return tokens
