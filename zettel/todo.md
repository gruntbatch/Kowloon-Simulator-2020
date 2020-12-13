TODO
====

  - ~~Create an index of area names~~
  - ~~Load areas from the index~~
  - ~~Build a map from areas~~
  - ~~Clear depth buffer when rendering~~
  - ~~Render recursive transformations~~
  - ~~Export lights from Blender~~
  - ~~Render scenes using lights~~
  - ~~Instead of transforming objects, transform the view~~
  - ~~Tune light falloff to reflect Blender~~
  - Make mouse sensitivity a configurable variable
  - Add `@glow` support to the export script
  - Fisheye lens distortion
  - Play background music
  - Play ambient sound effects
  - Play dimensional sound effects
  - Convert all python scripts to Python3
  - Some kind of call back when passing between areas?
  - Get rid of all the `2>nul` nonsense on Windows Makefiles
  - Implement a flyaround camera
  - Render portals farthest to nearest
  - Convert the engine to a queued jobs/worker system
  
World Generation
----------------

Iterate through all of the instanced areas, and connect one random portal of each to another. Then, iterate over all of the remaining unlinked portals and link them at random. Theis _should_ result in a world that is guaranteed to be free of isolated islands of areas, while still resulting in a nice, twisted path to walk.
