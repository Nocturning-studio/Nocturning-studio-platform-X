#pragma once
#include "d3dx9.h"

class ENGINE_API CShaderMacros
{
public:
	enum State
	{ 
		Enable	= 'e', // Define: +, Name: definition
		Disable = 'd', // Define: -, Name: d
		Undef	= 'u', // Define: -, Name: u
	};

	struct MacroImpl 
	{
		LPCSTR Name;
		LPCSTR Definition;
		State State; 
	};

private:
	xr_vector<MacroImpl> macros_impl; // advanced macros

	std::string name; // cache
	xr_vector<D3DXMACRO> macros; // cache

	MacroImpl* find(LPCSTR Name);

  public:
	void add(BOOL Enabled, string32 Name, string32 Definition);
	void add(string32 Name, int value);
	void add(string32 Name, string32 Definition);
	void add(CShaderMacros& Macros);
	void undef(string32 Name);
	void clear();

	xr_vector<MacroImpl>& get_impl();
	std::string& get_name();
	xr_vector<D3DXMACRO>& get_macros();
};
