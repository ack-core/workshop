#!/usr/bin/env python3

# Tool to convert the pixels of one color in a *.png to a flat triangle mesh
#
# Pixel (x, y) is the vertex (x, 0, y) - image column is 'x', image row is 'z', 'y' is
# always 0. All four image corners must carry the color, so the mesh spans the whole
# image rectangle and the triangulation needs no artificial vertices to start from.
#
# Output, four lines:
#   vertex count
#   vertices, single space inside one, double space between them
#   index count (3 per triangle)
#   indices, single space inside a triangle, double space between triangles

import argparse
import re
import sys

import png

COLOR_PATTERN = re.compile(r"\A0x([0-9a-fA-F]{6})\Z")


class TrianError(Exception):
    pass


def orient2d(a: (int, int), b: (int, int), c: (int, int)) -> int:
    """
    Sign of the turn a -> b -> c: positive is the winding the output format wants,
    clockwise seen from +y looking down, and 0 means the three are collinear.
    """
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def in_circumcircle(a: (int, int), b: (int, int), c: (int, int), d: (int, int)) -> bool:
    """Whether 'd' is strictly inside the circumcircle of a positively oriented (a, b, c)."""
    ax, az = a[0] - d[0], a[1] - d[1]
    bx, bz = b[0] - d[0], b[1] - d[1]
    cx, cz = c[0] - d[0], c[1] - d[1]

    # The 3x3 determinant of [ax, az, ax*ax + az*az] over the three vertices, expanded
    # along the last column. Coordinates are integers, so this is exact - none of the
    # usual Delaunay robustness failures can happen
    return ((ax * ax + az * az) * (bx * cz - bz * cx)
            - (bx * bx + bz * bz) * (ax * cz - az * cx)
            + (cx * cx + cz * cz) * (ax * bz - az * bx)) > 0


class Mesh:
    """
    Delaunay triangulation of a set of points, built by incremental Bowyer-Watson.

    Triangles are kept positively oriented, which is already the winding the output
    format asks for, so nothing is reordered on the way out. Neighbor 'k' of a triangle
    is the one across the edge opposite its vertex 'k', -1 where there is none.
    Triangles consumed by an insertion are marked dead rather than removed, so the
    indices already handed out stay valid.
    """

    def __init__(self, vertices: [(int, int)], corners: (int, int, int, int)):
        self._verts = vertices
        self._tri = []
        self._adj = []

        # The image rectangle split along the top-left to bottom-right diagonal. Its four
        # corners are cocircular, so either diagonal is a valid Delaunay triangulation of
        # them; the choice is fixed to keep the output deterministic. Every remaining
        # point lies inside this rectangle, and starting from a valid Delaunay
        # triangulation of a subset whose hull contains the rest gives the true Delaunay
        # triangulation of the whole set
        top_left, top_right, bottom_left, bottom_right = corners
        self._add(top_left, top_right, bottom_right, -1, 1, -1)
        self._add(top_left, bottom_right, bottom_left, -1, -1, 0)

        # Raster order puts consecutive points next to each other, so seeding each search
        # with the triangle the last insertion made keeps the walk a step or two long
        seed = 1
        placed = set(corners)
        for vertex in range(0, len(vertices)):
            if vertex not in placed:
                seed = self._insert(vertex, seed)

    def _add(self, a: int, b: int, c: int, n_a: int, n_b: int, n_c: int) -> int:
        self._tri += [a, b, c]
        self._adj += [n_a, n_b, n_c]

        return len(self._tri) // 3 - 1

    def _edge(self, t: int, k: int) -> (int, int):
        """The edge of triangle 't' opposite its vertex 'k', the one neighbor 'k' is across."""
        return self._tri[t * 3 + (k + 1) % 3], self._tri[t * 3 + (k + 2) % 3]

    def _locate(self, p: (int, int), seed: int) -> int:
        """Walk from 'seed' towards 'p', crossing any edge that 'p' lies on the far side of."""
        t = seed

        for _ in range(0, len(self._tri)):
            for k in range(0, 3):
                a, b = self._edge(t, k)
                if orient2d(self._verts[a], self._verts[b], p) < 0:
                    t = self._adj[t * 3 + k]
                    # Only reachable if 'p' is outside the rectangle, which the corner
                    # check rules out
                    if t < 0:
                        raise TrianError("vertex {} is outside the image".format(p))
                    break
            else:
                return t

        raise TrianError("could not locate vertex {}".format(p))

    def _in_circumcircle(self, t: int, p: (int, int)) -> bool:
        a, b, c = self._tri[t * 3:t * 3 + 3]

        return in_circumcircle(self._verts[a], self._verts[b], self._verts[c], p)

    def _insert(self, vertex: int, seed: int) -> int:
        p = self._verts[vertex]
        start = self._locate(p, seed)

        # Carve out every triangle whose circumcircle contains the new vertex. What is
        # left is a hole, star shaped around the vertex, whose boundary edges are the ones
        # facing a triangle that stayed - kept along with the slot pointing back at us
        inside = {start}
        boundary = []
        pending = [start]
        while pending:
            t = pending.pop()
            for k in range(0, 3):
                n = self._adj[t * 3 + k]
                if n in inside:
                    continue

                if n >= 0 and self._in_circumcircle(n, p):
                    inside.add(n)
                    pending.append(n)
                else:
                    slot = self._adj[n * 3:n * 3 + 3].index(t) if n >= 0 else -1
                    boundary.append(self._edge(t, k) + (n, slot))

        for t in inside:
            self._tri[t * 3] = -1

        # Fan the new vertex to the hole boundary. An edge the vertex lies on would give a
        # zero area triangle and is dropped instead, which splits that edge in two - this
        # happens for every vertex on the image border. At most one edge is ever dropped:
        # a boundary edge collinear with the vertex has to contain it, because the circle
        # through the edge's ends only covers their own stretch of that line. Nor is there
        # adjacency to repair across it, since a vertex on an inner edge sits inside the
        # circumcircles on both sides, which puts that edge inside the hole, not on its
        # boundary
        fan = {}
        last = start
        for a, b, n, slot in boundary:
            if orient2d(self._verts[a], self._verts[b], p) == 0:
                continue

            last = self._add(a, b, vertex, -1, -1, n)
            if n >= 0:
                self._adj[n * 3 + slot] = last

            # Slot 0 of the new triangle spans (b, vertex) and slot 1 spans (vertex, a);
            # the fan triangle next to it along that edge holds the same pair reversed
            fan[(b, vertex)] = (last, 0)
            fan[(vertex, a)] = (last, 1)

        # A hole boundary has at least three edges and at most one of them is dropped, so
        # the fan always spans it. Were it ever empty, 'last' would still name a triangle
        # this insertion just killed, and the next walk would start from stale adjacency
        # and quietly mislocate instead of failing
        if not fan:
            raise TrianError("degenerate fan at vertex {}".format(p))

        for (a, b), (t, k) in fan.items():
            neighbor = fan.get((b, a))
            if neighbor is not None:
                self._adj[t * 3 + k] = neighbor[0]

        return last

    def triangles(self) -> [(int, int, int)]:
        return [tuple(self._tri[t:t + 3]) for t in range(0, len(self._tri), 3) if self._tri[t] >= 0]


def parse_color(text: str) -> (int, int, int):
    """Parse '0xBBGGRR' - red is the lowest byte, so '0x0000ff' is (255, 0, 0)."""
    match = COLOR_PATTERN.match(text)
    if match is None:
        raise TrianError("'{}' is not a color, expected '0xBBGGRR' like '0x0000ff' for red".format(text))

    value = int(match.group(1), 16)

    return value & 0xff, (value >> 8) & 0xff, (value >> 16) & 0xff


def read_vertices(src: str, color: (int, int, int)) -> (int, int, [(int, int)]):
    """
    Read a png and collect the pixels of 'color' as (x, z) points, in raster order.

    A fully transparent pixel never matches, whatever its rgb says.
    """
    width, height, rows, _ = png.Reader(filename=src).asRGBA8()

    vertices = []
    for z, row in enumerate(rows):
        for x in range(0, width):
            pixel = row[x * 4:x * 4 + 4]
            if pixel[3] != 0 and tuple(pixel[0:3]) == color:
                vertices.append((x, z))

    return width, height, vertices


def find_corners(src: str, text: str, width: int, height: int, vertices: [(int, int)]) -> (int, int, int, int):
    """Check the input is usable and return the indices of the four image corners."""
    if width < 2 or height < 2:
        raise TrianError("'{}' is {}x{}, need at least 2x2".format(src, width, height))

    if not vertices:
        raise TrianError("no pixel of color {} in '{}'".format(text, src))

    index_of = {vertex: i for i, vertex in enumerate(vertices)}
    corners = [(0, 0), (width - 1, 0), (0, height - 1), (width - 1, height - 1)]

    missing = [corner for corner in corners if corner not in index_of]
    if missing:
        raise TrianError("'{}' has no {} pixel at corner(s) {}".format(
            src, text, ", ".join("({}, {})".format(x, z) for x, z in missing)))

    return tuple(index_of[corner] for corner in corners)


def write_mesh(dst: str, vertices: [(int, int)], triangles: [(int, int, int)]):
    with open(dst, mode="w") as dst_file:
        dst_file.write("{}\n".format(len(vertices)))
        dst_file.write("  ".join("{} 0 {}".format(x, z) for x, z in vertices) + "\n")
        dst_file.write("{}\n".format(len(triangles) * 3))
        dst_file.write("  ".join("{} {} {}".format(a, b, c) for a, b, c in triangles) + "\n")


def convert_png(src: str, text: str, dst: str):
    color = parse_color(text)
    width, height, vertices = read_vertices(src, color)
    corners = find_corners(src, text, width, height, vertices)

    triangles = Mesh(vertices, corners).triangles()
    write_mesh(dst, vertices, triangles)

    print("'{}' -> '{}' ({}x{}, {} vertices, {} triangles)".format(
        src, dst, width, height, len(vertices), len(triangles)))


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="png2trian",
        description="Tool to convert the pixels of one color in a *.png to a flat triangle mesh",
        epilog="example: png2trian.py -s trian-00.png -c 0x0000ff -d trian-00.txt")
    parser.add_argument("-s", "--src", type=str, required=True, help="Source *.png file")
    parser.add_argument("-c", "--color", type=str, required=True,
                        help="Color marking a vertex, '0xBBGGRR', e.g. '0x0000ff' for red")
    parser.add_argument("-d", "--dst", type=str, required=True, help="Destination text file")
    args = parser.parse_args()

    try:
        convert_png(args.src, args.color, args.dst)
    except (TrianError, OSError, png.Error) as e:
        print("Error: {}".format(e), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
