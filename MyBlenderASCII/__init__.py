# -*- coding:utf-8 -*-

#  ***** GPL LICENSE BLOCK *****
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#  All rights reserved.
#  ***** GPL LICENSE BLOCK *****

import bpy

bl_info = {
	'name': 'ASC-Importer',
	'description': 'Various tools for handle geodata',
	'author': 'AlexGianelli',
	'license': 'GPL',
	'deps': '',
	'version': (1, 0, 0),
	'blender': (2, 83, 0),
	'location': 'View3D > Tools > ASC',
	'warning': '',
	'wiki_url': '--wiki--',
	'tracker_url': '--issues--',
	'link': '',
	'support': 'COMMUNITY',
	'category': '3D View'
	}

class BlenderVersionError(Exception):
	pass

if bl_info['blender'] > bpy.app.version:
	raise BlenderVersionError(f"This addon requires Blender >= {bl_info['blender']}")

#Modules
IMPORT_ASC = True

import os, sys, tempfile
from datetime import datetime

def getAppData():
	home = os.path.expanduser('~')
	loc = os.path.join(home, '.basc')
	if not os.path.exists(loc):
		os.mkdir(loc)
	return loc

APP_DATA = getAppData()

import logging
from logging.handlers import RotatingFileHandler
#temporary set log level, will be overriden reading addon prefs
#logsFormat = "%(levelname)s:%(name)s:%(lineno)d:%(message)s"
logsFormat = '{levelname}:{name}:{lineno}:{message}'
logsFileName = 'basc.log'
try:
	#logsFilePath = os.path.join(os.path.dirname(__file__), logsFileName)
	logsFilePath = os.path.join(APP_DATA, logsFileName)
	#logging.basicConfig(level=logging.getLevelName('DEBUG'), format=logsFormat, style='{', filename=logsFilePath, filemode='w')
	logHandler = RotatingFileHandler(logsFilePath, mode='a', maxBytes=512000, backupCount=1)
except PermissionError:
	#logsFilePath = os.path.join(bpy.app.tempdir, logsFileName)
	logsFilePath = os.path.join(tempfile.gettempdir(), logsFileName)
	logHandler = RotatingFileHandler(logsFilePath, mode='a', maxBytes=512000, backupCount=1)
logHandler.setFormatter(logging.Formatter(logsFormat, style='{'))
logger = logging.getLogger(__name__)
logger.addHandler(logHandler)
logger.setLevel(logging.DEBUG)
logger.info('###### Starting new Blender session : {}'.format(datetime.now().strftime('%Y-%m-%d %H:%M:%S')))

def _excepthook(exc_type, exc_value, exc_traceback):
	if 'ASC-Importer' in exc_traceback.tb_frame.f_code.co_filename:
		logger.error("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))
	sys.__excepthook__(exc_type, exc_value, exc_traceback)

sys.excepthook = _excepthook #warn, this is a global variable, can be overrided by another addon

####
'''
Workaround for `sys.excepthook` thread
https://stackoverflow.com/questions/1643327/sys-excepthook-and-threading
'''
import threading

init_original = threading.Thread.__init__

def init(self, *args, **kwargs):

	init_original(self, *args, **kwargs)
	run_original = self.run

	def run_with_except_hook(*args2, **kwargs2):
		try:
			run_original(*args2, **kwargs2)
		except Exception:
			sys.excepthook(*sys.exc_info())

	self.run = run_with_except_hook

threading.Thread.__init__ = init


import ssl
if (not os.environ.get('PYTHONHTTPSVERIFY', '') and
	getattr(ssl, '_create_unverified_context', None)):
	ssl._create_default_https_context = ssl._create_unverified_context

#from .core.checkdeps import HAS_GDAL, HAS_PYPROJ, HAS_PIL, HAS_IMGIO
from .core.settings import settings

#Import all modules which contains classes that must be registed (classes derived from bpy.types.*)
from . import prefs
from . import geoscene

if IMPORT_ASC:
	from .operators import io_import_asc

import bpy.utils.previews as iconsLib
icons_dict = {}

class ASC_logs(bpy.types.Operator):
	bl_idname = "basc.logs"
	bl_description = 'Display logs'
	bl_label = "Logs"

	def execute(self, context):
		if logsFileName in bpy.data.texts:
			logs = bpy.data.texts[logsFileName]
		else:
			logs = bpy.data.texts.load(logsFilePath)
		bpy.ops.screen.area_split(direction='VERTICAL', factor=0.5)
		area = bpy.context.area
		area.type = 'TEXT_EDITOR'
		area.spaces[0].text = logs
		bpy.ops.text.reload()
		return {'FINISHED'}

class Menu_ASC_import(bpy.types.Menu):
	bl_label = "Import"
	def draw(self, context):
		if IMPORT_ASC:
			self.layout.operator('importgis.asc_file', icon_value=icons_dict["asc"].icon_id, text="ESRI ASCII Grid (.asc)")

class Menu_ASC(bpy.types.Menu):
	bl_label = "ASC"
	# Set the menu operators and draw functions
	def draw(self, context):
		layout = self.layout
		layout.menu('Menu_ASC_import', icon='IMPORT')
		layout.separator()
		layout.operator("basc.logs", icon='TEXT')

menus = [
Menu_ASC,
Menu_ASC_import,
]

def add_gis_menu(self, context):
	if context.mode == 'OBJECT':
		self.layout.menu('Menu_ASC')


def register():
	#icons
	global icons_dict
	icons_dict = iconsLib.new()
	icons_dir = os.path.join(os.path.dirname(__file__), "icons")
	for icon in os.listdir(icons_dir):
		name, ext = os.path.splitext(icon)
		icons_dict.load(name, os.path.join(icons_dir, icon), 'IMAGE')

	#operators
	prefs.register()
	geoscene.register()

	bpy.utils.register_class(ASC_logs)

	for menu in menus:
		try:
			bpy.utils.register_class(menu)
		except ValueError as e:
			logger.warning('{} is already registered, now unregister and retry... '.format(menu))
			bpy.utils.unregister_class(menu)
			bpy.utils.register_class(menu)

	if IMPORT_ASC:
		io_import_asc.register()

	#menus
	bpy.types.VIEW3D_MT_editor_menus.append(add_gis_menu)

	#Setup prefs
	preferences = bpy.context.preferences.addons[__package__].preferences
	logger.setLevel(logging.getLevelName(preferences.logLevel)) #will affect all child logger

	#update core settings according to addon prefs
	settings.proj_engine = preferences.projEngine
	settings.img_engine = preferences.imgEngine


def unregister():
	global icons_dict
	iconsLib.remove(icons_dict)

	bpy.types.VIEW3D_MT_editor_menus.remove(add_gis_menu)

	for menu in menus:
		bpy.utils.unregister_class(menu)

	prefs.unregister()
	geoscene.unregister()

	bpy.utils.unregister_class(ASC_logs)

	if IMPORT_ASC:
		io_import_asc.unregister()

if __name__ == "__main__":
	register()
