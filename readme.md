<img width="445" height="441" alt="O" src="https://github.com/user-attachments/assets/04095994-97d3-441d-85e0-c23bfa01d702" />

# Prior

- **CMake 3.20+**               — [https://cmake.org/download/](https://cmake.org/download/)
- **C++17** (MSVC)              — [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
- **SFML 3.x**                  — [https://www.sfml-dev.org/download.php](https://www.sfml-dev.org/download.php)
- **OpenGL**                    — `opengl32` on Windows (linked by CMake)

## Build

```text
cmake -B build -DSFML_ROOT=C:/SFML
cmake --build build --config Release
```

## Run

```text
.\build\Release\run.exe
```

# Function

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                                                                            │
│  Rules: 8 triangular faces, 6x6 barycentric stickers per face (288 total). │           
│                                                                            │
│  Goal: each face one uniform hue (8 spaced shades)                         │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
```

# History

```text
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  400 BCE — ANCIENT GREECE                                                │
  │  Plato links the octahedron to air; five regular solids in philosophy    │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  300 BCE — EUCLID                                                        │
  │  Elements classifies the regular solids including the octahedron         │
  └───────────────────────────────────┬──────────────────────────────────────┘
                                      │
                                      v
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  1800s — 1900s                                                           │
  │  Crystallography and group theory; symmetry of polyhedra                 │
  └───────────────────────────────────┬──────────────────────────────────────┘
```

# Structure

```text
├── main.cpp                        # (frontend)          # entry loop, square clip, hud, orbit, keyboard routing.
├── renderer.cpp                    # (frontend)          # stars, lighting, octa draw, animated slab rotation.
├── renderer.h                      # (frontend)          # declare renderer API and TurnAnimOcta.
├── window_shape.cpp                # (frontend)          # win32 region: inset square on client area.
├── window_shape.h                  # (frontend)          # declare applySquareWindowShape.

├── octa_state.cpp                  # (backend)           # cap-slab permutations, scramble, undo, 120 deg turns.
├── octa_state.h                      # (backend)           # OctaState, OctaMove, OctaFace constants.
├── octa_mesh.cpp                   # (backend)           # build 8-face mesh; sticker order matches state.
├── octa_mesh.h                     # (backend)           # OctaMesh, MeshTriangle, buildOctaMesh.
├── math3.h                         # (backend)           # vec3, normalize, rotateAroundAxis helpers.

├── CMakeLists.txt                  #                     # SFML 3, OpenGL, run target, post-build DLL copy.
├── .gitignore                      #                     # ignore build dirs, logs, IDE files.
└── readme.md                       #                     #
```
