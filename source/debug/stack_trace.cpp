/*!
    \file stack_trace.cpp
    \brief Stack trace snapshot provider implementation
    \author Ivan Shynkarenka
    \date 09.02.2016
    \copyright MIT License
*/

#include "debug/stack_trace.h"
#include "threads/critical_section.h"

#include <cstring>
#include <iomanip>
#include <mutex>
#include <sstream>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <DbgHelp.h>
#elif defined(unix) || defined(__unix) || defined(__unix__)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace CppCommon {

std::string StackTrace::Frame::to_string() const
{
    std::stringstream stream;
    // Format stack trace frame address
    std::ios_base::fmtflags flags = stream.flags();
    stream << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2 * sizeof(uintptr_t)) << (uintptr_t)address << ": ";
    stream.flags(flags);
    // Format stack trace frame other fields
    stream << (module.empty() ? "<unknown>" : module) << '!';
    stream << (function.empty() ? "??" : function) << ' ';
    stream << filename;
    if (line > 0)
        stream << '(' << line << ')';
    return stream.str();
}

StackTrace::StackTrace(int skip)
{
#if defined(_WIN32) || defined(_WIN64)
    const int capacity = 1024;
    void* frames[capacity];

    // Capture the current stack trace
    USHORT captured = CaptureStackBackTrace(skip + 1, capacity, frames, nullptr);

    // Resize stack trace frames vector
    _frames.resize(captured);

    // Capture stack trace snapshot under the critical section
    static CriticalSection cs;
    std::lock_guard<CriticalSection> locker(cs);

    // Get the current process handle
    HANDLE hProcess = GetCurrentProcess();

    // Fill all captured frames with symbol information
    for (int i = 0; i < captured; ++i)
    {
        auto& frame = _frames[i];

        // Get the frame address
        frame.address = frames[i];

        // Get the frame module
        IMAGEHLP_MODULE64 module;
        ZeroMemory(&module, sizeof(module));
        module.SizeOfStruct = sizeof(module);
        if (SymGetModuleInfo64(hProcess, (DWORD64)frame.address, &module))
        {
            const char* image = std::strrchr(module.ImageName, '\\');
            if (image != nullptr)
                frame.module = image + 1;
        }

        // Get the frame function
        char symbol[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        ZeroMemory(&symbol, sizeof(symbol));
        PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbol;
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        if (SymFromAddr(hProcess, (DWORD64)frame.address, nullptr, pSymbol))
        {
            char buffer[4096];
            if (UnDecorateSymbolName(pSymbol->Name, buffer, sizeof(buffer), UNDNAME_NAME_ONLY) > 0)
                frame.function = buffer;
        }

        // Get the frame file name and line number
        DWORD offset = 0;
        IMAGEHLP_LINE64 line;
        ZeroMemory(&line, sizeof(line));
        line.SizeOfStruct = sizeof(line);
        if (SymGetLineFromAddr64(hProcess, (DWORD64)frame.address, &offset, &line))
        {
            if (line.FileName != nullptr)
                frame.filename = line.FileName;
            frame.line = line.LineNumber;
        }
    }
#elif defined(unix) || defined(__unix) || defined(__unix__)
    const int capacity = 1024;
    void* frames[capacity];

    // Capture the current stack trace
    int captured = backtrace(frames, capacity);
    int index = skip + 1;
    int size = captured - index;

    // Check the current stack trace size
    if (size <= 0)
        return;

    // Resize stack trace frames vector
    _frames.resize(size);

    // Fill all captured frames with symbol information
    for (int i = 0; i < size; ++i)
    {
        auto& frame = _frames[i];

        // Get the frame address
        frame.address = frames[index + i];

        // Get the frame information
        Dl_info info;
        if (dladdr(frames[index + i], &info) == 0)
            continue;

        // Get the frame module
        if (info.dli_fname != nullptr)
        {
            const char* module = std::strrchr(info.dli_fname, '/');
            if (module != nullptr)
                frame.module = module + 1;
        }

        // Get the frame function
        if (info.dli_sname != nullptr)
        {
            // Demangle symbol name if need
            int status;
            char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);
            if ((status == 0) && (demangled != nullptr))
            {
                frame.function = demangled;
                free(demangled);
            }
            else
                frame.function = info.dli_sname;
        }
    }
#endif
}

std::string StackTrace::to_string() const
{
    std::stringstream stream;
    for (auto& frame : _frames)
        stream << frame.to_string() << std::endl;
    return stream.str();
}

} // namespace CppCommon