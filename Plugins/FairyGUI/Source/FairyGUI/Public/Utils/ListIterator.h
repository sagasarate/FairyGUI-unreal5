#pragma once
#include "CoreMinimal.h"

enum EASY_DATA_STORAGE_MODE
{
	EDSM_STATIC,
	EDSM_NEW_ONCE,
	EDSM_NEW_EVERY_TIME,
};

template <class CONTAINER_TYPE, class VALUE_TYPE> class const_list_iterator
{
protected:
	const CONTAINER_TYPE* m_pContainer;
	const void*				  m_Pos;

public:
	const_list_iterator()
	{
		m_pContainer = NULL;
		m_Pos = NULL;
	}
	const_list_iterator(const CONTAINER_TYPE* pContainer, const void* Pos)
	{
		m_pContainer = pContainer;
		m_Pos = Pos;
	}
	const_list_iterator(const const_list_iterator& itr)
	{
		m_pContainer = itr.m_pContainer;
		m_Pos = itr.m_Pos;
	}
	const VALUE_TYPE& operator*()
	{
		static VALUE_TYPE EmptyObject;
		const VALUE_TYPE* pObject = m_pContainer->GetObject(m_Pos);
		if (pObject)
			return *pObject;
		else
			return EmptyObject;
	}
	const_list_iterator& operator++()
	{
		m_Pos = m_pContainer->GetNextObjectPos(m_Pos);
		return *this;
	}
	const_list_iterator& operator=(const const_list_iterator& itr)
	{
		m_pContainer = itr.m_pContainer;
		m_Pos = itr.m_Pos;
		return *this;
	}
	bool operator!=(const const_list_iterator& itr) { return m_Pos != itr.m_Pos; }
};

template <class CONTAINER_TYPE, class VALUE_TYPE> class list_iterator
{
protected:
	CONTAINER_TYPE* m_pContainer;
	const void*		m_Pos;

public:
	list_iterator()
	{
		m_pContainer = NULL;
		m_Pos = NULL;
	}
	list_iterator(CONTAINER_TYPE* pContainer, const void* Pos)
	{
		m_pContainer = pContainer;
		m_Pos = Pos;
	}
	list_iterator(const list_iterator& itr)
	{
		m_pContainer = itr.m_pContainer;
		m_Pos = itr.m_Pos;
	}
	VALUE_TYPE& operator*()
	{
		static VALUE_TYPE EmptyObject;
		VALUE_TYPE*		  pObject = m_pContainer->GetObject(m_Pos);
		if (pObject)
			return *pObject;
		else
			return EmptyObject;
	}
	list_iterator& operator++()
	{
		m_Pos = m_pContainer->GetNextObjectPos(m_Pos);
		return *this;
	}
	list_iterator& operator=(const list_iterator& itr)
	{
		m_pContainer = itr.m_pContainer;
		m_Pos = itr.m_Pos;
		return *this;
	}
	bool operator!=(const list_iterator& itr) { return m_Pos != itr.m_Pos; }
};
