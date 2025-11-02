#pragma once

IC float sRgbToLinear(float vValue)
{
	return vValue * (vValue * (vValue * 0.305306011f + 0.682171111f) + 0.012522878f);
}

IC Fvector3 sRgbToLinear(Fvector3 vValue)
{
	vValue.x = sRgbToLinear(vValue.x);
	vValue.y = sRgbToLinear(vValue.y);
	vValue.z = sRgbToLinear(vValue.z);

	return vValue;
}

IC Fvector3 sRgbToLinear(float R, float G, float B)
{
	Fvector3 vValue = {0, 0, 0};

	vValue.x = sRgbToLinear(R);
	vValue.y = sRgbToLinear(G);
	vValue.z = sRgbToLinear(B);

	return vValue;
}