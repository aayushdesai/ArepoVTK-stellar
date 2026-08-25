#ifndef AREPO_RT_STELLAR_FIELD_REGISTRY_H
#define AREPO_RT_STELLAR_FIELD_REGISTRY_H

#include <cstddef>
#include <string>
#include <vector>

namespace arepovtk {

enum class FieldSourceKind {
  SnapshotScalar,
  SnapshotVectorMagnitude,
  Derived
};

enum class PreferredScale {
  Linear,
  Log10,
  SymLog
};

struct FieldDescriptor {
  int legacy_index;
  std::string canonical_name;
  std::vector<std::string> aliases;
  std::string snapshot_dataset;
  FieldSourceKind source_kind;
  PreferredScale preferred_scale;
  std::string units;
  bool stellar_priority;
};

class StellarFieldRegistry {
 public:
  static const StellarFieldRegistry &LegacyCompatible();

  const FieldDescriptor *Find(const std::string &name) const;
  const FieldDescriptor *FindByLegacyIndex(int legacy_index) const;
  const std::vector<FieldDescriptor> &Fields() const;
  std::size_t Size() const;

 private:
  StellarFieldRegistry();
  void Add(const FieldDescriptor &descriptor);

  std::vector<FieldDescriptor> fields_;
};

const char *ToString(FieldSourceKind kind);
const char *ToString(PreferredScale scale);

}  // namespace arepovtk

#endif
