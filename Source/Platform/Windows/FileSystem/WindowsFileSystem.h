#pragma once

#include <cstddef>
#include <span>
#include <Windows.h>
#include <winternl.h>

#include <Platform/Windows/Syscalls/WindowsSyscalls.h>

struct WindowsFileSystem {
    static HANDLE openFileForReading(UNICODE_STRING fileName) noexcept
    {
        HANDLE handle;
        IO_STATUS_BLOCK statusBlock{};
        OBJECT_ATTRIBUTES objectAttributes{
            .Length = sizeof(OBJECT_ATTRIBUTES),
            .RootDirectory = nullptr,
            .ObjectName = &fileName,
            .Attributes = OBJ_CASE_INSENSITIVE,
            .SecurityDescriptor = nullptr,
            .SecurityQualityOfService = nullptr
        };
        if (NT_SUCCESS(WindowsSyscalls::NtCreateFile(&handle, FILE_GENERIC_READ, &objectAttributes, &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0)))
            return handle;
        return INVALID_HANDLE_VALUE;
    }

    static HANDLE createFileForOverwrite(std::wstring_view ntPath) noexcept
    {
        HANDLE handle;
        IO_STATUS_BLOCK statusBlock{};
        UNICODE_STRING fileName{
            .Length = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
            .MaximumLength = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
            .Buffer = const_cast<wchar_t*>(ntPath.data())
        };
        OBJECT_ATTRIBUTES objectAttributes{
            .Length = sizeof(OBJECT_ATTRIBUTES),
            .RootDirectory = nullptr,
            .ObjectName = &fileName,
            .Attributes = OBJ_CASE_INSENSITIVE,
            .SecurityDescriptor = nullptr,
            .SecurityQualityOfService = nullptr
        };
        if (NT_SUCCESS(WindowsSyscalls::NtCreateFile(&handle, FILE_GENERIC_WRITE | DELETE, &objectAttributes, &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0)))
            return handle;
        return INVALID_HANDLE_VALUE;
    }

    static void createDirectory(std::wstring_view ntPath) noexcept
    {
        IO_STATUS_BLOCK statusBlock{};
        UNICODE_STRING directoryName{
            .Length = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
            .MaximumLength = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
            .Buffer = const_cast<wchar_t*>(ntPath.data())
        };
        OBJECT_ATTRIBUTES objectAttributes{
            .Length = sizeof(OBJECT_ATTRIBUTES),
            .RootDirectory = nullptr,
            .ObjectName = &directoryName,
            .Attributes = OBJ_CASE_INSENSITIVE,
            .SecurityDescriptor = nullptr,
            .SecurityQualityOfService = nullptr
        };
        
        HANDLE handle;
        auto ntStatus = WindowsSyscalls::NtCreateFile(&handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &objectAttributes, &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_CREATE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, nullptr, 0);
        if (NT_SUCCESS(ntStatus))
            WindowsSyscalls::NtClose(handle);
    }

    // static void deleteFile(std::wstring_view ntPath) noexcept
    // {
    //     IO_STATUS_BLOCK statusBlock{};
    //     UNICODE_STRING directoryName{
    //         .Length = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
    //         .MaximumLength = static_cast<USHORT>(ntPath.length() * sizeof(wchar_t)),
    //         .Buffer = const_cast<wchar_t*>(ntPath.data())
    //     };
    //     OBJECT_ATTRIBUTES objectAttributes{
    //         .Length = sizeof(OBJECT_ATTRIBUTES),
    //         .RootDirectory = nullptr,
    //         .ObjectName = &directoryName,
    //         .Attributes = OBJ_CASE_INSENSITIVE,
    //         .SecurityDescriptor = nullptr,
    //         .SecurityQualityOfService = nullptr
    //     };
    //     if (HANDLE handle; NT_SUCCESS(WindowsSyscalls::NtCreateFile(&handle, DELETE, &objectAttributes, &statusBlock, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE, nullptr, 0)))
    //         WindowsSyscalls::NtClose(handle);
    // }

    static std::size_t readFile(HANDLE fileHandle, std::size_t fileOffset, void* buffer, std::size_t bufferSize) noexcept
    {
        IO_STATUS_BLOCK statusBlock{};
        LARGE_INTEGER offset{.QuadPart{static_cast<LONGLONG>(fileOffset)}};
        if (NT_SUCCESS(WindowsSyscalls::NtReadFile(fileHandle, nullptr, nullptr, nullptr, &statusBlock, buffer, static_cast<ULONG>(bufferSize), &offset, nullptr)) && statusBlock.Information <= bufferSize) {
            return statusBlock.Information;
        }
        return 0;
    }

    static std::size_t writeFile(HANDLE fileHandle, std::size_t fileOffset, void* buffer, std::size_t bufferSize) noexcept
    {
        IO_STATUS_BLOCK statusBlock{};
        LARGE_INTEGER offset{.QuadPart{static_cast<LONGLONG>(fileOffset)}};
        if (NT_SUCCESS(WindowsSyscalls::NtWriteFile(fileHandle, nullptr, nullptr, nullptr, &statusBlock, buffer, static_cast<ULONG>(bufferSize), &offset, nullptr)) && statusBlock.Information <= bufferSize) {
            return statusBlock.Information;
        }
        return 0;
    }

    static void renameFile(HANDLE fileHandle, std::wstring_view newFilePath) noexcept
    {
        IO_STATUS_BLOCK statusBlock{};

        constexpr auto kMaxPathLength{200};
        if (newFilePath.length() > kMaxPathLength) {
            assert(false);
            return;
        }

        alignas(FILE_RENAME_INFO) std::byte buffer[sizeof(FILE_RENAME_INFO) + kMaxPathLength * sizeof(wchar_t)];

        FILE_RENAME_INFO* info = new (&buffer) FILE_RENAME_INFO{};
        info->ReplaceIfExists = TRUE;
        info->FileNameLength = static_cast<DWORD>(newFilePath.length() * sizeof(wchar_t));
        std::memcpy(&info->FileName, newFilePath.data(), newFilePath.length() * sizeof(wchar_t));
        constexpr wchar_t kNullTerminator{L'\0'};
        std::memcpy(reinterpret_cast<std::byte*>(&info->FileName) + newFilePath.length() * sizeof(wchar_t), &kNullTerminator, sizeof(kNullTerminator));

        constexpr std::underlying_type_t<FILE_INFORMATION_CLASS> fileRenameInformationClass{10};
        FILE_INFORMATION_CLASS fileRenameInformationClassEnum{};
        std::memcpy(&fileRenameInformationClassEnum, &fileRenameInformationClass, sizeof(fileRenameInformationClass));
        WindowsSyscalls::NtSetInformationFile(fileHandle, &statusBlock, info, sizeof(FILE_RENAME_INFO) + kMaxPathLength * sizeof(wchar_t), fileRenameInformationClassEnum);
    }
};
