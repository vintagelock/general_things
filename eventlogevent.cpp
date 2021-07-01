void EventLogEvent( PVOID obj, NTSTATUS event, NTSTATUS status, ULONG uniqueness, ULONG len, PVOID data, WCHAR *text )
{
	ULONG					i, j, k;
	PIO_ERROR_LOG_PACKET	errorLogEntry;

	// check to see if some of the caller's raw data is to be included in the event log packet
	if( len )
	{
		// determine the number of data bytes to place in error log message
		j = MIN( ERROR_LOG_MAXIMUM_SIZE - sizeof( IO_ERROR_LOG_PACKET ), len );
		j = j / sizeof( ULONG ) * sizeof( ULONG ); // make sure bytes is a multiple of sizeof(ULONG)
	}
	else
		// there is no extension data so just build basic log entry
		j = 0;

	// calculate where the text will go into the error log packet
	k = sizeof( IO_ERROR_LOG_PACKET ) + j;

	// determine if we need space for the text
	if( text != NULL )
	{
		// calculate how big the error log packet needs to be to hold everything
		i = k + (((ULONG)wcslen( text ) + 1) * (ULONG)sizeof( WCHAR ));

		// determine if the text will fit into the error log packet
		if( i > ERROR_LOG_MAXIMUM_SIZE )
			// can't fit the text
			i = k;
	}
	else
		i = k;

	// allocate the error log packet
	errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry( obj, (UCHAR)i );

	// make sure the system actually allocated an error log packet
	if( errorLogEntry != NULL )
	{
		// initialize the error log packet
		errorLogEntry->MajorFunctionCode = 0;
		errorLogEntry->RetryCount = 0;
		errorLogEntry->DumpDataSize = (USHORT)j;// bytes of data before any unicode string in DumpData
		errorLogEntry->EventCategory = 0;  // set to 0 unless we have custom error log handling application
		errorLogEntry->ErrorCode = event;
		errorLogEntry->UniqueErrorValue = uniqueness;
		errorLogEntry->FinalStatus = status;
		errorLogEntry->SequenceNumber = 0;
		errorLogEntry->IoControlCode = 0;

		if( j )
			// copy the dump data from our device extension into the error log packet
			RtlCopyMemory( &errorLogEntry->DumpData, data, j );

		if( i > k )
		{
			// we are able to include text string
			errorLogEntry->NumberOfStrings = 1;
			errorLogEntry->StringOffset = (USHORT)k;

			// last sizeof(WCHAR) bytes in text should be 0
			RtlCopyMemory( (char *)errorLogEntry + k, text, i - k );

		}
		else
		{
			// couldn't include or caller didn't specify a text string
			errorLogEntry->NumberOfStrings = 0;
			errorLogEntry->StringOffset = 0;
		}

		// request that the write the error log packet to the error log file

		IoWriteErrorLogEntry( errorLogEntry );
	}
}
