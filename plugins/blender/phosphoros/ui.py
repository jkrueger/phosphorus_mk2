import bpy

from bpy.types import (
    Panel,
    Operator,
)

class PhosphorosButtonsPanel:
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "render"
    COMPAT_ENGINES = {'PHOSPHOROS'}

    @classmethod
    def poll(cls, context):
        return context.engine in cls.COMPAT_ENGINES

class PHOSPHOROS_RENDER_PT_sampling(PhosphorosButtonsPanel, Panel):
    bl_label = "Sampling"

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        pscene = scene.phosphoros

        layout.use_property_split = True
        layout.use_property_decorate = False

        col = layout.column(align=True)
        col.prop(pscene, "samples_per_pixel", text="SPP")
        col.prop(pscene, "paths_per_sample", text="PPS")
        col.prop(pscene, "max_path_depth", text="Path Depth")

class PHOSPHOROS_RENDER_PT_options(PhosphorosButtonsPanel, Panel):
    bl_label = "Options"

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        pscene = scene.phosphoros

        layout.use_property_split = True
        layout.use_property_decorate = False

        col = layout.column(align=True)
        col.prop(pscene, "render_diffuse_scene", text="Render Diffuse Scene")

def register():
    from bpy.utils import register_class

    register_class(PHOSPHOROS_RENDER_PT_sampling)
    register_class(PHOSPHOROS_RENDER_PT_options)

def unregister():
    from bpy.utils import unregister_class

    unregister_class(PHOSPHOROS_RENDER_PT_sampling)
    unregister_class(PHOSPHOROS_RENDER_PT_options)
