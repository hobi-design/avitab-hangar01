# AviTab + HANGAR01

This is a fork of [AviTab](https://github.com/TeamAvitab/avitab) with an added HANGAR01 checklist app. It adds a "Hangar01" entry to the AviTab launcher that fetches interactive checklists from [hangar01.space](https://hangar01.space) directly in the tablet.

For everything not related to the HANGAR01 app, refer to the [upstream AviTab documentation](https://github.com/TeamAvitab/avitab).

---

## Installation

1. Download the zip for your platform from the [latest release](https://github.com/hobi-design/avitab-hangar01/releases/latest).
2. Locate your X-Plane installation directory. Inside it, navigate to `Resources/plugins/`.
3. Copy the `Avitab` folder from the downloaded artifact into the `plugins` directory.
   - If an existing AviTab installation is present, merge the folders rather than replacing them. At minimum, replace only the `Avitab.xpl` binary for your platform.
4. Launch X-Plane. The HANGAR01 app will appear in the AviTab launcher.

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
