#include "../src/stellar_field_registry.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace {

void Require(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  using arepovtk::FieldDescriptor;
  using arepovtk::FieldSourceKind;
  using arepovtk::PreferredScale;
  using arepovtk::StellarFieldRegistry;

  const StellarFieldRegistry &registry =
      StellarFieldRegistry::LegacyCompatible();
  Require(registry.Size() == 9, "legacy registry must contain nine slots");

  const char *expected_names[] = {"Density", "Temp", "VelMag", "Entropy",
                                  "Metal",   "SzY",  "XRay",   "BMag",
                                  "ShockDeDt"};
  std::set<std::string> names;
  for (int index = 0; index < 9; ++index) {
    const FieldDescriptor *field = registry.FindByLegacyIndex(index);
    Require(field != nullptr, "every legacy index must resolve");
    Require(field->canonical_name == expected_names[index],
            "legacy index mapping changed");
    Require(names.insert(field->canonical_name).second,
            "canonical field names must be unique");
    Require(registry.Find(field->canonical_name) == field,
            "canonical lookup must return the indexed descriptor");
  }

  const FieldDescriptor *temperature = registry.Find("Temperature");
  Require(temperature != nullptr && temperature->legacy_index == 1,
          "Temperature alias must resolve to Temp");
  Require(temperature->snapshot_dataset == "InternalEnergy",
          "Temp must preserve its legacy snapshot source");

  const FieldDescriptor *velocity = registry.Find("VelocityMagnitude");
  Require(velocity != nullptr && velocity->legacy_index == 2,
          "VelocityMagnitude alias must resolve to VelMag");
  Require(velocity->source_kind == FieldSourceKind::SnapshotVectorMagnitude,
          "VelMag must be identified as a vector magnitude");

  const FieldDescriptor *magnetic =
      registry.Find("MagneticFieldMagnitude");
  Require(magnetic != nullptr && magnetic->legacy_index == 7,
          "MagneticFieldMagnitude alias must resolve to BMag");
  Require(magnetic->stellar_priority,
          "magnetic-field magnitude must be stellar priority");

  Require(registry.Find("NotAField") == nullptr,
          "unknown field lookup must fail without fallback");
  Require(registry.FindByLegacyIndex(-1) == nullptr,
          "negative legacy index must not resolve");
  Require(registry.FindByLegacyIndex(9) == nullptr,
          "out-of-range legacy index must not resolve");

  Require(std::string(arepovtk::ToString(FieldSourceKind::Derived)) ==
              "derived",
          "source-kind serialization changed");
  Require(std::string(arepovtk::ToString(PreferredScale::SymLog)) ==
              "symlog",
          "scale serialization changed");

  std::cout << "STELLAR_FIELD_REGISTRY_TEST_OK fields=" << registry.Size()
            << '\n';
  return 0;
}
