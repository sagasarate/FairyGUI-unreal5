#pragma once
#include "CoreMinimal.h"
#include "ListIterator.h"



template <class T, int StorageMode = EDSM_STATIC>
class CIDStorage
{
protected:
	template <int Mode, class OBJECT_TYPE>
	struct _StorageNode
	{
		uint32 ID;
		_StorageNode *pPrev;
		_StorageNode *pNext;
		bool IsFree;
		bool IsUsed;
		void InitObject()
		{
		}
		OBJECT_TYPE &GetObjectRef()
		{
			return Object;
		}
		OBJECT_TYPE *GetObjectPointer()
		{
			return &Object;
		}
		const OBJECT_TYPE &GetObjectRef() const
		{
			return Object;
		}
		const OBJECT_TYPE *GetObjectPointer() const
		{
			return &Object;
		}
		void NewObject()
		{
		}
		void DeleteObject()
		{
		}
		void FinalReleaseObject()
		{
		}

	protected:
		OBJECT_TYPE Object;
	};

	template <class OBJECT_TYPE>
	struct _StorageNode<EDSM_NEW_ONCE, OBJECT_TYPE>
	{
		uint32 ID;
		_StorageNode *pPrev;
		_StorageNode *pNext;
		bool IsFree;
		bool IsUsed;
		void InitObject()
		{
			pObject = nullptr;
		}
		OBJECT_TYPE &GetObjectRef()
		{
			return *pObject;
		}
		OBJECT_TYPE *GetObjectPointer()
		{
			return pObject;
		}
		const OBJECT_TYPE &GetObjectRef() const
		{
			return *pObject;
		}
		const OBJECT_TYPE *GetObjectPointer() const
		{
			return pObject;
		}
		void NewObject()
		{
			if (pObject == nullptr)
				pObject = new OBJECT_TYPE;
		}
		void DeleteObject()
		{
		}
		void FinalReleaseObject()
		{
			SAFE_DELETE(pObject);
		}

	protected:
		OBJECT_TYPE *pObject;
	};

	template <class OBJECT_TYPE>
	struct _StorageNode<EDSM_NEW_EVERY_TIME, OBJECT_TYPE>
	{
		uint32 ID;
		_StorageNode *pPrev;
		_StorageNode *pNext;
		bool IsFree;
		bool IsUsed;
		void InitObject()
		{
			pObject = nullptr;
		}
		OBJECT_TYPE &GetObjectRef()
		{
			return *pObject;
		}
		OBJECT_TYPE *GetObjectPointer()
		{
			return pObject;
		}
		const OBJECT_TYPE &GetObjectRef() const
		{
			return *pObject;
		}
		const OBJECT_TYPE *GetObjectPointer() const
		{
			return pObject;
		}
		void NewObject()
		{
			if (pObject == nullptr)
				pObject = new OBJECT_TYPE;
		}
		void DeleteObject()
		{
			SAFE_DELETE(pObject);
		}
		void FinalReleaseObject()
		{
			SAFE_DELETE(pObject);
		}

	protected:
		OBJECT_TYPE *pObject;
	};

	typedef _StorageNode<StorageMode, T> StorageNode;

	struct OBJECT_BUFF_PAGE_INFO
	{
		StorageNode *pObjectBuffer;
		uint32 BufferSize;
	};

	TArray<OBJECT_BUFF_PAGE_INFO> m_ObjectBuffPages;
	StorageNode *m_pFreeListHead;
	StorageNode *m_pFreeListTail;
	StorageNode *m_pObjectListHead;
	StorageNode *m_pObjectListTail;
	uint32 m_ObjectCount;
	uint32 m_UsedObjectCount;
	uint32 m_GrowSize;
	uint32 m_GrowLimit;

public:
	typedef list_iterator<CIDStorage, T> iterator;
	typedef const_list_iterator<CIDStorage, T> const_iterator;

public:
	CIDStorage()
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
	}
	CIDStorage(uint32 Size, uint32 GrowSize = 0, uint32 GrowLimit = 0)
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_ObjectCount = 0;
		m_UsedObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
		Create(Size, GrowSize, GrowLimit);
	}
	~CIDStorage()
	{
		Destory();
	}
	bool Create(uint32 Size, uint32 GrowSize = 0, uint32 GrowLimit = 0)
	{
		Destory();
		if (Size)
		{
			m_GrowSize = GrowSize;
			m_GrowLimit = GrowLimit;
			return CreateBufferPage(Size);
		}
		return true;
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
	bool IsCreated() const
	{
		return m_ObjectBuffPages.Num() != 0;
	}
	void Destory()
	{
		for (int32 i = 0; i < m_ObjectBuffPages.Num(); i++)
		{
			for (uint32 j = 0; j < m_ObjectBuffPages[i].BufferSize; j++)
			{
				m_ObjectBuffPages[i].pObjectBuffer[j].FinalReleaseObject();
			}
			delete[] m_ObjectBuffPages[i].pObjectBuffer;
		}
		m_ObjectBuffPages.Empty();
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
		m_ObjectCount = 0;
		m_GrowSize = 0;
		m_GrowLimit = 0;
	}
	void Clear()
	{
		m_pFreeListHead = nullptr;
		m_pFreeListTail = nullptr;
		m_pObjectListHead = nullptr;
		m_pObjectListTail = nullptr;
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
		m_ObjectCount = 0;

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
	uint32 NewObject(T **ppObject)
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeAfter(pNode, m_pObjectListTail);
			*ppObject = pNode->GetObjectPointer();
			return pNode->ID;
		}
		*ppObject = nullptr;
		return 0;
	}

	T *NewObject()
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeAfter(pNode, m_pObjectListTail);
			pNode->GetObjectRef().SetID(pNode->ID);
			return pNode->GetObjectPointer();
		}

		return nullptr;
	}

	uint32 AddObject(const T &Object)
	{
		uint32 ID;
		T *pObject;
		ID = NewObject(&pObject);
		if (ID)
		{
			*pObject = Object;
			return ID;
		}
		return 0;
	}
	const void* InsertAfter(const void* Pos = nullptr)
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeAfter(pNode, (StorageNode *)Pos);
			return pNode;
		}
		return nullptr;
	}
	const void* InsertAfter(const T &Object, const void* Pos = nullptr)
	{
		StorageNode *pNode = (StorageNode *)InsertAfter(Pos);
		if (pNode)
		{
			pNode->GetObjectRef() = Object;
			return pNode;
		}
		return nullptr;
	}
	const void* InsertBefore(const void* Pos = nullptr)
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeBefore(pNode, (StorageNode *)Pos);
			return pNode;
		}
		return nullptr;
	}
	const void* InsertBefore(const T &Object, const void* Pos = nullptr)
	{
		StorageNode *pNode = (StorageNode *)InsertBefore(Pos);
		if (pNode)
		{
			pNode->GetObjectRef() = Object;
			return pNode;
		}
		return nullptr;
	}
	const void* InsertSorted(const T &Object)
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			pNode->GetObjectRef() = Object;
			StorageNode *pHead = m_pObjectListHead;
			while (pHead && pHead->GetObjectRef() < Object)
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
			return pNode;
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
			const OBJECT_BUFF_PAGE_INFO &FirstPage = m_ObjectBuffPages[0];
			if (ID < FirstPage.BufferSize)
			{
				if (!FirstPage.pObjectBuffer[ID].IsFree)
					return &(FirstPage.pObjectBuffer[ID]);
			}
			else
			{
				ID -= FirstPage.BufferSize;
				int32 PageIndex = 1 + ID / m_GrowSize;
				uint32 Index = ID % m_GrowSize;
				if (PageIndex < m_ObjectBuffPages.Num())
				{
					const OBJECT_BUFF_PAGE_INFO &Page = m_ObjectBuffPages[PageIndex];
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
	T *GetObject(const void* Pos)
	{
		StorageNode *pNode = (StorageNode *)Pos;
		if (pNode)
		{
			if (!pNode->IsFree)
				return pNode->GetObjectPointer();
		}

		return nullptr;
	}
	const T *GetObject(const void* Pos) const
	{
		const StorageNode *pNode = (const StorageNode *)Pos;
		if (pNode)
		{
			if (!pNode->IsFree)
				return pNode->GetObjectPointer();
		}

		return nullptr;
	}
	T *GetObject(uint32 ID)
	{
		return GetObject(GetObjectPosByID(ID));
	}
	const T *GetObject(uint32 ID) const
	{
		return GetObject(GetObjectPosByID(ID));
	}
	uint32 GetObjectIDByPos(const void* Pos) const
	{
		const StorageNode *pNode = (const StorageNode *)Pos;
		if (pNode)
			return pNode->ID;
		else
			return 0;
	}

	bool DeleteObject(uint32 ID)
	{
		return DeleteObjectByPos(GetObjectPosByID(ID));
	}
	bool DeleteObjectByPos(const void* Pos)
	{
		StorageNode *pNode = (StorageNode *)Pos;
		if (pNode)
		{
			if (!pNode->IsFree)
			{
				DeleteNode(pNode);
				return true;
			}
		}
		return false;
		;
	}

	uint32 GetObjectCount() const
	{
		return m_ObjectCount;
	}
	uint32 GetUsedObjectCount() const
	{
		return m_UsedObjectCount;
	}

	const void* GetFirstObjectPos() const
	{
		return m_pObjectListHead;
	}

	void* GetLastObjectPos() const
	{
		return m_pObjectListTail;
	}
	const void* GetNextObjectPos(const void* Pos) const
	{
		if (Pos)
		{
			const StorageNode *pNode = (const StorageNode *)Pos;
			return pNode->pNext;
		}
		return nullptr;
	}
	const void* GetPrevObjectPos(const void* Pos) const
	{
		if (Pos)
		{
			const StorageNode *pNode = (const StorageNode *)Pos;
			return pNode->pPrev;
		}
		return nullptr;
	}

	T *GetNextObject(const void* &Pos)
	{
		if (Pos)
		{
			StorageNode *pNode = (StorageNode *)Pos;
			Pos = pNode->pNext;
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}

	T *GetPrevObject(const void* &Pos)
	{
		if (Pos)
		{
			StorageNode *pNode = (StorageNode *)Pos;
			Pos = pNode->pPrev;
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	const T *GetNextObject(const void* &Pos) const
	{
		if (Pos)
		{
			const StorageNode *pNode = (const StorageNode *)Pos;
			Pos = pNode->pNext;
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}

	const T *GetPrevObject(const void* &Pos) const
	{
		if (Pos)
		{
			const StorageNode *pNode = (const StorageNode *)Pos;
			Pos = pNode->pPrev;
			return pNode->GetObjectPointer();
		}
		return nullptr;
	}
	bool MoveToBefore(const void* Pos, const void* Target)
	{
		if (Pos && Pos != Target && m_ObjectCount > 1)
		{
			StorageNode *pNode = (StorageNode *)Pos;
			StorageNode *pBefore = (StorageNode *)Target;
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
			StorageNode *pNode = (StorageNode *)Pos;
			StorageNode *pAfter = (StorageNode *)Target;
			PickNode(pNode);
			InsertNodeAfter(pNode, pAfter);
			return true;
		}
		return false;
	}
	bool MoveSorted(const void* Pos)
	{
		StorageNode *pNode = (StorageNode *)Pos;
		if (pNode)
		{
			PickNode(pNode);
			StorageNode *pHead = m_pObjectListHead;
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

	/** 稳定排序，使用 operator< 比较，相等元素保持原有顺序 */
	void StableSort()
	{
		StableSort([](const T& A, const T& B) { return A < B; });
	}

	/** 稳定排序，使用自定义比较函数 Pred(A,B) 返回 true 表示 A 排在 B 前面 */
	template <typename Predicate>
	void StableSort(const Predicate& Pred)
	{
		if (m_ObjectCount <= 1)
			return;

		m_pObjectListHead = MergeSort(m_pObjectListHead, Pred);

		// 修复 pPrev 和 tail
		m_pObjectListTail = m_pObjectListHead;
		if (m_pObjectListHead)
		{
			m_pObjectListHead->pPrev = nullptr;
			StorageNode* pCurr = m_pObjectListHead;
			while (pCurr->pNext)
			{
				pCurr->pNext->pPrev = pCurr;
				pCurr = pCurr->pNext;
			}
			m_pObjectListTail = pCurr;
		}
	}

	const void* PushFront()
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeBefore(pNode, m_pObjectListHead);
			return pNode;
		}
		return nullptr;
	}

	const void* PushFront(const T &Object)
	{
		StorageNode *pNode = (StorageNode *)PushFront();
		if (pNode)
		{
			pNode->GetObjectRef() = Object;
			return pNode;
		}
		return nullptr;
	}

	const void* PushBack()
	{
		StorageNode *pNode = NewNode();
		if (pNode)
		{
			InsertNodeAfter(pNode, m_pObjectListTail);
			return pNode;
		}
		return nullptr;
	}

	const void* PushBack(const T &Object)
	{
		StorageNode *pNode = (StorageNode *)PushBack();
		if (pNode)
		{
			pNode->GetObjectRef() = Object;
			return pNode;
		}
		return nullptr;
	}

	bool PopFront(T &Object)
	{
		if (m_pObjectListHead)
		{
			Object = m_pObjectListHead->GetObjectRef();
			DeleteNode(m_pObjectListHead);
			return true;
		}
		return false;
	}

	bool PopBack(T &Object)
	{
		if (m_pObjectListTail)
		{
			Object = m_pObjectListTail->GetObjectRef();
			DeleteNode(m_pObjectListTail);
			return true;
		}
		return false;
	}
	const void* Find(const T &Object) const
	{
		const StorageNode *pNode = m_pObjectListHead;
		while (pNode)
		{
			if (pNode->Object == Object)
				return pNode;
			pNode = pNode->pNext;
		}
		return nullptr;
	}
	void Verfy(uint32 &UsedCount, uint32 &FreeCount) const
	{
		UsedCount = 0;
		FreeCount = 0;
		uint32 ObjectBuffSize = GetBufferSize();

		const StorageNode *pNode = m_pObjectListHead;
		while (pNode && UsedCount < ObjectBuffSize)
		{
			pNode = pNode->pNext;
			UsedCount++;
		}

		pNode = m_pFreeListHead;
		while (pNode && FreeCount < ObjectBuffSize)
		{
			pNode = pNode->pNext;
			FreeCount++;
		}
	}
	iterator begin()
	{
		return iterator(this, GetFirstObjectPos());
	}

	iterator end()
	{
		return iterator(this, nullptr);
	}
	const_iterator begin() const
	{
		return const_iterator(this, GetFirstObjectPos());
	}

	const_iterator end() const
	{
		return const_iterator(this, nullptr);
	}
	const void* GetFreeObjectPosByID(uint32 ID)
	{
		if (ID == 0)
			return nullptr;
		if (m_ObjectBuffPages.Num())
		{
			ID--;
			const OBJECT_BUFF_PAGE_INFO &FirstPage = m_ObjectBuffPages[0];
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
				if (PageIndex < m_ObjectBuffPages.Num())
				{
					const OBJECT_BUFF_PAGE_INFO &Page = m_ObjectBuffPages[PageIndex];
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
	T *GetFreeObject(const void* Pos)
	{
		StorageNode *pNode = (StorageNode *)Pos;
		if (pNode)
		{
			if (pNode->IsFree)
				return pNode->GetObjectPointer();
		}

		return nullptr;
	}
	void ReleaseFreeObject(const void* Pos)
	{
		StorageNode *pNode = (StorageNode *)Pos;
		if (pNode)
		{
			if (pNode->IsFree)
				return pNode->FinalReleaseObject();
		}
	}

protected:
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
		PrintImportantLog(TEXT("Create %u Total %u"), Size, GetBufferSize() + Size);
#endif
		uint32 IDStart = GetBufferSize() + 1;
		ClearBuffer(PageInfo, IDStart, true);
		m_ObjectBuffPages.Add(PageInfo);
		return true;
	}
	void ClearBuffer(OBJECT_BUFF_PAGE_INFO &PageInfo, uint32 IDStart, bool IsInit)
	{
		StorageNode *pFreeListHead = nullptr;
		StorageNode *pFreeListTail = nullptr;

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
	StorageNode *NewNode()
	{
		if (m_pFreeListHead == nullptr && m_GrowSize)
		{
			CreateBufferPage(m_GrowSize);
		}
		if (m_pFreeListHead)
		{
			StorageNode *pNode;

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

			pNode->pPrev = nullptr;
			pNode->pNext = nullptr;
			pNode->IsFree = false;
			if (!pNode->IsUsed)
			{
				pNode->IsUsed = true;
				m_UsedObjectCount++;
			}
			pNode->NewObject();
			m_ObjectCount++;
			return pNode;
		}
		return nullptr;
	}
	void InsertNodeBefore(StorageNode *pNode, StorageNode *pBefore)
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
	void InsertNodeAfter(StorageNode *pNode, StorageNode *pAfter)
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
	void PickNode(StorageNode *pNode)
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
	void DeleteNode(StorageNode *pNode)
	{
		PickNode(pNode);
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

	template <typename Predicate>
	StorageNode* MergeSort(StorageNode* pHead, const Predicate& Pred)
	{
		if (!pHead || !pHead->pNext)
			return pHead;

		// 快慢指针找中点
		StorageNode* pSlow = pHead;
		StorageNode* pFast = pHead->pNext;
		while (pFast && pFast->pNext)
		{
			pSlow = pSlow->pNext;
			pFast = pFast->pNext->pNext;
		}

		StorageNode* pMid = pSlow->pNext;
		pSlow->pNext = nullptr;
		if (pMid)
			pMid->pPrev = nullptr;

		StorageNode* pLeft = MergeSort(pHead, Pred);
		StorageNode* pRight = MergeSort(pMid, Pred);

		return Merge(pLeft, pRight, Pred);
	}

	template <typename Predicate>
	StorageNode* Merge(StorageNode* pLeft, StorageNode* pRight, const Predicate& Pred)
	{
		StorageNode Dummy;
		Dummy.pNext = nullptr;
		Dummy.pPrev = nullptr;
		StorageNode* pTail = &Dummy;

		while (pLeft && pRight)
		{
			// 当 right < left 时取 right，否则取 left（保证稳定性）
			if (Pred(pRight->GetObjectRef(), pLeft->GetObjectRef()))
			{
				pTail->pNext = pRight;
				pRight->pPrev = pTail;
				pRight = pRight->pNext;
			}
			else
			{
				pTail->pNext = pLeft;
				pLeft->pPrev = pTail;
				pLeft = pLeft->pNext;
			}
			pTail = pTail->pNext;
		}

		pTail->pNext = pLeft ? pLeft : pRight;
		if (pTail->pNext)
			pTail->pNext->pPrev = pTail;

		StorageNode* pResult = Dummy.pNext;
		if (pResult)
			pResult->pPrev = nullptr;
		return pResult;
	}
};
