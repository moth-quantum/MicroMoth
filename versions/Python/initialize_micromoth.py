# Builds versions/Python/micromoth.py from the commented template at the top of the repository,
# then runs the tests against the result.
#
# Run it with Python 3 from any folder:   python3 versions/Python/initialize_micromoth.py
# Add --check to only report whether the committed copy is up to date, without writing it (used by CI).
#
# Comments, docstrings and blank lines are removed with Python's own tokenizer, so a '#' or quote inside
# a string can never be mistaken for a comment. The result is smaller, to better fit on a microcontroller.

import io
import os
import subprocess
import sys
import tokenize

HERE = os.path.dirname(os.path.abspath(__file__))
TEMPLATE = os.path.join(HERE, '..', '..', 'micromoth.py')
TARGET = os.path.join(HERE, 'micromoth.py')

HEADER = ('# MicroMoth (C) Copyright Moth Quantum 2024, 2026; (C) Copyright IBM 2023. Apache License 2.0.\n'
          '# Generated from micromoth.py at the top of the repository: edit that file, not this one.\n')


def strip(source):
    '''Returns the source without comments, docstrings or blank lines.'''
    tokens = list(tokenize.generate_tokens(io.StringIO(source).readline))
    kept = []
    previous = tokenize.INDENT  # The start of the file counts as the start of a statement.
    for i, tok in enumerate(tokens):
        if tok.type == tokenize.COMMENT:
            continue
        # A string that makes up a whole statement by itself is a docstring.
        if tok.type == tokenize.STRING and previous in (tokenize.INDENT, tokenize.DEDENT, tokenize.NEWLINE):
            following = [t for t in tokens[i + 1:] if t.type not in (tokenize.COMMENT, tokenize.NL)]
            if following and following[0].type == tokenize.NEWLINE:
                continue
        kept.append(tok)
        if tok.type not in (tokenize.NL, tokenize.COMMENT):
            previous = tok.type
    text = tokenize.untokenize(kept)
    lines = [line.rstrip() for line in text.split('\n')]
    return '\n'.join(line for line in lines if line.strip()) + '\n'


def main():
    with open(TEMPLATE) as f:
        stripped = HEADER + strip(f.read())
    compile(stripped, TARGET, 'exec')  # Fails loudly if stripping ever produced broken code.

    if '--check' in sys.argv:
        with open(TARGET) as f:
            if f.read() != stripped:
                print('versions/Python/micromoth.py is out of date. Run versions/Python/initialize_micromoth.py.')
                sys.exit(1)
        print('versions/Python/micromoth.py is up to date.')
        return

    with open(TARGET, 'w') as f:
        f.write(stripped)
    print('Comments removed from the top-level micromoth.py: ' + str(len(stripped)) + ' bytes.')
    sys.exit(subprocess.call([sys.executable, os.path.join(HERE, 'tests.py')]))


if __name__ == '__main__':
    main()
