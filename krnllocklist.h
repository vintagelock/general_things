
// Ben Lewis

template <class Ty>
class CLockList
{
	typedef struct _LockList
	{
		LIST_ENTRY	_next;
		Ty			_entry;
	}LOCK_LIST;

public:
	CLockList() { FltInitializePushLock( &m_lock );  InitializeListHead( &m_list ); }
	~CLockList() { RemoveAllEntries(); FltDeletePushLock( &m_lock ); }

	BOOLEAN IsEntryExists( Ty aEntry )
	{
		BOOLEAN bRet = FALSE;

		FltAcquirePushLockShared( &m_lock );

		for( PLIST_ENTRY pEntry = m_list.Flink; pEntry != &m_list; pEntry = pEntry->Flink )
		{
			LOCK_LIST * pMyListEntry = (LOCK_LIST *)CONTAINING_RECORD( pEntry,LOCK_LIST,_next );

			if( aEntry == pMyListEntry->pEntry )
			{
				bRet = TRUE;
				break;
			}
		}

		FltReleasePushLock( &m_lock );

		return bRet;
	}

	NTSTATUS InsertToHead( Ty aEntry )
	{
		LOCK_LIST * pEntry = (LOCK_LIST *)ExAllocatePoolWithTag( NonPagedPool,sizeof( LOCK_LIST ),MRMW_TAG );

		if( pEntry == NULL )
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		pEntry->pEntry = aEntry;

		FltAcquirePushLockExclusive( &m_lock );
		InsertHeadList( &m_list,&pEntry->_next );
		FltReleasePushLock( &m_lock );

		return STATUS_SUCCESS;
	}

	NTSTATUS InsertToTail( Ty aEntry )
	{
		LOCK_LIST * pEntry = (LOCK_LIST *)ExAllocatePoolWithTag( NonPagedPool,sizeof( LOCK_LIST ),MRMW_TAG );

		if( pEntry == NULL )
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		pEntry->pEntry = aEntry;

		FltAcquirePushLockExclusive( &m_lock );
		InsertTailList( &m_list,&pEntry->_next );
		FltReleasePushLock( &m_lock );

		return STATUS_SUCCESS;
	}

	VOID RemoveHeadEntry()
	{
		FltAcquirePushLockExclusive( &m_lock );
		PLIST_ENTRY	pEntry = RemoveHeadList( &m_list );
		LOCK_LIST *	pMyListEntry = (LOCK_LIST *)CONTAINING_RECORD( pEntry,LOCK_LIST,_next );
		ExFreePoolWithTag( pMyListEntry,MRMW_TAG );
		FltReleasePushLock( &m_lock );
	}

	VOID RemoveTailEntry()
	{
		FltAcquirePushLockExclusive( &m_lock );
		PLIST_ENTRY pEntry = RemoveTailList( &m_list );
		LOCK_LIST * pMyListEntry = (LOCK_LIST *)CONTAINING_RECORD( pEntry,LOCK_LIST,_next );
		ExFreePoolWithTag( pMyListEntry,MRMW_TAG );
		FltReleasePushLock( &m_lock );
	}

	VOID RemoveEntry( Ty aEntry )
	{
		FltAcquirePushLockExclusive( &m_lock );

		for( PLIST_ENTRY pEntry = m_list.Flink; pEntry != &m_list; pEntry = pEntry->Flink )
		{
			LOCK_LIST * pMyListEntry = (LOCK_LIST *)CONTAINING_RECORD( pEntry,LOCK_LIST,_next );

			if( aEntry == pMyListEntry->pEntry )
			{
				RemoveEntryList( pEntry );
				ExFreePoolWithTag( pMyListEntry,MRMW_TAG );
				break;
			}
		}

		FltReleasePushLock( &m_lock );
	}

	VOID RemoveAllEntries()
	{
		FltAcquirePushLockExclusive( &m_lock );

		while( !IsListEmpty( &m_list ) )
		{
			PLIST_ENTRY pEntry = RemoveTailList( &m_list );
			LOCK_LIST * pMyListEntry = CONTAINING_RECORD( pEntry,LOCK_LIST,_next );
			ExFreePoolWithTag( pMyListEntry,MRMW_TAG );
		}
		FltReleasePushLock( &m_lock );
	}

protected:
private:
	EX_PUSH_LOCK	m_lock;
	LIST_ENTRY		m_list;
};

