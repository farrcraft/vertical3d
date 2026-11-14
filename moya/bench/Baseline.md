# moya_bench baseline

`moya_bench` run in the `x64-Release` build at `c8cba0d2` with the textured room added, with
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
| small triangles | sse42 | 1048352 | 133.8 | 1299.5 | 57.5 | 11.19 | 100.0 | 9.30 | 45.9 |
| small triangles | avx2 | 1048352 | 133.8 | 1312.7 | 57.5 | 12.26 | 100.0 | 10.69 | 45.9 |
| large triangles | sse42 | 32 | 0.1 | 1.1 | 47.1 | 8.02 | 71.9 | 6.84 | 48.6 |
| large triangles | avx2 | 32 | 0.1 | 1.1 | 47.1 | 9.07 | 71.9 | 6.82 | 48.6 |
| curves | sse42 | 98304 | 10.0 | 61.6 | 59.5 | 3.85 | 46.1 | 3.93 | 59.2 |
| curves | avx2 | 98304 | 10.0 | 59.2 | 59.5 | 4.42 | 46.1 | 4.07 | 59.2 |
| instances | sse42 | 20000 | 3.5 | 156.3 | 56.8 | 8.32 | 100.0 | 7.84 | 47.3 |
| instances | avx2 | 20000 | 3.5 | 157.9 | 56.8 | 9.34 | 100.0 | 8.46 | 47.3 |
| motion | sse42 | 133120 | 27.9 | 356.3 | 134.1 | 6.19 | 100.0 | 6.69 | 58.1 |
| motion | avx2 | 133120 | 27.9 | 355.5 | 134.1 | 6.40 | 100.0 | 7.15 | 58.1 |

## The lit and textured rooms

The integrator over the lit room and the textured room, run in the same build on the same
machine. Each rate is of the fastest of three timed renders, over the passes alone. The rays are
counted in a fourth render, which also gives the shares of every thread's time in traversal, in
materials' programs and in texture lookups. A program's share holds the lookups it makes. Two runs
differ by up to a tenth in the rates.

| lit scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % | program % | texture % |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| lit room | sse42 | 1048364 | 65536 | 16 | 1.639 | 6.530 | 3.98 | 32.6 | 0.0 | 0.0 |
| lit room | avx2 | 1048364 | 65536 | 16 | 1.714 | 6.827 | 3.98 | 30.6 | 0.0 | 0.0 |
| textured room | sse42 | 1048364 | 65536 | 16 | 0.306 | 1.120 | 3.66 | 5.0 | 76.5 | 75.3 |
| textured room | avx2 | 1048364 | 65536 | 16 | 0.300 | 1.096 | 3.66 | 4.9 | 76.4 | 75.1 |
