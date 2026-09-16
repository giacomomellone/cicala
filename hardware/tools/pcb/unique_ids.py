"""Give cloned footprint items unique, deterministic board UUIDs."""
from collections import Counter
from uuid import NAMESPACE_URL, uuid5
import sys

import ksexp


def uuid_nodes(node, path=()):
    if node[0] == 'uuid':
        yield path, node
    for i, child in enumerate(node):
        if isinstance(child, list):
            yield from uuid_nodes(child, path + (i,))


def normalize(board):
    counts = Counter(n[1] for _, n in uuid_nodes(board))
    changed = 0
    for fp in ksexp.children(board, 'footprint'):
        ref = ksexp.ref_of(fp)
        for path, node in uuid_nodes(fp):
            if counts[node[1]] > 1:
                node[1] = str(uuid5(NAMESPACE_URL, f'cicala/rev-a/{ref}/{path}/{node[1]}'))
                changed += 1
    remaining = [n[1] for _, n in uuid_nodes(board)]
    if len(remaining) != len(set(remaining)):
        raise ValueError('Duplicate UUID outside a footprint')
    return changed


def normalize_file(path):
    board = ksexp.load(path)
    changed = normalize(board)
    if changed:
        ksexp.save(path, board)
    return changed


if __name__ == '__main__':
    print('reidentified', normalize_file(sys.argv[1]), 'copied footprint items')
