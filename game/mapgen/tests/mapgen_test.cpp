// DD game project
// Copyright (C) 2026 Alexander Boldyrev <boldir@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see https://www.gnu.org/licenses/.

#include "framework/Common.h"
#include "framework/FileSystem.h"
#include "game/mapgen/MapGenCmds.h"
#include "idlib/Lib.h"
#include "idlib/MapFile.h"
#include "sys/platform.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>

idCommon* common = NULL;
idFileSystem* fileSystem = NULL;

const char* idFile::GetName(void) {
    return "";
}
const char* idFile::GetFullPath(void) {
    return "";
}
int idFile::Read(void* buffer, int len) {
    return 0;
}
int idFile::Write(const void* buffer, int len) {
    return 0;
}
int idFile::Length(void) {
    return 0;
}
ID_TIME_T idFile::Timestamp(void) {
    return 0;
}
int idFile::Tell(void) {
    return 0;
}
void idFile::ForceFlush(void) { }
void idFile::Flush(void) { }
int idFile::Seek(long offset, fsOrigin_t origin) {
    return 0;
}
void idFile::Rewind(void) { }
int idFile::Printf(const char* fmt, ...) {
    return 0;
}
int idFile::VPrintf(const char* fmt, va_list arg) {
    return 0;
}
int idFile::WriteFloatString(const char* fmt, ...) {
    return 0;
}
int idFile::ReadInt(int& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadUnsignedInt(unsigned int& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadShort(short& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadUnsignedShort(unsigned short& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadChar(char& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadUnsignedChar(unsigned char& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadFloat(float& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadBool(bool& value) {
    return Read(&value, sizeof(value));
}
int idFile::ReadString(idStr& string) {
    string = "";
    return 0;
}
int idFile::ReadVec2(idVec2& vec) {
    return Read(vec.ToFloatPtr(), sizeof(float) * 2);
}
int idFile::ReadVec3(idVec3& vec) {
    return Read(vec.ToFloatPtr(), sizeof(float) * 3);
}
int idFile::ReadVec4(idVec4& vec) {
    return Read(vec.ToFloatPtr(), sizeof(float) * 4);
}
int idFile::ReadVec6(idVec6& vec) {
    return Read(vec.ToFloatPtr(), sizeof(float) * 6);
}
int idFile::ReadMat3(idMat3& mat) {
    return Read(mat.ToFloatPtr(), sizeof(float) * 9);
}
int idFile::WriteInt(const int value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteUnsignedInt(const unsigned int value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteShort(const short value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteUnsignedShort(unsigned short value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteChar(const char value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteUnsignedChar(unsigned char value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteFloat(const float value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteBool(const bool value) {
    return Write(&value, sizeof(value));
}
int idFile::WriteString(const char* string) {
    return Write(string, idStr::Length(string) + 1);
}
int idFile::WriteVec2(const idVec2& vec) {
    return Write(vec.ToFloatPtr(), sizeof(float) * 2);
}
int idFile::WriteVec3(const idVec3& vec) {
    return Write(vec.ToFloatPtr(), sizeof(float) * 3);
}
int idFile::WriteVec4(const idVec4& vec) {
    return Write(vec.ToFloatPtr(), sizeof(float) * 4);
}
int idFile::WriteVec6(const idVec6& vec) {
    return Write(vec.ToFloatPtr(), sizeof(float) * 6);
}
int idFile::WriteMat3(const idMat3& mat) {
    return Write(mat.ToFloatPtr(), sizeof(float) * 9);
}

class TestFailure : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) { }
};

static void Expect(bool condition, const char* message) {
    if (!condition) {
        throw TestFailure(message);
    }
}

static void ExpectString(const char* actual, const char* expected, const char* message) {
    if (idStr::Icmp(actual, expected) != 0) {
        throw TestFailure(std::string(message) + ": expected '" + expected + "', got '" + actual + "'");
    }
}

static void ExpectContains(const char* actual, const char* expected, const char* message) {
    if (std::string(actual).find(expected) == std::string::npos) {
        throw TestFailure(std::string(message) + ": expected '" + actual + "' to contain '" + expected + "'");
    }
}

static void ExpectNear(float actual, float expected, float epsilon, const char* message) {
    if (idMath::Fabs(actual - expected) > epsilon) {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "%s: expected %.3f, got %.3f", message, expected, actual);
        throw TestFailure(buffer);
    }
}

class TestCommon : public idCommon {
public:
    virtual void Init(int argc, char** argv) { }
    virtual void Shutdown(void) { }
    virtual void Quit(void) { }
    virtual bool IsInitialized(void) const { return true; }
    virtual void Frame(void) { }
    virtual void GUIFrame(bool execCmd, bool network) { }
    virtual void Async(void) { }
    virtual void StartupVariable(const char* match, bool once) { }
    virtual void InitTool(const toolFlag_t tool, const idDict* dict) { }
    virtual void ActivateTool(bool active) { }
    virtual void WriteConfigToFile(const char* filename) { }
    virtual void WriteFlaggedCVarsToFile(const char* filename, int flags, const char* setCmd) { }
    virtual void BeginRedirect(char* buffer, int buffersize, void (*flush)(const char*)) { }
    virtual void EndRedirect(void) { }
    virtual void SetRefreshOnPrint(bool set) { }
    virtual void Printf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stdout, fmt, args);
        va_end(args);
    }
    virtual void VPrintf(const char* fmt, va_list arg) { std::vfprintf(stdout, fmt, arg); }
    virtual void DPrintf(const char* fmt, ...) { }
    virtual void Warning(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        std::fputc('\n', stderr);
        va_end(args);
    }
    virtual void DWarning(const char* fmt, ...) { }
    virtual void PrintWarnings(void) { }
    virtual void ClearWarnings(const char* reason) { }
    virtual void Error(const char* fmt, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        throw TestFailure(buffer);
    }
    virtual void FatalError(const char* fmt, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        throw TestFailure(buffer);
    }
    virtual const idLangDict* GetLanguageDict(void) { return NULL; }
    virtual const char* KeysFromBinding(const char* bind) { return ""; }
    virtual const char* BindingFromKey(const char* key) { return ""; }
    virtual int ButtonState(int key) { return 0; }
    virtual int KeyState(int key) { return 0; }
    virtual bool SetCallback(CallbackType cbt, FunctionPointer cb, void* userArg) { return false; }
    virtual bool GetAdditionalFunction(FunctionType ft, FunctionPointer* out_fnptr, void** out_userArg) {
        if (out_fnptr != NULL) {
            *out_fnptr = NULL;
        }
        if (out_userArg != NULL) {
            *out_userArg = NULL;
        }
        return false;
    }
};

class TestFile : public idFile {
public:
    TestFile(const char* fileName, const std::string& contents, bool forWriting)
        : name(fileName)
        , data(contents)
        , position(0)
        , writable(forWriting) { }

    virtual const char* GetName(void) { return name.c_str(); }
    virtual const char* GetFullPath(void) { return name.c_str(); }
    virtual int Read(void* buffer, int len) {
        if (writable || len <= 0) {
            return 0;
        }
        int remaining = static_cast<int>(data.size()) - position;
        int count = len < remaining ? len : remaining;
        if (count > 0) {
            std::memcpy(buffer, data.data() + position, count);
            position += count;
        }
        return count;
    }
    virtual int Write(const void* buffer, int len) {
        if (!writable || len <= 0) {
            return 0;
        }
        data.append(static_cast<const char*>(buffer), len);
        position += len;
        return len;
    }
    virtual int Length(void) { return static_cast<int>(data.size()); }
    virtual ID_TIME_T Timestamp(void) { return 1; }
    virtual int Tell(void) { return position; }
    virtual void ForceFlush(void) { }
    virtual void Flush(void) { }
    virtual int Seek(long offset, fsOrigin_t origin) {
        int base = 0;
        if (origin == FS_SEEK_CUR) {
            base = position;
        } else if (origin == FS_SEEK_END) {
            base = static_cast<int>(data.size());
        }
        int next = base + static_cast<int>(offset);
        if (next < 0) {
            next = 0;
        }
        if (next > static_cast<int>(data.size())) {
            next = static_cast<int>(data.size());
        }
        position = next;
        return position;
    }
    virtual void Rewind(void) { position = 0; }
    virtual int Printf(const char* fmt, ...) {
        char buffer[8192];
        va_list args;
        va_start(args, fmt);
        int count = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        if (count > 0) {
            Write(buffer, count);
        }
        return count;
    }
    virtual int VPrintf(const char* fmt, va_list arg) {
        char buffer[8192];
        int count = std::vsnprintf(buffer, sizeof(buffer), fmt, arg);
        if (count > 0) {
            Write(buffer, count);
        }
        return count;
    }
    virtual int WriteFloatString(const char* fmt, ...) {
        char buffer[8192];
        va_list args;
        va_start(args, fmt);
        int count = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        if (count > 0) {
            Write(buffer, count);
        }
        return count;
    }

    virtual int ReadInt(int& value) { return Read(&value, sizeof(value)); }
    virtual int ReadUnsignedInt(unsigned int& value) { return Read(&value, sizeof(value)); }
    virtual int ReadShort(short& value) { return Read(&value, sizeof(value)); }
    virtual int ReadUnsignedShort(unsigned short& value) { return Read(&value, sizeof(value)); }
    virtual int ReadChar(char& value) { return Read(&value, sizeof(value)); }
    virtual int ReadUnsignedChar(unsigned char& value) { return Read(&value, sizeof(value)); }
    virtual int ReadFloat(float& value) { return Read(&value, sizeof(value)); }
    virtual int ReadBool(bool& value) { return Read(&value, sizeof(value)); }
    virtual int ReadString(idStr& string) { return 0; }
    virtual int ReadVec2(idVec2& vec) { return Read(vec.ToFloatPtr(), sizeof(float) * 2); }
    virtual int ReadVec3(idVec3& vec) { return Read(vec.ToFloatPtr(), sizeof(float) * 3); }
    virtual int ReadVec4(idVec4& vec) { return Read(vec.ToFloatPtr(), sizeof(float) * 4); }
    virtual int ReadVec6(idVec6& vec) { return Read(vec.ToFloatPtr(), sizeof(float) * 6); }
    virtual int ReadMat3(idMat3& mat) { return Read(mat.ToFloatPtr(), sizeof(float) * 9); }

    virtual int WriteInt(const int value) { return Write(&value, sizeof(value)); }
    virtual int WriteUnsignedInt(const unsigned int value) { return Write(&value, sizeof(value)); }
    virtual int WriteShort(const short value) { return Write(&value, sizeof(value)); }
    virtual int WriteUnsignedShort(unsigned short value) { return Write(&value, sizeof(value)); }
    virtual int WriteChar(const char value) { return Write(&value, sizeof(value)); }
    virtual int WriteUnsignedChar(unsigned char value) { return Write(&value, sizeof(value)); }
    virtual int WriteFloat(const float value) { return Write(&value, sizeof(value)); }
    virtual int WriteBool(const bool value) { return Write(&value, sizeof(value)); }
    virtual int WriteString(const char* string) { return Write(string, idStr::Length(string) + 1); }
    virtual int WriteVec2(const idVec2& vec) { return Write(vec.ToFloatPtr(), sizeof(float) * 2); }
    virtual int WriteVec3(const idVec3& vec) { return Write(vec.ToFloatPtr(), sizeof(float) * 3); }
    virtual int WriteVec4(const idVec4& vec) { return Write(vec.ToFloatPtr(), sizeof(float) * 4); }
    virtual int WriteVec6(const idVec6& vec) { return Write(vec.ToFloatPtr(), sizeof(float) * 6); }
    virtual int WriteMat3(const idMat3& mat) { return Write(mat.ToFloatPtr(), sizeof(float) * 9); }

    bool IsWritable(void) const { return writable; }
    const std::string& GetData(void) const { return data; }

private:
    std::string name;
    std::string data;
    int position;
    bool writable;
};

class TestFileSystem : public idFileSystem {
public:
    TestFileSystem()
        : failWrites(false) { }

    void AddFile(const char* path, const char* contents) { files[path] = contents; }
    void SetFailWrites(bool fail) { failWrites = fail; }

    const char* GetFileContents(const char* path) const {
        std::map<std::string, std::string>::const_iterator it = files.find(path);
        return it == files.end() ? NULL : it->second.c_str();
    }

    virtual void Init(void) { }
    virtual void Restart(void) { }
    virtual void Shutdown(bool reloading) { }
    virtual bool IsInitialized(void) const { return true; }
    virtual bool PerformingCopyFiles(void) const { return false; }
    virtual idModList* ListMods(void) { return NULL; }
    virtual void FreeModList(idModList* modList) { }
    virtual idFileList* ListFiles(const char* relativePath, const char* extension, bool sort = false,
        bool fullRelativePath = false, const char* gamedir = NULL) {
        return NULL;
    }
    virtual idFileList* ListFilesTree(
        const char* relativePath, const char* extension, bool sort = false, const char* gamedir = NULL) {
        return NULL;
    }
    virtual void FreeFileList(idFileList* fileList) { }
    virtual const char* OSPathToRelativePath(const char* OSPath) { return OSPath; }
    virtual const char* RelativePathToOSPath(const char* relativePath, const char* basePath = "fs_devpath") {
        return relativePath;
    }
    virtual const char* BuildOSPath(const char* base, const char* game, const char* relativePath) {
        return relativePath;
    }
    virtual void CreateOSPath(const char* OSPath) { }
    virtual bool FileIsInPAK(const char* relativePath) { return false; }
    virtual void UpdatePureServerChecksums(void) { }
    virtual fsPureReply_t SetPureServerChecksums(
        const int pureChecksums[MAX_PURE_PAKS], int missingChecksums[MAX_PURE_PAKS]) {
        return PURE_OK;
    }
    virtual void GetPureServerChecksums(int checksums[MAX_PURE_PAKS]) { checksums[0] = 0; }
    virtual void SetRestartChecksums(const int pureChecksums[MAX_PURE_PAKS]) { }
    virtual void ClearPureChecksums(void) { }
    virtual int ReadFile(const char* relativePath, void** buffer, ID_TIME_T* timestamp = NULL) {
        std::map<std::string, std::string>::const_iterator it = files.find(relativePath);
        if (it == files.end()) {
            return -1;
        }
        if (timestamp != NULL) {
            *timestamp = 1;
        }
        if (buffer != NULL) {
            char* copy = static_cast<char*>(std::malloc(it->second.size() + 1));
            std::memcpy(copy, it->second.data(), it->second.size());
            copy[it->second.size()] = '\0';
            *buffer = copy;
        }
        return static_cast<int>(it->second.size());
    }
    virtual void FreeFile(void* buffer) { std::free(buffer); }
    virtual int WriteFile(
        const char* relativePath, const void* buffer, int size, const char* basePath = "fs_savepath") {
        files[relativePath] = std::string(static_cast<const char*>(buffer), size);
        return size;
    }
    virtual void RemoveFile(const char* relativePath) { files.erase(relativePath); }
    virtual idFile* OpenFileRead(const char* relativePath, bool allowCopyFiles = true, const char* gamedir = NULL) {
        std::map<std::string, std::string>::const_iterator it = files.find(relativePath);
        return it == files.end() ? NULL : new TestFile(relativePath, it->second, false);
    }
    virtual idFile* OpenFileWrite(const char* relativePath, const char* basePath = "fs_savepath") {
        if (failWrites) {
            return NULL;
        }
        return new TestFile(relativePath, "", true);
    }
    virtual idFile* OpenFileAppend(const char* filename, bool sync = false, const char* basePath = "fs_basepath") {
        return NULL;
    }
    virtual idFile* OpenFileByMode(const char* relativePath, fsMode_t mode) {
        if (mode == FS_READ) {
            return OpenFileRead(relativePath);
        }
        if (mode == FS_WRITE) {
            return OpenFileWrite(relativePath);
        }
        return NULL;
    }
    virtual idFile* OpenExplicitFileRead(const char* OSPath) { return OpenFileRead(OSPath); }
    virtual idFile* OpenExplicitFileWrite(const char* OSPath) { return OpenFileWrite(OSPath); }
    virtual void CloseFile(idFile* f) {
        TestFile* testFile = static_cast<TestFile*>(f);
        if (testFile->IsWritable()) {
            files[testFile->GetName()] = testFile->GetData();
        }
        delete testFile;
    }
    virtual void BackgroundDownload(backgroundDownload_t* bgl) { }
    virtual void ResetReadCount(void) { }
    virtual int GetReadCount(void) { return 0; }
    virtual void AddToReadCount(int c) { }
    virtual void FindDLL(const char* basename, char dllPath[MAX_OSPATH]) { dllPath[0] = '\0'; }
    virtual void ClearDirCache(void) { }
    virtual bool HasD3XP(void) { return false; }
    virtual bool RunningD3XP(void) { return false; }
    virtual void CopyFile(const char* fromOSPath, const char* toOSPath) { }
    virtual int ValidateDownloadPakForChecksum(int checksum, char path[MAX_STRING_CHARS]) { return 0; }
    virtual idFile* MakeTemporaryFile(void) { return new TestFile("<temporary>", "", true); }
    virtual int AddZipFile(const char* path) { return 0; }
    virtual findFile_t FindFile(const char* path, bool scheduleAddons = false) {
        return files.find(path) == files.end() ? FIND_NO : FIND_YES;
    }
    virtual int GetNumMaps() { return 0; }
    virtual const idDict* GetMapDecl(int i) { return NULL; }
    virtual void FindMapScreenshot(const char* path, char* buf, int len) {
        if (len > 0) {
            buf[0] = '\0';
        }
    }
    virtual bool FilenameCompare(const char* s1, const char* s2) const { return idStr::IcmpPath(s1, s2) == 0; }

private:
    std::map<std::string, std::string> files;
    bool failWrites;
};

static const char* MAPGEN_GATE2_MAP
    = "Version 2\n"
      "{\n"
      "\"classname\" \"worldspawn\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( 1 0 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( -1 0 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n"
      "{\n"
      "\"classname\" \"func_static\"\n"
      "\"name\" \"slot_0\"\n"
      "\"model\" \"slot_0\"\n"
      "\"origin\" \"0 0 0\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
      "  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n"
      "{\n"
      "\"classname\" \"info_null\"\n"
      "\"name\" \"gate_marker\"\n"
      "\"model\" \"models/mapobjects/test.lwo\"\n"
      "\"target\" \"gate_marker\"\n"
      "\"team\" \"gate_team\"\n"
      "\"angle\" \"0\"\n"
      "\"origin\" \"64 0 0\"\n"
      "}\n";

static const char* MAPGEN_TESTGG_MAP
    = "Version 2\n"
      "{\n"
      "\"classname\" \"worldspawn\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( 1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n"
      "{\n"
      "\"classname\" \"func_static\"\n"
      "\"name\" \"slot_gg0\"\n"
      "\"model\" \"slot_gg0\"\n"
      "\"origin\" \"256 0 0\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( -1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
      "  ( 1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n"
      "{\n"
      "\"classname\" \"func_static\"\n"
      "\"name\" \"slot_gg1\"\n"
      "\"model\" \"slot_gg1\"\n"
      "\"origin\" \"-256 0 0\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
      "  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n"
      "{\n"
      "\"classname\" \"info_null\"\n"
      "\"name\" \"testgg_marker\"\n"
      "\"target\" \"testgg_marker\"\n"
      "\"team\" \"testgg_team\"\n"
      "\"origin\" \"0 64 0\"\n"
      "}\n";

static const char* MAPGEN_TESTGG_MISSING_SLOT_MAP
    = "Version 2\n"
      "{\n"
      "\"classname\" \"worldspawn\"\n"
      "}\n"
      "{\n"
      "\"classname\" \"func_static\"\n"
      "\"name\" \"slot_gg0\"\n"
      "\"origin\" \"256 0 0\"\n"
      "{\n"
      " brushDef3\n"
      " {\n"
      "  ( -1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
      "  ( 1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 -1 0 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
      " }\n"
      "}\n"
      "}\n";

static void AddGate2TestggFiles(TestFileSystem& testFileSystem) {
    testFileSystem.AddFile("maps/mapgen/gate2.map", MAPGEN_GATE2_MAP);
    testFileSystem.AddFile("maps/mapgen/testgg.map", MAPGEN_TESTGG_MAP);
}

static void RunMapGenJoinPlanTest(TestFileSystem& testFileSystem) {
    testFileSystem.RemoveFile("maps/mapgen/current.map");
    AddGate2TestggFiles(testFileSystem);

    const char* outputMapName = "maps/mapgen/current.map";
    idStr status;
    Expect(MapGen_Join("gate2_testgg", outputMapName, status), status.c_str());
    ExpectContains(status.c_str(), "2 joins", "unexpected success status");

    idMapFile generatedMap;
    Expect(generatedMap.Parse("maps/mapgen/current", true), "generated join-plan map could not be parsed");
    Expect(generatedMap.GetNumEntities() == 8, "expected testgg and two gate instances");
    Expect(
        generatedMap.GetEntity(0)->GetNumPrimitives() == 3, "expected merged worldspawn primitives from all instances");

    idMapEntity* testggMarker = generatedMap.FindEntity("m0__testgg_marker");
    idMapEntity* firstSlot = generatedMap.FindEntity("m1__slot_0");
    idMapEntity* firstMarker = generatedMap.FindEntity("m1__gate_marker");
    idMapEntity* secondSlot = generatedMap.FindEntity("m2__slot_0");
    idMapEntity* secondMarker = generatedMap.FindEntity("m2__gate_marker");
    Expect(testggMarker != NULL, "missing prefixed testgg marker");
    Expect(firstSlot != NULL && firstMarker != NULL, "missing first gate instance");
    Expect(secondSlot != NULL && secondMarker != NULL, "missing second gate instance");
    Expect(generatedMap.FindEntity("m0__slot_gg0") != NULL, "missing first prefixed destination slot");
    Expect(generatedMap.FindEntity("m0__slot_gg1") != NULL, "missing second prefixed destination slot");
    ExpectString(generatedMap.FindEntity("m0__slot_gg0")->epairs.GetString("model"), "m0__slot_gg0",
        "unexpected first destination slot model");
    ExpectString(generatedMap.FindEntity("m0__slot_gg1")->epairs.GetString("model"), "m0__slot_gg1",
        "unexpected second destination slot model");
    ExpectString(firstSlot->epairs.GetString("model"), "m1__slot_0", "unexpected first gate slot model");
    ExpectString(secondSlot->epairs.GetString("model"), "m2__slot_0", "unexpected second gate slot model");

    ExpectString(testggMarker->epairs.GetString("target"), "m0__testgg_marker", "unexpected testgg target");
    ExpectString(testggMarker->epairs.GetString("team"), "m0__testgg_team", "unexpected testgg team");
    ExpectString(firstMarker->epairs.GetString("target"), "m1__gate_marker", "unexpected first gate target");
    ExpectString(firstMarker->epairs.GetString("team"), "m1__gate_team", "unexpected first gate team");
    ExpectString(firstMarker->epairs.GetString("model"), "models/mapobjects/test.lwo",
        "external model path was unexpectedly prefixed");
    ExpectString(secondMarker->epairs.GetString("target"), "m2__gate_marker", "unexpected second gate target");
    ExpectString(secondMarker->epairs.GetString("team"), "m2__gate_team", "unexpected second gate team");

    idVec3 firstSlotOrigin;
    idVec3 firstMarkerOrigin;
    idVec3 secondSlotOrigin;
    idVec3 secondMarkerOrigin;
    firstSlot->epairs.GetVector("origin", "0 0 0", firstSlotOrigin);
    firstMarker->epairs.GetVector("origin", "0 0 0", firstMarkerOrigin);
    secondSlot->epairs.GetVector("origin", "0 0 0", secondSlotOrigin);
    secondMarker->epairs.GetVector("origin", "0 0 0", secondMarkerOrigin);
    ExpectNear(firstSlotOrigin.x, 256.0f, 0.01f, "first gate slot origin x");
    ExpectNear(firstMarkerOrigin.x, 320.0f, 0.01f, "first gate marker origin x");
    ExpectNear(secondSlotOrigin.x, -256.0f, 0.01f, "second gate slot origin x");
    ExpectNear(secondMarkerOrigin.x, -320.0f, 0.01f, "second gate marker origin x");

    float secondAngle;
    Expect(secondMarker->epairs.GetFloat("angle", "0", secondAngle), "second gate angle is missing");
    ExpectNear(secondAngle, 180.0f, 0.01f, "second gate angle");
}

static void RunMapGenPlanFailureTests(TestFileSystem& testFileSystem) {
    const char* outputMapName = "maps/mapgen/current.map";
    idStr status;

    testFileSystem.RemoveFile("maps/mapgen/current.map");
    Expect(!MapGen_Join("missing_plan", outputMapName, status), "unknown plan unexpectedly succeeded");
    ExpectContains(status.c_str(), "unknown mapgen plan", "unexpected unknown-plan status");

    testFileSystem.RemoveFile("maps/mapgen/gate2.map");
    testFileSystem.AddFile("maps/mapgen/testgg.map", MAPGEN_TESTGG_MAP);
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "missing source map unexpectedly succeeded");
    ExpectContains(status.c_str(), "maps/mapgen/gate2.map", "unexpected missing-source status");

    testFileSystem.AddFile("maps/mapgen/gate2.map", MAPGEN_GATE2_MAP);
    testFileSystem.AddFile("maps/mapgen/testgg.map", MAPGEN_TESTGG_MISSING_SLOT_MAP);
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "missing destination slot unexpectedly succeeded");
    ExpectContains(status.c_str(), "slot_gg1", "unexpected missing-slot status");
    Expect(testFileSystem.GetFileContents("maps/mapgen/current.map") == NULL, "failed plan wrote an output map");

    std::string horizontalGate = MAPGEN_GATE2_MAP;
    std::string slotEntity = "\"name\" \"slot_0\"";
    std::string verticalSide = "( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\"";
    std::string horizontalSide = "( 0 0 1 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\"";
    std::string::size_type slotOffset = horizontalGate.find(slotEntity);
    std::string::size_type sideOffset = horizontalGate.find(verticalSide, slotOffset);
    Expect(
        slotOffset != std::string::npos && sideOffset != std::string::npos, "could not build horizontal gate fixture");
    horizontalGate.replace(sideOffset, verticalSide.size(), horizontalSide);
    testFileSystem.AddFile("maps/mapgen/testgg.map", MAPGEN_TESTGG_MAP);
    testFileSystem.AddFile("maps/mapgen/gate2.map", horizontalGate.c_str());
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "horizontal source slot unexpectedly succeeded");
    ExpectContains(status.c_str(), "face must be vertical", "unexpected horizontal-slot status");

    std::string multipleFaceGate = MAPGEN_GATE2_MAP;
    std::string oppositeSide = "( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\"";
    std::string secondSlotSide = "( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\"";
    slotOffset = multipleFaceGate.find(slotEntity);
    sideOffset = multipleFaceGate.find(oppositeSide, slotOffset);
    Expect(slotOffset != std::string::npos && sideOffset != std::string::npos,
        "could not build multiple-face gate fixture");
    multipleFaceGate.replace(sideOffset, oppositeSide.size(), secondSlotSide);
    testFileSystem.AddFile("maps/mapgen/gate2.map", multipleFaceGate.c_str());
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "multiple source slot faces unexpectedly succeeded");
    ExpectContains(status.c_str(), "multiple 'textures/common/mapgen_slot' faces", "unexpected multiple-face status");

    std::string openSlotGate = MAPGEN_GATE2_MAP;
    std::string missingSide = "  ( 0 0 -1 -32 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n";
    slotOffset = openSlotGate.find(slotEntity);
    sideOffset = openSlotGate.find(missingSide, slotOffset);
    Expect(
        slotOffset != std::string::npos && sideOffset != std::string::npos, "could not build open-slot gate fixture");
    openSlotGate.erase(sideOffset, missingSide.size());
    testFileSystem.AddFile("maps/mapgen/gate2.map", openSlotGate.c_str());
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "open source slot brush unexpectedly succeeded");
    ExpectContains(status.c_str(), "face must form a finite brush polygon", "unexpected open-slot status");

    AddGate2TestggFiles(testFileSystem);
    testFileSystem.SetFailWrites(true);
    status.Clear();
    Expect(!MapGen_Join("gate2_testgg", outputMapName, status), "output write failure unexpectedly succeeded");
    testFileSystem.SetFailWrites(false);
    ExpectContains(status.c_str(), "could not write", "unexpected write-failure status");
}

int main(int argc, char** argv) {
    TestCommon testCommon;
    TestFileSystem testFileSystem;
    bool idLibInitialized = false;

    common = &testCommon;
    fileSystem = &testFileSystem;
    idLib::common = &testCommon;
    idLib::fileSystem = &testFileSystem;

    try {
        idLib::Init();
        idLibInitialized = true;
        RunMapGenJoinPlanTest(testFileSystem);
        RunMapGenPlanFailureTests(testFileSystem);
    } catch (const std::exception& ex) {
        if (idLibInitialized) {
            idLib::ShutDown();
        }
        std::fprintf(stderr, "mapgen_test failed: %s\n", ex.what());
        return 1;
    }

    idLib::ShutDown();
    std::printf("mapgen_test passed\n");
    return 0;
}
