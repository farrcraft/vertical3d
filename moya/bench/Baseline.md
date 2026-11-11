# moya_bench baseline

`moya_bench` run in the `x64-Release` build at `48ab6093`, with nothing else running. Compare a
change against this table on the same machine, in the same build. Two runs of one build differ
by up to a tenth in the rates, so a smaller difference is noise.

- **Machine:** AMD Ryzen 7 5800X, 8 cores and 16 threads, Windows 11, MSVC 19.51. It has no
  AVX-512, so the table has no row for it.
- **Rays:** 1048576 of each kind, the fastest of three traces. A ray is traced for its nearest
  hit and for whether it meets anything, so each ray is two traversals.
- **Memory:** the hierarchies' nodes, references, instances and keys. The primitives' own points
  are not counted.

| scene | isa | primitives | gather ms | build ms | bytes per primitive | coherent Mrays/s | hit % | incoherent Mrays/s | hit % |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| small triangles | sse42 | 1048352 | 129.9 | 1347.8 | 57.5 | 11.16 | 100.0 | 10.28 | 45.9 |
| small triangles | avx2 | 1048352 | 129.9 | 1388.1 | 57.5 | 12.51 | 100.0 | 11.22 | 45.9 |
| large triangles | sse42 | 32 | 0.1 | 1.0 | 47.0 | 8.63 | 71.9 | 6.56 | 48.6 |
| large triangles | avx2 | 32 | 0.1 | 1.0 | 47.0 | 8.44 | 71.9 | 6.83 | 48.6 |
| curves | sse42 | 98304 | 11.6 | 65.4 | 59.5 | 4.05 | 46.1 | 4.20 | 59.2 |
| curves | avx2 | 98304 | 11.6 | 78.6 | 59.5 | 4.43 | 46.1 | 4.01 | 59.2 |
| instances | sse42 | 20000 | 4.4 | 182.5 | 56.8 | 7.43 | 100.0 | 7.24 | 47.3 |
| instances | avx2 | 20000 | 4.4 | 171.4 | 56.8 | 9.17 | 100.0 | 8.24 | 47.3 |
| motion | sse42 | 133120 | 31.1 | 380.9 | 134.0 | 5.87 | 100.0 | 6.39 | 58.1 |
| motion | avx2 | 133120 | 31.1 | 374.8 | 134.0 | 6.49 | 100.0 | 6.84 | 58.1 |

## The lit room

The integrator over the lit room, run in the same build on the same machine. Each
rate is of the fastest of three timed renders, over the passes alone. The rays are counted in a
fourth render, which also gives the share of every thread's time in traversal. Two runs differ by
up to a tenth in the rates.

| lit scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| lit room | sse42 | 1048364 | 65536 | 16 | 1.973 | 7.863 | 3.99 | 38.2 |
| lit room | avx2 | 1048364 | 65536 | 16 | 2.071 | 8.252 | 3.99 | 34.6 |
