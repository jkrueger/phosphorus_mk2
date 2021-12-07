#pragma once

namespace blender {
  namespace util {
    /* Get the string representation of an enum value from Blender */
    inline std::string get_enum_identifier(PointerRNA &ptr, const char *name) {
      PropertyRNA *prop = RNA_struct_find_property(&ptr, name);
      const char *identifier = "";
      int value = RNA_property_enum_get(&ptr, prop);

      RNA_property_enum_identifier(NULL, &ptr, prop, value, &identifier);

      return identifier;
    }

    inline bool has_subdivision(BL::Object& o) {
      for (auto i=0; i<o.modifiers.length(); ++i) {
        BL::Modifier mod = o.modifiers[i];
        // TODO: test for viewport rendering instead if preview render
        bool enabled = mod.show_render();

        if (enabled && mod.type() == BL::Modifier::type_SUBSURF) {
          return true;
        }
      }
      return false;
    }
  }
}
