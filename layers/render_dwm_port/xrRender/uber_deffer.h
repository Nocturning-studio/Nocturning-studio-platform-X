#pragma once

void uber			(CBlender_Compile& C, bool hq,	LPCSTR _vspec, LPCSTR _pspec, BOOL _aref, LPCSTR _detail_replace = 0, bool DO_NOT_FINISH = false);
void uber_shadow	(CBlender_Compile& C,			LPCSTR _vspec);
void uber_planar	(CBlender_Compile& C,			LPCSTR _vspec, BOOL _aref = FALSE);
void uber_rsm		(CBlender_Compile& C,			LPCSTR _vspec, BOOL _aref = FALSE);
void uber_zprepass	(CBlender_Compile& C,			LPCSTR _vspec, BOOL _aref = FALSE, BOOL _tess = FALSE);
