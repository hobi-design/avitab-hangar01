# AviTab + HANGAR01

This is a fork of [AviTab](https://github.com/TeamAvitab/avitab) with an added HANGAR01 checklist app. It adds a "Hangar01" entry to the AviTab launcher that fetches interactive checklists from [hangar01.space](https://hangar01.space) directly in the tablet.

For everything not related to the HANGAR01 app, refer to the [upstream AviTab documentation](https://github.com/TeamAvitab/avitab).

---

## Installation

The downloaded zip contains a folder named `Avitab` alongside `INSTALLATION.md` and `RELEASE_NOTES.md`.

### New install

1. Download the zip for your platform from the [latest release](https://github.com/hobi-design/avitab-hangar01/releases/latest) and extract it.
2. Locate your X-Plane installation directory and navigate to `Resources/plugins/`.
3. Copy the `Avitab` folder into `Resources/plugins/`.
4. Launch X-Plane. The Hangar01 app appears in the AviTab launcher.

### Updating an existing AviTab install

This fork's only change is the compiled plugin binary, so the simplest and safest update is to replace just that one file:

- macOS: `Resources/plugins/Avitab/mac_x64/Avitab.xpl`
- Windows: `Resources/plugins/Avitab/win_x64/Avitab.xpl`
- Linux: `Resources/plugins/Avitab/lin_x64/Avitab.xpl`

Copy the matching `Avitab.xpl` from the zip over the existing one.

Do not delete your existing `Avitab` folder and replace it wholesale, as that removes any charts you have added under `Avitab/charts/`. Your settings and preferences are not affected either way: they are stored separately and are not included in the package, so updating never resets them.

---

## Using the HANGAR01 app

1. Open AviTab in X-Plane via the Plugins menu or your assigned key binding.
2. Tap the Hangar01 icon in the launcher.
3. Select an aircraft from the list. The checklist loads from the HANGAR01 API.
4. Use the Quick / Normal / Full buttons to filter checklist items by detail level.
5. Tap a checklist item to check it off. Items marked with (i) display an info note at the bottom of the screen when tapped.
6. Tap the back arrow to return to the aircraft list.

---

## Building from source

See [BUILDING.md](docs/BUILDING.md) for full instructions.

The short version:

```
cmake --preset release
cmake --build --preset release
```

The `install/` directory will contain the plugin package and a standalone desktop app for GUI testing without X-Plane.

CI builds for macOS, Linux, and Windows run automatically on every push. Tagged releases (`v*`) publish zips to the [releases page](https://github.com/hobi-design/avitab-hangar01/releases).

---

## License

[GNU Affero General Public License v3](https://www.gnu.org/licenses/agpl-3.0.html), inherited from the upstream AviTab project.
