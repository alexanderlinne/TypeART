#include "meta/Cache.hpp"

namespace meta {

Cache::Cache(const Database& db) {
  create_mappings(db.getMeta());
}

void Cache::add(meta::Meta& meta) {
  add_mappings_for(meta);
}

meta::Meta* Cache::lookup(const meta::Meta& meta) const {
  if (auto meta_string = meta::dyn_cast<meta::String>(&meta)) {
    return lookup_string(meta_string->get_data());
  } else if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(&meta)) {
    return lookup_structure_type(structure_type->get_identifier());
  } else if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(&meta)) {
    return lookup_subprogram(subprogram->get_linkage_name());
  } else {
    for (auto info : meta_info) {
      if (meta == *info) {
        return info;
      }
    }
  }
  return nullptr;
}

meta::String* Cache::lookup_string(const std::string& value) const {
  if (auto it = string_store.find(value); it != string_store.end()) {
    return it->second;
  }
  return nullptr;
}

meta::di::StructureType* Cache::lookup_structure_type(const std::string& identifier) const {
  if (auto it = structure_store.find(identifier); it != structure_store.end()) {
    return it->second;
  }
  return nullptr;
}

meta::di::Subprogram* Cache::lookup_subprogram(const std::string& linkage_name) const {
  if (auto it = subprogram_store.find(linkage_name); it != subprogram_store.end()) {
    return it->second;
  }
  return nullptr;
}

void Cache::add_mappings_for(meta::Meta& meta) {
  if (auto meta_string = meta::dyn_cast<meta::String>(&meta)) {
    string_store.try_emplace(meta_string->get_data(), meta_string);
  } else if (auto structure_type = meta::dyn_cast<meta::di::StructureType>(&meta)) {
    structure_store.try_emplace(structure_type->get_identifier(), structure_type);
  } else if (auto subprogram = meta::dyn_cast<meta::di::Subprogram>(&meta)) {
    subprogram_store.try_emplace(subprogram->get_linkage_name(), subprogram);
  } else {
    meta_info.push_back(&meta);
  }
}

void Cache::create_mappings(const std::vector<std::unique_ptr<Meta>>& db_info) {
  for (const auto& elem : db_info) {
    if (!elem) {
      continue;
    }
    add_mappings_for(*elem);
  }
}

}  // namespace meta
