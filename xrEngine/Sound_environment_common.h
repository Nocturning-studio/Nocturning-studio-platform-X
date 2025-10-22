///////////////////////////////////////////////////////////////////////////////////
// Created: 10.12.2024
// Author: NSDeathman
// EAX Environment Data Structure
///////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////

static const Fvector3 BalancedHemisphereDirections[] = {
	// Основные направления (первое полушарие)
	{0.000000, 0.000000, 1.000000},	  {0.577350, 0.577350, 0.577350},	{0.577350, -0.577350, 0.577350},
	{-0.577350, 0.577350, 0.577350},  {-0.577350, -0.577350, 0.577350}, {0.850651, 0.000000, 0.525731},
	{-0.850651, 0.000000, 0.525731},  {0.525731, 0.850651, 0.000000},	{0.525731, -0.850651, 0.000000},
	{0.000000, 0.525731, 0.850651},	  {0.000000, -0.525731, 0.850651},	{0.309017, 0.000000, 0.951057},
	{-0.309017, 0.000000, 0.951057},  {0.000000, 0.309017, 0.951057},	{0.000000, -0.309017, 0.951057},
	{0.809017, 0.309017, 0.500000},	  {0.809017, -0.309017, 0.500000},	{-0.809017, 0.309017, 0.500000},
	{-0.809017, -0.309017, 0.500000}, {0.500000, 0.809017, 0.309017},	{0.500000, -0.809017, 0.309017},
	{-0.500000, 0.809017, 0.309017},  {-0.500000, -0.809017, 0.309017}};

static const u32 BALANCED_DIRECTIONS_COUNT = sizeof(BalancedHemisphereDirections) / sizeof(Fvector3);
static const float SMART_RAY_DISTANCE = 10000.0f;

// Структура для хранения информации о попадании луча
struct SRayHitInfo
{
	Fvector vPosition;
	Fvector vNormal;
	float fDistance;
	u32 material_type;
	bool bHit;

	SRayHitInfo() : fDistance(0), material_type(0), bHit(false)
	{
		vPosition.set(0, 0, 0);
		vNormal.set(0, 0, 0);
	}
};

struct SReflectionAnalysis
{
	float fPrimaryReflections;	 // Первичные отражения
	float fSecondaryReflections; // Вторичные отражения
	float fReflectionDelay;		 // Средняя задержка отражений
	float fReflectionEnergy;	 // Общая энергия отражений
	float fSurfaceComplexity;	 // Сложность поверхностей
};

// Физические акустические параметры
struct SPhysicalAcoustics
{
	float fRoomVolume;		 // Объем помещения (м³)
	float fEchoStrength;	 // Сила эха (0-1) на основе физической модели
	float fReverbTime;		 // Время реверберации (сек)
	float fCriticalDistance; // Критическое расстояние (м)
	float fAdjustedRadius;	 // Исправленный радиус с учетом открытости
};

// Усовершенствованная система направлений для точного анализа
static const Fvector3 DetailedSphereDirections[] = {
	// Основные оси (6 направлений)
	{1.000000, 0.000000, 0.000000},
	{-1.000000, 0.000000, 0.000000},
	{0.000000, 1.000000, 0.000000},
	{0.000000, -1.000000, 0.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, -1.000000},

	// Углы куба (8 направлений)
	{0.577350, 0.577350, 0.577350},
	{0.577350, 0.577350, -0.577350},
	{0.577350, -0.577350, 0.577350},
	{0.577350, -0.577350, -0.577350},
	{-0.577350, 0.577350, 0.577350},
	{-0.577350, 0.577350, -0.577350},
	{-0.577350, -0.577350, 0.577350},
	{-0.577350, -0.577350, -0.577350},

	// Дополнительные направления для лучшего покрытия (42 направления)
	{0.850651, 0.000000, 0.525731},
	{-0.850651, 0.000000, 0.525731},
	{0.525731, 0.850651, 0.000000},
	{0.525731, -0.850651, 0.000000},
	{0.000000, 0.525731, 0.850651},
	{0.000000, -0.525731, 0.850651},
	{0.309017, 0.000000, 0.951057},
	{-0.309017, 0.000000, 0.951057},
	{0.000000, 0.309017, 0.951057},
	{0.000000, -0.309017, 0.951057},
	{0.809017, 0.309017, 0.500000},
	{0.809017, -0.309017, 0.500000},
	{-0.809017, 0.309017, 0.500000},
	{-0.809017, -0.309017, 0.500000},
	{0.500000, 0.809017, 0.309017},
	{0.500000, -0.809017, 0.309017},
	{-0.500000, 0.809017, 0.309017},
	{-0.500000, -0.809017, 0.309017},
	{0.309017, 0.500000, 0.809017},
	{-0.309017, 0.500000, 0.809017},
	{0.309017, -0.500000, 0.809017},
	{-0.309017, -0.500000, 0.809017},
	{0.000000, 0.809017, 0.587785},
	{0.000000, -0.809017, 0.587785},
	{0.587785, 0.000000, 0.809017},
	{-0.587785, 0.000000, 0.809017},
	{0.688191, 0.500000, 0.525731},
	{-0.688191, 0.500000, 0.525731},
	{0.688191, -0.500000, 0.525731},
	{-0.688191, -0.500000, 0.525731},
	{0.500000, 0.688191, 0.525731},
	{-0.500000, 0.688191, 0.525731},
	{0.500000, -0.688191, 0.525731},
	{-0.500000, -0.688191, 0.525731},
	{0.525731, 0.500000, 0.688191},
	{-0.525731, 0.500000, 0.688191},
	{0.525731, -0.500000, 0.688191},
	{-0.525731, -0.500000, 0.688191},
	{0.362357, 0.262866, 0.894427},
	{-0.362357, 0.262866, 0.894427},
	{0.362357, -0.262866, 0.894427},
	{-0.362357, -0.262866, 0.894427},
	{0.262866, 0.362357, 0.894427},
	{-0.262866, 0.362357, 0.894427},
	{0.262866, -0.362357, 0.894427},
	{-0.262866, -0.362357, 0.894427},
	{0.425325, 0.688191, 0.587785},
	{-0.425325, 0.688191, 0.587785},
	{0.425325, -0.688191, 0.587785},
	{-0.425325, -0.688191, 0.587785}};

static const u32 DETAILED_DIRECTIONS_COUNT = sizeof(DetailedSphereDirections) / sizeof(Fvector3);

static const float MAX_RAY_DISTANCE = 10000.0f; // Максимальное расстояние трассировки
static const float MIN_RAY_DISTANCE = 0.1f;		// Минимальное расстояние трассировки

struct SEAXEnvironmentData
{
	SReflectionAnalysis sReflectionData;
	SPhysicalAcoustics sPhysicalData;

	// Основные геометрические параметры
	float fEnvironmentRadius;	// Средний радиус окружения (метры)
	float fEnvironmentVariance; // Разброс расстояний (0-1), определяет "открытость"
	float fEnvironmentDensity; // Плотность окружения (0-1), отношение попавших лучей

	// Акустические свойства
	float fFogDensity;	 // Плотность тумана/воздуха
	float fRoomSize;	 // Нормализованный размер помещения (0-1)
	float fReflectivity; // Коэффициент отражения поверхностей (0-1)

	// Производные параметры для EAX
	float fOpenness;	 // Степень открытости пространства (0-1)
	float fEnclosedness; // Степень закрытости пространства (0-1)
	float fUniformity;	 // Равномерность распределения поверхностей (0-1)

	// Временные метки и валидация
	u32 dwFrameStamp; // Кадр, в котором были обновлены данные
	bool bDataValid;  // Флаг валидности данных

	// Конструктор по умолчанию
	SEAXEnvironmentData()
	{
		Reset();
	}

	// Сброс к значениям по умолчанию
	void Reset()
	{
		fEnvironmentRadius = 10.0f;
		fEnvironmentVariance = 0.5f;
		fEnvironmentDensity = 0.5f;
		fFogDensity = 0.0f;
		fRoomSize = 0.5f;
		fReflectivity = 0.5f;
		fOpenness = 0.5f;
		fEnclosedness = 0.5f;
		fUniformity = 0.5f;

		// Сброс физических параметров
		sPhysicalData.fRoomVolume = 100.0f;
		sPhysicalData.fEchoStrength = 0.3f;
		sPhysicalData.fReverbTime = 1.0f;
		sPhysicalData.fCriticalDistance = 5.0f;

		// Сброс анализа отражений
		sReflectionData.fPrimaryReflections = 0.1f;
		sReflectionData.fSecondaryReflections = 0.05f;
		sReflectionData.fReflectionDelay = 0.02f;
		sReflectionData.fReflectionEnergy = 0.1f;
		sReflectionData.fSurfaceComplexity = 0.3f;

		dwFrameStamp = 0;
		bDataValid = false;
	}

	// Копирование данных
	void CopyFrom(const SEAXEnvironmentData& other)
	{
		fEnvironmentRadius = other.fEnvironmentRadius;
		fEnvironmentVariance = other.fEnvironmentVariance;
		fEnvironmentDensity = other.fEnvironmentDensity;
		fFogDensity = other.fFogDensity;
		fRoomSize = other.fRoomSize;
		fReflectivity = other.fReflectivity;
		fOpenness = other.fOpenness;
		fEnclosedness = other.fEnclosedness;
		fUniformity = other.fUniformity;

		// Копирование физических параметров
		sPhysicalData = other.sPhysicalData;

		// Копирование анализа отражений
		sReflectionData = other.sReflectionData;

		dwFrameStamp = other.dwFrameStamp;
		bDataValid = other.bDataValid;
	}

	// Проверка на значительное изменение
	bool HasSignificantChange(const SEAXEnvironmentData& other, float threshold = 0.1f) const
	{
		if (!bDataValid || !other.bDataValid)
			return true;

		return (fabs(fEnvironmentRadius - other.fEnvironmentRadius) > threshold * 5.0f) ||
			   (fabs(fEnvironmentDensity - other.fEnvironmentDensity) > threshold) ||
			   (fabs(fFogDensity - other.fFogDensity) > threshold) || (fabs(fOpenness - other.fOpenness) > threshold);
	}

	// Расчет производных параметров
	void CalculateDerivedParameters()
	{
		// Нормализация радиуса для расчетов
		float normalizedRadius = fEnvironmentRadius / 500.0f; // предполагаем макс радиус 500м
		clamp(normalizedRadius, 0.1f, 1.0f);

		// Основные производные параметры
		fOpenness = 1.0f - fEnvironmentDensity;
		fEnclosedness = fEnvironmentDensity;
		fUniformity = 1.0f - fEnvironmentVariance;

		// Размер помещения зависит от радиуса и плотности
		fRoomSize = normalizedRadius * (0.3f + 0.7f * fOpenness);

		// Коэффициент отражения зависит от равномерности и плотности
		fReflectivity = 0.3f + 0.7f * (fUniformity * fEnclosedness);

		// Корректировка границ
		clamp(fOpenness, 0.0f, 1.0f);
		clamp(fEnclosedness, 0.0f, 1.0f);
		clamp(fUniformity, 0.0f, 1.0f);
		clamp(fRoomSize, 0.1f, 1.0f);
		clamp(fReflectivity, 0.1f, 1.0f);

		bDataValid = true;
	}

	// Сериализация для отладки
	void DumpToLog(const char* prefix = "") const
	{
		Msg("%sEAX Environment Data:", prefix);
		Msg("%s  Radius: %.2f", prefix, fEnvironmentRadius);
		Msg("%s  Volume: %.0f m³", prefix, sPhysicalData.fRoomVolume);
		Msg("%s  EchoStrength: %.2f", prefix, sPhysicalData.fEchoStrength);
		Msg("%s  ReverbTime: %.2f", prefix, sPhysicalData.fReverbTime);
		Msg("%s  Density: %.3f", prefix, fEnvironmentDensity);
		Msg("%s  Fog Density: %.3f", prefix, fFogDensity);
		Msg("%s  Openness: %.3f", prefix, fOpenness);
		Msg("%s  Enclosedness: %.3f", prefix, fEnclosedness);
		Msg("%s  Uniformity: %.3f", prefix, fUniformity);
		Msg("%s  Room Size: %.3f", prefix, fRoomSize);
		Msg("%s  Reflectivity: %.3f", prefix, fReflectivity);
		Msg("%s  Valid: %s", prefix, bDataValid ? "true" : "false");
	}
};