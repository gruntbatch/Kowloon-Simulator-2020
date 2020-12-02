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
        pprint.pprint(areas, compact=False, indent=2)
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
        area["navmesh"] = structure_navmesh(area["navmesh"])
        area["portals"] = structure_portals(area["portals"])
        link_navmesh_to_portals(area["navmesh"], area["portals"])


def structure_navmesh(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bmesh.ops.transform(bm, matrix=obj.matrix_world, verts=bm.verts)

    triangles = list()

    if len(bm.faces) > 64:
        raise Exception("{} has more than 64 triangles".format(obj.name))

    # I'm not actually sure that bm.faces is guaranteed to iterate in
    # order of the face indices, so we perform a short test to enforce
    # that it does
    for i, face in enumerate(bm.faces):
        if i != face.index:
            raise Exception("`bm.faces` does not iterate in index order")

        triangle = list()

        if len(face.loops) != 3:
            raise Exception("Triangulation failed")
        
        for loop in face.loops:
            edge = dict({"vert": loop.vert.co})

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

        triangles.append(triangle)

    bm.free()

    return triangles


def structure_portals(objs):
    portals = list()

    for obj in objs:
        portals.append({
            "transform": obj.matrix_world
        })

    return portals


def link_navmesh_to_portals(navmesh, portals):
    for triangle in navmesh:
        for edge in range(3):
            a = triangle[edge]
            b = triangle[(edge + 1) % 3]

            for i, portal in enumerate(portals):
                transform = portal["transform"]
                scale = transform.to_scale()

                # Transform the vertices into local space
                transform = transform.inverted()
                va = transform @ a["vert"]
                vb = transform @ b["vert"]

                print(va, vb)

                def in_box(p, b):
                    return (-b.x <= p.x and p.x <= b.x
                            and -b.y <= p.y and p.y <= b.y
                            and -b.z <= p.z and p.z <= b.z)

                if in_box(va, scale) and in_box(vb, scale):
                    triangle[edge]["to"] = 2
                    triangle[edge]["target"] = i
                    if "edge" in portal:
                        raise Exception("Already an edge!")
                    else:
                        portal["edge"] = edge


def export_areas(filepath, areas):
    with open(filepath, "w", encoding="utf8", newline="\n") as f:
        fw = f.write

        fw("# NAVIGATION\n")
        fw("\n")

        # TODO Save each area to its own set of files
        for area in areas:
            for i, triangle in enumerate(area["navmesh"]):
                to = [x["to"] for x in triangle]
                target = [x["target"] for x in triangle]
                vert = [x["vert"] for x in triangle]
                fw("triangle ")
                fw("{} ".format(i))
                fw("{},{},{} ".format(*to))
                fw("{},{},{} ".format(*target))
                fw("{} {} {}\n".format(*["{:.3f},{:.3f},{:.3f}".format(*x.to_tuple()) for x in vert]))
            
            fw("\n")
            for i, portal in enumerate(area["portals"]):
                p, r, s = portal["transform"].decompose()
                fw("portal ")
                fw("{} ".format(i))
                fw("{} ".format(int(round(s.x))))
                fw("{},{},{} ".format(*p.to_tuple()))
                fw("{},{},{},{}\n".format(r.x, r.y, r.z, r.w))
