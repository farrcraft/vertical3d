# imagetool

A command-line tool over `api/image`. It reads an image, can report its size, can cut out a
rectangle, and can write the result in another format. It also packs images into a sprite sheet
and cuts a sheet back into its sprites.

```
imagetool --file in.png --info
imagetool --file sheet.png --crop 64,0,32,32 --outfile sprite.tga
imagetool --file photo.jpg --outfile photo.bmp
imagetool --pack sprites/ --sheet units --outfile data/units.png
imagetool --unpack data/sprites.json --sheet units
```

| Option | Effect |
|---|---|
| `--file <path>` | The image to read. Without it, the tool prints the help |
| `--info` | Print the width, height and bits per pixel of the image read |
| `--outfile <path>` | Write the image to this file |
| `--crop x,y,width,height` | Cut out this rectangle before writing. Requires `--outfile`, four non-negative whole numbers, and a rectangle inside the image |
| `--pack <dir or files>` | Pack these images, and every image in these directories, into one sheet. Requires `--sheet`, and takes no `--file`, `--crop` or `--info` |
| `--sheet <name>` | The name of the sheet to pack, or the one sheet to unpack |
| `--unpack <sprites.json>` | Cut every sprite of the document's sheets, or of the one `--sheet` names, out of its image |
| `--silent` | Do not print the progress lines. `--info` still prints |
| `--help` | Print the options |

The format is chosen by the file extension, for reading and writing alike: `png`, `jpg` or
`jpeg`, `tga` and `bmp`. Relative paths resolve against the working directory. The tool exits
with a failure status and a message on any error.

**A pack writes two files.** The sheet goes to `--outfile`, or to `<sheet>.png` when there is
none, and `sprites.json` goes beside it. Each sprite is named for its file without the
extension, and a directory's images are packed in name order. A `sprites.json` that is already
there is read first, and only the sheet of that name is replaced, so one document can hold
several sheets. A sheet is at most 4096 pixels a side, and sprites that do not fit are refused.
Write the sheet as png, tga or bmp: a jpeg sheet changes its pixels, and the sprites no longer
match the files they came from.

**An unpack writes one png for each sprite**, to `<sheet>/<sprite>.png` beside the document.
The sheet's image is read from beside the document, and has to be the size the sheet states. A
sheet or sprite name with a directory in it is refused.
