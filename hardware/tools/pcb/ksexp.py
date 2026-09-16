"""Minimal reader/writer for KiCad S-expression files.

Round-trips a file byte-for-byte when nothing is changed, so a diff shows only
the edit that was intended.
"""
import re

class Sym(str):
    """A bare token, as opposed to a quoted string."""
    __slots__ = ()

_ESC = {'n': '\n', 'r': '\r', 't': '\t', '"': '"', '\\': '\\'}


def parse(text):
    pos = 0
    n = len(text)

    def skip_ws():
        nonlocal pos
        while pos < n and text[pos] in ' \t\r\n':
            pos += 1

    def read_node():
        nonlocal pos
        skip_ws()
        if text[pos] != '(':
            raise ValueError('expected ( at %d: %r' % (pos, text[pos:pos+30]))
        pos += 1
        items = []
        while True:
            skip_ws()
            if pos >= n:
                raise ValueError('unterminated list')
            ch = text[pos]
            if ch == ')':
                pos += 1
                return items
            if ch == '(':
                items.append(read_node())
            elif ch == '"':
                pos += 1
                buf = []
                while text[pos] != '"':
                    if text[pos] == '\\':
                        pos += 1
                        buf.append(_ESC.get(text[pos], text[pos]))
                    else:
                        buf.append(text[pos])
                    pos += 1
                pos += 1
                items.append(''.join(buf))
            else:
                start = pos
                while pos < n and text[pos] not in ' \t\r\n()':
                    pos += 1
                items.append(Sym(text[start:pos]))
    node = read_node()
    return node


def quote(s):
    out = s.replace('\\', '\\\\').replace('"', '\\"')
    out = out.replace('\n', '\\n')
    return '"%s"' % out


def dumps(node, indent=0, tab='\t'):
    """Serialise in KiCad's own layout: one child list per line, atoms inline."""
    pad = tab * indent
    head = node[0]
    atoms = []
    i = 1
    while i < len(node) and not isinstance(node[i], list):
        atoms.append(node[i])
        i += 1
    rest = node[i:]
    line = pad + '(' + (head if isinstance(head, Sym) else quote(head))
    for a in atoms:
        line += ' ' + (a if isinstance(a, Sym) else quote(a))
    if not rest:
        return line + ')'
    if head == 'pts' and all(isinstance(c, list) and c[0] == 'xy' for c in rest):
        # KiCad keeps a point list on one line under its (pts ...) header.
        inner = ' '.join('(xy %s %s)' % (c[1], c[2]) for c in rest)
        return '\n'.join([line, tab * (indent + 1) + inner, pad + ')'])

    parts = [line]
    for child in rest:
        if isinstance(child, list):
            parts.append(dumps(child, indent + 1, tab))
        else:
            parts.append(tab * (indent + 1) + (child if isinstance(child, Sym) else quote(child)))
    parts.append(pad + ')')
    return '\n'.join(parts)


# ---- convenience accessors -------------------------------------------------

def head(node):
    return str(node[0])


def children(node, name):
    return [c for c in node if isinstance(c, list) and head(c) == name]


def child(node, name):
    for c in node:
        if isinstance(c, list) and head(c) == name:
            return c
    return None


def atoms(node):
    return [a for a in node[1:] if not isinstance(a, list)]


def prop(sym, name):
    """Return the (property "name" "value") list of a symbol, or None."""
    for c in children(sym, 'property'):
        if len(c) > 1 and c[1] == name:
            return c
    return None


def prop_value(sym, name, default=None):
    p = prop(sym, name)
    return p[2] if p is not None and len(p) > 2 else default


def set_prop_value(sym, name, value):
    p = prop(sym, name)
    if p is None:
        raise KeyError(name)
    p[2] = value


def ref_of(sym):
    return prop_value(sym, 'Reference')


def load(path):
    with open(path) as fh:
        return parse(fh.read())


def save(path, node, trailing_newline=True):
    text = dumps(node)
    if trailing_newline:
        text += '\n'
    with open(path, 'w') as fh:
        fh.write(text)
