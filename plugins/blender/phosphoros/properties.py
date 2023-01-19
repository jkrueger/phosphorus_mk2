import bpy

from bpy.props import (
    BoolProperty,
    EnumProperty,
    FloatProperty,
    IntProperty,
    PointerProperty,
    StringProperty,
)

from math import pi

class PhosphorosRenderSettings(bpy.types.PropertyGroup):

    samples_per_pixel: IntProperty(
        name="Samples Per Pixel",
        description="Number of samples to render per pixel",
        min=1, max=2097151,
        default=9,
    )

    paths_per_sample: IntProperty(
        name="Paths Per Sample",
        description="Number of paths to trace per pixel sample",
        min=1, max=2097151,
        default=9,
    )

    max_path_depth: IntProperty(
        name="Maximum Path Depth",
        description="Terminate all paths at this depth",
        min=1, max=2097151,
        default=9,
    )

    render_diffuse_scene: BoolProperty(
        name="Render Diffuse Scene",
        description="Render the scene as if every surface is a diffuse reflector",
        default=False,
    )

    @classmethod
    def register(cls):
        bpy.types.Scene.phosphoros = PointerProperty(
            name="Phosphoros Render Settings",
            description="Phosphoros Render Settings",
            type=cls,
        )

    @classmethod
    def unregister(cls):
        del bpy.types.Scene.phosphoros

def register():
    bpy.utils.register_class(PhosphorosRenderSettings)

def unregister():
    bpy.utils.unregister_class(PhosphorosRenderSettings)

