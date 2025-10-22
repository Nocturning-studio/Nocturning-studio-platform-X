///////////////////////////////////////////////////////////////////////////////////
// Created: 22.10.2025
// Author: NSDeathman
// Path tracing EAX - Calculation Components
// Nocturning studio for X-Platform
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////
#include "Sound_environment_context.h"
///////////////////////////////////////////////////////////////////////////////////
namespace EnvironmentCalculations
{
/**
 * @brief Calculates continuous multipliers for environment parameters
 */
void CalculateContinuousMultipliers(EnvironmentContext& context);

/**
 * @brief Calculates basic geometric properties from raycast data
 */
void CalculateGeometricProperties(EnvironmentContext& context);

/**
 * @brief Calculates material-aware reflectivity
 */
void CalculateReflectivity(EnvironmentContext& context);

/**
 * @brief Calculates echo parameters using physical acoustics
 */
void CalculateEchoParameters(EnvironmentContext& context);

/**
 * @brief Calculates reverb parameters using physical acoustics
 */
void CalculateReverbParameters(EnvironmentContext& context);

/**
 * @brief Calculates reflection parameters
 */
void CalculateReflectionParameters(EnvironmentContext& context);

/**
 * @brief Final mapping to EAX parameters
 */
void MapToEAXParameters(EnvironmentContext& context);
} // namespace EnvironmentCalculations
