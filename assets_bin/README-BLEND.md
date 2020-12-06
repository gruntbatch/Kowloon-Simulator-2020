Blender Files
=============

Structuring Blender Files
-------------------------

The export script now works on instances of linked collections. So in `library.blend`, you can create several props, and put each prop in it's own collection. In `level.blend`, you can create instances of those prop collections, and the export script _should_ handle those automatically.

There is a caveat: If `libraryA.blend` has an object whose mesh is named `Mesh.001` and `libraryB.blend` also has an object whose mesh is named `Mesh.001`, there will be a conflict, and something will be overwritten. I'll hopefully fix that.

Exporting Blender Files
-----------------------

The export script is no longer in an addon. Instead, it is called automatically when running `make assets`. This should be quicker, and it also allows us to use multiple `.blend` files without the hassle of opening every one of them to export stuff. It _should_ all be done automagically.

Tagging
-------

You can put text before tags:

	Will be ignored @tag

Some tags will require arguments to follow:

    @tag arg1 arg2
	
However, anything after the last required argument will be ignored:

	@tag arg1 arg2 the rest will be ignored

@area
-----

  - You must supply a name argument:

        @area name
		
	Any text after `name` will be ignored:
	
	    @area uses_this but not this

  - You cannot nest `@areas`
  - Each `@area` must have a `@navmesh` and at least one `@portal`
  
@export
-------

  - This item, and all of it's children, will be exported and saved
  - This will save instances of linked collections (!)
  - Automatically exports mesh files
  
@ignore
-------

  - Must be the first tag in an object's name (!):
  
        @ignore @other_tag @another_tag
	
	This will not be ignored, and might cause an error:
	
	    @other_tag @ignore @another_tag
		
	However, this is okay:
	
	    I am not a tag @ignore @other_tag
  
  - The script will ignore this item, and all children

@navmesh
--------

  - Can have no more than 64 triangles
  - Must be connected to at least one `@portal`
  - A vertex can only belong to one `@portal`
  
@portal
-------

  - Must be connected to a `@navmesh`
  - Can be connected to no more than two vertices of a `@navmesh`
