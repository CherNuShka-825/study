# HW1: regex -> DFA -> minimization

Minimal implementation for the homework:

1. token regular expressions are defined in `token_specs.py`;
2. Thompson construction builds an NFA;
3. subset construction builds a DFA;
4. Hopcroft algorithm minimizes the DFA;
5. `main.py` writes the minimized transition table to `dfa.json`;
6. tests use `pytest`.

## Run

```bash
python main.py
```

## Tests

```bash
python -m pytest -q
```

The exact Funny regex specification was not present in the provided homework file, so the token rules cover only the token kinds and examples explicitly listed there.
