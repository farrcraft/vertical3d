# moya_bench baseline

`moya_bench` run in the `x64-Release` build at `4f4d8279` with the room with outputs added, with
nothing else running. Compare a change against this table on the same machine, in the same build.
Two runs of one build differ by up to a tenth in the rates, so a smaller difference is noise.

- **Machine:** AMD Ryzen 7 5800X, 8 cores and 16 threads, Windows 11, MSVC 19.51. It has no
  AVX-512, so the table has no row for it.
- **Rays:** 1048576 of each kind, the fastest of three traces. A ray is traced for its nearest
  hit and for whether it meets anything, so each ray is two traversals.
- **Memory:** the hierarchies' nodes, references, instances and keys. The primitives' own points
  are not counted.

| scene | isa | primitives | gather ms | build ms | bytes per primitive | coherent Mrays/s | hit % | incoherent Mrays/s | hit % |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| small triangles | sse42 | 1048352 | 158.3 | 1440.4 | 57.5 | 10.87 | 100.0 | 10.32 | 45.9 |
| small triangles | avx2 | 1048352 | 158.3 | 1437.3 | 57.5 | 13.38 | 100.0 | 10.95 | 45.9 |
| large triangles | sse42 | 32 | 0.0 | 1.0 | 47.1 | 8.78 | 71.9 | 6.95 | 48.6 |
| large triangles | avx2 | 32 | 0.0 | 1.0 | 47.1 | 8.80 | 71.9 | 7.27 | 48.6 |
| curves | sse42 | 98304 | 11.6 | 66.3 | 59.5 | 4.12 | 46.1 | 4.04 | 59.2 |
| curves | avx2 | 98304 | 11.6 | 74.9 | 59.5 | 4.30 | 46.1 | 4.15 | 59.2 |
| instances | sse42 | 20000 | 3.0 | 161.1 | 56.8 | 8.61 | 100.0 | 7.96 | 47.3 |
| instances | avx2 | 20000 | 3.0 | 167.7 | 56.8 | 9.88 | 100.0 | 9.39 | 47.3 |
| motion | sse42 | 133120 | 26.3 | 388.9 | 134.1 | 6.24 | 100.0 | 6.49 | 58.1 |
| motion | avx2 | 133120 | 26.3 | 382.3 | 134.1 | 6.40 | 100.0 | 7.00 | 58.1 |

## The lit and textured rooms

The integrator over the lit room and the textured room, run in the same build on the same
machine. Each rate is of the fastest of three timed renders, over the passes alone. The rays are
counted in a fourth render, which also gives the shares of every thread's time in traversal, in
materials' programs and in texture lookups. A program's share holds the lookups it makes. Two runs
differ by up to a tenth in the rates.

| lit scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % | program % | texture % |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| lit room | sse42 | 1048364 | 65536 | 16 | 1.821 | 7.254 | 3.98 | 34.0 | 0.0 | 0.0 |
| lit room | avx2 | 1048364 | 65536 | 16 | 1.705 | 6.793 | 3.98 | 32.6 | 0.0 | 0.0 |
| textured room | sse42 | 1048364 | 65536 | 16 | 0.323 | 1.180 | 3.66 | 4.6 | 82.5 | 81.4 |
| textured room | avx2 | 1048364 | 65536 | 16 | 0.324 | 1.186 | 3.66 | 4.7 | 82.1 | 80.9 |

## The room with outputs

The lit room with the outputs a compositor asks for, run in the same build on the same machine.
They are five light path expressions, a channel for each of two light groups, Cryptomatte layers
of objects and of materials, and a deep file. Each round renders the plain lit room and then the
room with outputs, and each rate is of the fastest of three. The plain room's rate here is from
these rounds, not the table above. The coverage and deep shares are of the time the host takes
after each pass to merge Cryptomatte coverage and to join deep lists.

| outputs | isa | Msamples/s | rate of the lit room % | coverage % | deep % |
|---|---|---:|---:|---:|---:|
| lit room with outputs | sse42 | 1.574 | 93.9 | 1.0 | 1.4 |
| lit room with outputs | avx2 | 1.650 | 94.6 | 1.1 | 1.7 |

## The scenes of media, subsurface and hair

The fog room, the subsurface room and the hair lock, run at `e49b17d9` with them added, on the
same machine in the same build. Each rate is of the fastest of three timed renders, and the
shares are of a fourth, counted, as the lit and textured rooms' are. Tracking's share holds the
traversal, grid lookups and programs it makes. The lit room ran at 1.366 Msamples/s at sse42 and
1.420 at avx2 in the same run, so each scene can be read against it. Two runs differ by up to a
tenth in the rates.

| media scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % | tracking % | grid % | program % | hair % |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| fog room | sse42 | 1048364 | 65536 | 16 | 0.273 | 2.105 | 7.70 | 12.2 | 80.8 | 19.4 | 18.7 | 0.0 |
| fog room | avx2 | 1048364 | 65536 | 16 | 0.297 | 2.288 | 7.70 | 11.1 | 81.5 | 17.3 | 20.1 | 0.0 |
| subsurface room | sse42 | 1048367 | 65536 | 16 | 0.783 | 6.147 | 7.85 | 40.2 | 18.8 | 0.0 | 0.0 | 0.0 |
| subsurface room | avx2 | 1048367 | 65536 | 16 | 0.840 | 6.597 | 7.85 | 38.9 | 19.3 | 0.0 | 0.0 | 0.0 |
| hair lock | sse42 | 81920 | 65536 | 16 | 1.130 | 1.620 | 1.43 | 56.1 | 0.0 | 0.0 | 0.0 | 14.1 |
| hair lock | avx2 | 81920 | 65536 | 16 | 1.151 | 1.650 | 1.43 | 55.6 | 0.0 | 0.0 | 0.0 | 14.2 |

## Edits in a render session

A render session over the painted room, the lit room with a material on its walls, run in the
`x64-Release` build on the same machine with the interactive plan's step 14. Each row is one
build: from nothing, then after one edit of each kind, each sampled to its end before the next.
It gives each part's milliseconds as `Session::lastBuild()` gives them, what the build made
again, and the time from the edit to the session's first picture, which is coarse. The lit room
ran at 1.393 Msamples/s at sse42 and 1.352 at avx2 in the same run.

| interactive | isa | edit | geometry ms | surfaces ms | volumes ms | textures ms | lights ms | filter ms | film ms | rebuilt | first picture ms |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---|---:|
| painted room | sse42 | from nothing | 3764.0 | 68.9 | 0.0 | 0.0 | 75.7 | 0.1 | 2.3 | 5 geometries, 1 groups, 1 materials, 4 lights, light tree, volumes, surfaces, filter, film | 3960.6 |
| painted room | sse42 | camera move | 583.8 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 1.7 | nothing | 606.9 |
| painted room | sse42 | instance move | 338.9 | 60.4 | 0.0 | 0.0 | 15.3 | 0.0 | 1.4 | 1 groups, light tree, surfaces | 432.4 |
| painted room | sse42 | material change | 872.6 | 109.3 | 0.0 | 0.0 | 24.0 | 0.0 | 1.8 | 1 materials, surfaces | 1045.9 |
| painted room | sse42 | light change | 451.1 | 85.7 | 0.0 | 0.0 | 30.0 | 0.0 | 1.6 | 1 geometries, 1 groups, 1 lights, light tree, surfaces | 611.0 |
| painted room | avx2 | from nothing | 3823.1 | 58.7 | 0.0 | 0.0 | 81.6 | 0.2 | 2.2 | 5 geometries, 1 groups, 1 materials, 4 lights, light tree, volumes, surfaces, filter, film | 3983.6 |
| painted room | avx2 | camera move | 313.2 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 1.2 | nothing | 328.4 |
| painted room | avx2 | instance move | 316.8 | 57.5 | 0.0 | 0.0 | 15.4 | 0.0 | 1.6 | 1 groups, light tree, surfaces | 409.9 |
| painted room | avx2 | material change | 853.4 | 83.9 | 0.0 | 0.0 | 34.8 | 0.0 | 1.4 | 1 materials, surfaces | 1019.1 |
| painted room | avx2 | light change | 300.5 | 56.2 | 0.0 | 0.0 | 25.2 | 0.0 | 1.8 | 1 geometries, 1 groups, 1 lights, light tree, surfaces | 400.0 |

A camera move builds no hierarchy, surfaces, lights, filter or film. Its time is in the geometry's
lap: the gather places every geometry in new prototypes, and the assembly is laid out again,
though every hierarchy is kept. That is most of the time to a first picture after any edit but a
material change, whose lap also compiles the material.
