#ifndef AREPO_VTK_STELLAR_FEATURE_LANDMARKS_V066_H
#define AREPO_VTK_STELLAR_FEATURE_LANDMARKS_V066_H

#include <string>
#include <vector>

#include "stellar_feature_diagnostics_v064.h"

struct StellarWeightedValueV066 {
  double value;
  double weight;
};

double stellarWeightedQuantileV066(
    std::vector<StellarWeightedValueV066> values,
    double total_weight,
    double quantile);

bool stellarWriteFeatureLandmarksV066(
    const std::string &output_path,
    const std::string &scene_path,
    const StellarTransferParameters &parameters,
    const StellarFeatureProbeSummaryV064 &summary,
    std::string *error);

#endif
