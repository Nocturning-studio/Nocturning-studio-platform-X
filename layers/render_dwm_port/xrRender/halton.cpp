#include "stdafx.h"
#include "halton.h"

namespace Halton
{
	u8 Halton2(int index, int numFrames)
	{
		static const u8 sequence[] = {
			8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15, 0
		};

		return sequence[index % numFrames];
	}

	u8 Halton3(int index, int numFrames)
	{
		// This is a halton 3 sequence hand-modified to remove duplicates
		static const u8 sequence[] = {
			5, 10, 1, 7, 12, 3, 9, 14, 0, 6, 11, 2, 8, 13, 4, 15
		};

		return sequence[index % numFrames];
	}

	u8 Gen(int index, int component, int numFrames)
	{
		switch (component)
		{
		case 0: return Halton2(index, numFrames);
		case 1: return Halton3(index, numFrames);
		default: return 0;
		}
	}
}