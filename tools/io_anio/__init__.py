bl_info = {'name': 'anio',
           'version': (0, 0, 0),
           'blender': (2, 82, 0),
           'location': 'File > Import-Export',
           'category': 'Import-Export', }

# Check if the addon is being reloaded.
if 'bpy' in locals():
    # Reload any relevant modules.
    import importlib
    from . import io_mesh
    importlib.reload(io_mesh)

else:
    import bpy
    from . import io_mesh


# Only needed if you want to add an item into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(io_mesh.ExportMesh.bl_idname, text='Export ANIO Mesh')


def register():
    bpy.utils.register_class(io_mesh.ExportMesh)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(io_mesh.ExportMesh)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == '__main__':
    register()
