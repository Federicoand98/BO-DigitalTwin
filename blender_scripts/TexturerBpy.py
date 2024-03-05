import bpy
import os
from mathutils import *
from math import *
import glob
from PIL import Image, ImageDraw, ImageStat

bpy.ops.wm.save_mainfile()

###############################
# i tile google devono essere allineati in base a un preciso standard per permettere una esportazione congrua.
# il primo import di Blender GIS deve essere il tile 32_685000_4928000.shp, può poi essere eliminato e si possono
# importare altri shapefile (che verranno posizionati di conseguenza) ed effettuare la sovrapposizione della mesh
# google manualmente. questa operazione si rende necessaria perchè la georeferenziarione verrà persa una volta che
# avverrà l'export di blender dei singoli edifici.
###############################

# ===============================================
# parametri generali

object_name_prefix = '32_'
cam_distance = 5
tex_folder = bpy.path.abspath( '' )
tex_scale = 10
atlas_margin = 5 
atlas_w = 4096
atlas_path = tex_folder + '_atlas.png' 
do_render_eevee = False  # 2 secondi a immagine
do_render_opengl= True   # 10X speedup

# tiles_folder = ""
# roof_folder = ""
do_import_roof = False
roof_files = []
shape_files = []
shape_ref_files = []
tile_names = []

# ===============================================

# textures serve a tenere l'associazione tra le texture e le facce
# e viene usato nel seguito per impostare gli UV relativi all'atlas

class Tex:
    def __init__( self, fname, face, mesh, w, h ):
        self.mesh = mesh
        self.face = face
        self.name = fname.replace('//out/','').replace('.png','')
        self.fname = bpy.path.abspath(fname)
        self.image = None
        self.w = w
        self.h = h
        self.x0 = 0
        self.y0 = 0
        self.x1 = 0
        self.y1 = 0
        
    def open(self):
        self.image = Image.open(self.fname)
    
    def close(self):
        self.image.close()
    

# ===============================================
class CameraSetup:
    def __init__(self, camera_object):
        self.camera_object = camera_object
        self.configure_camera()

    def configure_camera(self):
        cc = self.camera_object.data
        cc.type = 'ORTHO'
        cc.clip_start = 0.1
        cc.clip_end = 10
        cc.ortho_scale = 0.5
        cc.display_size = 0.1

    def calculate_max_dimension(self, mesh, face, face_center, face_normal):
        fw = 0
        fh = 0
        if fabs( face_normal.z ) < 0.1 : 
            u = Vector((0,0,1)) # up vector
            s = face_normal.cross(u)      # side vector    
            xmin =  1e8
            xmax = -1e8
            zmin =  1e8
            zmax = -1e8
            for vi in face.vertices:
                v = mesh.vertices[ vi ].co
                cv = Vector(( v.x-face_center.x, v.y-face_center.y, v.z-face_center.z ))
                x = cv.dot(s) # distanza con segno di v rispetto a c lungo s 
                xmin = min(x,xmin)        
                xmax = max(x,xmax)        
                zmin = min(v.z,zmin)        
                zmax = max(v.z,zmax)        
            fw = xmax - xmin
            fh = zmax - zmin
        
        return fw, fh
        
    def position_camera(self, mesh, face, face_normal, face_center, cam_distance):
        co = self.camera_object
        co.location.x = face_center.x + face_normal.x * cam_distance
        co.location.y = face_center.y + face_normal.y * cam_distance
        co.location.z = face_center.z
        co.rotation_mode = 'XYZ'
        co.rotation_euler.x = pi / 2
        co.rotation_euler.y = 0
        co.rotation_euler.z = atan2(face_normal.y, face_normal.x) + pi/2
        fw, fh = self.calculate_max_dimension(mesh, face, face_center, face_normal)
        co.data.ortho_scale = max(fw, fh)
        return fw, fh

# ===============================================
class TextureAtlas:
    def __init__(self, atlas_width, atlas_height, atlas_margin):
        self.atlas_width = atlas_width
        self.atlas_height = atlas_height
        self.atlas_margin = atlas_margin
        self.textures = []

    def add_texture(self, texture):
        self.textures.append(texture)

    def generate_atlas(self, atlas_path):
        # Ordina le texture per altezza in modo decrescente per un packing più efficiente
        self.textures.sort(key=lambda tex: tex.h, reverse=True)

        m = self.atlas_margin
        y = m
        i = 0
        ni = len(self.textures)
        row = 0

        while i<ni:
            w = self.textures[i].w + m
            h = self.textures[i].h + m
            j=i+1
            while j<ni and ((w + self.textures[j].w + 2*m ) < self.atlas_width):
                w += self.textures[j].w + m
                j += 1
            x = m
            for k in range(i,j):
                t = self.textures[k]
                t.x0 = x
                t.y0 = y
                t.x1 = x + t.w 
                t.y1 = y + t.h
                x += t.w 
                x += m
            y = y+h
            i=j
            row +=1
        
        self.atlas_height = y

        atlas = Image.new('RGB', (self.atlas_width, self.atlas_height))
        atlas_draw = ImageDraw.Draw(atlas)
        atlas_draw.rectangle( [(0,0),(m,m)], fill=(170,113,94) ) # roof color

        for tex in self.textures:
            # Apri l'immagine della texture
            tex.open()
            # Posiziona la texture nell'atlas
            atlas.paste(tex.image, (tex.x0, tex.y0))
            # Chiude l'immagine della texture per liberare risorse
            tex.close()

        # Salva l'immagine atlas finale
        atlas.save(atlas_path)
    
    def set_uv_map(self):
        for tex in self.textures:
            face = tex.face
            mesh = tex.mesh
            face.material_index = 0

            n = face.normal  
            c = face.center

            for vert_idx, loop_idx in zip(face.vertices, face.loop_indices):
                uv_coords = mesh.uv_layers.active.data[loop_idx].uv
                
                v = mesh.vertices[vert_idx].co
                dz = v.z-c.z
                if dz > 0:
                    uv_coords.y = 1-float(tex.y0)/self.atlas_height
                else:
                    uv_coords.y = 1-float(tex.y1)/self.atlas_height
                
                u = Vector((0,0,1)) # up vector
                s = n.cross(u)
                cv = Vector(( v.x-c.x, v.y-c.y, v.z-c.z ))
                if cv.dot( s ) < 0:
                    uv_coords.x = float(tex.x1)/self.atlas_width
                else:
                    uv_coords.x = float(tex.x0)/self.atlas_width
    
    def save_atlas(self):
        bpy.ops.wm.save_mainfile()
        a = bpy.data.images[self.atlas_path]
        if a:
            a.filepath = self.atlas_path
            a.reload()
        print('==========done==========')


# ===============================================
class TextureRenderer:
    @staticmethod
    def render_texture(face, mesh, camera_setup, folder_path, tex_scale, do_render_eevee, do_render_opengl, camera_distance):
        # Calcolo della larghezza e altezza della faccia (fw, fh)
        fw, fh = 0, 0  # Qui andrà il codice per calcolare fw e fh basato sulle normali e centro della faccia
        
        # Posizionamento della camera
        #camera_setup.position_camera(face.normal, face.center, camera_distance, max(fw, fh))
        camera_setup.position_camera(mesh, face, face.normal, face.center, cam_distance)

        # Renderizzazione e salvataggio dell'immagine
        dname = mesh.name.replace('.', '-').replace('_', '-')
        fname = f"{dname}_{face.index}.png"
        image_filename = os.path.join(folder_path, fname)

        w = int(fw * tex_scale)
        h = int(fh * tex_scale)

        bpy.context.scene.render.resolution_x = w
        bpy.context.scene.render.resolution_y = h
        bpy.context.scene.render.filepath = image_filename

        if do_render_eevee:
            bpy.ops.render.render(write_still=True)
        if do_render_opengl:
            bpy.ops.render.opengl(write_still=True)

        return image_filename, w, h

# ===============================================
class SceneSetup:
    @staticmethod
    def configure_scene_visibility(low_collection_name, google_object_name, hide_low=True):
        # Nascondo gli oggetti low, visualizzo l'oggetto google
        collection = bpy.data.collections.get(low_collection_name)
        if collection:
            collection.hide_viewport = hide_low
            collection.hide_render = hide_low

        google = bpy.data.objects.get(google_object_name)
        if google:
            google.hide_viewport = not hide_low
            google.hide_render = not hide_low

    @staticmethod
    def setup_material_for_mesh(mesh, material_name):
        # Recupero o creo il materiale e lo assegno alla mesh
        atlas = bpy.data.materials.get(material_name)
        if not atlas:
            atlas = bpy.data.materials.new(name=material_name)

        # Rimuovo i materiali esistenti dalla mesh e inserisco l'atlas
        mesh.materials.clear()
        mesh.materials.append(atlas)

        # recupero o creo l'uv map
        index = mesh.uv_layers.find('UVMap')
        if index == -1:
            mesh.uv_layers.new(name='UVMap')
            index = mesh.uv_layers.find('UVMap')
            if index == -1:
                print('Cant find UVMap for object')

# ===============================================
class Main():
    def __init__(self, cam_object_name='Camera', google_object_name='google', low_collection_name='low',
                 atlas_width=4096, atlas_height=4096, atlas_margin=10, tex_folder='//textures/', tex_scale=1.0,
                 cam_distance=1.5, do_render_eevee=True, do_render_opengl=False):

        self.camera_setup = CameraSetup(bpy.data.objects[cam_object_name])
        #TODO: Fix atlas width e height
        self.texture_atlas = TextureAtlas(atlas_width, atlas_height, atlas_margin)
        self.tex_folder = tex_folder
        self.tex_scale = tex_scale
        self.cam_distance = cam_distance
        self.do_render_eevee = do_render_eevee
        self.do_render_opengl = do_render_opengl

        #TODO: aggiungere area.type == 'VIEW_3D'

        # Scene configuration
        SceneSetup.configure_scene_visibility(low_collection_name, google_object_name, hide_low=True)

    def execute_texturing_process(self):
        # Assumendo che l'oggetto 'google' e la collezione 'low' siano configurati correttamente
        D = [x for x in bpy.data.objects if x.name.startswith("32_") and "_ref" not in x.name]

        for d in D:
            folder_path = os.path.join(self.tex_folder, d.name)
            if not os.path.exists(folder_path):
                os.makedirs(folder_path)

            SceneSetup.setup_material_for_mesh(d.data, "M_" + d.name)

            counter = 0
            for face in d.data.polygons:
                if face.normal.z < 0.1:
                    counter += 1

                    texture_path, w, h = TextureRenderer.render_texture(face, d.data, self.camera_setup, folder_path, self.tex_scale, self.do_render_eevee, self.do_render_opengl, self.cam_distance)

                    tex = Tex(texture_path, face, d.data, w, h)
                    self.texture_atlas.add_texture(tex)
                else:
                    for vert_idx, loop_idx in zip(face.vertices, face.loop_indices):
                        uv_coords = d.data.uv_layers.active.data[loop_idx].uv
                        uv_coords.x = 0
                        uv_coords.y = 0

        # Dopo aver raccolto tutte le texture
        self.texture_atlas.generate_atlas(os.path.join(self.tex_folder, "final_atlas.png"))
        self.texture_atlas.set_uv_map()




# def texturing(i):
#     print('==========begin==========')
#     # recupero la camera
#     co = bpy.data.objects['Camera'] # co == camera object
#     cc = co.data                    # cc == camera camera 
#     cc.type = 'ORTHO'
#     cc.clip_start = 0.1
#     cc.clip_end = 10
#     cc.ortho_scale = 0.5
#     cc.display_size = 0.1
#
#     # setto la view3d in modo 'camera'
#
#     for area in bpy.context.screen.areas:
#         if area.type == 'VIEW_3D':
#             area.spaces[0].region_3d.view_perspective = 'CAMERA'
#             break
#
#     # nascondo gli oggetti low, visualizzo l'oggetto google 
#
#     collection = bpy.data.collections['low']
#     #collection = bpy.data.collections[tile_names[i]]
#     if collection : 
#         collection.hide_viewport = True
#         collection.hide_render = True
#
#     google = bpy.data.objects['google']
#     if google : 
#         google.hide_viewport = False
#         google.hide_render = False
#
#     # recupero il materiale 'atlas'
#
#     #atlas = bpy.data.materials['atlas3']
#     #atlas = bpy.data.materials[tile_names[i]]
#
#     textures = []
#
#     # recupero gli oggetti low-poly in base al nome
#
#     D = [ x for x in bpy.data.objects if x.name.startswith(object_name_prefix) and "_ref" not in x.name]
#
#     for d in D:
#         folder_path = tex_folder + d.name + "/"
#         atlas_path = folder_path + d.name + '_atlas.png'
#         
#         if not os.path.exists(folder_path):
#             os.makedirs(folder_path)
#         
#         material_name = "M_" + d.name
#         atlas = bpy.data.materials.get(material_name)
#         if not atlas:
#             atlas = bpy.data.materials.new(name=material_name)
#
#         # rimuovo i materiali dalla mesh e inserisco l'atlas   
#
#         m = d.data # m == mesh
#         m.materials.clear()
#         m.materials.append(atlas)
#
#         # recupero o creo l'uv_map
#
#         index = m.uv_layers.find('UVMap')
#         if index == -1:
#             m.uv_layers.new( name='UVMap')
#             index = m.uv_layers.find('UVMap')
#             if index == -1:
#                 print( 'cant find UVMap for object', d.name )
#                 continue
#         
#         # contatore delle facce
#
#         counter =  0    
#
#         # itero sulle facce della mesh
#
#         for face in m.polygons: 
#             n = face.normal  
#             c = face.center
#             
#             if n.z < 0.1 : # faccia laterale ( la tolleranza puo essere importante )
#                 counter +=1
#                 
#                 # determino larghezza fw ed altezza fh della faccia            
#                 # ( la faccia puo adesso avere piu di 4 vertici )
#
#                 fw = 0
#                 fh = 0
#                 if fabs( n.z ) < 0.1 : 
#                     
#                     u = Vector((0,0,1)) # up vector
#                     s = n.cross(u)      # side vector    
#                     xmin =  1e8
#                     xmax = -1e8
#                     zmin =  1e8
#                     zmax = -1e8
#                     for vi in face.vertices:
#                         v = m.vertices[ vi ].co
#                         cv = Vector(( v.x-c.x, v.y-c.y, v.z-c.z ))
#                         x = cv.dot(s) # distanza con segno di v rispetto a c lungo s 
#                         xmin = min(x,xmin)        
#                         xmax = max(x,xmax)        
#                         zmin = min(v.z,zmin)        
#                         zmax = max(v.z,zmax)        
#                     fw = xmax - xmin
#                     fh = zmax - zmin
#
#                 # posiziono la camera
#                 
#                 co.location.x = c.x + n.x * cam_distance
#                 co.location.y = c.y + n.y * cam_distance
#                 co.location.z = c.z 
#                 co.rotation_mode = 'XYZ'
#                 co.rotation_euler.x = pi / 2
#                 co.rotation_euler.y = 0
#                 co.rotation_euler.z = atan2( n.y, n.x ) + pi/2                    
#                 cc.ortho_scale = max( fw, fh )
#
#                 # renderizzo e salvo
#                 
#                 # l'underscore deve separare il nome oggetto dal nome faccia
#                 dname = d.name.replace('.','-').replace('_','-')    
#                 
#                 fname =  dname + '_' + str(counter) + ".png"
#                 #image_filename = tex_folder + fname
#                 image_filename = folder_path + fname
#                 
#                 w = int(fw*tex_scale)
#                 h = int(fh*tex_scale)
#                 
#                 bpy.context.scene.render.resolution_x = w
#                 bpy.context.scene.render.resolution_y = h
#                 bpy.context.scene.render.filepath = image_filename
#                 if do_render_eevee: bpy.ops.render.render(write_still = True)
#                 if do_render_opengl: bpy.ops.render.opengl( write_still=True)
#                 
#                 # salvo la faccia corrente e l'immagine generata in un oggetto Texture
#                 textures.append( Tex(image_filename, face, m, w, h ) )
#
#             else:
#                 # setto gli uv delle facce top/bottom
#                 for vert_idx, loop_idx in zip(face.vertices, face.loop_indices):
#                     uv_coords = m.uv_layers.active.data[loop_idx].uv
#                     uv_coords.x = 0
#                     uv_coords.y = 0
#                 
#     # sorto le texture in base all'altezza
#     textures.sort( key=lambda x : x.h, reverse =True ) 
#
#     # distribuisco le texture su righe larghe non piu di atlas_w
#     m = atlas_margin
#     y = m
#     i = 0 
#     ni = len(textures)
#     row = 0
#     while i<ni:
#         w = textures[i].w + m
#         h = textures[i].h + m
#         j=i+1
#         while j<ni and ((w + textures[j].w + 2*m ) < atlas_w):
#             w += textures[j].w + m
#             j += 1
#         x = m
#         for k in range(i,j):
#             t = textures[k]
#             t.x0 = x
#             t.y0 = y
#             t.x1 = x + t.w 
#             t.y1 = y + t.h
#             x += t.w 
#             x += m
#         y = y+h
#         i=j
#         row +=1
#
#     atlas_h = y
#
#     # creo e salvo l'atlas
#     print('making atlas', atlas_path,atlas_w,atlas_h)
#     atlas = Image.new( mode='RGB', size=(atlas_w, atlas_h) )
#     atlas_draw = ImageDraw.Draw(atlas)
#     atlas_draw.rectangle( [(0,0),(m,m)], fill=(170,113,94) ) # roof color
#     
#     for t in textures:
#         t.open()
#         atlas.paste( t.image, (t.x0, t.y0) )
#         t.close()
#         
#     atlas.save( atlas_path )
#
#     # ripercorro tutte le facce e imposto l' uv_map e il materiale
#     for t in textures:
#         face = t.face
#         mesh = t.mesh    
#         face.material_index = 0
#
#         n = face.normal  
#         c = face.center
#
#         for vert_idx, loop_idx in zip(face.vertices, face.loop_indices):
#             uv_coords = mesh.uv_layers.active.data[loop_idx].uv
#             
#             v = mesh.vertices[vert_idx].co # v == vertice
#             dz = v.z-c.z              
#             if dz > 0:
#                 uv_coords.y = 1-float(t.y0)/atlas_h
#             else:
#                 uv_coords.y = 1-float(t.y1)/atlas_h
#             
#             u = Vector((0,0,1)) # up vector
#             s = n.cross(u)      # side vector
#             cv = Vector(( v.x-c.x, v.y-c.y, v.z-c.z ))
#             if cv.dot( s ) < 0:
#                 uv_coords.x = float(t.x1)/atlas_w
#             else:
#                 uv_coords.x = float(t.x0)/atlas_w
#
#
#     bpy.ops.wm.save_mainfile()
#
#     a = bpy.data.images[atlas_path]
#     if a: 
#         a.filepath = atlas_path
#         a.reload()
#
#     print('==========done==========')
#


def load_name_arays():
    for current_dir, subdirs, files in os.walk(tiles_folder):
        dname = os.path.basename(current_dir)

        for file in files:
            if file == f"{dname}.shp":
                shape_files.append(os.path.join(current_dir, file))
                tile_names.append(dname)
            elif file == f"{dname}_ref.shp":
                shape_ref_files.append(os.path.join(current_dir, file))
    
    #roof_files = [os.path.join(roof_folder, f) for f in os.listdir(roof_folder) if os.path.isfile(os.path.join(roof_folder, f))]


def import_all(separate = True):
    for shp in shape_files:
        try:
            bpy.ops.importgis.shapefile(filepath=shp, shpCRS="EPSG:3857", elevSource="FIELD", fieldElevName="Q_PIEDE", fieldExtrudeName="ALT_GRONDA", extrusionAxis='Z', separateObjects=separate, fieldObjName="COD_OGG")
        except:
            print("Exception")

    for shp in shape_ref_files:
        try:
            bpy.ops.importgis.shapefile(filepath=shp, shpCRS="EPSG:3857", elevSource="FIELD", fieldElevName="Q_PIEDE", fieldExtrudeName="ALT_GRONDA", extrusionAxis='Z', separateObjects=separate, fieldObjName="COD_OGG")
        except:
            print("Exception")


def import_at(index, import_ref = False):
    try:
        bpy.ops.importgis.shapefile(filepath=shape_files[index], shpCRS="EPSG:3857", elevSource="FIELD", fieldElevName="Q_PIEDE", fieldExtrudeName="ALT_GRONDA", extrusionAxis='Z', separateObjects=False, fieldObjName="COD_OGG")
    except:
        print("Exception")

    if import_ref:
        try:
            bpy.ops.importgis.shapefile(filepath=shape_ref_files[index], shpCRS="EPSG:3857")
        except:
            print("Exception")


def import_ply_at(index):
    bpy.ops.wm.ply_import(filepath=roof_files[index])


def remove_from_collection(name):
    collection = bpy.data.collections.get(name)
    if collection:
        objs = list(collection.objects)

        for obj in objs:
            bpy.context.scene.collection.objects.link(obj)
        bpy.context.scene.collection.children.unlink(collection)
        bpy.data.collections.remove(collection)


def set_shape_parent(ref):
    for obj in bpy.context.scene.objects:
        if obj.name == 'google':
            continue
        if obj.name == 'Camera':
            continue
        
        name = ref.name
        if obj.name != name or obj.name != name.replace("_ref", ""):
            obj.select_set(True)

    ref.select_set(True)

    bpy.context.view_layer.objects.active = ref
    bpy.ops.object.parent_set(type='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    


def insert_into_low_collection(collection_name):
    collection = bpy.data.collections.get(collection_name)
    
    if collection is None:
        collection = bpy.data.collections.new(collection_name)
        bpy.context.scene.collection.children.link(collection)
    
    for obj in bpy.context.scene.objects:
        if obj.name == 'google':
            continue
        if obj.name == 'Camera':
            continue
        
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        collection.objects.link(obj)
        bpy.context.scene.collection.objects.unlink(obj)
        bpy.context.view_layer.objects.active = None
        bpy.ops.object.select_all(action='DESELECT')
        



def export():
    # select
    bpy.ops.export_scene.fbx(
        filepath=bpy.path.abspath(f"name.fbx"),
        use_selection=True
    )
    # deselect



def run():
    load_name_arays()

    mask = ["32_685000_4930000", "32_685500_4930000", "32_686000_4930000", "32_686500_4930000"]

    for i in range(len(tile_names)):
        if tile_names[i] in mask:
            import_at(i)

            ref = bpy.context.scene.objects[f"{tile_names[i]}_ref"]
            name = ref.name.replace("_ref", "")
            xroof = float(name.split("_")[1])
            yroof = float(name.split("_")[2])
            xref = ref.location.x
            yref = ref.location.y

            remove_from_collection(name)
            #set_shape_parent(ref)
            
            #insert_into_low_collection(tile_names[i])

            tex_folder = bpy.path.abspath( f"//{tile_names[i]}/" )

            # texturing
            #texturing(i)x

            # align roofs
            if do_import_roof:
                import_ply_at(i)

                roof = bpy.context.scene.objects[f"{name}"]

                bpy.ops.object.empty_add(type='PLAIN_AXES', location=(xroof, yroof, 0))
                roof_ref = bpy.context.object
                roof_ref.name = f"{name}_ref_roof"

                bpy.ops.object.select_all(action='DESELECT')

                roof.select_set(True)
                roof_ref.select_set(True)
                bpy.context.view_layer.objects.active = roof_ref

                # bpy.ops.object.parent_set(type='OBJECT', keep_transform=False)

                bpy.ops.transform.translate(value=(xref - xroof, yref - yroof, 0))


#run()
#texturing(0)

main = Main()
main.execute_texturing_process()
