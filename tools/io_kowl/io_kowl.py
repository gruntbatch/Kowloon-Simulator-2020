import bmesh
import bpy
import bpy_extras
from collections import deque
import mathutils
import pprint


class ExportArea(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "kowl.export_map"
    bl_label = "Export Kowl Area"

    # Give the ExportHelper class a file extension to use
    filename_ext = '.area'
    filter_glob = bpy.props.StringProperty(default="*.area",
                                           options={'HIDDEN'},
                                           maxlen=255)

    def execute(self, context):
        areas = list()
        parse_areas(context.scene.collection, areas)
        structure_areas(areas)
        export_areas(self.filepath, areas)
        return {"FINISHED"}


def parse_areas(collection, areas):
    for tokens, child in tokenize(collection.children):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@area":
                area = {"name": tokens.popleft()}
                parse_area(child, area)
                areas.append(area)

            else:
                pass

        else:
            parse_areas(child, areas)


def parse_area(collection, area):
    for tokens, child in tokenize(collection.objects):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@navmesh":
                if "navmesh" in area:
                    raise Exception("Multiple navmeshes supplied for `{}`".format(area["name"]))
                area["navmesh"] = child

            elif token == "@portal":
                area.setdefault("portals", list()).append(child)

            elif token == "@var":
                area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append([child])

            else:
                pass

            
    for tokens, child in tokenize(collection.children):
        while tokens:
            token = tokens.popleft()

            if token == "@ignore":
                break

            elif token == "@var":
                area.setdefault("vars", dict()).setdefault(tokens.popleft(), list()).append(list(child.objects))

            else:
                pass

        else:
            parse_area(child, area)


def tokenize(children):
    for child in children:
        yield deque(child.name.rsplit('.', 1)[0].split()), child


def structure_areas(areas):
    for area in areas:
        # Navmesh
        area["navmesh"] = structure_navmesh(area["navmesh"])

        # Portals


def structure_navmesh(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bmesh.ops.transform(bm, matrix=obj.matrix_world, verts=bm.verts)

    triangles = dict()

    if len(bm.faces) > 64:
        raise Exception("{} has more than 64 triangles".format(obj.name))

    for face in bm.faces:
        triangle = triangles.setdefault(face.index, list())

        if len(face.loops) != 3:
            raise Exception("Triangulation failed")
        
        for loop in face.loops:
            edge = dict({"vert": loop.vert.co.to_tuple()})

            link_faces = loop.edge.link_faces

            if len(link_faces) == 1:
                edge["to"] = 0
                edge["target"] = 0
            elif len(link_faces) == 2:
                edge["to"] = 1
                for i, l in enumerate(link_faces):
                    if l.index != face.index:
                        edge["target"] = l.index
            else:
                raise Exception("Non-manifold geometry")

            triangle.append(edge)

    bm.free()

    return triangles


def export_areas(filepath, areas):
    with open(filepath, "w", encoding="utf8", newline="\n") as f:
        fw = f.write

        fw("# NAVIGATION\n")
        fw("\n")

        for area in areas:
            for triangle in area["navmesh"].values():
                to = [x["to"] for x in triangle]
                target = [x["target"] for x in triangle]
                vert = [x["vert"] for x in triangle]
                fw("triangle ")
                fw("{},{},{} ".format(*to))
                fw("{},{},{} ".format(*target))
                fw("{} {} {}\n".format(*["{:.3f},{:.3f},{:.3f}".format(*x) for x in vert]))
