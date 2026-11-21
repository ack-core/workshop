import argparse
import math
import random
import sys

MIN_ANGLE = math.radians(20.0)
MIN_VECTOR_LENGTH = 0.1
LOOKUP_SIZE = 16

def random_unit():
    while True:
        v = [random.gauss(0.0, 1.0) for _ in range(3)]
        l = math.sqrt(sum(c * c for c in v))
        if l > 1e-6:
            return [c / l for c in v]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def scale(v, k):
    return [c * k for c in v]


def make_set(m, min_angle, attempts):
    max_cos = math.cos(min_angle)
    result = [random_unit()]
    for _ in range(attempts):
        if len(result) == m:
            return [scale(v, random.uniform(MIN_VECTOR_LENGTH, 1.0)) for v in result]
        v = random_unit()
        if all(abs(dot(v, u)) < max_cos for u in result):
            result.append(v)
    sys.exit(f"failed to form set of {len(result) + 1} vectors with min angle {min_angle}")


def mat_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def rotation(ax, ay, az):
    cx, sx, cy, sy, cz, sz = math.cos(ax), math.sin(ax), math.cos(ay), math.sin(ay), math.cos(az), math.sin(az)
    rx = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    ry = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    rz = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]
    return mat_mul(rz, mat_mul(ry, rx))


def rotation_angle(a, b):
    trace = sum(a[k][i] * b[k][i] for i in range(3) for k in range(3))
    return math.acos(max(-1.0, min(1.0, (trace - 1.0) * 0.5)))


def make_rotations(n, min_angle, attempts):
    result = [[[1, 0, 0], [0, 1, 0], [0, 0, 1]]]
    for _ in range(attempts):
        if len(result) == n:
            return result
        r = rotation(*(random.uniform(-math.pi, math.pi) for _ in range(3)))
        if all(rotation_angle(r, other) > min_angle for other in result):
            result.append(r)
    sys.exit(f"failed to form {len(result) + 1} sets with min angle {min_angle}")


def make_lookup(n):
    while True:
        s = [i % n for i in range(LOOKUP_SIZE)]
        random.shuffle(s)
        if n < 2 or all(s[i] != s[i - 1] for i in range(LOOKUP_SIZE)):
            return s


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("n", type=int)
    parser.add_argument("m", type=int)
    parser.add_argument("--seed", type=int)
    parser.add_argument("--attempts", type=int, default=100000)
    args = parser.parse_args()

    random.seed(args.seed)
    base = make_set(args.m, MIN_ANGLE, args.attempts)
    rotations = make_rotations(args.n, MIN_ANGLE, args.attempts)

    print(f"rndv[{args.n * args.m}] : float3 =")
    for r in rotations:
        vectors = ([dot(row, v) for row in r] for v in base)
        print("    " + "".join("[" + ",".join(f"{c:.2f}" for c in v) + "]" for v in vectors))

    lookup = [make_lookup(args.n) for _ in range(3)]
    print(f"rndo[{LOOKUP_SIZE}] : int4 =")
    print("    " + "".join(f"[{x},{y},{z},0]" for x, y, z in zip(*lookup)))


if __name__ == "__main__":
    main()
