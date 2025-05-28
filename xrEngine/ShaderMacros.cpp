#include "stdafx.h"

#include "ShaderMacros.h"

CShaderMacros::MacroImpl* CShaderMacros::find(LPCSTR Name)
{
	if (Name == NULL)
		return NULL;

	for (auto& it : macros_impl)
	{
		if (xr_strcmp(Name, it.Name) == 0)
			return &it;
	}

	return NULL;
}

void CShaderMacros::add(BOOL Enabled, string32 Name, string32 Definition)
{
	MacroImpl* pMacro = find(Name);

	if (pMacro)
	{
		pMacro->Definition = Definition;
	}
	else
	{
		MacroImpl macro;
		macro.Name = Name;
		macro.Definition = Definition;
		macro.State = Enabled ? Enable : Disable;

		macros_impl.push_back(macro);
	}
}

void CShaderMacros::add(string32 Name, int value)
{
	string32 Definition;
	sprintf(Definition, "%d", value);
	add(TRUE, Name, Definition);
}

void CShaderMacros::add(string32 Name, string32 Definition)
{
	add(TRUE, Name, Definition);
}

void CShaderMacros::add(CShaderMacros& Macros)
{
	MacroImpl* pMacro = NULL;

	for (auto& it : Macros.macros_impl)
	{
		pMacro = find(it.Name);

		if (pMacro)
		{
			pMacro->Definition = it.Definition;
			pMacro->State = it.State;
		}
		else
		{
			macros_impl.push_back(it);
		}
	}
}

void CShaderMacros::undef(string32 Name)
{
	MacroImpl* pMacro = find(Name);
	
	if (pMacro)
	{
		pMacro->State = Undef;
	}
}

void CShaderMacros::clear()
{
	macros_impl.clear();
}

xr_vector<CShaderMacros::MacroImpl>& CShaderMacros::get_impl()
{
	return macros_impl;
}

std::string& CShaderMacros::get_name()
{
	name.clear();

	for (auto& it : macros_impl)
	{
		if (it.Definition)
		{
			if (it.State == Enable)
			{
				name += it.Definition;
			}
			else
			{
				for (u32 j = 0; j < strnlen_s(it.Definition, 128); ++j)
					name += it.State;
			}
		}
	}

	return name;
}

xr_vector<D3DXMACRO>& CShaderMacros::get_macros()
{
	macros.clear();

	for (auto& it : macros_impl)
		if (it.State == Enable)
			macros.push_back({it.Name, it.Definition});

	return macros;
}