#pragma once

IC float sRgbToLinear(float vValue)
{
	return vValue * (vValue * (vValue * 0.305306011f + 0.682171111f) + 0.012522878f);
}