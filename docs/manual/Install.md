# Install Vertical3D

A release of Vertical3D holds every app: the editor, the offline renderer moya, imagetool, and
five demos (pong, tetris, voxel, odyssey and diorama). They share one version number.

Download a release from the [releases page](https://github.com/farrcraft/vertical3d/releases).
Each release's notes list what changed since the one before. A release candidate is marked as a
pre-release.

## On Windows

A release has two downloads for Windows, which hold the same apps:

- `vertical3d-<version>-windows-x64-setup.exe`, an installer.
- `vertical3d-<version>-windows-x64.zip`, the same files, to unpack where you choose.

### Use the installer

1. Run `vertical3d-<version>-windows-x64-setup.exe`. It asks for administrator permission, and
   installs into `C:\Program Files\Vertical3D`.
2. The installer is not signed, so Windows SmartScreen may say that it protected your PC. Choose
   **More info**, then **Run anyway**.
3. A `Vertical3D` folder in the Start menu holds the editor and each demo.

moya and imagetool run from a command prompt, and have no Start menu entry. Run each one by its
full path, from a folder of your own:

```
"C:\Program Files\Vertical3D\moya.exe" --version
```

An installer for a newer version removes the older one first. To remove Vertical3D, use
**Settings > Apps > Installed apps**.

### Use the zip

Unpack the zip into any folder, and run the apps from there. Keep the files together: each app
reads the `data` folder beside it.

## On Linux

A release has `vertical3d-<version>-linux-x86_64.flatpak`, a Flatpak bundle. It needs Flatpak
installed, and the Flathub remote, which supplies the runtime the apps run on:

```
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user vertical3d-<version>-linux-x86_64.flatpak
```

The editor and each demo appear in the desktop's list of applications. From a terminal, the
editor runs with `flatpak run com.farrcraft.Vertical3D`, and another app with `--command`:

```
flatpak run --command=moya com.farrcraft.Vertical3D --file scene.rib --output scene.png
flatpak run --command=imagetool com.farrcraft.Vertical3D --help
flatpak run --command=pong com.farrcraft.Vertical3D
```

The apps in the Flatpak see only your documents folder. Keep the files that moya and imagetool
read and write there, and run them from a terminal in that folder.

The Flatpak does not update itself. To move to a newer version, download its bundle and install
it over the old one:

```
flatpak install --user --reinstall vertical3d-<version>-linux-x86_64.flatpak
```

To remove Vertical3D, run `flatpak uninstall --user com.farrcraft.Vertical3D`.

## Where the apps keep your files

- **The editor keeps its projects** in a `Vertical3D` folder in your documents.
- **The editor and the demos keep their settings and their log, `v3d.log`,** in your user
  folder, and never in the folder they are installed in. On Windows that is
  `%APPDATA%\Vertical3D\<app>`. In the Flatpak it is
  `~/.var/app/com.farrcraft.Vertical3D/data/Vertical3D/<app>`. The editor's folder is named
  `Editor`, and each demo's is named after it, such as `Pong`.
- **moya and imagetool write `v3d.log`** in the folder you run them from.

## Check the version

Every app prints its version with `--version`. The editor also shows it under **Help > About**.

## Build from source

The apps can also be built from the repository.
[Build.md](../contributing/Build.md) says how. A build puts each app in its own directory under
`out/build/<preset>/`, such as `out/build/x64-Debug/vertical3d/`. A Debug build is slower than a
release, which is a Release build.
