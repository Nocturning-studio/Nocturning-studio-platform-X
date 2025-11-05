#pragma once

#define TAA_MAX_OBJ 8

struct taa_matrices
{
	//Fmatrix m_V;
	//Fmatrix	m_P;
	Fmatrix	m_VP;
	//Fmatrix	m_W;
	//Fmatrix	m_invW;
	//Fmatrix	m_WV;
	//Fmatrix	m_WVP;
};

enum taa_jitter_modes {taa_1x, taa_2x, taa_4x, taa_8x, taa_16x};

class CTAA
{
private:
	taa_matrices tM, M;

public:
	CTAA						();

	// Set xforms into backend
	void			set_xforms	(const Fmatrix& World, const Fmatrix& View, const Fmatrix& Project);

	// Set xforms into backend
	void			set_xforms	(const taa_matrices& iM);

	// Get matrices from backend
	taa_matrices	get_xforms	();

	// Save last matrices from backend
	void			fix_xforms	();

	static float	calc_jitter_x();
	static float	calc_jitter_y();

private:
	static float	calc_jitter(const int sequence[], int count, int resolution);
};

extern CTAA TAA;