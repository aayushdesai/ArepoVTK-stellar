#include "stellar_field_registry.h"

#include <stdexcept>

namespace arepovtk {
namespace {

FieldDescriptor MakeField(int index, const char *canonical_name,
                          const std::vector<std::string> &aliases,
                          const char *snapshot_dataset,
                          FieldSourceKind source_kind,
                          PreferredScale preferred_scale,
                          const char *units, bool stellar_priority) {
  FieldDescriptor field;
  field.legacy_index = index;
  field.canonical_name = canonical_name;
  field.aliases = aliases;
  field.snapshot_dataset = snapshot_dataset;
  field.source_kind = source_kind;
  field.preferred_scale = preferred_scale;
  field.units = units;
  field.stellar_priority = stellar_priority;
  return field;
}

}  // namespace

const StellarFieldRegistry &StellarFieldRegistry::LegacyCompatible() {
  static const StellarFieldRegistry registry;
  return registry;
}

StellarFieldRegistry::StellarFieldRegistry() {
  Add(MakeField(0, "Density", {}, "Density", FieldSourceKind::SnapshotScalar,
                PreferredScale::Log10, "code_density", true));
  Add(MakeField(1, "Temp", {"Temperature"}, "InternalEnergy",
                FieldSourceKind::SnapshotScalar, PreferredScale::Log10,
                "kelvin_or_code_specific_energy", true));
  Add(MakeField(2, "VelMag", {"VelocityMagnitude"}, "Velocities",
                FieldSourceKind::SnapshotVectorMagnitude,
                PreferredScale::Log10, "code_velocity", true));
  Add(MakeField(3, "Entropy", {}, "", FieldSourceKind::Derived,
                PreferredScale::Log10, "code_entropy_proxy", true));
  Add(MakeField(4, "Metal", {"Metallicity"}, "Metallicity",
                FieldSourceKind::SnapshotScalar, PreferredScale::Log10,
                "mass_fraction", false));
  Add(MakeField(5, "SzY", {}, "", FieldSourceKind::Derived,
                PreferredScale::Log10, "legacy_proxy", false));
  Add(MakeField(6, "XRay", {}, "", FieldSourceKind::Derived,
                PreferredScale::Log10, "legacy_proxy", false));
  Add(MakeField(7, "BMag", {"MagneticFieldMagnitude"}, "MagneticField",
                FieldSourceKind::SnapshotVectorMagnitude,
                PreferredScale::Log10, "microgauss", true));
  Add(MakeField(8, "ShockDeDt", {}, "", FieldSourceKind::Derived,
                PreferredScale::SymLog, "legacy_proxy", false));
}

void StellarFieldRegistry::Add(const FieldDescriptor &descriptor) {
  if (descriptor.legacy_index < 0 || descriptor.canonical_name.empty()) {
    throw std::logic_error("invalid field descriptor");
  }
  if (FindByLegacyIndex(descriptor.legacy_index) != nullptr) {
    throw std::logic_error("duplicate legacy field index");
  }
  if (Find(descriptor.canonical_name) != nullptr) {
    throw std::logic_error("duplicate field name");
  }
  for (const std::string &alias : descriptor.aliases) {
    if (alias.empty() || Find(alias) != nullptr || alias == descriptor.canonical_name) {
      throw std::logic_error("invalid or duplicate field alias");
    }
  }
  fields_.push_back(descriptor);
}

const FieldDescriptor *StellarFieldRegistry::Find(const std::string &name) const {
  for (const FieldDescriptor &field : fields_) {
    if (field.canonical_name == name) {
      return &field;
    }
    for (const std::string &alias : field.aliases) {
      if (alias == name) {
        return &field;
      }
    }
  }
  return nullptr;
}

const FieldDescriptor *StellarFieldRegistry::FindByLegacyIndex(
    int legacy_index) const {
  for (const FieldDescriptor &field : fields_) {
    if (field.legacy_index == legacy_index) {
      return &field;
    }
  }
  return nullptr;
}

const std::vector<FieldDescriptor> &StellarFieldRegistry::Fields() const {
  return fields_;
}

std::size_t StellarFieldRegistry::Size() const { return fields_.size(); }

const char *ToString(FieldSourceKind kind) {
  switch (kind) {
    case FieldSourceKind::SnapshotScalar:
      return "snapshot_scalar";
    case FieldSourceKind::SnapshotVectorMagnitude:
      return "snapshot_vector_magnitude";
    case FieldSourceKind::Derived:
      return "derived";
  }
  throw std::logic_error("unknown FieldSourceKind");
}

const char *ToString(PreferredScale scale) {
  switch (scale) {
    case PreferredScale::Linear:
      return "linear";
    case PreferredScale::Log10:
      return "log10";
    case PreferredScale::SymLog:
      return "symlog";
  }
  throw std::logic_error("unknown PreferredScale");
}

}  // namespace arepovtk
