////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: Deathman
// Refactored code: Build identification
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;
////////////////////////////////////////////////////////////////////////////////
static LPSTR month_id[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int start_day = 24;
static int start_month = 8;
static int start_year = 2022;
////////////////////////////////////////////////////////////////////////////////
void compute_build_id()
{
	build_date = __DATE__;

	int days;
	int months = 0;
	int years;
	string16 month;
	string256 buffer;
	strcpy_s(buffer, __DATE__);
	(void)sscanf(buffer, "%s %d %d", month, &days, &years);

	for (int i = 0; i < 12; i++)
	{
		if (_stricmp(month_id[i], month))
			continue;

		months = i;
		break;
	}

	build_id = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		build_id += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		build_id -= days_in_month[i];
}
////////////////////////////////////////////////////////////////////////////////

