bl_info = {'name': 'kowl',
           'version': (0, 1, 0),
           'blender': (2, 91, 0),
           'location': 'File > Import-Export',
           'category': 'Import-Export',}

# Check if the addon is being reloaded
if 'bpy' in locals():
    # Reload any relevant modules
    import importlib
    from . import io_kowl
    importlib.reload(io_kowl)

else:
    import bpy
    from . import io_kowl


def menu_func_export(self, context):
    self.layout.operator(io_kowl.ExportArea.bl_idname, text=io_kowl.ExportArea.bl_label)
    

def register():
    bpy.utils.register_class(io_kowl.ExportArea)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(io_kowl.ExportArea)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == '__main__':
    register()
