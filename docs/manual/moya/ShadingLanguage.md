# The shading language

moya runs shaders written in the RenderMan Shading Language (SL). This page lists the parts of
the language moya supports, the variables and functions it provides, and its built-in shaders.
[Shaders.md](Shaders.md) walks through writing one.

## Shader files

- **A shader is source code.** moya compiles it the first time a scene names it. There is no
  compiled shader file and no separate compiler.
- **moya looks for the file `<name>.sl`** in each directory on `Option "searchpath" "shader"`,
  where `<name>` is the name the scene gives. A file there wins over a built-in shader of the
  same name.
- **A file can hold several shaders.** moya uses the one whose name matches. A file that holds
  exactly one shader supplies it, whatever name its source gives it.
- **The C preprocessor is not run.** A line starting with `#`, such as `#include`, is an error.
- Comments are `/* ... */` and `// ...`. A block comment does not nest.

## Shader types

| Type | Runs | Writes |
|---|---|---|
| `surface` | At each point of a surface, named by `Surface` | `Ci`, `Oi` |
| `light` | For each point a light shines on, named by `LightSource` or `AreaLightSource` | `Cl`, `L` |
| `displacement` | At each point of a surface, before its surface shader, named by `Displacement` | `P`, `N` |
| `imager` | At each pixel of the finished picture, named by `Imager` | `Ci`, `Oi`, `alpha` |

A `volume` shader parses, but moya does not run it. A scene that names one is told the shader
is not supported.

## Shape of a shader

```
surface name(float Kd = 1; color tint = 1) {
    float helper(float x) {
        return x * x;
    }
    Ci = Cs * helper(Kd);
    Oi = Os;
}
```

- **Parameters** are separated by semicolons, and each has a default. A scene can set any of
  them by name.
- **A parameter is uniform unless declared `varying`**, or given a varying value in the body.
  A uniform parameter holds one value for the whole surface. Give a parameter `output` to let
  another shader read it (see [Message passing](#message-passing)).
- **A function is defined at the top of the shader's body**, before the statements, and only
  there. A function outside a shader is an error. A function cannot call itself, directly or
  through another function.
- A function's formal parameters are separated by semicolons or commas. An `output` formal
  passes its value back to the caller, which must pass a variable of the same type.

## Types

| Type | Holds |
|---|---|
| `float` | One number |
| `color` | Red, green and blue |
| `point` | A position |
| `vector` | A direction |
| `normal` | A surface normal |
| `matrix` | A 4 by 4 matrix |
| `string` | Text |
| `void` | No value, for a function that returns nothing |

- **`uniform` and `varying`** set whether a variable holds one value for every point or one
  value for each point. moya works out which a variable needs when you give neither. A variable
  declared `uniform` that is given a varying value is an error.
- **A float converts to the other types.** A float assigned to a colour, point, vector or normal
  fills every component. A float assigned to a matrix is that number on the diagonal.
- **A cast can name a space or a colour space.** `point "world" (0, 0, 0)` is the world origin,
  given in the current space. `color "rgb" (1, 0, 0)` is red. A colour space other than
  `"rgb"` is reported, and the colour is not changed. A space on a matrix, float or string is an
  error.
- **A cast picks between functions that differ in their result.** `color noise(P)` gives three
  separate patterns, and `float texture(name)` gives the first channel.
- **Arrays are not supported.**

## Operators and statements

- Arithmetic: `+`, `-`, `*`, `/`, and unary `-`. On colours and points they work on each
  component.
- **`.` is the dot product and `^` the cross product**, of points, vectors and normals. Both
  bind tighter than `*`.
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`. `==` and `!=` compare every component, and
  compare strings by their text.
- Logic: `&&`, `||`, `!`, and the conditional `a ? b : c`.
- Assignment: `=`, `+=`, `-=`, `*=`, `/=`.
- Statements: `if` and `else`, `for`, `while`, `break`, `continue` and `return`.
- Matrices follow RenderMan's order. A matrix written as 16 numbers is row by row, and the last
  row holds the translation. `A * B` applies `A` first, then `B`.

The keywords are the five shader types, the eight types, `uniform`, `varying`, the statements
above, `illuminance`, `illuminate` and `solar`. Any other word can name a variable or a function.

## Lighting

- **`illuminance(P, axis, angle) { ... }`**, in a surface shader, runs its body once for each
  light that shines on `P` from within `angle` radians of `axis`. Inside the body, `L` points from
  the surface toward the light, and `Cl` is the light that arrives. `L` and `Cl` mean nothing
  outside the body.
- **`illuminate(from) { ... }`**, in a light shader, shines from the point `from` in every
  direction. `illuminate(from, axis, angle)` shines inside a cone. Inside the body, `L` points
  from the light toward the point being lit, `Ps`.
- **`solar(axis, angle) { ... }`**, in a light shader, shines from far away along `axis`, like
  the sun.
- **A light with neither `illuminate` nor `solar` is ambient.** It has no direction, and
  `ambient()` adds it up. `illuminance` does not visit it.
- A `break` or `return` inside `illuminance` ends the loop for that point. It visits no later
  light there.

The built-in lights cast shadows by calling `transmission(Ps, Ps + L)`. A light shader you write
casts shadows only if it calls `transmission()`.

## Global variables

moya sets these before a shader runs. A shader can write only the ones marked.

| Variable | Type | Surface | Displacement | Light | Imager | Holds |
|---|---|---|---|---|---|---|
| `P` | point | read | read, write | read | read | The point being shaded. In a light, the origin of the light's placement, or an area light's sample |
| `N` | normal | read | read, write | | | The shading normal |
| `Ng` | normal | read | read | | | The geometric normal, from the primitive's winding |
| `I` | vector | read | read | | | From the camera to `P` |
| `E` | point | read | read | | | The camera's position |
| `Cs`, `Os` | color | read | | | | The colour and opacity the scene gave the surface |
| `s`, `t` | float | read | read | | | Texture coordinates |
| `u`, `v` | float | read | read | | | The surface's own parameters |
| `du`, `dv` | float | read | read | | | The step in `u` and `v` to the next shading point |
| `Ci`, `Oi` | color | write | | | read, write | The colour and opacity a surface gives, or a pixel's |
| `Ps` | point | | | read | | The point the light is shining on |
| `L` | vector | in `illuminance` | | write | | The direction between the surface and the light |
| `Cl` | color | in `illuminance` | | write | | The light that arrives |
| `alpha` | float | | | | read, write | How much of the pixel is covered |

- **A surface that never writes `Oi`** has the opacity in `Os`.
- **Under an orthographic camera**, `I` is `(0, 0, 1)` in camera space.
- **The current space** is camera space when the reyes hider shades and world space when the
  ray-trace hider does. Read a position through `ptransform()` to get the same answer under
  either hider.

## Spaces

A space names a coordinate system. The space functions and casts take these names:

| Name | Space |
|---|---|
| `"current"` | The space the shader runs in |
| `"object"` | The primitive's own space, as the transformation stood when it was given |
| `"shader"` | Where the scene placed the shader, as the transformation stood at `Surface`, `LightSource` and the others. A light runs in its own placement |
| `"world"` | World space |
| `"camera"` | Camera space, with the camera at the origin looking along +z |
| `"screen"` | After the projection |
| `"raster"` | Pixels, from the upper left corner of the picture |
| `"NDC"` | Raster space divided by the picture size, 0 to 1 across and down |
| A saved name | A space the scene saved with `CoordinateSystem` |

A point moved into or out of `"screen"`, `"raster"` or `"NDC"` is divided by its w, as a
projection requires. A vector or a normal is not.

## Built-in functions

### Maths

| Function | Returns |
|---|---|
| `abs`, `sign`, `floor`, `ceil`, `round`, `sqrt`, `exp`, `log(x)` | That function, of a float or of each component |
| `mod(a, b)`, `pow(a, b)`, `min(a, b)`, `max(a, b)` | That function, of each component |
| `clamp(x, lo, hi)`, `mix(a, b, f)` | `x` held to `lo` to `hi`. `a` blended toward `b` by `f` |
| `log(x, base)` | The logarithm of a float to a base |
| `step(edge, x)`, `smoothstep(lo, hi, x)` | 0 below the edge and 1 above. A smooth step from `lo` to `hi` |
| `radians`, `degrees`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan(x)`, `atan(y, x)` | Trigonometry, of floats only |

The componentwise functions take a float, a colour or a position, and a float argument applies
to every component. `max(0, Ci)` is a colour. Their result is the type the arguments combine
to, as with `+`, so `max(Cs, P)` is an error.

### Geometry

| Function | Returns |
|---|---|
| `length(v)`, `distance(p, q)` | A length |
| `normalize(v)` | `v` at length 1, of the same type |
| `faceforward(N, I)` | `N`, turned to face against `I` by the geometric normal `Ng` |
| `faceforward(N, I, Nref)` | `N`, turned to face against `I` by `Nref` |
| `reflect(I, N)` | `I` reflected about `N` |
| `refract(I, N, eta)` | `I` refracted through `N`, where eta is the ratio of the indices of refraction |
| `fresnel(I, N, eta, kr, kt)` | Writes into `kr` and `kt` how much a dielectric reflects and lets through |
| `fresnel(I, N, eta, kr, kt, R, T)` | Also writes the reflected and refracted directions into `R` and `T` |
| `depth(p)` | The depth of a point, from 0 at the near clipping distance to 1 at the far one |
| `xcomp`, `ycomp`, `zcomp`, `comp(x, i)` | One component |
| `setxcomp`, `setycomp`, `setzcomp`, `setcomp(x, i, value)` | Set one component of a variable |

Two-argument `faceforward` needs `Ng`, so a light shader must give the third argument.

### Spaces and matrices

| Function | Returns |
|---|---|
| `ptransform(space, p)`, `ptransform(from, to, p)` | A point moved from the current space, or from `from`, into a space |
| `vtransform`, `ntransform` | The same, for a vector and a normal |
| `mtransform(space, m)` | A matrix followed by the change into a space |
| `ctransform(space, c)`, `ctransform(from, to, c)` | A colour in another colour space. Only `"rgb"` is implemented |
| `determinant(m)` | The determinant |
| `translate(m, v)`, `rotate(m, angle, axis)`, `scale(m, v)` | `m` with a transformation applied before it |

`ptransform` is the function RenderMan calls `transform`.

### Derivatives

| Function | Returns |
|---|---|
| `Du(x)`, `Dv(x)` | How fast a float, a colour or a point changes along `u` or `v`. A point's derivative is a vector |
| `Deriv(x, y)` | How fast `x` changes with `y` |
| `area(P)` | The area of the patch between this shading point and the next |
| `calculatenormal(P)` | The normal of the surface `P` lies on. Use it after moving `P` |

A derivative compares a shading point with its neighbour, so it is as fine as the shading
points are. The derivative of a uniform value is zero.

### Lighting and tracing

| Function | Returns |
|---|---|
| `ambient()` | The light from every ambient light |
| `diffuse(N)` | The diffuse light arriving along `N`, from every light in front of it |
| `specular(N, V, roughness)` | The highlight seen from direction `V`. A smaller `roughness` gives a smaller, sharper highlight |
| `phong(N, V, size)` | A Phong highlight. A larger `size` gives a smaller highlight |
| `transmission(from, to)` | How much light passes between two points. Each object between them lets through one minus its `Os` |
| `trace(P, direction)` | The colour seen along a ray. It returns the background past the trace depth |

A shader may define its own `diffuse`, `specular` or `phong`, which then replaces the built-in.

### Patterns and textures

| Function | Returns |
|---|---|
| `noise(x)`, `noise(x, y)`, `noise(p)` | Perlin noise from 0 to 1, of a float, a pair or a point. A float, or a colour, point or vector by a cast |
| `texture(name)`, `texture(name, s, t)` | The image `name` at the point's `s` and `t`, or at the given coordinates. See [Textures.md](Textures.md) |

`noise` gives the same pattern on every run and every machine, and repeats every 256 units.

### Message passing

A shader can read an `output` parameter of another shader that runs at the same point:

| Function | Reads |
|---|---|
| `surface("name", v)` | In a light shader, the surface being lit |
| `displacement("name", v)` | In a surface shader, the displacement shader that ran on the same grid |
| `lightsource("name", v)` | In a surface shader, inside `illuminance`, the light being visited |

Each copies the parameter into `v` and returns 1, or returns 0 and leaves `v` as it was. The
parameter must be declared `output` and have `v`'s type. Under the ray-trace hider,
`displacement()` always returns 0.

## Built-in shaders

These are compiled into moya, so they need no files.

| Shader | Type | Parameters | What it does |
|---|---|---|---|
| `constant` | surface | none | The surface's own colour, unlit. The default surface |
| `matte` | surface | `Ka` 1, `Kd` 1 | Ambient and diffuse light |
| `metal` | surface | `Ka` 1, `Ks` 1, `roughness` 0.1 | A highlight in the surface's colour |
| `plastic` | surface | `Ka` 1, `Kd` 0.5, `Ks` 0.5, `roughness` 0.1, `specularcolor` 1 | Diffuse light and a white highlight |
| `paintedplastic` | surface | As `plastic`, and `texturename` | `plastic`, with the colour multiplied by an image |
| `shinymetal` | surface | `Ka` 1, `Ks` 1, `Kr` 1, `roughness` 0.1 | `metal`, with a traced reflection |
| `glass` | surface | `Ka` 0, `Ks` 0.5, `Kr` 1, `Kt` 1, `roughness` 0.05, `eta` 1.5 | Traced reflection and refraction |
| `ambientlight` | light | `intensity`, `lightcolor` | See [Lights.md](Lights.md#the-built-in-lights) |
| `distantlight` | light | `intensity`, `lightcolor`, `from`, `to` | |
| `pointlight` | light | `intensity`, `lightcolor`, `from` | |
| `spotlight` | light | `intensity`, `lightcolor`, `from`, `to`, `coneangle`, `conedeltaangle`, `beamdistribution` | |
| `arealight` | light | `intensity`, `lightcolor` | |
| `background` | imager | `background` 0 | Paints a colour behind what the scene left uncovered |

- `glass` is opaque to the camera: it shows what is behind it by tracing a refracted ray. Its
  `Opacity` sets how dark its shadow is. `Kt` and `eta` have no standard type, so pass them as
  `"float Kt"` and `"float eta"`.
- `shinymetal` and `glass` trace rays, and show the background where a ray meets nothing.

## Not supported

- `volume` shaders.
- Arrays.
- The preprocessor, and functions defined outside a shader.
- `shadow()`. It compiles, returns 0, and is reported once. Shadows come from `transmission()`.
- `environment()`, `cellnoise()`, `spline()`, `random()` and other functions this page does not
  list. A call to one is an error.
- A colour space other than `"rgb"`.
- `PI` and the other named constants. Write the number.
- `printf()` compiles, but moya does not show what it prints.
