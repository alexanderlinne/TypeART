#pragma once

#include "Database.hpp"

#include <set>
#include <unordered_map>

namespace meta {

class Cache {
  std::unordered_map<std::string, String*> string_store;
  std::unordered_map<std::string, di::StructureType*> structure_store;
  std::unordered_map<std::string, di::Subprogram*> subprogram_store;
  std::vector<Meta*> meta_info;

 public:
  Cache(const Database& db);

  void add(Meta& meta);
  [[nodiscard]] Meta* lookup(const Meta& meta) const;

  [[nodiscard]] String* lookup_string(const std::string& value) const;
  [[nodiscard]] di::StructureType* lookup_structure_type(const std::string& identifier, const std::string& directory,
                                                         const std::string& file, size_t line) const;
  [[nodiscard]] di::Subprogram* lookup_subprogram(const std::string& linkage_name) const;

 private:
  void add_mappings_for(Meta& meta);
  void create_mappings(const std::vector<std::unique_ptr<Meta>>& db_info);
};

}  // namespace meta
