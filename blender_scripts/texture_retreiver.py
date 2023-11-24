import bpy
import bmesh
from bpy import context
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector, Matrix
import math

def start():
    i = 0
    j = 0
    for obj in context.scene.objects:
        if obj.type != 'MESH':
            continue

        print(f"Building {i}: started")

        for face in obj.data.polygons:
            angle = face.normal.angle((0, 0, 1))

            if not math.isclose(angle, 0, abs_tol=0.01) and not math.isclose(angle, math.pi, abs_tol=0.01):
                cam = create_camera_at_face(obj, face)
                if cam != None:
                    pass
                    # render_face(cam, i, j)
                    # delete_cam(cam)
                j += 1

        print(f"Building {i}: done!")
        i += 1


def render_face(camera, building_id, face_id):
    name = f"../Textures/image_{building_id}_{face_id}.png"
    bpy.context.scene.camera = camera
    bpy.context.scene.render.filepath = name
    bpy.ops.render.render(write_still=True)


def edit_mode(flag):
    if flag:
        try:
            bpy.ops.object.mode_set(mode='EDIT')
        except:
            pass
    else:
        bpy.ops.object.mode_set(mode='OBJECT')


def get_max_edge_length(obj, face):
    vertices = [obj.data.vertices[i].co for i in face.vertices]
    side_lengths = [math.dist(vertices[i], vertices[(i + 1) % len(vertices)]) for i in range(len(vertices))]
    return max(side_lengths)


def get_min_edge_length(obj, face):
    vertices = [obj.data.vertices[i].co for i in face.vertices]
    side_lengths = [math.dist(vertices[i], vertices[(i + 1) % len(vertices)]) for i in range(len(vertices))]
    return min(side_lengths)


def create_camera_at_face(obj, face):
    # TODO: trovare un modo più furbo per il centro faccia, per forme strane si rompe

    min_edge = get_min_edge_length(obj, face)
    if min_edge < 1:
        return None

    center = Vector((0, 0, 0))
    for vertex_idx in face.vertices:
        center += obj.matrix_world @ obj.data.vertices[vertex_idx].co
    
    center /= len(face.vertices)

    normal = -(obj.matrix_world.to_3x3() @ face.normal)

    # Creazione camera
    bpy.ops.object.camera_add(location=center - normal)
    cam = bpy.context.object
    cam.data.type = 'ORTHO'

    cam.rotation_euler = normal.to_track_quat('-Z', 'Y').to_euler()

    scale = get_max_edge_length(obj, face)
    cam.data.ortho_scale = scale

    return cam


def delete_cam(cam):
    bpy.ops.object.select_all(action='DESELECT')
    cam.select_set(True)
    bpy.ops.object.delete()


# ---------------------------------------------------------------------------- #
#                                     MAIN                                     #
# ---------------------------------------------------------------------------- #

start()
