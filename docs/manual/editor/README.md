# The Vertical3D editor manual

The Vertical3D editor is a 3D modelling program. You build a scene of meshes, curves, cameras
and lights, animate it, render it with moya, and save it as a project.

This manual is for someone who models with the editor. It describes what you see and what you
do, by the names the editor shows. It does not describe the source code.
[docs/editor/](../../editor/README.md) covers that, for someone who changes the editor.

## Starting the editor

Run `vertical3d.exe`. A build of the repository puts it in `out/build/x64-Debug/vertical3d/`,
and [Build.md](../../contributing/Build.md) says how to build it. The editor reads its settings
from the `data` directory beside the program, so run the copy that has that directory.

## The pages

Start with the tutorial if you have not used the editor before. Read the concepts page when a
word in a guide is new to you.

| Page | Type | Go to it to |
|---|---|---|
| [Tutorial.md](Tutorial.md) | Tutorial | Make a first scene, from starting the editor to reopening a saved project |
| [Concepts.md](Concepts.md) | Explanation | Learn the words this manual uses: projects, items, masks, areas, keys |
| [Modelling.md](Modelling.md) | How-to | Add primitives, select objects and their parts, move them, and split, extrude, inset and merge |
| [Scene.md](Scene.md) | How-to | Name items, group them, add cameras, lights and curves, and edit their properties |
| [Views.md](Views.md) | How-to | Move a view's camera, choose a fixed view, and change what a view draws |
| [Animation.md](Animation.md) | How-to | Key properties, edit keys in the timeline and the graph editor, and play the animation |
| [Rendering.md](Rendering.md) | How-to | Render with moya, change the render settings, and render every frame to files |
| [Projects.md](Projects.md) | How-to | Create, open and save a project, recover an autosave, import glTF or USD, and export RIB |
| [Workspace.md](Workspace.md) | How-to | Split and join areas, use tabs, and keep and switch workspaces |
| [Commands.md](Commands.md) | How-to | Find a command by name, and run commands as a script from the console |
| [Menus.md](Menus.md) | Reference | Look up every menu and item, and what each one does |
| [Bindings.md](Bindings.md) | Reference | Look up every key and mouse binding |
