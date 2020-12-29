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
  }
}
