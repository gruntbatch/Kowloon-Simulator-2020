TODO
====

  - Refactor exporter to store vertices and edges seperately
  - Establish some way of tying  together textures and meshes
  - Pathfinding
  - Parse config file
  - Parse command line arguments
  - Player controls
  - Music
  - Ambient sound effects
  - Sound effects
  - Target multiple platforms
  - Update emacs preferences to better deal with C
  - Convert the engine to a queued jobs/worker system
  
  
.nav

node [waypoint, terminus]
link
portal
mark

Area
----

  - 1 variant of each group
  
Nav
---

Mesh
----

Portal
------

Mark
----

Portal
------



Area
----

Variants:
collections with same name are variants

  - alley
  - grotto


Types of areas:
  - opaque
  - inset
  - transparent

Opaque areas render one on top of the other, however, they allow child transparent areas to render on top of them.

Node
----

Nodes are connected at "run-time" by raycasting connections within an area. Specially marked nodes can link between areas.

Node types:
  - link
  - replaceable
  
Street
------

Streets are a series of linked nodes, possibly circular, possibly terminating in dead ends.

A collection of nodes in a more-or-less straight line.

Types of streets:
  - street
  - plaza

Streets are singly linked nodes, at least three in length. Plazas are multi-connected nodes.

When naming streets, search ahead for legal route before naming.

When naming streets, favor established links vs joined links.

When forming streets, increase odds of a dead end or terminator the longer the street goes.

Links
-----

Possible link sizes:
  - 1
  - 3
  - 5
  
On larger link sizes, not all links need to be open (think like a castle-gate).
