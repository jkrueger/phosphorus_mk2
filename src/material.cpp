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
#include <OSL/rendererservices.h>
#pragma clang diagnostic pop

#include <OpenImageIO/sysutil.h>

#include <set>

using namespace OSL_NAMESPACE;

struct service_t : public RendererServices {
  static const ustring tangent;
  static const ustring object_random;

  // uncomment when actually needed to query scene properties
  // const scene_t* scene;

  /* describes the rendered object to osl */
  struct object_t {
    interaction_t& hit;
  };

  ~service_t() {
  }

/*
  bool transform_points(
    ShaderGlobals* sg
  , ustring from, ustring to
  , float time
  , const Vec3* Pin, Vec3* Pout, int npoints
  , TypeDesc::VECSEMANTICS vectype) 
  {


    for (auto i=0; i<npoints, ++i) {
      if (from == world && to == object) {
        Pout[i] = xform.to_local(Pin[i]);
      }
      else if (from == object && to == world) {
        Pout[i] = xform.to_world(Pin[i]);
      }
      else if (from == to) {
        Pout[i] = Pin[i];
      }
      else {
        std::cout 
        << "Don't know how to transform from " 
        << from << " to " << to 
        << std::endl;
      }
    }
  }
*/

  bool get_attribute(
    ShaderGlobals* sg
  , bool derivatives
  , ustring object
  , TypeDesc type
  , ustring name
  , void* val)
  {
    if (sg && sg->objdata) {
      object_t* o = (object_t*) sg->objdata;
      if (name == tangent) {
        *((Imath::V3f*) val) = o->hit.xform.tangent();
        return true;
      }
      if (name == object_random) {
        *((float*) val) = o->hit.mesh->object_random();
        return true;
      }
    }

    return false;
  }
};

const ustring service_t::tangent("geom:tangent");
const ustring service_t::object_random("object:random");

struct material_t::details_t {
  static const uint32_t PARAMETER_BUFFER_SIZE = 1024;
  
  static service_t*     service;
  static ShadingSystem* system;

  static thread_local PerThreadInfo*  pti;
  static thread_local ShadingContext* ctx;

  ShaderGroupRef group;

  buffer_t parameters;

  bool is_emitter;

  std::set<std::string> attributes;

  details_t()
    : is_emitter(false)
    , parameters(PARAMETER_BUFFER_SIZE)
  {}

  static void boot(const std::string& path) {
    using namespace bsdf;

    if (service && system) {
      // material subsystem was already booted
      return;
    }

    service = new service_t(/* ... */);
    system  = new ShadingSystem(service, nullptr, nullptr);

    static const char* raytypes[] = {
      "camera",
      "shadow",
      "diffuse",
      "specular",
      "glossy",
      "__reserved__", // reserved by hit state
      "__reserved__"  // reserved by hit masked state
    };

    const int num_raytypes = sizeof(raytypes) / sizeof(raytypes[0]);

    system->attribute("searchpath:shader", path + "/shaders");
    system->attribute("raytypes", TypeDesc(TypeDesc::STRING, num_raytypes), raytypes);
    system->texturesys()->attribute("max_memory_MB", 24000);
    system->texturesys()->attribute("autotile", 64);

    ClosureParam params[][32] = {
      {
        CLOSURE_FINISH_PARAM(lobes::empty_params_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::diffuse_t, n),
        CLOSURE_FINISH_PARAM(bsdf::lobes::diffuse_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::translucent_t, n),
        CLOSURE_FINISH_PARAM(bsdf::lobes::translucent_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::oren_nayar_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::oren_nayar_t, alpha),
        CLOSURE_FINISH_PARAM(bsdf::lobes::oren_nayar_t)
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
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::sheen_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::sheen_t, r),
        CLOSURE_FINISH_PARAM(bsdf::lobes::sheen_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::disney_retro_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::disney_retro_t, roughness),
        CLOSURE_FINISH_PARAM(bsdf::lobes::disney_retro_t)
      },
      {
        CLOSURE_VECTOR_PARAM(bsdf::lobes::disney_microfacet_t, n),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::disney_microfacet_t, xalpha),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::disney_microfacet_t, yalpha),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::disney_microfacet_t, eta),
        CLOSURE_FLOAT_PARAM(bsdf::lobes::disney_microfacet_t, metallic),
        CLOSURE_COLOR_PARAM(bsdf::lobes::disney_microfacet_t, cspec0),
      }
    };

    system->register_closure("emission", bsdf_t::Emissive, params[0], NULL, NULL);
    system->register_closure("background", bsdf_t::Background, params[0], NULL, NULL);
    system->register_closure("transparent", bsdf_t::Transparent, params[0], NULL, NULL);
    system->register_closure("diffuse", bsdf_t::Diffuse, params[1], NULL, NULL);
    system->register_closure("translucent", bsdf_t::Translucent, params[2], NULL, NULL);
    system->register_closure("oren_nayar", bsdf_t::OrenNayar, params[3], NULL, NULL);
    system->register_closure("reflection", bsdf_t::Reflection, params[4], NULL, NULL);
    system->register_closure("refraction", bsdf_t::Refraction, params[5], NULL, NULL);
    system->register_closure("microfacet", bsdf_t::Microfacet, params[6], NULL, NULL);
    system->register_closure("sheen", bsdf_t::Sheen, params[7], NULL, NULL);
    system->register_closure("disney_diffuse", bsdf_t::DisneyDiffuse, params[1], NULL, NULL);
    system->register_closure("disney_retro", bsdf_t::DisneyRetro, params[8], NULL, NULL);
    system->register_closure("disney_sheen", bsdf_t::DisneySheen, params[1], NULL, NULL);
    system->register_closure("disney_microfacet", bsdf_t::DisneyMicrofacet, params[9], NULL, NULL);
  }

  static void attach() {
    pti = system->create_thread_info();
    ctx = system->get_context(pti);
  }

  static void add_image(const std::string path, void* data) {
    std::cout << "Adding packed image" << std::endl;
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
        // if (result.bsdf) {
        //   result.bsdf->add_lobe(
        //       bsdf_t::Emissive
        //     , cw
        //     , nullptr);
        // }
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
      case bsdf_t::Translucent:
        if (result.bsdf) {
          result.bsdf->add_lobe(
            bsdf_t::Translucent
          , cw
          , component->as<bsdf::lobes::translucent_t>());
        }
        break;
      case bsdf_t::DisneyDiffuse:
        if (result.bsdf) {
          result.bsdf->add_lobe(
            bsdf_t::DisneyDiffuse
          , cw
          , component->as<bsdf::lobes::diffuse_t>());
        }
        break;
      case bsdf_t::DisneySheen:
        if (result.bsdf) {
          result.bsdf->add_lobe(
            bsdf_t::DisneySheen
          , cw
          , component->as<bsdf::lobes::diffuse_t>());
        }
        break;
    	case bsdf_t::OrenNayar:
    	  if (result.bsdf) {
    	    result.bsdf->add_lobe(
            bsdf_t::OrenNayar
          , cw
          , component->as<bsdf::lobes::oren_nayar_t>());
    	  }
    	  break;
      case bsdf_t::DisneyRetro:
        if (result.bsdf) {
          result.bsdf->add_lobe(
            bsdf_t::DisneyRetro
          , cw
          , component->as<bsdf::lobes::disney_retro_t>());
        }
        break;
    	case bsdf_t::Microfacet:
    	  if (result.bsdf) {
    	    result.bsdf->add_microfacet_lobe(
    	      bsdf_t::Microfacet
    	    , cw
    	    , component->as<bsdf::lobes::microfacet_t>());
    	  }
    	  break;
      case bsdf_t::DisneyMicrofacet:
        if (result.bsdf) {
          result.bsdf->add_lobe(
            bsdf_t::DisneyMicrofacet
          , cw
          , component->as<bsdf::lobes::disney_microfacet_t>());
        }
        break;
    	case bsdf_t::Sheen:
    	  if (result.bsdf) {
    	    result.bsdf->add_lobe(
    	      bsdf_t::Sheen
    	    , cw
    	    , component->as<bsdf::lobes::sheen_t>());
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
    	    , component->as<bsdf::lobes::transparent_params_t>());
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
  , const std::vector<Imath::Color3f>& cs)
  {
    const auto& c = cs[0];

    const auto p = material->details->parameters.write_float(c.x);
    material->details->parameters.write_float(c.y);
    material->details->parameters.write_float(c.z);

    for (auto i=1; i<cs.size(); ++i) {
      const auto& c = cs[i];
      material->details->parameters.write_float(c.x);
      material->details->parameters.write_float(c.y);
      material->details->parameters.write_float(c.z);
    }
    
    material_t::details_t::system->Parameter(
      name
    , TypeDesc(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::COLOR, cs.size())
    , p
    , false);
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

  void add_attribute(const std::string& name) {
    ustring attr(name);
    material->details->attributes.insert(name);
  }
};

material_t::material_t(const std::string& name)
  : details(new details_t())
  , name(name)
{}

material_t::~material_t() {
  delete details;
}

material_t::builder_t* material_t::builder() {
  return new material_builder_t(this);
}

void material_t::evaluate(
  allocator_t& allocator
, interactions_t& hits
, const active_t& active)
{
  for (auto i=0; i<active.num; ++i) {
    const auto index = active.index[i];
    auto& hit = hits[index];

    service_t::object_t obj{hit};

    ShaderGlobals sg;
    memset(&sg, 0, sizeof(ShaderGlobals));
    sg.P = hit.p;
    sg.I = hit.wi;
    sg.N = sg.Ng = hit.n;
    sg.u = hit.st.x;
    sg.v = hit.st.y;
    sg.backfacing = sg.N.dot(sg.I) < 0;
    sg.objdata = &obj;
    sg.raytype = hit.flags;

    details->execute(sg);

    if (sg.Ci) {
      shading_result_t result; 
      result.bsdf = new(allocator) bsdf_t();

      details->eval_closure(result, sg.Ci);

      hit.e    = result.e;
      hit.bsdf = result.bsdf;
    } else {
      hit.e = Imath::Color3f(0.0f);
      hit.bsdf = nullptr;

      std::stringstream ss;
      ss << "Can't evaluate material: " 
         << name << "id=" << id 
         << ", is_emitter=" << is_emitter() 
         << std::endl;
      std::cout << ss.str() << std::endl;
    }
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
  sg.backfacing = n.dot(wi) < 0;
  sg.P = p;
  sg.I = wi;
  sg.N = sg.Ng = n;
  sg.u = st.x;
  sg.v = st.y;

  details->execute(sg);
  result.bsdf = nullptr;

  if (sg.Ci) {
    details->eval_closure(result, sg.Ci);
  }
  else {
    std::stringstream ss;
      ss << "Can't evaluate material: " 
         << name << "id=" << id 
         << ", is_emitter=" << is_emitter() 
         << std::endl;
      std::cout << ss.str() << std::endl;
  }
}

bool material_t::is_emitter() const {
  return details->is_emitter;
}

bool material_t::has_attribute(const std::string& name) const {
  return details->attributes.count(name);
}

void material_t::boot(const parsed_options_t& options, const std::string& path) {
  details_t::boot(path);
}

void material_t::attach() {
  details_t::attach();
}
