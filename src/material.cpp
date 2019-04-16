#include "material.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "bsdf.hpp"
#include "bsdf/params.hpp"
#include "utils/allocator.hpp"
#include "utils/buffer.hpp"
#include "utils/color.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <OSL/genclosure.h>
#include <OSL/oslclosure.h>
#include <OSL/oslconfig.h>
#include <OSL/oslexec.h>
#pragma clang diagnostic pop

#include <OpenImageIO/sysutil.h>

using namespace OSL_NAMESPACE;

struct service_t : public RendererServices {
  ~service_t() {
  }
};

struct material_t::details_t {
  static const uint32_t PARAMETER_BUFFER_SIZE = 256;
  
  static service_t*     service;
  static ShadingSystem* system;

  static thread_local PerThreadInfo*  pti;
  static thread_local ShadingContext* ctx;

  struct empty_params_t
  {
    static const uint32_t flags = bsdf::TRANSMIT;
  };

  ShaderGroupRef group;

  buffer_t<PARAMETER_BUFFER_SIZE> parameters;

  bool is_emitter;

  details_t()
    : is_emitter(false)
  {}

  static void boot(const std::string& path) {
    using namespace bsdf;

    if (service && system) {
      // material subsystem was already booted
      return;
    }

    service = new service_t(/* ... */);
    system  = new ShadingSystem(service, nullptr, nullptr);

    system->attribute("searchpath:shader", path + "/shaders");

    ClosureParam params[][32] = {
      {
        CLOSURE_FINISH_PARAM(empty_params_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::diffuse_t, n),
        CLOSURE_FINISH_PARAM(bsdf::lobes::diffuse_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::oren_nayar_t::in_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::oren_nayar_t::in_t, alpha),
        CLOSURE_FINISH_PARAM(bsdf::lobes::oren_nayar_t::in_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::reflect_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::reflect_t, eta),
        CLOSURE_FINISH_PARAM(bsdf::lobes::reflect_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::refract_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::refract_t, eta),
        CLOSURE_FINISH_PARAM(bsdf::lobes::refract_t)
      },
      {
        CLOSURE_STRING_PARAM(bsdf::lobes::microfacet_t, distribution),
        CLOSURE_VECTOR_PARAM(bsdf::lobes::microfacet_t, n),
        CLOSURE_VECTOR_PARAM(bsdf::lobes::microfacet_t, u),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::microfacet_t, xalpha),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::microfacet_t, yalpha),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::microfacet_t, eta),
        CLOSURE_INT_PARAM(bsdf::lobes::microfacet_t, refract),
        CLOSURE_FINISH_PARAM(bsdf::lobes::microfacet_t)
      }
    };

    system->register_closure("emission", bsdf_t::Emissive, params[0], NULL, NULL);
    system->register_closure("background", bsdf_t::Background, params[0], NULL, NULL);
    system->register_closure("transparent", bsdf_t::Transparent, params[0], NULL, NULL);
    system->register_closure("diffuse", bsdf_t::Diffuse, params[1], NULL, NULL);
    system->register_closure("oren_nayar", bsdf_t::OrenNayar, params[2], NULL, NULL);
    system->register_closure("reflection", bsdf_t::Reflection, params[3], NULL, NULL);
    system->register_closure("refraction", bsdf_t::Refraction, params[4], NULL, NULL);
    system->register_closure("microfacet", bsdf_t::Microfacet, params[5], NULL, NULL);
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
    system->getattribute(group.get(), "num_closures_needed", num_closures);

    ustring* closures;
    system->getattribute(group.get(), "closures_needed", TypeDesc::PTR, &closures);

    for (auto i=0; i<num_closures; ++i) {
      if (closures[i] == "emission") {
        is_emitter = true;
      }
    }
  }

  void execute(ShaderGlobals& sg) {
    if (!system->execute(ctx, *group, sg)) {
      std::cout << "Failed to run shader" << std::endl;
    }
  }

  void eval_closure(
    shading_result_t& result
  , const ClosureColor* c
  , const Imath::Color3f w = Imath::Color3f(1,1,1)) const
  {
    // if (!c) {
    //   if (result.bsdf) {
    // 	result.bsdf->add_lobe(
    // 	  bsdf_t::Transparent
    // 	, w
    // 	, empty_params_t());
    //   }
    //   return;
    // }

    switch(c->id) {
    case ClosureColor::MUL:
      {
	const auto cw = w * c->as_mul()->weight;
	eval_closure(result, c->as_mul()->closure, cw);
	break;
      }
    case ClosureColor::ADD:
      eval_closure(result, c->as_add()->closureA, w);
      eval_closure(result, c->as_add()->closureB, w);
      break;
    default:
      {
	const auto component = c->as_comp();
	const auto cw = w * Imath::Color3f(component->w);

	switch(component->id) {
        case bsdf_t::Background:
          result.e = cw;
          break;
        case bsdf_t::Emissive:
          result.e = cw;
          break;
	case bsdf_t::Diffuse:
	  if (result.bsdf) {
	    result.bsdf->add_lobe(
	      bsdf_t::Diffuse
	    , cw
	    , component->as<bsdf::lobes::diffuse_t>());
	  }
	  break;
	case bsdf_t::OrenNayar:
	  if (result.bsdf) {
            const auto params = component->as<bsdf::lobes::oren_nayar_t::in_t>();
	    result.bsdf->add_lobe(
              bsdf_t::OrenNayar
            , cw
            , bsdf::lobes::oren_nayar_t(params));
	  }
	  break;
	case bsdf_t::Microfacet:
	  if (result.bsdf) {
	    result.bsdf->add_lobe(
	      bsdf_t::Microfacet
	    , cw
	    , component->as<bsdf::lobes::microfacet_t>());
	  }
	  break;
	case bsdf_t::Reflection:
	  if (result.bsdf) {
	    result.bsdf->add_lobe(
	      bsdf_t::Reflection
	    , cw
	    , component->as<bsdf::lobes::reflect_t>());
	  }
	  break;
	case bsdf_t::Refraction:
	  if (result.bsdf) {
	    result.bsdf->add_lobe(
	      bsdf_t::Refraction
	    , cw
	    , component->as<bsdf::lobes::refract_t>());
	  }
	  break;
	case bsdf_t::Transparent:
	  if (result.bsdf) {
	    result.bsdf->add_lobe(
	      bsdf_t::Transparent
	    , cw
	    , component->as<empty_params_t>());
	  }
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

  virtual ~material_builder_t() {
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
    const auto p = material->details->parameters.write_float(f);
    
    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeFloat
    , p
    , false);
  }

  void parameter(
    const std::string& name
  , int f)
  {
    const auto p = material->details->parameters.write_int(f);
    
    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeInt
    , p
    , false);
  }

  void parameter(
    const std::string& name
  , const Imath::Color3f& c)
  {
    const auto p = material->details->parameters.write_3f(c.x, c.y, c.z);

    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeColor
    , p
    , false);
  }

  void parameter(
    const std::string& name
  , const std::string& s)
  {
    ustring us(s);
    const auto p = material->details->parameters.write_ptr(us.c_str());

    material_t::details_t::system->Parameter(
      name
    , TypeDesc::TypeString
    , p
    , false);
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
  allocator_t& allocator
, interaction_t<>* hits
, const active_t<>& active)
{
  for (auto i=0; i<active.num; ++i) {
    const auto index = active.index[i];

    ShaderGlobals sg;
    memset(&sg, 0, sizeof(ShaderGlobals));
    sg.P = hits->p.at(index);
    sg.I = hits->wi.at(index);
    sg.N = sg.Ng = hits->n.at(index);
    sg.u = hits->s[index];
    sg.v = hits->t[index];
    sg.backfacing = sg.N.dot(sg.I) > 0;

    details->execute(sg);

    shading_result_t result; 
    result.bsdf = new(allocator) bsdf_t();

    details->eval_closure(result, sg.Ci);

    hits->e.from(index, result.e);
    hits->bsdf[index] = result.bsdf;
  }
}

void material_t::evaluate(
  const Imath::V3f& p
, const Imath::V3f& wi
, const Imath::V3f& n
, const Imath::V2f& st
, shading_result_t& result)
{
  ShaderGlobals sg;
  memset(&sg, 0, sizeof(ShaderGlobals));
  sg.P = p;
  sg.I = wi;
  sg.N = sg.Ng = n;
  sg.u = st.x;
  sg.v = st.y;
  sg.backfacing = sg.N.dot(sg.I) < 0;

  details->execute(sg);

  result.bsdf = nullptr;
  details->eval_closure(result, sg.Ci);
}

bool material_t::is_emitter() const {
  return details->is_emitter;
}

void material_t::boot(const parsed_options_t& options, const std::string& path) {
  details_t::boot(path);
}

void material_t::attach() {
  details_t::attach();
}
