#pragma once
#include "CoreMinimal.h"
#include "ListIterator.h"

template <class KEY, class T, int32 StorageMode = EDSM_STATIC> class CStaticMap
{
protected:
	enum enNodeColor
	{
		NC_NONE,
		NC_RED,
		NC_BLACK,
	};
	template <int32 Mode, class KEY_TYPE, class OBJECT_TYPE> struct _StorageNode
	{
		uint32			   ID;
		_StorageNode*	   pParent;
		_StorageNode*	   pRightChild;
		_StorageNode*	   pLeftChild;
		_StorageNode*	   pPrev;
		_StorageNode*	   pNext;
		enNodeColor		   Color;
		bool			   IsUsed;
		bool			   IsFree;
		KEY_TYPE		   Key;
		void			   InitObject() {}
		OBJECT_TYPE&	   GetObjectRef() { return Object; }
		OBJECT_TYPE*	   GetObjectPointer() { return &Object; }
		const OBJECT_TYPE& GetObjectRef() const { return Object; }
		const OBJECT_TYPE* GetObjectPointer() const { return &Object; }
		void			   NewObject() {}
		void			   DeleteObject() {}
		void			   FinalReleaseObject() {}

	protected:
		OBJECT_TYPE Object;
	};
	template <class KEY_TYPE, class OBJECT_TYPE> struct _StorageNode<EDSM_NEW_ONCE, KEY_TYPE, OBJECT_TYPE>
	{
		uint32			   ID;
		_StorageNode*	   pParent;
		_StorageNode*	   pRightChild;
		_StorageNode*	   pLeftChild;
		_StorageNode*	   pPrev;
		_StorageNode*	   pNext;
		enNodeColor		   Color;
		bool			   IsUsed;
		bool			   IsFree;
		KEY_TYPE		   Key;
		void			   InitObject() { pObject = nullptr; }
		OBJECT_TYPE&	   GetObjectRef() { return *pObject; }
		OBJECT_TYPE*	   GetObjectPointer() { return pObject; }
		const OBJECT_TYPE& GetObjectRef() const { return *pObject; }
		const OBJECT_TYPE* GetObjectPointer() const { return pObject; }
		void			   NewObject()
		{
			if (pObject == nullptr)
				pObject = new OBJECT_TYPE;
		}
		void DeleteObject() {}
		void FinalReleaseObject() { SAFE_DELETE(pObject); }

	protected:
		OBJECT_TYPE* pObject;
	};
	template <class KEY_TYPE, class OBJECT_TYPE> struct _StorageNode<EDSM_NEW_EVERY_TIME, KEY_TYPE, OBJECT_TYPE>
	{
		uint32			   ID;
		_StorageNode*	   pParent;
		_StorageNode*	   pRightChild;
		_StorageNode*	   pLeftChild;
		_StorageNode*	   pPrev;
		_StorageNode*	   pNext;
		enNodeColor		   Color;
		bool			   IsUsed;
		bool			   IsFree;
		KEY_TYPE		   Key;
		void			   InitObject() { pObject = nullptr; }
		OBJECT_TYPE&	   GetObjectRef() { return *pObject; }
		OBJECT_TYPE*	   GetObjectPointer() { return pObject; }
		const OBJECT_TYPE& GetObjectRef() const { return *pObject; }
		const OBJECT_TYPE* GetObjectPointer() const { return pObject; }
		void			   NewObject()
		{
			if (pObject == nullptr)
				pObject = new OBJECT_TYPE;
		}
		void DeleteObject()
		{
			if (pObject)
			{
				delete pObject;
				pObject = nullptr;
			}
		}
		void FinalReleaseObject()
		{
			if (pObject)
			{
				delete pObject;
				pObject = nullptr;
			}
		}

	protected:
		OBJECT_TYPE* pObject;
	};

	typedef _StorageNode<StorageMode, KEY, T> StorageNode;

	struct OBJECT_BUFF_PAGE_INFO
	{
		StorageNode* pObjectBuffer;
		uint32		 BufferSize;
	};

	TArray<OBJECT_BUFF_PAGE_INFO> m_ObjectBuffPages;
	StorageNode*				  m_pFreeListHead;
	StorageNode*				  m_pFreeListTail;
	StorageNode*				  m_pObjectListHead;
	StorageNode*				  m_pObjectListTail;
	StorageNode*				  m_pTreeRoot;
	uint32						  m_ObjectCount;
	uint32						  m_UsedObjectCount;
	uint32						  m_GrowSize;
	uint32						  m_GrowLimit;

	// int32				m_BlackCount;

public:
	typedef list_iterator<CStaticMap, T>	   iterator;
	typedef const_list_iterator<CStaticMap, T> const_iterator;

public:
	CStaticMap()
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_pTreeRoot = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
	}
	CStaticMap(uint32 Size, uint32 GrowSize = 0, uint32 GrowLimit = 0)
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_pTreeRoot = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
		Create(Size, GrowSize, GrowLimit);
	}
	~CStaticMap() { Destroy(); }
	bool Create(uint32 Size, uint32 GrowSize = 0, uint32 GrowLimit = 0)
	{
		Destroy();
		if (Size)
		{
			m_GrowSize = GrowSize;
			m_GrowLimit = GrowLimit;
			return CreateBufferPage(Size);
		}
		return true;
	}
	void Destroy()
	{
		for (int32 i = 0; i < m_ObjectBuffPages.Num(); i++)
		{
			for (uint32 j = 0; j < m_ObjectBuffPages[i].BufferSize; j++)
			{
				m_ObjectBuffPages[i].pObjectBuffer[j].FinalReleaseObject();
			}
			if (m_ObjectBuffPages[i].pObjectBuffer)
			{
				delete[] m_ObjectBuffPages[i].pObjectBuffer;
				m_ObjectBuffPages[i].pObjectBuffer = nullptr;
			}
		}
		m_ObjectBuffPages.Empty();
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_pTreeRoot = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
	}
	void Clear()
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_pTreeRoot = nullptr;
		m_ObjectCount = 0;

		uint32 IDStart = 1;
		for (int32 i = 0; i < m_ObjectBuffPages.Num(); i++)
		{
			ClearBuffer(m_ObjectBuffPages[i], IDStart, false);
			IDStart += m_ObjectBuffPages[i].BufferSize;
		}
	}
	void ClearToDefault()
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_pTreeRoot = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;

		uint32 IDStart = 1;
		if (m_ObjectBuffPages.Num() > 1)
		{
			for (int32 i = 1; i < m_ObjectBuffPages.Num(); i++)
			{
				for (uint32 j = 0; j < m_ObjectBuffPages[i].BufferSize; j++)
				{
					m_ObjectBuffPages[i].pObjectBuffer[j].FinalReleaseObject();
				}
				SAFE_DELETE_ARRAY(m_ObjectBuffPages[i].pObjectBuffer);
			}
			m_ObjectBuffPages.Resize(1);
		}
		for (int32 i = 0; i < m_ObjectBuffPages.Num(); i++)
		{
			ClearBuffer(m_ObjectBuffPages[i], IDStart, false);
			IDStart += m_ObjectBuffPages[i].BufferSize;
		}
	}
	bool Grow()
	{
		if (m_GrowSize)
		{
			CreateBufferPage(m_GrowSize);
			return true;
		}
		return false;
	}
	uint32 GetBufferSize() const
	{
		uint32 BufferSize = 0;
		for (int32 i = 0; i < m_ObjectBuffPages.Num(); i++)
		{
			BufferSize += m_ObjectBuffPages[i].BufferSize;
		}
		return BufferSize;
	}
	uint32 New(const KEY& Key, T** ppValue)
	{
		StorageNode* pNode = InsertNode(m_pTreeRoot, Key);
		if (pNode)
		{
			*ppValue = pNode->GetObjectPointer();
			if (pNode->Color == NC_NONE)
			{
				pNode->Color = NC_RED;
				BalanceInsert(pNode);
			}
			return pNode->ID;
		}
		return 0;
	}
	uint32 Insert(const KEY& Key, const T& Value)
	{
		StorageNode* pNode = InsertNode(m_pTreeRoot, Key);
		if (pNode)
		{
			pNode->GetObjectRef() = Value;
			if (pNode->Color == NC_NONE)
			{
				pNode->Color = NC_RED;
				BalanceInsert(pNode);
			}
			return pNode->ID;
		}
		return 0;
	}
	bool DeleteByID(uint32 ID)
	{
		StorageNode* pNode = (StorageNode*)GetObjectPosByID(ID);
		if (pNode)
		{
			StorageNode* pNeedBalanceNode = DeleteNode(pNode, true);
			if (pNeedBalanceNode)
			{
				BalanceDelete(pNeedBalanceNode);
			}
			return true;
		}
		return false;
	}
	bool DeleteByPos(void* Pos)
	{
		StorageNode* pNode = (StorageNode*)Pos;
		if (pNode)
		{
			StorageNode* pNeedBalanceNode = DeleteNode(pNode, true);
			if (pNeedBalanceNode)
			{
				BalanceDelete(pNeedBalanceNode);
			}
			return true;
		}
		return false;
	}
	bool Delete(const KEY& Key)
	{
		StorageNode* pNode = FindNode(m_pTreeRoot, Key);
		if (pNode)
		{
			StorageNode* pNeedBalanceNode = DeleteNode(pNode, true);
			if (pNeedBalanceNode)
			{
				BalanceDelete(pNeedBalanceNode);
			}
			return true;
		}
		return false;
	}
	bool ReMap(const KEY& OldKey, const KEY& NewKey)
	{
		StorageNode* pNode = FindNode(m_pTreeRoot, OldKey);
		if (pNode)
		{
			if (FindNode(m_pTreeRoot, NewKey) == nullptr)
			{
				StorageNode* pNeedBalanceNode = DeleteNode(pNode, false);
				if (pNeedBalanceNode)
				{
					BalanceDelete(pNeedBalanceNode);
				}
				pNode->Key = NewKey;
				pNeedBalanceNode = InsertNode(m_pTreeRoot, pNode);
				if (pNeedBalanceNode)
				{
					if (pNeedBalanceNode->Color == NC_NONE)
					{
						pNeedBalanceNode->Color = NC_RED;
						BalanceInsert(pNeedBalanceNode);
					}
					return true;
				}
			}
		}
		return false;
	}
	const void* FindPos(const KEY& Key) const { return FindNode(m_pTreeRoot, Key); }
	T*			Find(const KEY& Key)
	{
		StorageNode* pNode = FindNode(m_pTreeRoot, Key);
		if (pNode)
		{
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	const T* Find(const KEY& Key) const
	{
		const StorageNode* pNode = FindNode(m_pTreeRoot, Key);
		if (pNode)
		{
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	const void* FindNearPos(const KEY& Key) const { return (const void*)FindNodeNear(m_pTreeRoot, Key); }
	T*			FindNear(const KEY& Key)
	{
		StorageNode* pNode = FindNodeNear(m_pTreeRoot, Key);
		if (pNode)
		{
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	const T* FindNear(const KEY& Key) const
	{
		const StorageNode* pNode = FindNodeNear(m_pTreeRoot, Key);
		if (pNode)
		{
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	T* GetObject(const void* Pos)
	{
		StorageNode* pNode = (StorageNode*)Pos;
		if (Pos)
			return pNode->GetObjectPointer();
		return nullptr;
	}
	const T* GetObject(const void* Pos) const
	{
		const StorageNode* pNode = (const StorageNode*)Pos;
		if (pNode)
			return pNode->GetObjectPointer();
		else
			return nullptr;
	}
	T*		 GetObject(uint32 ID) { return GetObject(GetObjectPosByID(ID)); }
	const T* GetObject(uint32 ID) const { return GetObject(GetObjectPosByID(ID)); }

	uint32 GetObjectCount() const { return m_ObjectCount; }
	uint32 GetUsedObjectCount() const { return m_UsedObjectCount; }

	const void* GetFirstObjectPos() const { return m_pObjectListHead; }

	const void* GetLastObjectPos() const { return m_pObjectListTail; }
	const void* GetNextObjectPos(const void* Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			return pNode->pNext;
		}
		return nullptr;
	}
	const void* GetPrevObjectPos(const void* Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			return pNode->pPrev;
		}
		return nullptr;
	}
	const void* GetObjectPosByID(uint32 ID) const
	{
		if (ID == 0)
			return nullptr;
		if (m_ObjectBuffPages.Num())
		{
			ID--;
			const OBJECT_BUFF_PAGE_INFO& FirstPage = m_ObjectBuffPages[0];
			if (ID < FirstPage.BufferSize)
			{
				if (!FirstPage.pObjectBuffer[ID].IsFree)
					return &(FirstPage.pObjectBuffer[ID]);
			}
			else
			{
				ID -= FirstPage.BufferSize;
				uint32 PageIndex = 1 + ID / m_GrowSize;
				uint32 Index = ID % m_GrowSize;
				if (PageIndex < (uint32)m_ObjectBuffPages.Num())
				{
					const OBJECT_BUFF_PAGE_INFO& Page = m_ObjectBuffPages[PageIndex];
					if (Index < Page.BufferSize)
					{
						if (!Page.pObjectBuffer[Index].IsFree)
							return &(Page.pObjectBuffer[Index]);
					}
				}
			}
		}
		return nullptr;
	}
	uint32 GetObjectIDByPos(const void* Pos) const
	{
		const StorageNode* pNode = (const StorageNode*)Pos;
		if (pNode)
			return pNode->ID;
		else
			return 0;
	}
	T* GetNextObject(const void*& Pos, KEY& Key)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pNext;
				Key = pNode->Key;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	T* GetPrevObject(const void*& Pos, KEY& Key)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pPrev;
				Key = pNode->Key;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	const T* GetNextObject(const void*& Pos, KEY& Key) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pNext;
				Key = pNode->Key;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const T* GetPrevObject(const void*& Pos, KEY& Key) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pPrev;
				Key = pNode->Key;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	T* GetNextObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pNext;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	T* GetPrevObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pPrev;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	const T* GetNextObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pNext;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const T* GetPrevObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Pos = pNode->pPrev;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const void* GetSortedFirstObjectPos() const
	{
		if (m_pTreeRoot)
		{
			const StorageNode* pNode = m_pTreeRoot;
			while (pNode->pLeftChild)
				pNode = pNode->pLeftChild;
			return (const void*)pNode;
		}
		return nullptr;
	}

	const void* GetSortedLastObjectPos() const
	{
		if (m_pTreeRoot)
		{
			const StorageNode* pNode = m_pTreeRoot;
			while (pNode->pRightChild)
				pNode = pNode->pRightChild;
			return (const void*)pNode;
		}
		return nullptr;
	}
	T* GetSortedNextObject(const void*& Pos, KEY& Key)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Key = pNode->Key;
				T* pRet = pNode->GetObjectPointer();
				if (pNode->pRightChild)
				{
					pNode = pNode->pRightChild;
					while (pNode->pLeftChild)
						pNode = pNode->pLeftChild;
					Pos = pNode;
				}
				else
				{
					StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pRightChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	T* GetSortedPrevObject(const void*& Pos, KEY& Key)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Key = pNode->Key;
				T* pRet = pNode->GetObjectPointer();
				if (pNode->pLeftChild)
				{
					pNode = pNode->pLeftChild;
					while (pNode->pRightChild)
						pNode = pNode->pRightChild;
					Pos = pNode;
				}
				else
				{
					StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pLeftChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	const T* GetSortedNextObject(const void*& Pos, KEY& Key) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Key = pNode->Key;
				const T* pRet = pNode->GetObjectPointer();
				if (pNode->pRightChild)
				{
					pNode = pNode->pRightChild;
					while (pNode->pLeftChild)
						pNode = pNode->pLeftChild;
					Pos = (const void*)pNode;
				}
				else
				{
					const StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pRightChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = (const void*)pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const T* GetSortedPrevObject(const void*& Pos, KEY& Key) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				Key = pNode->Key;
				const T* pRet = pNode->GetObjectPointer();
				if (pNode->pLeftChild)
				{
					pNode = pNode->pLeftChild;
					while (pNode->pRightChild)
						pNode = pNode->pRightChild;
					Pos = (const void*)pNode;
				}
				else
				{
					const StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pLeftChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = (const void*)pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	T* GetSortedNextObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				T* pRet = pNode->GetObjectPointer();
				if (pNode->pRightChild)
				{
					pNode = pNode->pRightChild;
					while (pNode->pLeftChild)
						pNode = pNode->pLeftChild;
					Pos = pNode;
				}
				else
				{
					StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pRightChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	T* GetSortedPrevObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				T* pRet = pNode->GetObjectPointer();
				if (pNode->pLeftChild)
				{
					pNode = pNode->pLeftChild;
					while (pNode->pRightChild)
						pNode = pNode->pRightChild;
					Pos = pNode;
				}
				else
				{
					StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pLeftChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	const T* GetSortedNextObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				const T* pRet = pNode->GetObjectPointer();
				if (pNode->pRightChild)
				{
					pNode = pNode->pRightChild;
					while (pNode->pLeftChild)
						pNode = pNode->pLeftChild;
					Pos = (const void*)pNode;
				}
				else
				{
					const StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pRightChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = (const void*)pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const T* GetSortedPrevObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (!pNode->IsFree)
			{
				const T* pRet = pNode->GetObjectPointer();
				if (pNode->pLeftChild)
				{
					pNode = pNode->pLeftChild;
					while (pNode->pRightChild)
						pNode = pNode->pRightChild;
					Pos = (const void*)pNode;
				}
				else
				{
					const StorageNode* pParent = pNode->pParent;
					while (pParent && pNode == pParent->pLeftChild)
					{
						pNode = pParent;
						pParent = pParent->pParent;
					}
					Pos = (const void*)pParent;
				}
				return pRet;
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	bool MoveToBefore(const void* Pos, const void* Target)
	{
		if (Pos && Pos != Target && m_ObjectCount > 1)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			StorageNode* pBefore = (StorageNode*)Target;
			PickNode(pNode);
			InsertNodeBefore(pNode, pBefore);
			return true;
		}
		return false;
	}
	bool MoveToAfter(const void* Pos, const void* Target)
	{
		if (Pos && Pos != Target && m_ObjectCount > 1)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			StorageNode* pAfter = (StorageNode*)Target;
			PickNode(pNode);
			InsertNodeAfter(pNode, pAfter);
			return true;
		}
		return false;
	}
	bool MoveSorted(const void* Pos)
	{
		// 从小到大排列
		StorageNode* pNode = (StorageNode*)Pos;
		if (pNode)
		{
			PickNode(pNode);
			StorageNode* pHead = m_pObjectListHead;
			while (pHead && pHead->GetObjectRef() < pNode->GetObjectRef())
			{
				pHead = pHead->pNext;
			}
			if (pHead)
			{
				InsertNodeBefore(pNode, pHead);
			}
			else
			{
				InsertNodeAfter(pNode, pHead);
			}
			return true;
		}
		return false;
	}
	iterator begin() { return iterator(this, GetFirstObjectPos()); }

	iterator	   end() { return iterator(this, nullptr); }
	const_iterator begin() const { return const_iterator(this, GetFirstObjectPos()); }

	const_iterator end() const { return const_iterator(this, nullptr); }
	const void*	   GetFreeObjectPosByID(uint32 ID)
	{
		if (ID == 0)
			return nullptr;
		if (m_ObjectBuffPages.Num())
		{
			ID--;
			const OBJECT_BUFF_PAGE_INFO& FirstPage = m_ObjectBuffPages[0];
			if (ID < FirstPage.BufferSize)
			{
				if (FirstPage.pObjectBuffer[ID].IsFree)
					return &(FirstPage.pObjectBuffer[ID]);
			}
			else
			{
				ID -= FirstPage.BufferSize;
				uint32 PageIndex = 1 + ID / m_GrowSize;
				uint32 Index = ID % m_GrowSize;
				if (PageIndex < (uint32)m_ObjectBuffPages.Num())
				{
					const OBJECT_BUFF_PAGE_INFO& Page = m_ObjectBuffPages[PageIndex];
					if (Index < Page.BufferSize)
					{
						if (Page.pObjectBuffer[Index].IsFree)
							return &(Page.pObjectBuffer[Index]);
					}
				}
			}
		}
		return nullptr;
	}
	T* GetFreeObject(const void* Pos)
	{
		StorageNode* pNode = (StorageNode*)Pos;
		if (pNode)
		{
			if (pNode->IsFree)
				return pNode->GetObjectPointer();
		}

		return nullptr;
	}
	const void* GetFirstFreeObjectPos() const { return m_pFreeListHead; }

	const void* GetLastFreeObjectPos() const { return m_pFreeListTail; }
	T*			GetNextFreeObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (pNode->IsFree)
			{
				Pos = pNode->pNext;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	T* GetPrevFreeObject(const void*& Pos)
	{
		if (Pos)
		{
			StorageNode* pNode = (StorageNode*)Pos;
			if (pNode->IsFree)
			{
				Pos = pNode->pPrev;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	const T* GetNextFreeObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (pNode->IsFree)
			{
				Pos = pNode->pNext;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}

	const T* GetPrevFreeObject(const void*& Pos) const
	{
		if (Pos)
		{
			const StorageNode* pNode = (const StorageNode*)Pos;
			if (pNode->IsFree)
			{
				Pos = pNode->pPrev;
				return pNode->GetObjectPointer();
			}
			else
			{
				Pos = nullptr;
			}
		}
		return nullptr;
	}
	void ReleaseFreeObject(const void* Pos)
	{
		const StorageNode* pNode = (const StorageNode*)Pos;
		if (pNode)
		{
			if (pNode->IsFree)
				return pNode->FinalReleaseObject();
		}
	}

protected:
	StorageNode* FindNode(StorageNode* pRoot, const KEY& Key)
	{
		if (pRoot)
		{
			if (pRoot->Key > Key)
			{
				return FindNode(pRoot->pLeftChild, Key);
			}
			else if (pRoot->Key < Key)
			{
				return FindNode(pRoot->pRightChild, Key);
			}
			else
			{
				return pRoot;
			}
		}
		return nullptr;
	}
	const StorageNode* FindNode(StorageNode* pRoot, const KEY& Key) const
	{
		if (pRoot)
		{
			if (pRoot->Key > Key)
			{
				return FindNode(pRoot->pLeftChild, Key);
			}
			else if (pRoot->Key < Key)
			{
				return FindNode(pRoot->pRightChild, Key);
			}
			else
			{
				return pRoot;
			}
		}
		return nullptr;
	}
	StorageNode* FindNodeNear(StorageNode* pRoot, const KEY& Key)
	{
		if (pRoot)
		{
			if (pRoot->Key > Key)
			{
				if (pRoot->pLeftChild)
					return FindNodeNear(pRoot->pLeftChild, Key);
				else
					return pRoot;
			}
			else if (pRoot->Key < Key)
			{
				if (pRoot->pRightChild)
					return FindNodeNear(pRoot->pRightChild, Key);
				else
					return pRoot;
			}
			else
			{
				return pRoot;
			}
		}
		return nullptr;
	}
	const StorageNode* FindNodeNear(StorageNode* pRoot, const KEY& Key) const
	{
		if (pRoot)
		{
			if (pRoot->Key > Key)
			{
				if (pRoot->pLeftChild)
					return FindNodeNear(pRoot->pLeftChild, Key);
				else
					return pRoot;
			}
			else if (pRoot->Key < Key)
			{
				if (pRoot->pRightChild)
					return FindNodeNear(pRoot->pRightChild, Key);
				else
					return pRoot;
			}
			else
			{
				return pRoot;
			}
		}
		return nullptr;
	}
	StorageNode* InsertNode(StorageNode* pRoot, const KEY& Key)
	{
		if (pRoot)
		{
			if (pRoot->Key > Key)
			{
				if (pRoot->pLeftChild)
				{
					return InsertNode(pRoot->pLeftChild, Key);
				}
				else
				{
					StorageNode* pNewNode = NewNode(Key);
					if (pNewNode)
					{
						pNewNode->pParent = pRoot;
						pRoot->pLeftChild = pNewNode;
					}
					return pNewNode;
				}
			}
			else if (pRoot->Key < Key)
			{
				if (pRoot->pRightChild)
				{
					return InsertNode(pRoot->pRightChild, Key);
				}
				else
				{
					StorageNode* pNewNode = NewNode(Key);
					if (pNewNode)
					{
						pNewNode->pParent = pRoot;
						pRoot->pRightChild = pNewNode;
					}
					return pNewNode;
				}
			}
			else
			{
				return pRoot;
			}
		}
		else
		{
			m_pTreeRoot = NewNode(Key);
			return m_pTreeRoot;
		}
	}
	StorageNode* InsertNode(StorageNode* pRoot, StorageNode* pNewNode)
	{
		if (pRoot)
		{
			if (pRoot->Key > pNewNode->Key)
			{
				if (pRoot->pLeftChild)
				{
					return InsertNode(pRoot->pLeftChild, pNewNode);
				}
				else
				{
					pNewNode->pParent = pRoot;
					pRoot->pLeftChild = pNewNode;

					return pNewNode;
				}
			}
			else if (pRoot->Key < pNewNode->Key)
			{
				if (pRoot->pRightChild)
				{
					return InsertNode(pRoot->pRightChild, pNewNode);
				}
				else
				{

					pNewNode->pParent = pRoot;
					pRoot->pRightChild = pNewNode;

					return pNewNode;
				}
			}
			else
			{
				return pRoot;
			}
		}
		else
		{
			m_pTreeRoot = pNewNode;
			return m_pTreeRoot;
		}
	}
	StorageNode* DeleteNode(StorageNode* pNode, bool DeleteFromList)
	{
		if (pNode->pLeftChild && pNode->pRightChild)
		{
			StorageNode* pBackNode = pNode->pRightChild;
			while (pBackNode->pLeftChild)
				pBackNode = pBackNode->pLeftChild;

			// 交换父节点
			if (pBackNode == pNode->pRightChild)
			{
				if (pNode->pParent)
				{
					if (pNode->pParent->pLeftChild == pNode)
						pNode->pParent->pLeftChild = pBackNode;
					else
						pNode->pParent->pRightChild = pBackNode;
					pBackNode->pParent = pNode->pParent;
				}
				else
				{
					pBackNode->pParent = nullptr;
					m_pTreeRoot = pBackNode;
				}
				pNode->pParent = pBackNode;
			}
			else if (pNode->pParent && pBackNode->pParent)
			{
				if (pNode->pParent->pLeftChild == pNode)
				{
					pNode->pParent->pLeftChild = pBackNode;
					if (pBackNode->pParent->pLeftChild == pBackNode)
						pBackNode->pParent->pLeftChild = pNode;
					else
						pBackNode->pParent->pRightChild = pNode;
				}
				else
				{
					pNode->pParent->pRightChild = pBackNode;
					if (pBackNode->pParent->pLeftChild == pBackNode)
						pBackNode->pParent->pLeftChild = pNode;
					else
						pBackNode->pParent->pRightChild = pNode;
				}
				StorageNode* pTemp = pNode->pParent;
				pNode->pParent = pBackNode->pParent;
				pBackNode->pParent = pTemp;
			}
			else if (pNode->pParent)
			{
				if (pNode->pParent->pLeftChild == pNode)
					pNode->pParent->pLeftChild = pBackNode;
				else
					pNode->pParent->pRightChild = pBackNode;

				pBackNode->pParent = pNode->pParent;
				pNode->pParent = nullptr;
				m_pTreeRoot = pNode;
			}
			else
			{
				if (pBackNode->pParent)
				{
					if (pBackNode->pParent->pLeftChild == pBackNode)
						pBackNode->pParent->pLeftChild = pNode;
					else
						pBackNode->pParent->pRightChild = pNode;
				}
				pNode->pParent = pBackNode->pParent;
				pBackNode->pParent = nullptr;
				m_pTreeRoot = pBackNode;
			}

			// 交换左子节点
			pBackNode->pLeftChild = pNode->pLeftChild;
			pNode->pLeftChild->pParent = pBackNode;
			pNode->pLeftChild = nullptr;
			// 交换右子节点
			if (pBackNode != pNode->pRightChild)
			{
				if (pBackNode->pRightChild)
				{
					pNode->pRightChild->pParent = pBackNode;
					pBackNode->pRightChild->pParent = pNode;
					StorageNode* pTemp = pBackNode->pRightChild;
					pBackNode->pRightChild = pNode->pRightChild;
					pNode->pRightChild = pTemp;
				}
				else
				{
					pBackNode->pRightChild = pNode->pRightChild;
					pNode->pRightChild->pParent = pBackNode;
					pNode->pRightChild = nullptr;
				}
			}
			else
			{
				pNode->pRightChild = pBackNode->pRightChild;
				if (pBackNode->pRightChild)
					pBackNode->pRightChild->pParent = pNode;
				pBackNode->pRightChild = pNode;
			}
			// 交换颜色
			enNodeColor Color = pNode->Color;
			pNode->Color = pBackNode->Color;
			pBackNode->Color = Color;
		}

		StorageNode* pNeedBalanceNode = nullptr;

		if (pNode->pRightChild)
		{
			if (pNode->Color == NC_BLACK)
				pNeedBalanceNode = pNode->pRightChild;
			if (pNode->pParent)
			{
				pNode->pRightChild->pParent = pNode->pParent;
				if (pNode->pParent->pLeftChild == pNode)
					pNode->pParent->pLeftChild = pNode->pRightChild;
				else
					pNode->pParent->pRightChild = pNode->pRightChild;
				pNode->pParent = nullptr;
			}
			else
			{
				m_pTreeRoot = pNode->pRightChild;
				m_pTreeRoot->pParent = nullptr;
			}

			pNode->pRightChild = nullptr;
		}
		else if (pNode->pLeftChild)
		{
			if (pNode->Color == NC_BLACK)
				pNeedBalanceNode = pNode->pLeftChild;
			if (pNode == m_pTreeRoot)
			{
				m_pTreeRoot = pNode->pLeftChild;
				m_pTreeRoot->pParent = nullptr;
			}
			else
			{
				pNode->pLeftChild->pParent = pNode->pParent;
				if (pNode->pParent->pLeftChild == pNode)
					pNode->pParent->pLeftChild = pNode->pLeftChild;
				else
					pNode->pParent->pRightChild = pNode->pLeftChild;
				pNode->pParent = nullptr;
			}
			pNode->pLeftChild = nullptr;
		}
		else
		{
			if (pNode->pParent)
			{
				if (pNode->Color == NC_BLACK)
					pNeedBalanceNode = pNode->pParent;

				if (pNode->pParent->pLeftChild == pNode)
					pNode->pParent->pLeftChild = nullptr;
				else
					pNode->pParent->pRightChild = nullptr;
				pNode->pParent = nullptr;
			}
			else
			{
				m_pTreeRoot = nullptr;
			}
		}

		pNode->Color = NC_NONE;

		if (DeleteFromList)
			DeleteNodeFromList(pNode);

		return pNeedBalanceNode;
	}
	void DeleteNodeFromList(StorageNode* pNode)
	{
		if (pNode == m_pObjectListHead && pNode == m_pObjectListTail)
		{
			m_pObjectListHead = nullptr;
			m_pObjectListTail = nullptr;
		}
		else if (pNode == m_pObjectListHead)
		{
			m_pObjectListHead = pNode->pNext;
			m_pObjectListHead->pPrev = nullptr;
		}
		else if (pNode == m_pObjectListTail)
		{
			m_pObjectListTail = pNode->pPrev;
			m_pObjectListTail->pNext = nullptr;
		}
		else
		{
			pNode->pPrev->pNext = pNode->pNext;
			pNode->pNext->pPrev = pNode->pPrev;
		}

		pNode->IsFree = true;
		pNode->DeleteObject();

		pNode->pPrev = m_pFreeListTail;
		pNode->pNext = nullptr;
		if (m_pFreeListTail)
		{
			m_pFreeListTail->pNext = pNode;
			m_pFreeListTail = pNode;
		}
		else
		{
			m_pFreeListHead = pNode;
			m_pFreeListTail = pNode;
		}

		m_ObjectCount--;
	}
	bool CreateBufferPage(uint32 Size)
	{
		if (m_GrowLimit)
		{
			if ((uint32)m_ObjectBuffPages.Num() >= m_GrowLimit)
				return false;
		}

		OBJECT_BUFF_PAGE_INFO PageInfo;
		PageInfo.BufferSize = Size;
		PageInfo.pObjectBuffer = new StorageNode[Size];
		for (uint32 i = 0; i < Size; i++)
		{
			PageInfo.pObjectBuffer[i].IsUsed = false;
		}
#ifdef LOG_POOL_CREATE
		PrintImportantLog("Create %u Total %u", Size, GetBufferSize() + Size);
#endif
		uint32 IDStart = GetBufferSize() + 1;
		ClearBuffer(PageInfo, IDStart, true);
		m_ObjectBuffPages.Add(PageInfo);
		return true;
	}
	void ClearBuffer(OBJECT_BUFF_PAGE_INFO& PageInfo, uint32 IDStart, bool IsInit)
	{
		StorageNode* pFreeListHead = nullptr;
		StorageNode* pFreeListTail = nullptr;

		check(PageInfo.BufferSize != 0);
		if (PageInfo.BufferSize == 0)
			return;

		for (uint32 i = 0; i < PageInfo.BufferSize; i++)
		{
			PageInfo.pObjectBuffer[i].ID = i + IDStart;
			if (i == 0)
			{
				pFreeListHead = PageInfo.pObjectBuffer;
				PageInfo.pObjectBuffer[i].pPrev = nullptr;
			}
			else
			{
				PageInfo.pObjectBuffer[i].pPrev = &(PageInfo.pObjectBuffer[i - 1]);
			}
			if (i == PageInfo.BufferSize - 1)
			{
				pFreeListTail = PageInfo.pObjectBuffer + i;
				PageInfo.pObjectBuffer[i].pNext = nullptr;
			}
			else
			{
				PageInfo.pObjectBuffer[i].pNext = &(PageInfo.pObjectBuffer[i + 1]);
			}
			PageInfo.pObjectBuffer[i].pParent = nullptr;
			PageInfo.pObjectBuffer[i].pRightChild = nullptr;
			PageInfo.pObjectBuffer[i].pLeftChild = nullptr;
			PageInfo.pObjectBuffer[i].Color = NC_NONE;
			PageInfo.pObjectBuffer[i].IsFree = true;
			if (IsInit)
				PageInfo.pObjectBuffer[i].InitObject();
			else
				PageInfo.pObjectBuffer[i].DeleteObject();
		}
		if (m_pFreeListTail)
		{
			m_pFreeListTail->pNext = pFreeListHead;
			pFreeListHead->pPrev = m_pFreeListTail;
			m_pFreeListTail = pFreeListTail;
		}
		else
		{
			m_pFreeListHead = pFreeListHead;
			m_pFreeListTail = pFreeListTail;
		}
	}
	StorageNode* NewNode(const KEY& Key)
	{
		if (m_pFreeListHead == nullptr && m_GrowSize)
		{
			CreateBufferPage(m_GrowSize);
		}
		if (m_pFreeListHead)
		{
			StorageNode* pNode;

			pNode = m_pFreeListHead;
			if (m_pFreeListHead == m_pFreeListTail)
			{
				m_pFreeListHead = nullptr;
				m_pFreeListTail = nullptr;
			}
			else
			{
				m_pFreeListHead = pNode->pNext;
				if (m_pFreeListHead)
					m_pFreeListHead->pPrev = nullptr;
			}

			if (m_pObjectListTail)
			{
				m_pObjectListTail->pNext = pNode;
				pNode->pPrev = m_pObjectListTail;
				pNode->pNext = nullptr;
				m_pObjectListTail = pNode;
			}
			else
			{
				m_pObjectListHead = pNode;
				m_pObjectListTail = pNode;
				pNode->pPrev = nullptr;
				pNode->pNext = nullptr;
			}
			pNode->IsFree = false;
			if (!pNode->IsUsed)
			{
				pNode->IsUsed = true;
				m_UsedObjectCount++;
			}
			pNode->NewObject();
			pNode->Key = Key;
			pNode->Color = NC_NONE;
			pNode->pRightChild = nullptr;
			pNode->pLeftChild = nullptr;

			m_ObjectCount++;
			return pNode;
		}
		return nullptr;
	}
	StorageNode* RotateRight(StorageNode* pNode)
	{
		// 右旋必须保证有左子树
		StorageNode* pLeftChild = pNode->pLeftChild;
		if (pLeftChild == nullptr)
		{
			return nullptr;
		}

		if (pNode->pParent)
		{
			if (pNode->pParent->pLeftChild == pNode)
				pNode->pParent->pLeftChild = pLeftChild;
			else
				pNode->pParent->pRightChild = pLeftChild;
			pLeftChild->pParent = pNode->pParent;
		}
		else
		{
			pLeftChild->pParent = nullptr;
		}

		pNode->pParent = pLeftChild;

		pNode->pLeftChild = pLeftChild->pRightChild;
		if (pLeftChild->pRightChild)
			pLeftChild->pRightChild->pParent = pNode;
		pLeftChild->pRightChild = pNode;

		if (pLeftChild->pParent == nullptr)
			m_pTreeRoot = pLeftChild;
		return pLeftChild;
	}
	StorageNode* RotateLeft(StorageNode* pNode)
	{
		// 左旋必须保证有右子树
		StorageNode* pRightChild = pNode->pRightChild;
		if (pRightChild == nullptr)
		{
			return nullptr;
		}
		if (pNode->pParent)
		{
			if (pNode->pParent->pLeftChild == pNode)
				pNode->pParent->pLeftChild = pRightChild;
			else
				pNode->pParent->pRightChild = pRightChild;
			pRightChild->pParent = pNode->pParent;
		}
		else
		{
			pRightChild->pParent = nullptr;
		}

		pNode->pParent = pRightChild;
		pNode->pRightChild = pRightChild->pLeftChild;
		if (pRightChild->pLeftChild)
			pRightChild->pLeftChild->pParent = pNode;
		pRightChild->pLeftChild = pNode;
		if (pRightChild->pParent == nullptr)
			m_pTreeRoot = pRightChild;
		return pRightChild;
	}
	void BalanceInsert(StorageNode* pNode)
	{
		// check(pNode->Color==NC_RED);
		if (pNode == m_pTreeRoot)
		{
			pNode->Color = NC_BLACK;
		}
		else
		{
			check(pNode->pParent != nullptr);
			if (pNode->pParent->Color == NC_RED)
			{
				StorageNode* pParentParent = pNode->pParent->pParent;
				enNodeColor	 Color1;
				enNodeColor	 Color2;

				if (pParentParent->pLeftChild)
					Color1 = pParentParent->pLeftChild->Color;
				else
					Color1 = NC_BLACK;
				if (pParentParent->pRightChild)
					Color2 = pParentParent->pRightChild->Color;
				else
					Color2 = NC_BLACK;

				if (Color1 == NC_RED && Color2 == NC_RED)
				{
					pParentParent->Color = NC_RED;
					pParentParent->pLeftChild->Color = NC_BLACK;
					pParentParent->pRightChild->Color = NC_BLACK;
					BalanceInsert(pParentParent);
				}
				else
				{
					pNode->pParent->pParent->Color = NC_RED;
					StorageNode* pParentNode;
					if (pNode->pParent == pNode->pParent->pParent->pLeftChild)
					{
						if (pNode == pNode->pParent->pRightChild)
						{
							pParentNode = pNode;
							RotateLeft(pNode->pParent);
							RotateRight(pNode->pParent);
						}
						else
						{
							pParentNode = pNode->pParent;
							RotateRight(pNode->pParent->pParent);
						}
					}
					else
					{
						if (pNode == pNode->pParent->pLeftChild)
						{
							pParentNode = pNode;
							RotateRight(pNode->pParent);
							RotateLeft(pNode->pParent);
						}
						else
						{
							pParentNode = pNode->pParent;
							RotateLeft(pNode->pParent->pParent);
						}
					}
					pParentNode->Color = NC_BLACK;
				}
			}
		}
	}
	void BalanceDelete(StorageNode* pNode)
	{
		if (pNode->Color == NC_RED)
		{
			pNode->Color = NC_BLACK;
		}
		else if (pNode != m_pTreeRoot)
		{
			StorageNode* pBrother;
			if (pNode->pParent->pLeftChild == pNode)
				pBrother = pNode->pParent->pRightChild;
			else
				pBrother = pNode->pParent->pLeftChild;

			if (pBrother)
			{
				if (pBrother->Color == NC_RED)
				{
					pBrother->Color = NC_BLACK;
					pBrother->pParent->Color = NC_RED;
					if (pNode->pParent->pLeftChild == pNode)
						RotateLeft(pNode->pParent);
					else
						RotateRight(pNode->pParent);
					BalanceDelete(pNode);
				}
				else if (pBrother->pLeftChild && pBrother->pRightChild)
				{
					if (pBrother->pLeftChild->Color == NC_BLACK && pBrother->pRightChild->Color == NC_BLACK)
					{
						pBrother->Color = NC_RED;
						BalanceDelete(pBrother->pParent);
					}
					else if (pNode->pParent->pLeftChild == pNode)
					{
						if (pBrother->pRightChild->Color == NC_BLACK)
						{
							pBrother->pLeftChild->Color = NC_BLACK;
							pBrother->Color = NC_RED;
							RotateRight(pBrother);
							pBrother = pNode->pParent->pRightChild;
						}
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pRightChild->Color = NC_BLACK;
						RotateLeft(pNode->pParent);
					}
					else
					{
						if (pBrother->pLeftChild->Color == NC_BLACK)
						{
							pBrother->pRightChild->Color = NC_BLACK;
							pBrother->Color = NC_RED;
							RotateLeft(pBrother);
							pBrother = pNode->pParent->pLeftChild;
						}
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pLeftChild->Color = NC_BLACK;
						RotateRight(pNode->pParent);
					}
				}
				else if (pBrother->pLeftChild)
				{
					if (pBrother->pLeftChild->Color == NC_BLACK)
					{
						pBrother->Color = NC_RED;
						BalanceDelete(pBrother->pParent);
					}
					else if (pNode->pParent->pLeftChild == pNode)
					{
						pBrother->pLeftChild->Color = NC_BLACK;
						pBrother->Color = NC_RED;
						RotateRight(pBrother);
						pBrother = pNode->pParent->pRightChild;
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pRightChild->Color = NC_BLACK;
						RotateLeft(pNode->pParent);
					}
					else
					{
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pLeftChild->Color = NC_BLACK;
						RotateRight(pNode->pParent);
					}
				}
				else if (pBrother->pRightChild)
				{
					if (pBrother->pRightChild->Color == NC_BLACK)
					{
						pBrother->Color = NC_RED;
						BalanceDelete(pBrother->pParent);
					}
					else if (pNode->pParent->pLeftChild == pNode)
					{
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pRightChild->Color = NC_BLACK;
						RotateLeft(pNode->pParent);
					}
					else
					{
						pBrother->pRightChild->Color = NC_BLACK;
						pBrother->Color = NC_RED;
						RotateLeft(pBrother);
						pBrother = pNode->pParent->pLeftChild;
						pBrother->Color = pNode->pParent->Color;
						pNode->pParent->Color = NC_BLACK;
						pBrother->pLeftChild->Color = NC_BLACK;
						RotateRight(pNode->pParent);
					}
				}
				else
				{
					pBrother->Color = NC_RED;
					BalanceDelete(pBrother->pParent);
				}
			}
			else
			{
				BalanceDelete(pNode->pParent);
			}
		}
	}
	void PickNode(StorageNode* pNode)
	{
		if (pNode->pPrev)
			pNode->pPrev->pNext = pNode->pNext;
		if (pNode->pNext)
			pNode->pNext->pPrev = pNode->pPrev;
		if (pNode == m_pObjectListHead)
			m_pObjectListHead = pNode->pNext;
		if (pNode == m_pObjectListTail)
			m_pObjectListTail = pNode->pPrev;
		pNode->pPrev = nullptr;
		pNode->pNext = nullptr;
	}
	void InsertNodeBefore(StorageNode* pNode, StorageNode* pBefore)
	{
		if (pBefore == nullptr)
			pBefore = m_pObjectListHead;
		if (pBefore)
		{
			pNode->pPrev = pBefore->pPrev;
			pNode->pNext = pBefore;
			if (pBefore->pPrev)
				pBefore->pPrev->pNext = pNode;
			pBefore->pPrev = pNode;
		}
		else
		{
			m_pObjectListHead = pNode;
			m_pObjectListTail = pNode;
		}
		if (pBefore == m_pObjectListHead)
		{
			m_pObjectListHead = pNode;
		}
	}
	void InsertNodeAfter(StorageNode* pNode, StorageNode* pAfter)
	{
		if (pAfter == nullptr)
			pAfter = m_pObjectListTail;
		if (pAfter)
		{
			pNode->pPrev = pAfter;
			pNode->pNext = pAfter->pNext;
			if (pAfter->pNext)
				pAfter->pNext->pPrev = pNode;
			pAfter->pNext = pNode;
		}
		else
		{
			m_pObjectListHead = pNode;
			m_pObjectListTail = pNode;
		}
		if (pAfter == m_pObjectListTail)
		{
			m_pObjectListTail = pNode;
		}
	}
};