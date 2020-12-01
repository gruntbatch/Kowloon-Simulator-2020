import bpy
import bpy_extras
import bmesh


class ExportMesh(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = 'anio.export_mesh'
    bl_label = 'Export ANIO Mesh'

    # Give the ExportHelper class a file extension to use
    filename_ext = '.anio'

    filter_glob = bpy.props.StringProperty(default='*.anio',
                                           options={'HIDDEN'},
                                           maxlen=255)

    def execute(self, context):
        # TODO Loop over selected mesh objects
        # TODO Export indexed vertices
        v = gather_vertices(context.active_object.to_mesh())
        adjust_units(v, context.scene.unit_settings.system)
        write_mesh_data(self.filepath, v)
        return {'FINISHED'}


def gather_vertices(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    # TODO don't do this
    bm.to_mesh(mesh)
    bm.free()

    # Make our own list so it can be sorted to reduce context switching
    face_index_pairs = [(face, index) for index, face in enumerate(mesh.polygons)]

    mesh.calc_normals_split()

    vertices = []

    # https://blender.stackexchange.com/questions/57327/get-hard-shading-normals-in-bpy
    # https://github.com/blender/blender-addons/blob/master/io_scene_obj/export_obj.py
    for face, index in face_index_pairs:
        v = [mesh.vertices[v].co for v in face.vertices]
        loops = mesh.loops
        n = [(loops[l_idx].normal.x, loops[l_idx].normal.y, loops[l_idx].normal.z) for l_idx in face.loop_indices]

        for v, n in zip(v, n):
            vertices.append({'v': v, 'n': n})

    return vertices


def adjust_units(vertices, system):
    if system == 'IMPERIAL':
        for i in range(len(vertices)):
            vertices[i]['v'] = vertices[i]['v'] * 3.28084


def write_mesh_data(filepath, vertices):
    with open(filepath, 'w', encoding='utf8', newline='\n') as f:
        fw = f.write

        fw('# MESH\n')
        fw('\n')

        for i, v in enumerate(vertices):
            fw('{} {},{},{} {},{},{}\n'.format(i, *v['v'], *v['n']))
