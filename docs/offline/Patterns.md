# Patterns

A material's program compiles MaterialX's procedural nodes: its noises, its ramps and splits, and
its rotations. Each gives what MaterialX's own implementations give, so a MaterialX document shows
the same pattern in moya as in any renderer built on them. MaterialX's specification names each
noise but not its algorithm, so this page states the algorithms in full. The kernel code is
`kernel/program/Noise.h` and `kernel/program/Pattern.h`, and [Surfaces.md](Surfaces.md#the-node-program)
says how a program runs.

## The nodes

- **Noises**: `noise2d`, `noise3d`, `fractal2d`, `fractal3d`, `cellnoise2d`, `cellnoise3d`,
  `worleynoise2d` and `worleynoise3d`, of every output type MaterialX defines in code.
- **Ramps and splits**: `ramplr`, `ramptb`, `splitlr` and `splittb`.
- **Rotations**: `rotate2d` and `rotate3d`.
- **Nodes MaterialX defines as graphs**, such as `checkerboard`, `ramp4`, `place2d` and the
  colour noises, are flattened into these and the program's other nodes.
- **An input given no value reads the geometry its definition names.** `UV0` reads `st`.
  `Pobject`, `Nobject`, `Tobject` and `Bobject` read the shading point's position, normal, tangent
  and bitangent in the geometry's own space, and `Pworld` and its kin read them in the world. The
  read is compiled once, however many inputs make it. An input set to a value keeps the value.

Each instruction is an `Op`, from `Op::Noise` to `Op::Rotate3d`, and `program::patternOf()` runs
it. An integer input, such as `octaves` or `style`, is held as a float, as every integer is.

## The hash

Every noise numbers a lattice cell by its whole coordinates and hashes them with Bob Jenkins'
`hashword` from lookup3, which is in the public domain. The words are 32-bit and unsigned, and a
negative coordinate is hashed as its two's-complement bits. A coordinate is the floor of the
point's, held to the range a 32-bit integer holds.

- **The seed** of a hash of n words is `0xdeadbeef + 4n + 13`. Each of `a`, `b` and `c` starts
  from it.
- **Up to three words** are added to `a`, `b` and `c` in order, and the hash is `final(a, b, c)`.
- **Four words**: the first three are added, `mix(a, b, c)` runs, the fourth is added to `a`, and
  the hash is `final(a, b, c)`.
- **A hash as a number from zero to one** is the hash divided by `0xffffffff`, both as 32-bit
  floats. The divisor rounds to 2³², so the number is below one.

`mix` and `final` are lookup3's, where `rot(x, k)` turns `x` left by `k` bits:

```
mix:   a -= c; a ^= rot(c,  4); c += b;    b -= a; b ^= rot(a,  6); a += c;
       c -= b; c ^= rot(b,  8); b += a;    a -= c; a ^= rot(c, 16); c += b;
       b -= a; b ^= rot(a, 19); a += c;    c -= b; c ^= rot(b,  4); b += a;

final: c ^= b; c -= rot(b, 14);    a ^= c; a -= rot(c, 11);    b ^= a; b -= rot(a, 25);
       c ^= b; c -= rot(b, 16);    a ^= c; a -= rot(c,  4);    b ^= a; b -= rot(a, 14);
       c ^= b; c -= rot(b, 24);    the hash is c
```

## Perlin noise

Perlin noise is Ken Perlin's improved noise, from *Improving Noise* (SIGGRAPH 2002), in two
dimensions and in three. It lies roughly between -1 and 1, and is zero at every lattice point.

- **The cell** is the floor of each coordinate, and `f` is the point's offset from that corner.
- **Each corner of the cell** is hashed by its whole coordinates, two words in two dimensions and
  three in three. Its gradient's dot product with the point's offset from the corner is taken
  from the hash's low bits, `h`.
- **In three dimensions**, `h` is the hash's low four bits. `u` is x where `h < 8`, and y
  otherwise. `v` is y where `h < 4`, x where `h` is 12 or 14, and z otherwise. The dot product
  is `u + v`, with `u` negated where bit 0 of `h` is set and `v` negated where bit 1 is. These
  are the twelve edges of a cube, four of them twice.
- **In two dimensions**, `h` is the low three bits. `u` is x where `h < 4`, and y otherwise, and
  `v` is twice the other coordinate. The signs are taken as in three dimensions. These are the
  eight gradients (±1, ±2) and (±2, ±1).
- **The corners are mixed** along x, then y, then z, each by `fade(f)`, where
  `fade(t) = t³(t(6t − 15) + 10)`.
- **The result is scaled** by 0.6616 in two dimensions and by 0.9820 in three, to fill about
  [-1, 1].

A noise of three channels takes its gradients from the bytes of each corner's one hash. The first
channel reads bits 0 to 7, the second 8 to 15, and the third 16 to 23. The first channel is
therefore the noise of one channel.

The node's output is its noise times `amplitude`, plus `pivot`. A float output is the noise of one
channel. A vector2 is the first two channels of the noise of three, and a vector3 all three. A
vector4 is those three and the noise of one channel at the point plus (19, 73), or plus
(19, 73, 29) in three dimensions.

## Fractal noise

A fractal sums `octaves` Perlin noises. The first is at the point and of weight one. Each after
it is at the point times `lacunarity` again, and of the weight before times `diminish`. moya sums
at most 32 octaves, and no fewer than none.

A float output is the fractal of one channel, and a vector3 the fractal of three channels. A
vector2 is two fractals of one channel, the second at the point plus (19, 193), or plus
(19, 193, 17) in three dimensions. A vector4 is the fractal of three channels and the fractal of
one at that offset point. The output is the fractal times `amplitude`, with no pivot.

## Cell noise

Cell noise gives a number from zero to one for the unit cell a point lies in.

- **One channel** is the hash of the cell's whole coordinates, two words or three, as a number
  from zero to one.
- **Three channels in two dimensions** are the hashes of the three words (x, y, k), for k of
  zero, one and two.
- **Three channels in three dimensions** are the hashes of the four words (x, y, z, k), for k of
  zero, one and two.

`cellnoise2d` and `cellnoise3d` give one channel. Worley noise reads three.

## Worley noise

Worley noise is Steven Worley's cellular noise, from *A Cellular Texture Basis Function*
(SIGGRAPH 1996). Each cell holds one feature point, and the noise is the distance to the nearest.

- **The cell** is the floor of each coordinate, and `f` is the point's offset from that corner.
- **The cells searched** are the cell and its neighbours, 3 × 3 in two dimensions and 3 × 3 × 3
  in three. They are taken with the x offset outermost and the last coordinate's innermost, each
  from -1 to 1.
- **A neighbour's feature point**, at offset `d` from the cell, is `d + 0.5 + jitter × (c − 0.5)`.
  `c` is the first two or three channels of the cell noise of the cell at `d`. A jitter of zero
  puts each feature point at its cell's middle.
- **The three least squared distances** from `f` are kept in order, from 10⁶. The nearest feature
  point is the first whose squared distance is less than the least before it.
- **The distance style**, `style` zero, gives the square roots of the least one, two or three
  squared distances, for a float, a vector2 or a vector3.
- **The solid style**, `style` one, gives the cell noise at the nearest feature point. A float
  is its one channel, and a vector2 or a vector3 the first two or all three of its three.

## Ramps and splits

- **`ramplr`** mixes from `valuel` to `valuer` as u goes from zero to one, and `ramptb` from
  `valueb` to `valuet` as v does. The coordinate is held to [0, 1].
- **`splitlr`** gives `valuel` where u is below `center`, and `valuer` where it is not.
  `splittb` gives `valueb` where v is below `center`, and `valuet` where it is not.

MaterialX's implementations smooth a split's step over a pixel, by the screen derivatives of
the coordinate the split reads. The program carries no derivatives, so moya's step is sharp.

## Rotations

- **`rotate2d`** turns a vector by `amount` degrees: (x cos a + y sin a, y cos a − x sin a). The
  turn is clockwise with v upward.
- **`rotate3d`** turns a vector `v` about `axis`, made of unit length as `k`, by `amount` degrees:
  `v cos a + (v × k) sin a + k (k · v)(1 − cos a)`. The turn is clockwise as seen from the axis's
  tip. An axis of no length leaves the vector as it is.

MaterialX's specification calls a positive turn counter-clockwise. Its GLSL and OSL
implementations turn clockwise, and so does every renderer built on them, so moya turns clockwise.
`place2d` turns its texture coordinates through `rotate2d`.

## How the patterns are checked

`moya_program_noises_give_what_materialx_gives_in_osl_test` in `moya/tests/ProgramTest.cxx`
compares every noise with the values Open Shading Language gives for MaterialX's OSL
implementation, at eight points. The points lie near the origin, far from it, below zero and on a
lattice point. Each value agrees within 10⁻⁵.
