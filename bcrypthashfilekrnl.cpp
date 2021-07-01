NTSTATUS CryptoHashFile( __in PUNICODE_STRING FileName, __out PUCHAR *Hash, __out PULONG HashSize )
{
	NTSTATUS			status = STATUS_UNSUCCESSFUL;
	BCRYPT_ALG_HANDLE	hashAlgHandle = NULL;
	ULONG				querySize;
	ULONG				hashObjectSize;
	ULONG				hashSize;
	PUCHAR				hashObject = NULL;
	PUCHAR				hash = NULL;
	BCRYPT_HASH_HANDLE	hashHandle = NULL;
	OBJECT_ATTRIBUTES	oa;
	IO_STATUS_BLOCK		iosb;
	HANDLE				fileHandle = NULL;
	ULONG				remainingBytes;
	ULONG				bytesToRead;
	PUCHAR				buffer = NULL;

	FILE_STANDARD_INFORMATION standardInfo;

	PAGED_CODE();

	__try
	{
		// Open the hash algorithm and allocate memory for the hash object.
		status = BCryptOpenAlgorithmProvider( &hashAlgHandle, BCRYPT_SHA1_ALGORITHM, NULL, 0 );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		status = BCryptGetProperty( hashAlgHandle, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hashObjectSize, sizeof( ULONG ), &querySize, 0 );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		status = BCryptGetProperty( hashAlgHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof( ULONG ), &querySize, 0 );

		if( !NT_SUCCESS( status ) )  {
			__leave;
		}

		hashObject = (PUCHAR)ExAllocatePoolWithTag( PagedPool, hashObjectSize, DLP_DRV_POOL_TAG_MESOCARP_CRYPTOGEN );

		if( hashObject == NULL )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		hash = (PUCHAR)ExAllocatePoolWithTag( PagedPool, hashSize, DLP_DRV_POOL_TAG_MESOCARP_CRYPTOGEN );

		if( hash == NULL )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		status = BCryptCreateHash( hashAlgHandle, &hashHandle, hashObject, hashObjectSize, NULL, 0, 0 );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		// Open the file and compute the hash.
		InitializeObjectAttributes( &oa, 
			FileName, 
			OBJ_KERNEL_HANDLE, 
			NULL, 
			NULL );

		status = ZwCreateFile( &fileHandle, 
			FILE_GENERIC_READ, 
			&oa,
			&iosb, 
			NULL, 
			FILE_ATTRIBUTE_NORMAL, 
			FILE_SHARE_READ, 
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 
			NULL, 
			0 );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		status = ZwQueryInformationFile( fileHandle, 
			&iosb, 
			&standardInfo, 
			sizeof( FILE_STANDARD_INFORMATION ), 
			FileStandardInformation );

		if( !NT_SUCCESS( status ) ) {
			__leave;
		}

		if( standardInfo.EndOfFile.QuadPart <= 0 )
		{
			status = STATUS_UNSUCCESSFUL;
			__leave;
		}

		// We set a resonable max size for the file (FILE_MAX_SIZE_HASH == 32mb)
		// so that we do not needlessly hold up the kernel.  If the filesize is greater
		// that that we indicate that the file was too large
		if( standardInfo.EndOfFile.QuadPart > FILE_MAX_SIZE_HASH ) {
			status = STATUS_FILE_TOO_LARGE;
			__leave;
		}

		buffer = (PUCHAR)ExAllocatePoolWithTag( PagedPool, FILE_BUFFER_SIZE_HASH, DLP_DRV_POOL_TAG_MESOCARP_GENERAL );

		if( buffer == NULL )
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		remainingBytes = (ULONG)standardInfo.EndOfFile.QuadPart;

		while( remainingBytes != 0 )
		{
			bytesToRead = FILE_BUFFER_SIZE_HASH;
			
			if( bytesToRead > remainingBytes ) bytesToRead = remainingBytes;

			status = ZwReadFile( fileHandle, NULL, NULL, NULL, &iosb, buffer, bytesToRead, NULL, NULL );

			if( !NT_SUCCESS( status ) ) {
				__leave;
			}

			if( (ULONG)iosb.Information != bytesToRead )
			{
				status = STATUS_INTERNAL_ERROR;
				__leave;
			}

			if( !NT_SUCCESS( status = BCryptHashData( hashHandle, buffer, bytesToRead, 0 ) ) ) 
				__leave;

			remainingBytes -= bytesToRead;
		}

		status = BCryptFinishHash( hashHandle, hash, hashSize, 0 );

		if( !NT_SUCCESS( status ) ) 
			__leave;

		if( NT_SUCCESS( status ) )
		{
			*Hash = hash;
			*HashSize = hashSize;

			hash = NULL;	// Don't free this in the cleanup section
		}
	}
	__finally
	{
		if( buffer ) ExFreePoolWithTag( buffer, DLP_DRV_POOL_TAG_MESOCARP_GENERAL );
		if( fileHandle ) ZwClose( fileHandle );
		if( hashHandle ) BCryptDestroyHash( hashHandle );
		if( hash ) ExFreePoolWithTag(hash, DLP_DRV_POOL_TAG_MESOCARP_CRYPTOGEN );
		if( hashObject ) ExFreePoolWithTag( hashObject, DLP_DRV_POOL_TAG_MESOCARP_CRYPTOGEN );
		if( hashAlgHandle ) BCryptCloseAlgorithmProvider( hashAlgHandle, 0 );
	}

	return status;
}
