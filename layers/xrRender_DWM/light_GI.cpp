#include "StdAfx.h"
#include "light.h"

IC bool		pred_LI(const light_indirect& A, const light_indirect& B)
{
	return A.E > B.E;
}

void	light::gi_generate()
{
	return;
}
