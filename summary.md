# HANGAR01 - AviTab Integration Summary

## Project Context
We are integrating the HANGAR01 checklists into the X-Plane "AviTab" VR tablet. 
- **The old idea:** Generate PDFs. (Discarded)
- **The new strategy:** Build a native custom C++ app directly inside AviTab that fetches checklist data from the HANGAR01 web app via a JSON API.

## Architecture
- **AviTab Side (C++):** Uses AviTab's internal `apis::RESTClient` to fetch JSON from HANGAR01, parses it with `nlohmann/json`, and uses AviTab's built-in LVGL UI toolkit (wrapped in C++ widgets like `List`, `Label`, `Window`) to render natively.
- **Web App Side:** Exposes checklist data via a standard JSON endpoint (Sanity CMS GROQ API).

## Work Completed So Far
1. **GitHub Actions Cloud Build Setup:**
   - Created `.github/workflows/build.yml` to compile the standalone desktop app (`avitab_desktop`) without requiring local compilation.
   - **Runner Configuration:** Specifically targets `macos-14` (Apple Silicon) for fast queue times, but cross-compiles for Intel Macs (`x86_64`) targeting macOS 12.0 (Monterey) using CMake flags `-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0` and `-DCMAKE_OSX_ARCHITECTURES=x86_64`.
   - Generates an `avitab-build-mac` artifact containing `install/avitab/tools/avitab_desktop`.
2. **C++ App Scaffolding:**
   - Created `src/core/apps/Hangar01App.cpp` and `Hangar01App.h`.
   - Registered the app in `AppLauncher.cpp` with a placeholder icon and added it to the `AppId` enum in `AppFunctions.h`.
   - Added the new source files to `src/core/apps/CMakeLists.txt`.
3. **API Integration (Aircraft List):**
   - Implemented a `std::thread` to make a non-blocking asynchronous `GET` request to the Sanity API on app launch.
   - Parses the JSON response and populates an interactive `List` widget with the fetched aircraft models.

---

## API Reference Notes (Provided by User verbatim)

HANGAR01 Checklist Data — API Reference
There is no custom REST endpoint. The data lives in Sanity CMS and is queried directly via the Sanity GROQ API. This is the same source the web app uses — no middleware, no auth for read access on the production dataset.

1. Endpoint
GET https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=<GROQ>
To fetch a single aircraft's full checklist by slug (e.g. b737):

https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=*[_type=="aircraft"&&slug.current==$slug][0]{model,slug,manufacturer,checklist}&$slug="b737"
To fetch all aircraft slugs (for discovery):

https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=*[_type=="aircraft"&&!comingSoon]{model,slug}
2. Authentication
None required. The production dataset is public read. No headers, no API key needed for GET requests.

3. JSON payload shape
{
  "result": {
    "model": "Boeing 737 NG",
    "slug": { "current": "b737" },
    "manufacturer": "Boeing",
    "checklist": [
      {
        "_key": "phase_abc123",
        "title": "Preflight",
        "groups": [
          {
            "_key": "group_xyz456",
            "title": "Cockpit Preparation",
            "items": [
              {
                "_key": "item_001",
                "label": "Parking Brake",
                "action": "SET",
                "info": "Ensure handle is pulled and light is on.",
                "tier": "quick"
              },
              {
                "_key": "item_002",
                "label": "Battery Switch",
                "action": "ON",
                "tier": "normal"
              }
            ]
          }
        ]
      },
      {
        "_key": "phase_def789",
        "title": "Engine Start",
        "groups": [...]
      }
    ]
  }
}
4. Data structure summary
Field	Type	Notes
model	string	Display name e.g. "Boeing 737 NG"
slug.current	string	URL/ID key e.g. "b737"
checklist[]	Phase[]	Top-level phases
phase.title	string	e.g. "Preflight", "Engine Start"
phase.groups[]	Group[]	Sub-groups within a phase
group.title	string	e.g. "Cockpit Preparation"
item.label	string	The checklist item name e.g. "Battery Switch"
item.action	string	The expected action e.g. "ON", "SET", "CHECK"
item.info	string?	Optional explanatory note
item.tier	"quick" | "normal" | "full"	Filter items by detail level
5. Available aircraft slugs
Query this to get the current list:

https://3qw97t0o.api.sanity.io/v2023-05-03/data/query/production?query=*[_type=="aircraft"&&!comingSoon]{model,"slug":slug.current}
Known slugs currently in the dataset: b737, a320, a310, a330, b747, b787, c208, cj4, hondajet, longitude, tbm930
