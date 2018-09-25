#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "bsdf.hpp"
#include "bsdf/params.hpp"
#include "utils/color.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <OSL/genclosure.h>
#include <OSL/oslclosure.h>
#include <OSL/oslconfig.h>
#include <OSL/oslexec.h>
#pragma clang diagnostic pop

using namespace OSL_v1_10;

struct service_t : public RendererServices {
  ~service_t() {
  }
};

struct material_t::details_t {
  static service_t*     service;
  static ShadingSystem* system;

  static thread_local PerThreadInfo*  pti;
  static thread_local ShadingContext* ctx;

  union parameter_t {
    float f;
  };

  struct empty_params_t
  {};

  ShaderGroupRef group;

  parameter_t  storage[256];
  parameter_t* parameter;

  bool is_emitter;

  details_t()
    : parameter(storage), is_emitter(false)
  {}

  static void boot() {
    using namespace bsdf;
    
    service = new service_t(/* ... */);
    system  = new ShadingSystem(service, nullptr, nullptr);

    ClosureParam params[][32] = {
      {
       CLOSURE_FINISH_PARAMS(empty_params_t)
      },
      {
       CLOSURE_VECTOR_PARAM(bsdf::lobes::diffuse_t, n),
       CLOSURE_FINISH_PARAM(bsdf::lobes::diffuse_t)
      }
    };

    system->register_closure("emission", bsdf_t::Emissive, params[0], NULL, NULL);
    system->register_closure("diffuse", bsdf_t::Diffuse, params[1], NULL, NULL);
  }

  static void attach() {
    pti = system->create_thread_info();
    ctx = system->get_context(pti);
  }

  void init() {
    group = system->ShaderGroupBegin();
  }

  void finalize() {
    system->ShaderGroupEnd();

    int num_closures = 0;
    system->getattribute(group, "num_closures_needed", num_closures);

    ustring* closures;
    system->getattribute(group, "num_closures", TypeDesc::PTR, &closures);
    
    for (auto i=0; i<num_closures; ++i) {
      if (closures[i] == "emission") {
        is_light = true;
      }
    }
  }

  void execute(ShaderGlobals& sg) {
    system->execute(ctx, *group, sg);
  }

  void eval_closure(state_t<>* state, uint32_t index, const ClosureColor* c, const color_t w = color_t()) const {
    switch(c->id) {
    case ClosureColor::MUL:
      {
	const auto cw = w * c->as_mul()->weight;
	eval_closure(bsdf, c->as_mul()->closure, cw);
	break;
      }
    case ClosureColor::ADD:
      eval_closure(bsdf, c->as_add()->closureA, w);
      eval_closure(bsdf, c->as_add()->closureB, w);
      break;
    default:
      {
	const auto cw = w * c->as_mul()->weight;
	const auto component = c->as_comp();
	switch(component->id) {
        case bsdf_t::Emissive:
          state->shading.e += cw;
	case bsdf_t::Diffuse:
	  state->shading.bsdf[index]->add_lobe(
            bsdf_t::Diffuse
          , cw
          , component->as<bsdf::lobes::diffuse_t>());
	  break;
	}
      }
    }
  }
};

service_t* material_t::details_t::service = nullptr;
ShadingSystem* material_t::details_t::system = nullptr;

thread_local PerThreadInfo* material_t::details_t::pti = nullptr;
thread_local ShadingContext* material_t::details_t::ctx = nullptr;

struct material_builder_t : public material_t::builder_t {
  material_t* material;

  material_builder_t(material_t* material)
    : material(material)
  {
    material->details->init();
  }

  ~material_builder_t() {
    material->details->finalize();
  }

  void shader(
    const std::string& name
  , const std::string& layer
  , const std::string& type)
  {
    material_t::details_t::system->Shader(type, name, layer);
  }

  void connect(
    const std::string& src_param
  , const std::string& dst_param
  , const std::string& src_layer
  , const std::string& dst_layer)
  {
    material_t::details_t::system->ConnectShaders(
      src_layer
    , src_param
    , dst_layer
    , dst_param);
  }

  void parameter(
    const std::string& name
  , float f)
  {
    material->details->parameter->f = f;
    
    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeFloat
    , material->details->parameter);

    material->details->parameter++;
  }

  void parameter(
    const std::string& name
  , const color_t& c)
  {
    material->details->parameter[0].f = c.r;
    material->details->parameter[1].f = c.g;
    material->details->parameter[2].f = c.b;

    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeColor
    , material->details->parameter);

    material->details->parameter+=3;
  }
};

material_t::material_t()
  : details(new details_t())
{}

material_t::~material_t() {
  delete details;
}

material_t::builder_t* material_t::builder() {
  return new material_builder_t(this);
}

void material_t::evaluate(
  const scene_t& scene
, pipeline_state_t<>* state
, const active_t<>& active)
{
  for (auto i=0; i<active.num; ++i) {
    const auto index = active.index[i];
    const auto mesh  = scene.mesh(index);

    mesh->shading_parameters(state, i);

    ShaderGlobals sg;
    memset(&sg, 0, sizeof(ShaderGlobals));
    sg.P = state->rays.p.at(index);
    sg.I = state->rays.wi.at(index);
    sg.N = sg.Ng = state->shading.n.at(index);
    sg.u = state->shading.u[index];
    sg.v = state->shading.v[index];
    sg.backfacing = sg.N.dot(sg.I) < 0;

    details->execute(sg);

    state->shading.bsdf[index] = new bsdf_t();
    
    details->eval_closure(state, index, sg.Ci);
  }
}

bool material_t::is_emitter() const {
  return details->is_emitter;
}

void material_t::boot(const parsed_options_t& options) {
  details_t::boot();
}

void material_t::attach() {
  details_t::attach();
}
