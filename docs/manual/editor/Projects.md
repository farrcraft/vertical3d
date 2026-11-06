# Projects and files

How to create, open and save a project, get back work an autosave kept, bring in a glTF or USD
file, and export the scene as RIB. [Concepts.md](Concepts.md#projects-and-scenes) defines a
project.

## The file chooser

Opening, saving under a new name, and importing each show the file chooser. It has a line
saying which directory it lists, the list, a name box, and the buttons **Accept** and
**Cancel**.

- The list starts with `..`, then the directories, then the files. When you open a file, it
  lists only the kinds of file that can be opened.
- Click `..` to go up a directory. Click a directory to go into it.
- Click a file to put its name in the name box, or type a name there.
- Click **Accept** to choose the name in the box, in the directory listed.
- Click **Cancel** to close the chooser and do nothing.

The chooser starts in the directory that holds the open project.

## Create a project

The editor starts with an empty scene in a project named `project`, in a `Vertical3D` folder in
your documents. To start a project of your own:

1. Choose **File > Save As**.
2. Go to the directory the project should be made in.
3. Type a name for the project in the name box.
4. Click **Accept**.

The editor makes a directory of that name and saves the scene into it. The name must not be a
directory that is there already. The new project is the open one, and the caption names it.

## Save

Press Ctrl+S, or choose **File > Save**. The scene is written to the open project, without
asking. The ` *` after the project's name, in the caption and in the bar at the top of the
window, goes away.

To save under a new name, choose **File > Save As**, as above. The project you were in keeps
what it held at its last save.

A project directory holds:

| Path | What it holds |
|---|---|
| `project.json` | The project's description. You open a project by choosing this file |
| `scenes/` | The scene |
| `renders/` | The pictures a batch render writes |
| `autosave/` | The autosave, while there are changes you have not saved |
| `assets/`, `cache/` | Nothing. A save makes them, and the editor writes nothing into them |

A project does not save what is selected, or how the areas are arranged.
[Workspace.md](Workspace.md#keep-an-arrangement) says where the arrangement is kept.

## Open a project

1. Press Ctrl+O, or choose **File > Open**.
2. Go into the project's directory.
3. Click `project.json`, then click **Accept**.

The scene replaces the one you had. The history is cleared, so the open cannot be undone. The
project shows the workspace it was last shown in.

## Answer the prompt about unsaved changes

Opening a project, quitting, and closing the window each ask first when the open project has
unsaved changes. The prompt asks "Save the changes to" the project's name.

- **Save** saves the project, then goes on. If the save fails, nothing else happens.
- **Discard** goes on without saving, and throws away the autosave.
- **Cancel** does nothing.

## Recover unsaved work

While a project has unsaved changes, the editor saves them every 60 seconds into the project's
`autosave` directory. It never writes them into the scene itself. A save removes the autosave.

If the editor closes with changes still unsaved, or stops without closing, the autosave stays.
When you next open that project, a prompt asks "Recover the unsaved changes to" the project's
name. The editor also asks at startup, for the default project.

- **Recover** reads the autosave into the scene. The project is left with unsaved changes. Save
  it to keep them.
- **Discard** removes the autosave.

## Import a glTF or USD file

1. Choose **File > Import > glTF** or **File > Import > USD**.
2. Choose the file and click **Accept**.

glTF files end in `.gltf` or `.glb`. USD files end in `.usd`, `.usda`, `.usdc` or `.usdz`.

The file's scene is added to yours as one step, which one undo removes. The items at the top of
the import are selected afterwards.

- Each node of the file becomes an item: a mesh, a camera, a light, a curve, or a group for a
  node with none of these. The file's parents are kept.
- Each item takes its node's name, with a number after it if the name is taken.
- A glTF file holds only triangles. The editor joins pairs of triangles back into four-sided
  faces where they make one.
- Animation keys come in on the frames their times give, at the scene's frame rate. The frame
  range does not change.
- Lengths are scaled from the file's units into the scene's unit of length.

Anything the editor cannot hold, such as a skin, is left out. The notice says how many things
were left out, and the log lists them. A file the editor cannot read changes nothing, and the
notice says why.

## Export RIB

Choose **File > Export > RIB**. The editor writes the scene as a RIB file named `export.rib`
in the project's folder, and replaces any file already there. RIB is the scene format moya reads.

The file renders the current frame, as a render would, from the same camera and with the same
render settings. Rendering it with moya writes `export.png` beside it. The editor cannot read a
RIB file back.
