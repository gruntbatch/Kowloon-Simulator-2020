bl_info = {
        "name" : "46 MESH EXPORTER",
        "author" : "name",
        "version" : (0, 0, 2),
        "blender" : (2, 79, 0),
        "location" : "File > Import-Export",
        "category" : "Import-Export"
        }

import os

import bpy
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator
from bpy_extras.io_utils import ExportHelper
import mathutils

###############################################################################
#   Vertex Class
###############################################################################
class Vertex():
    def __init__(self, v, n, c, u, b, w):
        self.v = [int(x *  100.0) for x in v]
        self.n = [int(x * 4096.0) for x in n]
        self.c = [int(x *  256.0) for x in c]
        self.u = [int(x *  256.0) for x in u]
        self.b = b
        self.w = [int(x *  256.0) for x in w]

###############################################################################
#   Mesh Export
###############################################################################

#######################################
#   Helper functions
#######################################
def triangulate_mesh(me):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()

def mesh_to_weight_list(ob, me):
    """
    Takes a mesh and return its group names and a list of lists,
    one list per vertex.
    aligning the each vert list with the group names,
    each list contains float value for the weight.
    """

    # clear the vert group.
    group_names = [g.name for g in ob.vertex_groups]
    group_names_tot = len(group_names)

    if not group_names_tot:
        # no verts? return a vert aligned empty list
        return [[] for i in range(len(me.vertices))], []
    else:
        weight_ls = [[0.0] * group_names_tot for i in range(len(me.vertices))]

    for i, v in enumerate(me.vertices):
        for g in v.groups:
            # possible weights are out of range
            index = g.group
            if index < group_names_tot:
                weight_ls[i][index] = g.weight

    return group_names, weight_ls

def get_influential_weights(bids, bws):

    a_id = 0
    a_w = 0.0
    b_id = 0
    b_w = 0.0
    c_id = 0
    c_w = 0.0
    d_id = 0
    d_w = 0.0

    for i, bw in enumerate(bws):
        if (bw > a_w):
            d_w = c_w
            d_id = c_id
            c_w = b_w
            c_id = b_id
            b_w = a_w
            b_id = a_id
            a_w = bw
            a_id = bids[i]

    return [a_id, b_id, c_id, d_id], [a_w, b_w, c_w, d_w]

def hash_bids(bids):
    return [bid.__hash__() for bid in bids]

def gather_mesh_data(ob):

    me = ob.to_mesh(bpy.context.scene, False, 'PREVIEW', calc_tessface=False)
    triangulate_mesh(me)

    bids, bws = mesh_to_weight_list(ob, me)

    vertices = []

    me.calc_normals_split()

    for poly in me.polygons:
        for loop_index in range(poly.loop_start, poly.loop_start + poly.loop_total):
            i = me.loops[loop_index].vertex_index

            # Geometric data
            v = me.vertices[me.loops[loop_index].vertex_index].co
            n = me.vertices[me.loops[loop_index].vertex_index].normal

            # Vertex color data
            vc = me.vertex_colors[0].data[loop_index].color if (len(me.vertex_colors) > 0) else [0, 0, 0]
            c = [vc[0], vc[1], vc[2], 1.0]

            # UV data
            u = mathutils.Vector((0, 0, 0, 0))
            if (len(me.uv_layers) > 0): u.xy = me.uv_layers[0].data[loop_index].uv
            if (len(me.uv_layers) > 1): u.zw = me.uv_layers[1].data[loop_index].uv

            # Bone data
            w = bws[i] if (len(bws) > i and bws[i] is not None) else [0.0, 0.0, 0.0, 0.0]
            b, w = get_influential_weights(bids, w)
            b = hash_bids(b)

            vertex = Vertex(v, n, c, u, b, w)
            
            vertices.append(vertex)

    return vertices

def write_mesh_data(filepath, vertices):
    with open(filepath, "w", encoding="utf8", newline="\n") as f:
        fw = f.write

        # Write header
        fw('# MESH NON-BINARY\n')

        fw("LEN {}\n".format(len(vertices)))
        fw("\n")
        for i, v in enumerate(vertices):
            fw("V {} {} {} {}\n".format(i, *v.v))
            fw("N {} {} {} {}\n".format(i, *v.n))
            fw("U {} {} {}\n".format(i, *v.u))
            fw("C {} {} {} {}\n".format(i, *v.c))

#######################################
#   Mesh Export Operator
#######################################

class Export46Mesh(Operator, ExportHelper):
    '''EXPORT A MESH INTO THE 46 ENGINE FORMAT.'''
    bl_idname = "export_46.mesh"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "EXPORT 46 MESH"

    # ExportHelper mixin class uses this
    filename_ext = ".mes"

    filter_glob = StringProperty(
            default="*.mes",
            options={'HIDDEN'},
            maxlen=255,  # Max internal buffer length, longer would be clamped.
            )

    '''
    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    use_setting = BoolProperty(
            name="Example Boolean",
            description="Example Tooltip",
            default=True,
            )

    type = EnumProperty(
            name="Example Enum",
            description="Choose between two items",
            items=(('OPT_A', "First Option", "Description one"),
                   ('OPT_B', "Second Option", "Description two")),
            default='OPT_A',
            )
    '''

    def execute(self, context):
        # return write_some_data(context, self.filepath, self.use_setting)
        v = gather_mesh_data(context.active_object)
        write_mesh_data(self.filepath, v)
        return {'FINISHED'}

###############################################################################
#   Level Export
###############################################################################

#######################################
#   Helper functions
#######################################

#
#   Models
#

#
#   Lights
#

#
#   Cameras
#

#
#   Instanced Objects
#

#######################################
#   Level Export Operator
#######################################

'''
Models
Lights
Cameras
Instanced objects
'''

class Export46Level(Operator, ExportHelper):
    '''EXPORT SCENE TO 46 ENGINE'''
    bl_idname = "export_46.level"
    bl_label = "EXPORT 46 LEVEL"

    filename_ext = ".lev"

    filter_glob = StringProperty(
        default="*.lev",
        options={'HIDDEN'},
        maxlen=255,
    )

    selection_only = BoolProperty(
        name="Selection Only",
        description="Export only the selected items.",
        default=False,
    )

    def execute(self, context):
        # Get objects

        # For every object, dispatch it to the appropriate method
        return {'FINISHED'}


#######################################
#   Level Export Operator
#######################################

# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(Export46Mesh.bl_idname, text="EXPORT 46 MESH")


def register():
    bpy.utils.register_class(Export46Mesh)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(Export46Mesh)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
