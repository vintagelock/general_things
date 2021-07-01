NTSTATUS GetProcessLaunchSID( HANDLE processId,TOKEN_USER** userToken, UNICODE_STRING* uniSIDString )
{
	NTSTATUS			status = MESOCARP_ERROR_INITVALUE;
	PEPROCESS			eProcess = NULL;
	PACCESS_TOKEN		pAccessToken = NULL;

	__try
	{
		//KdBreakPoint();

		status = PsLookupProcessByProcessId( processId, &eProcess );

		if( !NT_SUCCESS( status ) ) {
			DoTraceMessage( FLAG_GENERAL,"%!FUNC! PsLookupProcessByProcessId Failed: %08x\n",status );
			__leave;
		}

		pAccessToken = PsReferencePrimaryToken( eProcess );

		if( pAccessToken == NULL ) {
			status = STATUS_INVALID_TOKEN;
			__leave;
		}

		status = SeQueryInformationToken( pAccessToken,TokenUser,(PVOID*)(userToken) );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		if( uniSIDString )
		{
			status = RtlConvertSidToUnicodeString( uniSIDString, ((PTOKEN_USER)*userToken)->User.Sid, TRUE );
		}
	}
	__finally
	{
		if( pAccessToken != NULL )
		{
			PsDereferencePrimaryToken( pAccessToken );
		}

		if( eProcess != NULL )
		{
			ObDereferenceObject( eProcess );
		}
	}

	return status;
}
