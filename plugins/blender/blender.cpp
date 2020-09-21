#include "session.hpp"

#include <Python.h>

#include <MEM_guardedalloc.h>
#include <RNA_access.h>
#include <RNA_blender_cpp.h>
#include <RNA_types.h>

#include <iostream>

void *pylong_as_voidptr_typesafe(PyObject *object) {
  if(object == Py_None) {
    return NULL;
  }
  return PyLong_AsVoidPtr(object);
}

void python_thread_state_save(void** python_thread_state) {
  *python_thread_state = (void*)PyEval_SaveThread();
}

void python_thread_state_restore(void** python_thread_state) {
  PyEval_RestoreThread((PyThreadState*)*python_thread_state);
  *python_thread_state = NULL;
}

PyObject* init_func(PyObject* /*self*/, PyObject* args) {
  PyObject *path, *resource_path, *user_path;
  int headless;

  if(!PyArg_ParseTuple(args, "OOOi", &path, &resource_path, &user_path, &headless)) {
    return NULL;
  }

  blender::session_t::resources = _PyUnicode_AsString(resource_path);
  blender::session_t::path = _PyUnicode_AsString(path);

  Py_RETURN_NONE;
}

PyObject* exit_func(PyObject* /*self*/, PyObject* /*args*/) {
  Py_RETURN_NONE;
}

PyObject* create_func(PyObject* /*self*/, PyObject* args) {
  PyObject *pyengine, *pyuserpref, *pydata, *pyregion, *pyv3d, *pyrv3d;
  int preview_osl;

  if(!PyArg_ParseTuple(
        args
      , "OOOOOOi"
      , &pyengine
      , &pyuserpref
      , &pydata
      , &pyregion
      , &pyv3d
      , &pyrv3d
      , &preview_osl))
  {
    return NULL;
  }

  PointerRNA engineptr;
  RNA_pointer_create(NULL, &RNA_RenderEngine, (void*)PyLong_AsVoidPtr(pyengine), &engineptr);
  BL::RenderEngine engine(engineptr);

  PointerRNA userprefptr;
  RNA_pointer_create(NULL, &RNA_Preferences, (void*)PyLong_AsVoidPtr(pyuserpref), &userprefptr);
  BL::Preferences userpref(userprefptr);

  PointerRNA dataptr;
  RNA_main_pointer_create((Main*)PyLong_AsVoidPtr(pydata), &dataptr);
  BL::BlendData data(dataptr);

  PointerRNA regionptr;
  RNA_pointer_create(NULL, &RNA_Region, pylong_as_voidptr_typesafe(pyregion), &regionptr);
  BL::Region region(regionptr);

  PointerRNA v3dptr;
  RNA_pointer_create(NULL, &RNA_SpaceView3D, pylong_as_voidptr_typesafe(pyv3d), &v3dptr);
  BL::SpaceView3D v3d(v3dptr);

  PointerRNA rv3dptr;
  RNA_pointer_create(NULL, &RNA_RegionView3D, pylong_as_voidptr_typesafe(pyrv3d), &rv3dptr);
  BL::RegionView3D rv3d(rv3dptr);

  blender::session_t* session;

  if(rv3d) {
    int width = region.width();
    int height = region.height();
    
    session = new blender::session_t(engine, userpref, data, v3d, rv3d, width, height);
  }
  else {
    session = new blender::session_t(engine, userpref, data, preview_osl);
  }

  return PyLong_FromVoidPtr(session);
}

PyObject* free_func(PyObject* /*self*/, PyObject* value) {
  delete (blender::session_t*) PyLong_AsVoidPtr(value);

  Py_RETURN_NONE;
}

PyObject* reset_func(PyObject* /*self*/, PyObject* args)
{
  PyObject* pysession, *pydata, *pydepsgraph;

  if(!PyArg_ParseTuple(args, "OOO", &pysession, &pydepsgraph, &pydata))
    return NULL;

  blender::session_t* session = (blender::session_t*) PyLong_AsVoidPtr(pysession);

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph depsgraph(depsgraphptr);

  PointerRNA dataptr;
  RNA_main_pointer_create((Main*)PyLong_AsVoidPtr(pydata), &dataptr);
  BL::BlendData data(dataptr);

  python_thread_state_save(&session->python_thread_state);

  session->reset(data, depsgraph);

  python_thread_state_restore(&session->python_thread_state);

  Py_RETURN_NONE;
}

PyObject* render_func(PyObject* /*self*/, PyObject* args) {
  PyObject *pysession, *pydepsgraph;

  if(!PyArg_ParseTuple(args, "OO", &pysession, &pydepsgraph)) {
    return NULL;
  }

  blender::session_t* session = (blender::session_t*) PyLong_AsVoidPtr(pysession);

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID*)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph depsgraph(depsgraphptr);

  python_thread_state_save(&session->python_thread_state);

  session->render(depsgraph);

  python_thread_state_restore(&session->python_thread_state);

  Py_RETURN_NONE;
}

PyMethodDef methods[] = {
  {"init", init_func, METH_VARARGS, ""},
  {"exit", exit_func, METH_VARARGS, ""},
  {"create", create_func, METH_VARARGS, ""},
  {"free", free_func, METH_O, ""},
  {"reset", reset_func, METH_VARARGS, ""},
  {"render", render_func, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef python_module = {
  PyModuleDef_HEAD_INIT,
  "_phosphoros",
  "Blender phosphoros renderer integration",
  -1,
  methods,
  NULL, NULL, NULL, NULL
};

extern "C" {
  void* PyInit__phosphoros()
  {
    PyObject* out = PyModule_Create2(&python_module, PYTHON_API_VERSION);

    // TODO: add additional objects to module
  
    return (void*) out;
  }
}
