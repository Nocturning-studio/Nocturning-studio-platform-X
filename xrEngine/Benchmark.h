////////////////////////////////////////////////////////////////////////////////
// Created: 14.01.2025
// Author: Deathman
// Refactored code: Benchmark realization
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
////////////////////////////////////////////////////////////////////////////////
string512 g_sBenchmarkName;
////////////////////////////////////////////////////////////////////////////////
void doBenchmark(LPCSTR name)
{
	/*
	g_bBenchmark = true;
	string_path in_file;
	FS.update_path(in_file, "$app_data_root$", name);
	CInifile ini(in_file);
	int test_count = ini.line_count("benchmark");
	LPCSTR test_name, t;
	shared_str test_command;
	for (int i = 0; i < test_count; ++i)
	{
		ini.r_line("benchmark", i, &test_name, &t);
		strcpy_s(g_sBenchmarkName, test_name);

		test_command = ini.r_string_wb("benchmark", test_name);
		strcpy_s(Core.Params, *test_command);
		_strlwr_s(Core.Params);

		InitInput();
		if (i)
		{
			ZeroMemory(&HW, sizeof(CHW));
			InitEngine();
		}

		Engine.External.Initialize();

#pragma todo("Deathman to Deathman: Починить бенчмарк")
#pragma todo("Deathman to Deathman: Отдельное сообщение DiscordAPI для бенчмарка")

		Startup();
	}
	*/
}
////////////////////////////////////////////////////////////////////////////////
