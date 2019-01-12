
bl_info = {
    "name": "Phosphoros",
    "author": "Jan Krueger <jan.krueger@gmail.com>",
    "version": (1, 0, 0),
    "blender": (2, 80, 0),
    "location": "Info header, render engine menu",
    "description": "Phospohoros renderer integration",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "support": "COMMUNITY",
    "category": "Render"}

import bpy

from . import (
    renderer,
)

class Phosphoros(bpy.types.RenderEngine):
    bl_idname = "PHOSPHOROS"
    bl_label = "Phosphoros"
    bl_use_shading_nodes = True

    def __init__(self):
        self.session = None

    def __del__(self):
        renderer.free(self)
        pass

    def update(self, data, depsgraph):
        if not self.session:
            renderer.create(self, data)
        renderer.reset(self, depsgraph, data)

    def render(self, depsgraph):
        renderer.render(self, depsgraph)

def register():
    from bpy.utils import register_class
    from . import properties
    from . import ui
    renderer.init()
    properties.register()
    ui.register()
    register_class(Phosphoros)

def unregister():
    from bpy.utils import unregister_class
    from . import properties
    from . import ui
    properties.unregister()
    ui.unregister()
    unregister_class(Phosphoros)
