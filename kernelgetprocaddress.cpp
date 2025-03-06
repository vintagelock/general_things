_IRQL_requires_max_(PASSIVE_LEVEL)
    _Must_inspect_result_
    PVOID _Internal_KernelGetProcAddress(
        _In_ PCHAR ModuleName,
        _In_ PCHAR pFunctionName)
{
    if (ModuleName == NULL || pFunctionName == NULL)
    {
        DbgPrint("Invalid input parameters\n");
        return NULL;
    }

    PVOID pFunctionAddress = NULL;
    ULONG modulesSize = 0;
    ULONG moduleCount;
    NTSTATUS status;
    AUX_MODULE_EXTENDED_INFO *modulesPtr = NULL;
    PVOID moduleBase = NULL;

    status = AuxKlibInitialize();

    if (!NT_SUCCESS(status))
    {
        DbgPrint("Failed AuxKlibInitialize\n");
        return NULL;
    }

    status = AuxKlibQueryModuleInformation(&modulesSize, sizeof(AUX_MODULE_EXTENDED_INFO), NULL);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("Failed AuxKlibQueryModuleInformation\n");
        return NULL;
    }

    if (modulesSize > 0)
    {
        moduleCount = modulesSize / sizeof(AUX_MODULE_EXTENDED_INFO);

        modulesPtr = (AUX_MODULE_EXTENDED_INFO *)ExAllocatePoolZero(NonPagedPool, modulesSize, 'aaaa');

        if (modulesPtr != NULL)
        {
            status = AuxKlibQueryModuleInformation(&modulesSize, sizeof(AUX_MODULE_EXTENDED_INFO), modulesPtr);

            if (NT_SUCCESS(status))
            {
                for (ULONG i = 0; i < moduleCount; i++)
                {
                    if (strcmp((char *)modulesPtr[i].FullPathName, (char *)ModuleName) == 0)
                    {
                        moduleBase = modulesPtr[i].BasicInfo.ImageBase;
                        break;
                    }
                }
            }

            ExFreePool(modulesPtr);
        }
    }

    if (moduleBase != NULL)
    {
        PIMAGE_EXPORT_DIRECTORY exports = AuxKlibGetImageExportDirectory(moduleBase);
        PULONG functions = NULL;
        PULONG names = NULL;
        PULONG ordinals = NULL;
        PCHAR function_name = NULL;

        functions = (PULONG)((ULONG_PTR)moduleBase + exports->AddressOfFunctions);
        names = (PULONG)((ULONG_PTR)moduleBase + exports->AddressOfNames);
        ordinals = (PULONG)((ULONG_PTR)moduleBase + exports->AddressOfNameOrdinals);

        for (ULONG i = 0; i < exports->NumberOfFunctions; i++)
        {
            ULONG ord = ordinals[i];
            function_name = ((char *)moduleBase) + names[i];

            if (_stricmp(function_name, pFunctionName) == 0)
            {
                pFunctionAddress = ((char *)moduleBase + functions[ord]);
                break;
            }
        }
    }

    return pFunctionAddress;
}

//////////////////////////////////////////////////////////////////////////
