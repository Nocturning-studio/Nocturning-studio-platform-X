///////////////////////////////////////////////////////////////////////////////////
// Module - Sound_environment_calculations.h
// Author: NSDeathman
///////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Sound_environment_context.h"

namespace EnvironmentCalculations
{
// Main pipeline functions
void CalculateEnvironmentalProperties(EnvironmentContext& context);
void CalculateEAXParameters(EnvironmentContext& context);
bool ValidatePhysicalContext(const EnvironmentContext& context);

// Sub-steps (exposed if needed, mostly internal)
void CalculateGeometricAcoustics(EnvironmentContext& context);
void CalculateMaterialProperties(EnvironmentContext& context);
void CalculateRoomAcoustics(EnvironmentContext& context);
void ApplyPsychoacoustics(EnvironmentContext& context);
} // namespace EnvironmentCalculations
