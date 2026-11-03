# Write a surface shader

A surface shader works out the colour and opacity of each point on a surface. moya compiles
shaders from source written in the shading language. There is no separate compile step and no
compiled shader file. [ShadingLanguage.md](ShadingLanguage.md) lists what the language holds.

## 1. Write the shader

Save this as `stripes.sl` in a directory of its own, such as `C:\renders\shaders`:

```
/* Stripes of two colours along s, lit like matte. */
surface stripes(float Ka = 1; float Kd = 1; float frequency = 8;
        color stripecolor = color (1, 1, 1)) {
    normal Nf = faceforward(normalize(N), I);
    float band = step(0.5, mod(s * frequency, 1));
    color base = mix(Cs, stripecolor, band);
    Oi = Os;
    Ci = Os * base * (Ka * ambient() + Kd * diffuse(Nf));
}
```

- `surface stripes(...)` names the shader and lists its parameters, each with a default.
- `N` is the surface normal, `I` the direction from the camera to the point, and `s` a texture
  coordinate. moya sets these before the shader runs.
- `faceforward` turns the normal toward the camera, so both sides of a surface are lit.
- `ambient()` and `diffuse()` add up the light that reaches the point.
- The shader's results are `Ci`, the colour, and `Oi`, the opacity. `Cs` and `Os` are the colour
  and opacity the scene gave the surface.

Name the file after the shader. moya looks for the file `stripes.sl` when a scene asks for
`stripes`.

## 2. Tell moya where the shader is

moya finds a shader on the shader search path. Set it before `WorldBegin`:

```
Option "searchpath" "shader" ["C:/renders/shaders:&"]
```

- The path is a list of directories, separated by colons. `&` stands for the path as it was
  before, so the line above adds a directory without removing any.
- A single letter before a colon is a drive letter, so `C:/renders/shaders` is one directory.
- Write `/` between directories. A backslash in a RIB string starts an escape.
- A relative directory is read from the directory moya was started in, which is moya's own. Give
  full paths.
- A shader on the search path is used instead of a built-in shader with the same name.

## 3. Use the shader

Name the shader with `Surface`, and pass its parameters:

```
Format 160 120 1
Projection "perspective" "fov" [40]
Option "searchpath" "shader" ["C:/renders/shaders:&"]
WorldBegin
LightSource "ambientlight" 1 "intensity" [0.2]
LightSource "distantlight" 2 "to" [0.3 -0.5 1]
Color [0.2 0.3 0.8]
Surface "stripes" "float frequency" [6] "color stripecolor" [1 0.9 0.2]
Polygon "P" [-1.5 -1 5  1.5 -1 5  1.5 1 5  -1.5 1 5] "st" [0 0  1 0  1 1  0 1]
WorldEnd
```

The quad shows six yellow and six blue stripes.

- **Give each parameter its type**, as in `"float frequency"`. moya has the types of RI's
  standard names, such as `Kd` and `roughness`, and of no others. A parameter with no type is
  skipped with a warning, and the shader uses its default. `Declare "frequency" "float"`
  before the request gives the type once for the rest of the file.
- **Give a polygon `"st"`** when a shader reads `s` or `t`. Without it, each polygon has its own
  coordinates. Under the reyes hider, each piece a large polygon is split into then runs from 0
  to 1, and the pattern breaks into patches.

## Use the lights directly

`diffuse()` and `specular()` cover most surfaces. To treat each light yourself, loop over the
lights with `illuminance`. Inside the loop, `L` points from the surface toward the light, and
`Cl` is the light's colour. This shader cuts the diffuse light into flat bands:

```
/* Diffuse light cut into a few flat bands. */
surface banded(float bands = 3) {
    normal Nf = faceforward(normalize(N), I);
    float lit = 0;
    illuminance(P, Nf, 1.5707963) {
        lit += comp(Cl, 0) * (normalize(L) . Nf);
    }
    lit = floor(lit * bands) / bands;
    Oi = Os;
    Ci = Os * Cs * (0.2 + lit);
}
```

`illuminance(P, Nf, 1.5707963)` visits each light that shines on the side `Nf` faces, within 90
degrees of it. In the shading language `.` is the dot product, and `^` the cross product. The
language has no `PI`, so write the number.

## When a shader fails

A shader that does not parse or compile is logged as an `[error]` in `v3d.log`, with the line
and column of the fault:

```
[error] the shader 'broken' in C:/renders/shaders/broken.sl does not compile -
'undefinedthing' is not declared at line 2, column 15
```

moya reports each broken shader once. A surface whose shader fails is drawn with `matte`
instead. A shader named that moya cannot find is logged in the same way.

`printf` compiles, but moya does not show what it prints. To see a value, write it into `Ci`.
