import json
import sys

from automata import build_minimized_dfa
from lexer import lex


def save_dfa(dfa, filename):
    data = {
        "alphabet": "ASCII 0..127",
        "start_state": dfa["start"],
        "trap_state": dfa["trap"],
        "accepting_states": {
            str(state): info for state, info in dfa["accepting"].items()
        },
        "transitions": dfa["transitions"],
    }

    with open(filename, "w", encoding="utf-8") as file:
        json.dump(data, file, indent=2)


def main():
    dfa, minimized = build_minimized_dfa()

    save_dfa(minimized, "dfa.json")

    print("DFA before minimization:", len(dfa["transitions"]))
    print("DFA after minimization:", len(minimized["transitions"]))
    print("Trap state:", minimized["trap"])

    if len(sys.argv) > 1:
        text = " ".join(sys.argv[1:])

        try:
            tokens = lex(text, minimized)

            print("\nTokens:")
            for name, lexeme in tokens:
                print(f"{name}: {lexeme!r}")

        except ValueError as error:
            print("\nError:", error)


if __name__ == "__main__":
    main()
