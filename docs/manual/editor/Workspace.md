# Arranging the workspace

How to change which editor an area shows, split and join areas, use tabs, and keep and switch
workspaces. [Concepts.md](Concepts.md#areas-editors-and-workspaces) defines areas, editors and
workspaces.

## Switch workspaces

The bar at the top of the window has a tab for each workspace, after the menus. The tab of the
workspace shown is raised and in bold. Click a tab to show its workspace.

Press Ctrl+Page Down or Ctrl+Page Up, or choose **Window > Next Workspace** or
**Window > Previous Workspace**, to step through them in turn.

| Workspace | What it shows |
|---|---|
| Modelling | Front, Top, Left and Perspective views, with the outliner over the properties editor down the right |
| Perspective | One perspective view |
| Animation | A perspective view, the outliner and the properties editor, and the timeline, graph editor and console as tabs along the foot |
| Shading | A perspective view shaded smooth, with the properties editor beside it |
| Rendering | The render view, with the render settings beside it |

## Use an area's header

Each area has a header along its top. Its first menu is named for the editor the area shows,
such as Viewport, and a panel's header writes the name beside the menu's icon. In an area with
tabs the menu is at the left of the strip of tabs instead. It holds:

| Item | What it does |
|---|---|
| Editor > Viewport, Outliner, Properties, Timeline, Graph Editor, Console, Render View, Render Settings | Shows that editor in place of the one shown |
| Split > Left, Right, Top, Bottom | Divides the area in two. The new half is on that side, and shows a copy of the editor |
| Join > Left, Right, Above, Below | Gives the area's room to the area on that side. The area's editors close |
| Swap > Left, Right, Above, Below | Exchanges what the area holds with the area on that side |
| New Tab | Adds a viewport as a new tab |
| Close Tab | Closes the editor shown, unless it is the area's only one |
| Maximise | Gives the area the room of every area, or gives every area its room back |

A join works only with an area in the same row or column. An area too small to hold two areas
is not split.

The button at the right end of the header, or of the strip of tabs, maximises the area too.
While an area is maximised the button restores the areas, and the header says which key does
the same.

## Show an editor with a key

Move the cursor over an area and press a key to show that editor there:

| Key | Editor |
|---|---|
| Shift+F4 | Console |
| Shift+F5 | Viewport |
| Shift+F6 | Graph Editor |
| Shift+F7 | Properties |
| Shift+F9 | Outliner |
| Shift+F10 | Render Settings |
| Shift+F11 | Render View |
| Shift+F12 | Timeline |

## Resize areas

Areas are separated by thin gaps. Each gap is a divider.

- **Drag a divider** to give room to one side and take it from the other. Neither side gets
  smaller than its least size.
- **Hold Ctrl while you drag** to snap the divider to tenths of the space.
- **Press Escape or click the right button** during the drag to put the divider back.
- **Double-click a divider** to give the two sides the same room.

The pointer turns into a resize cursor over a divider. After half a second, a hint beside it
lists these actions.

## Use tabs

An area holding two or more editors shows a strip of tabs along its top, one for each editor,
after the area's menu. Click a tab to show its editor. Click **+** after the tabs and choose an
editor to add it as a new tab. Under the strip a viewport keeps its header of view menus, and
any other editor starts straight under the tabs.

Drag a tab to move its editor:

- **Drop it near the edge of an area** to make a new area on that edge, half the size of the one
  it landed on. The place it will go is shaded while you drag.
- **Drop it anywhere else in another area** to add it to that area's tabs.
- **Drop it in its own area** to leave it where it was.

An area left with no editors gives its room to the area beside it.

## Open an editor in a window of its own

**Render > Render** opens a window of its own when no area holds a render view.
[Rendering.md](Rendering.md#render-the-current-frame) describes it. That window has areas,
headers and tabs of its own, and can show any editor. Its close button closes it. Closing the
main window quits the editor.

## Keep an arrangement

The editor remembers how you arranged each workspace, for each project. It saves this in your
user settings, not in the project, so arranging areas never leaves the project with unsaved
changes. The settings are a file named `settings.json`, in the `Vertical3D\Editor` directory
of your user profile's application data.

- Switching workspaces, opening another project and quitting each record the arrangement.
- Showing a workspace again shows the project's own arrangement of it.
- A project opens in the workspace it was last shown in.
- Windows of their own are kept with the workspace. Showing the workspace opens them again.

To make the arrangement shown the workspace's own, for every project that has not arranged it,
choose **Window > Save Workspace**.

To put the workspace shown back as the editor comes with it, choose **Window > Reset Workspace**.
This forgets the open project's arrangement of it too.

## Share a workspace

1. Choose **Window > Export Workspace**.
2. Type a file name, and click **Accept**. The file is saved as `.json`.

To bring one in, choose **Window > Import Workspace**, choose the file, and click **Accept**.
Each workspace in the file is kept, and replaces any workspace of the same name. The first one
is shown.
