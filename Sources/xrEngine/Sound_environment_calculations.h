#pragma once
#include "Sound_environment_context.h"

namespace EnvironmentCalculations
{
void CalculateEnvironmentalProperties(EnvironmentContext& context);
void CalculateEAXParameters(EnvironmentContext& context);
bool ValidatePhysicalContext(const EnvironmentContext& context);

void CalculateGeometricAcoustics(EnvironmentContext& context);
void CalculateMaterialProperties(EnvironmentContext& context);
void CalculateRoomAcoustics(EnvironmentContext& context);
} // namespace EnvironmentCalculations
