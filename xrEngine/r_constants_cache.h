#ifndef r_constants_cacheH
#define r_constants_cacheH
#pragma once

#include "r_constants.h"

// Убираем зависимость от Device, используем упрощенную систему
// #include "device.h" // ЗАКОММЕНТИРУЕМ эту строку

template <class T, u32 limit> class R_constant_cache
{
  private:
	ALIGN(16) svector<T, limit> array;
	u32 lo, hi;

  public:
	R_constant_cache()
	{
		array.resize(limit);
		flush();
	}

	ICF T* access(u32 id)
	{
		return &array[id];
	}
	ICF void flush()
	{
		lo = hi = 0;
	}
	ICF void dirty(u32 _lo, u32 _hi)
	{
		if (_lo < lo)
			lo = _lo;
		if (_hi > hi)
			hi = _hi;
	}
	ICF u32 r_lo()
	{
		return lo;
	}
	ICF u32 r_hi()
	{
		return hi;
	}
};

class ENGINE_API R_constant_array
{
  public:
	typedef R_constant_cache<Fvector4, 256> t_f;

  public:
	ALIGN(16) t_f c_f;
	BOOL b_dirty;

  private:
	// УПРОЩАЕМ: убираем сложную dirty-систему с зависимостью от Device
	// Просто используем счетчик вместо фреймов
	u32 m_dirtyCounter;

  public:
	// Конструктор для инициализации
	R_constant_array() : b_dirty(FALSE), m_dirtyCounter(0)
	{
	}

	// УПРОЩЕННЫЕ Dirty-методы
	ICF void mark_dirty()
	{
		m_dirtyCounter++;
		b_dirty = TRUE;
	}

	ICF bool needs_flush() const
	{
		return b_dirty;
	}

	ICF void mark_flushed()
	{
		b_dirty = FALSE;
	}

	ICF void reset_dirty()
	{
		m_dirtyCounter = 0;
		b_dirty = FALSE;
	}

	ICF void force_dirty()
	{
		mark_dirty();
	}

	ICF u32 get_dirty_count() const
	{
		return m_dirtyCounter;
	}

	t_f& get_array_f()
	{
		return c_f;
	}

	// Существующие методы с добавлением mark_dirty()
	void set(R_constant* C, R_constant_load& L, const Fmatrix& A)
	{
		VERIFY(RC_float == C->type);
		Fvector4* it = c_f.access(L.index);
		switch (L.cls)
		{
		case RC_2x4:
			c_f.dirty(L.index, L.index + 2);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			break;
		case RC_3x4:
			c_f.dirty(L.index, L.index + 3);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			break;
		case RC_4x4:
			c_f.dirty(L.index, L.index + 4);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			break;
		default:
#ifdef DEBUG
			Debug.fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
		mark_dirty();
	}

	void set(R_constant* C, R_constant_load& L, const Fvector4& A)
	{
		VERIFY(RC_float == C->type);
		VERIFY(RC_1x4 == L.cls);
		c_f.access(L.index)->set(A);
		c_f.dirty(L.index, L.index + 1);
		mark_dirty();
	}

	void seta(R_constant* C, R_constant_load& L, u32 e, const Fmatrix& A)
	{
		VERIFY(RC_float == C->type);
		u32 base;
		Fvector4* it;
		switch (L.cls)
		{
		case RC_2x4:
			base = L.index + 2 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 2);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			break;
		case RC_3x4:
			base = L.index + 3 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 3);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			break;
		case RC_4x4:
			base = L.index + 4 * e;
			it = c_f.access(base);
			c_f.dirty(base, base + 4);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			break;
		default:
#ifdef DEBUG
			Debug.fatal(DEBUG_INFO, "Invalid constant run-time-type for '%s'", *C->name);
#else
			NODEFAULT;
#endif
		}
		mark_dirty();
	}

	void seta(R_constant* C, R_constant_load& L, u32 e, const Fvector4& A)
	{
		VERIFY(RC_float == C->type);
		VERIFY(RC_1x4 == L.cls);
		u32 base = L.index + e;
		c_f.access(base)->set(A);
		c_f.dirty(base, base + 1);
		mark_dirty();
	}

	// НОВЫЕ МЕТОДЫ для прямой установки по регистрам
	void set_direct(u32 register_index, const Fmatrix& A)
	{
		if (register_index + 3 < 256) // 256 - размер массива констант
		{
			Fvector4* it = c_f.access(register_index);
			it[0].set(A._11, A._21, A._31, A._41);
			it[1].set(A._12, A._22, A._32, A._42);
			it[2].set(A._13, A._23, A._33, A._43);
			it[3].set(A._14, A._24, A._34, A._44);
			c_f.dirty(register_index, register_index + 4);
			mark_dirty();
		}
	}

	void set_direct(u32 register_index, const Fvector4& A)
	{
		if (register_index < 256)
		{
			Fvector4* it = c_f.access(register_index);
			it->set(A.x, A.y, A.z, A.w);
			c_f.dirty(register_index, register_index + 1);
			mark_dirty();
		}
	}

	void seta_direct(u32 register_index, u32 count, const Fmatrix* A)
	{
		for (u32 i = 0; i < count; i++)
		{
			if (register_index + i * 4 + 3 < 256)
			{
				Fvector4* it = c_f.access(register_index + i * 4);
				const Fmatrix& M = A[i];
				it[0].set(M._11, M._21, M._31, M._41);
				it[1].set(M._12, M._22, M._32, M._42);
				it[2].set(M._13, M._23, M._33, M._43);
				it[3].set(M._14, M._24, M._34, M._44);
				c_f.dirty(register_index + i * 4, register_index + i * 4 + 4);
			}
		}
		mark_dirty();
	}

	void seta_direct(u32 register_index, u32 count, const Fvector4* A)
	{
		for (u32 i = 0; i < count; i++)
		{
			if (register_index + i < 256)
			{
				Fvector4* it = c_f.access(register_index + i);
				it->set(A[i].x, A[i].y, A[i].z, A[i].w);
				c_f.dirty(register_index + i, register_index + i + 1);
			}
		}
		mark_dirty();
	}
	u32 getFrame();
};

class ENGINE_API R_constants
{
  public:
	ALIGN(16) R_constant_array a_pixel;
	ALIGN(16) R_constant_array a_vertex;

  public:
	R_constants()
	{
	}

	void flush_cache();

	// УПРОЩАЕМ: убираем дублирование установки b_dirty
	// Методы set и seta уже вызывают mark_dirty() внутри R_constant_array
	ICF void set(R_constant* C, const Fmatrix& A)
	{
		if (C->destination & 1)
		{
			a_pixel.set(C, C->ps, A);
		}
		if (C->destination & 2)
		{
			a_vertex.set(C, C->vs, A);
		}
	}

	ICF void set(R_constant* C, const Fvector4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.set(C, C->ps, A);
		}
		if (C->destination & 2)
		{
			a_vertex.set(C, C->vs, A);
		}
	}

	ICF void set(R_constant* C, float x, float y, float z, float w)
	{
		Fvector4 data;
		data.set(x, y, z, w);
		set(C, data);
	}

	ICF void seta(R_constant* C, u32 e, const Fmatrix& A)
	{
		if (C->destination & 1)
		{
			a_pixel.seta(C, C->ps, e, A);
		}
		if (C->destination & 2)
		{
			a_vertex.seta(C, C->vs, e, A);
		}
	}

	ICF void seta(R_constant* C, u32 e, const Fvector4& A)
	{
		if (C->destination & 1)
		{
			a_pixel.seta(C, C->ps, e, A);
		}
		if (C->destination & 2)
		{
			a_vertex.seta(C, C->vs, e, A);
		}
	}

	ICF void seta(R_constant* C, u32 e, float x, float y, float z, float w)
	{
		Fvector4 data;
		data.set(x, y, z, w);
		seta(C, e, data);
	}

	// Методы для прямой установки по регистрам
	void set_vs_direct(u32 Register, const Fmatrix& A)
	{
		a_vertex.set_direct(Register, A);
	}

	void set_vs_direct(u32 Register, const Fvector4& A)
	{
		a_vertex.set_direct(Register, A);
	}

	void seta_vs_direct(u32 Register, u32 count, const Fmatrix* A)
	{
		a_vertex.seta_direct(Register, count, A);
	}

	void seta_vs_direct(u32 Register, u32 count, const Fvector4* A)
	{
		a_vertex.seta_direct(Register, count, A);
	}

	void set_ps_direct(u32 Register, const Fmatrix& A)
	{
		a_pixel.set_direct(Register, A);
	}

	void set_ps_direct(u32 Register, const Fvector4& A)
	{
		a_pixel.set_direct(Register, A);
	}

	void seta_ps_direct(u32 Register, u32 count, const Fmatrix* A)
	{
		a_pixel.seta_direct(Register, count, A);
	}

	void seta_ps_direct(u32 Register, u32 count, const Fvector4* A)
	{
		a_pixel.seta_direct(Register, count, A);
	}

	ICF void flush()
	{
		if (a_pixel.b_dirty || a_vertex.b_dirty)
			flush_cache();
	}

	ICF void force_dirty()
	{
		a_pixel.force_dirty();
		a_vertex.force_dirty();
	}

	ICF void reset_dirty()
	{
		a_pixel.reset_dirty();
		a_vertex.reset_dirty();
	}
};
#endif
