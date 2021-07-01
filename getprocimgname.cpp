NTSTATUS GetProcessImageName(HANDLE processId, PUNICODE_STRING ProcessImageName)
{
	NTSTATUS			status = MESOCARP_ERROR_INITVALUE;
	ULONG				returnedLength;
	ULONG				bufferLength;
	PUNICODE_STRING		imageName;
	HANDLE				hProcess = NULL;
	PVOID				buffer = NULL;
	PEPROCESS			eProcess = NULL;

	PAGED_CODE();	// this eliminates the possibility of the IDLE Thread/Process

	__try
	{
		if (gDynamicFunctions._ZwQueryInformationProcess == NULL) {
			DoTraceMessage(FLAG_GENERAL, "%!FUNC! No ptr to gDynamicFunctions._ZwQueryInformationProcess\n");
			status = STATUS_INVALID_PARAMETER;
			__leave;
		}

		status = PsLookupProcessByProcessId(processId, &eProcess);

		if (!NT_SUCCESS(status)) {
			DoTraceMessage(FLAG_GENERAL, "%!FUNC! PsLookupProcessByProcessId Failed: %08x\n", status);
			__leave;
		}

		status = ObOpenObjectByPointer(eProcess, 0, NULL, 0, 0, KernelMode, &hProcess);

		if (!NT_SUCCESS(status) ) {
			DoTraceMessage(FLAG_GENERAL, "%!FUNC! ObOpenObjectByPointer Failed: %08x\n", status);
			__leave;
		}

		/* Query the actual size of the process path */
		status = gDynamicFunctions._ZwQueryInformationProcess(hProcess,
			ProcessImageFileName,
			NULL,	// buffer
			0,		// buffer size
			&returnedLength);

		if (status != STATUS_INFO_LENGTH_MISMATCH) {
			__leave;
		}

		bufferLength = returnedLength - sizeof(UNICODE_STRING);

		if (ProcessImageName->MaximumLength < bufferLength)
		{
			ProcessImageName->MaximumLength = (USHORT)bufferLength;
			status = STATUS_BUFFER_OVERFLOW;

			__leave;
		}

		/* Allocate a temporary buffer to store the path name */
		buffer = ExAllocatePoolWithTag(NonPagedPool, returnedLength, DLP_DRV_POOL_TAG_MESOCARP_GENERAL);

		if (buffer == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		/* Retrieve the process path from the handle to the process */
		status = gDynamicFunctions._ZwQueryInformationProcess(hProcess,
			ProcessImageFileName,
			buffer,
			returnedLength,
			&returnedLength);

		if (!NT_SUCCESS(status)) {
			__leave;
		}

		imageName = (PUNICODE_STRING)buffer;
		RtlCopyUnicodeString(ProcessImageName, imageName);
	}
	__finally
	{
		if (eProcess != NULL) ObDereferenceObject(eProcess);
		if (buffer != NULL) ExFreePoolWithTag(buffer, DLP_DRV_POOL_TAG_MESOCARP_GENERAL);
	}

	return status;
}

