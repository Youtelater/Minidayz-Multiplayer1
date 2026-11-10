# System 1 — Nintendo 3DS game engine

## Scope
Reusable platform/runtime layer. Original game rules, world, UI and media-driven scene content belong to their own systems and attach through Scene.

## Runtime responsibilities
- Initializes and shuts down libctru, RomFS, Citro3D and Citro2D in dependency order.
- Runs while the 3DS application is active.
- Polls held, pressed and released buttons, circle pad and touch.
- Calculates frame delta and clamps simulation steps to 50 ms.
- Dispatches update and rendering to a non-owning active Scene.
- Renders scene callbacks independently to top and bottom screens.
- Provides a fixed-capacity sprite-sheet cache, sprite drawing and camera offset.
- Releases loaded sprite sheets and graphics resources during shutdown.

## Scene contract
Scenes are owned by the application layer. Engine::set_scene calls on_exit/on_enter. A scene must outlive its registration. Scene::render is called once per screen; implementations should branch on screen/UI state.

## Asset contract
Renderer::load_sheet accepts a RomFS .t3x path. The existing graphics pipeline converts PNG files in gfx/ into romfs/gfx/*.t3x. Source images are not modified.

## Verification boundary
CI compilation/linking is not hardware verification. Visual output, touch calibration, suspend/resume, memory pressure and asset loading still require testing on a 3DS. Gameplay scenes and rules are separate systems.
