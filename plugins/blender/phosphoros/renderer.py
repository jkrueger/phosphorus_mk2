
def init():
    import bpy
    from . import (_phosphoros)
    import os.path

    path = os.path.dirname(__file__)
    user_path = os.path.dirname(os.path.abspath(bpy.utils.user_resource('CONFIG', path='')))
    resource_path = os.path.dirname(os.path.abspath(bpy.utils.resource_path('LOCAL')))

    _phosphoros.init(path, resource_path, user_path, bpy.app.background)

def exit():
    from . import (_phosphoros)
    _phosphoros.exit()

def create(engine, data, region=None, v3d=None, rv3d=None, preview_osl=False):
    from . import (_phosphoros)
    import bpy

    data = data.as_pointer()
    prefs = bpy.context.preferences.as_pointer()
    if region:
        region = region.as_pointer()
    if v3d:
        v3d = v3d.as_pointer()
    if rv3d:
        rv3d = rv3d.as_pointer()

    engine.session = _phosphoros.create(
        engine.as_pointer(), prefs, data, region, v3d, rv3d, preview_osl)

def free(engine):
    if hasattr(engine, "session"):
        if engine.session:
            from . import (_phosphoros)
            _phosphoros.free(engine.session)
        del engine.session

def reset(engine, depsgraph, data):
    from . import (_phosphoros)
    if hasattr(engine, "session"):
        depsgraph = depsgraph.as_pointer()
        data = data.as_pointer()
        _phosphoros.reset(engine.session, depsgraph, data)

def render(engine, depsgraph):
    from . import (_phosphoros)
    if hasattr(engine, "session"):
        _phosphoros.render(engine.session, depsgraph.as_pointer())
