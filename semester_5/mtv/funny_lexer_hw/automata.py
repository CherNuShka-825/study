from collections import deque
from typing import Any

from token_specs import ASCII, EPS, TOKEN_SPECS


RegexNode = tuple[Any, ...]
DFA = dict[str, Any]


class RegexParser:
    """Small regex parser supporting only syntax needed by TOKEN_SPECS."""

    def __init__(self, text: str):
        self.text = text
        self.pos = 0

    def peek(self) -> str | None:
        if self.pos >= len(self.text):
            return None
        return self.text[self.pos]

    def take(self) -> str:
        ch = self.peek()
        if ch is None:
            raise ValueError("unexpected end of regex")

        self.pos += 1
        return ch

    def parse(self) -> RegexNode:
        node = self.parse_alt()
        if self.peek() is not None:
            raise ValueError("bad regex at position " + str(self.pos))
        return node

    def parse_alt(self) -> RegexNode:
        parts = [self.parse_concat()]

        while self.peek() == "|":
            self.take()
            parts.append(self.parse_concat())

        if len(parts) == 1:
            return parts[0]
        return ("alt", parts)

    def parse_concat(self) -> RegexNode:
        parts = []

        while True:
            ch = self.peek()
            if ch is None or ch in ")|":
                break
            parts.append(self.parse_repeat())

        if not parts:
            return ("empty",)
        if len(parts) == 1:
            return parts[0]
        return ("concat", parts)

    def parse_repeat(self) -> RegexNode:
        node = self.parse_atom()

        while True:
            op = self.peek()
            if op != "*" and op != "+":
                break

            self.take()
            if op == "*":
                node = ("star", node)
            else:
                node = ("plus", node)

        return node

    def parse_atom(self) -> RegexNode:
        ch = self.take()

        if ch == "(":
            node = self.parse_alt()
            if self.take() != ")":
                raise ValueError("expected ')'")
            return node

        if ch == "[":
            return ("chars", self.parse_char_class())

        if ch == "\\":
            return ("chars", {ord(self.parse_escape())})

        if ch in "*+)|":
            raise ValueError("unexpected symbol in regex: " + ch)

        return ("chars", {ord(ch)})

    def parse_escape(self) -> str:
        ch = self.take()
        escapes = {"n": "\n", "r": "\r", "t": "\t"}
        return escapes.get(ch, ch)

    def parse_class_char(self) -> str:
        if self.peek() == "\\":
            self.take()
            return self.parse_escape()
        return self.take()

    def parse_char_class(self) -> set[int]:
        negate = False
        if self.peek() == "^":
            self.take()
            negate = True

        chars: set[int] = set()
        first = True

        while True:
            ch = self.peek()
            if ch is None:
                raise ValueError("unclosed character class")

            if ch == "]" and not first:
                self.take()
                break

            left = self.parse_class_char()
            first = False

            if self.peek() == "-":
                self.take()

                if self.peek() == "]":
                    chars.add(ord(left))
                    chars.add(ord("-"))
                    continue

                right = self.parse_class_char()
                for code in range(ord(left), ord(right) + 1):
                    chars.add(code)
            else:
                chars.add(ord(left))

        if negate:
            chars = set(ASCII) - chars

        return chars


class NFA:
    def __init__(self):
        self.transitions: dict[int, dict[int | None, set[int]]] = {}
        self.accepting: dict[int, dict[str, Any]] = {}
        self.state_count = 0

    def new_state(self) -> int:
        state = self.state_count
        self.state_count += 1
        self.transitions[state] = {}
        return state

    def add_edge(self, src: int, symbol: int | None, dst: int) -> None:
        if symbol not in self.transitions[src]:
            self.transitions[src][symbol] = set()
        self.transitions[src][symbol].add(dst)


def thompson(nfa: NFA, node: RegexNode) -> tuple[int, int]:
    kind = node[0]

    if kind == "empty":
        start = nfa.new_state()
        end = nfa.new_state()
        nfa.add_edge(start, EPS, end)
        return start, end

    if kind == "chars":
        start = nfa.new_state()
        end = nfa.new_state()

        for symbol in node[1]:
            if symbol not in ASCII:
                raise ValueError("regex must contain only ASCII")
            nfa.add_edge(start, symbol, end)

        return start, end

    if kind == "concat":
        start, end = thompson(nfa, node[1][0])

        for part in node[1][1:]:
            next_start, next_end = thompson(nfa, part)
            nfa.add_edge(end, EPS, next_start)
            end = next_end

        return start, end

    if kind == "alt":
        start = nfa.new_state()
        end = nfa.new_state()

        for part in node[1]:
            part_start, part_end = thompson(nfa, part)
            nfa.add_edge(start, EPS, part_start)
            nfa.add_edge(part_end, EPS, end)

        return start, end

    if kind == "star":
        start = nfa.new_state()
        end = nfa.new_state()
        part_start, part_end = thompson(nfa, node[1])

        nfa.add_edge(start, EPS, end)
        nfa.add_edge(start, EPS, part_start)
        nfa.add_edge(part_end, EPS, part_start)
        nfa.add_edge(part_end, EPS, end)
        return start, end

    if kind == "plus":
        start = nfa.new_state()
        end = nfa.new_state()
        part_start, part_end = thompson(nfa, node[1])

        nfa.add_edge(start, EPS, part_start)
        nfa.add_edge(part_end, EPS, part_start)
        nfa.add_edge(part_end, EPS, end)
        return start, end

    raise ValueError("unknown regex node: " + str(kind))


def build_nfa() -> tuple[NFA, int]:
    nfa = NFA()
    global_start = nfa.new_state()

    for priority, (name, regex, skip) in enumerate(TOKEN_SPECS):
        tree = RegexParser(regex).parse()
        start, end = thompson(nfa, tree)

        nfa.add_edge(global_start, EPS, start)
        nfa.accepting[end] = {
            "name": name,
            "skip": skip,
            "priority": priority,
        }

    return nfa, global_start


def epsilon_closure(nfa: NFA, states: set[int]) -> frozenset[int]:
    result = set(states)
    stack = list(states)

    while stack:
        state = stack.pop()

        for dst in nfa.transitions[state].get(EPS, set()):
            if dst not in result:
                result.add(dst)
                stack.append(dst)

    return frozenset(result)


def move(nfa: NFA, states: frozenset[int], symbol: int) -> set[int]:
    result: set[int] = set()

    for state in states:
        result.update(nfa.transitions[state].get(symbol, set()))

    return result


def dfa_accepting_info(nfa: NFA, subset: frozenset[int]) -> dict[str, Any] | None:
    choices = []

    for state in subset:
        if state in nfa.accepting:
            choices.append(nfa.accepting[state])

    if not choices:
        return None

    best = min(choices, key=lambda item: item["priority"])
    return {"name": best["name"], "skip": best["skip"]}


def build_dfa(nfa: NFA, nfa_start: int) -> DFA:
    start_subset = epsilon_closure(nfa, {nfa_start})

    subset_ids: dict[frozenset[int], int] = {start_subset: 0}
    transitions: list[list[int]] = []
    accepting: dict[int, dict[str, Any]] = {}
    queue = deque([start_subset])

    while queue:
        subset = queue.popleft()
        state = subset_ids[subset]
        row = [0] * 128

        info = dfa_accepting_info(nfa, subset)
        if info is not None:
            accepting[state] = info

        for symbol in ASCII:
            next_subset = epsilon_closure(nfa, move(nfa, subset, symbol))

            if next_subset not in subset_ids:
                subset_ids[next_subset] = len(subset_ids)
                queue.append(next_subset)

            row[symbol] = subset_ids[next_subset]

        transitions.append(row)

    trap = subset_ids[frozenset()]

    return {
        "start": 0,
        "trap": trap,
        "transitions": transitions,
        "accepting": accepting,
    }


def accept_key(dfa: DFA, state: int) -> tuple[str, bool] | None:
    info = dfa["accepting"].get(state)
    if info is None:
        return None
    return info["name"], info["skip"]


def hopcroft(dfa: DFA) -> DFA:
    states = set(range(len(dfa["transitions"])))

    groups_by_type: dict[tuple[str, bool] | None, set[int]] = {}
    for state in states:
        key = accept_key(dfa, state)
        groups_by_type.setdefault(key, set()).add(state)

    partitions: list[set[int]] = list(groups_by_type.values())
    work = [group.copy() for group in partitions]

    while work:
        splitter = work.pop()

        for symbol in ASCII:
            predecessors: set[int] = set()

            for state in states:
                dst = dfa["transitions"][state][symbol]
                if dst in splitter:
                    predecessors.add(state)

            new_partitions: list[set[int]] = []

            for group in partitions:
                left = group & predecessors
                right = group - predecessors

                if not left or not right:
                    new_partitions.append(group)
                    continue

                new_partitions.append(left)
                new_partitions.append(right)

                if group in work:
                    work.remove(group)
                    work.append(left)
                    work.append(right)
                elif len(left) <= len(right):
                    work.append(left)
                else:
                    work.append(right)

            partitions = new_partitions

    start_group: set[int] | None = None
    other_groups: list[set[int]] = []

    for group in partitions:
        if dfa["start"] in group:
            start_group = group
        else:
            other_groups.append(group)

    if start_group is None:
        raise ValueError("start state group not found")

    other_groups.sort(key=min)
    partitions = [start_group] + other_groups

    old_to_new: dict[int, int] = {}
    for new_state, group in enumerate(partitions):
        for old_state in group:
            old_to_new[old_state] = new_state

    new_transitions: list[list[int]] = []
    new_accepting: dict[int, dict[str, Any]] = {}

    for new_state, group in enumerate(partitions):
        representative = next(iter(group))
        row = []

        for symbol in ASCII:
            old_dst = dfa["transitions"][representative][symbol]
            row.append(old_to_new[old_dst])

        new_transitions.append(row)

        if representative in dfa["accepting"]:
            new_accepting[new_state] = dfa["accepting"][representative]

    return {
        "start": old_to_new[dfa["start"]],
        "trap": old_to_new[dfa["trap"]],
        "transitions": new_transitions,
        "accepting": new_accepting,
    }


def build_minimized_dfa() -> tuple[DFA, DFA]:
    nfa, nfa_start = build_nfa()
    dfa = build_dfa(nfa, nfa_start)
    minimized = hopcroft(dfa)
    return dfa, minimized
