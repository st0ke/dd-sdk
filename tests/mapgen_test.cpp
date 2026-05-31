/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2026 st0ke <boldir@gmail.com>

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/Lib.h"
#include "idlib/MapFile.h"
#include "framework/Common.h"
#include "framework/FileSystem.h"
#include "game/mapgen/MapGen.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>

idCommon *common = NULL;
idFileSystem *fileSystem = NULL;

const char *idFile::GetName( void ) { return ""; }
const char *idFile::GetFullPath( void ) { return ""; }
int idFile::Read( void *buffer, int len ) { return 0; }
int idFile::Write( const void *buffer, int len ) { return 0; }
int idFile::Length( void ) { return 0; }
ID_TIME_T idFile::Timestamp( void ) { return 0; }
int idFile::Tell( void ) { return 0; }
void idFile::ForceFlush( void ) {}
void idFile::Flush( void ) {}
int idFile::Seek( long offset, fsOrigin_t origin ) { return 0; }
void idFile::Rewind( void ) {}
int idFile::Printf( const char *fmt, ... ) { return 0; }
int idFile::VPrintf( const char *fmt, va_list arg ) { return 0; }
int idFile::WriteFloatString( const char *fmt, ... ) { return 0; }
int idFile::ReadInt( int &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadUnsignedInt( unsigned int &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadShort( short &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadUnsignedShort( unsigned short &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadChar( char &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadUnsignedChar( unsigned char &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadFloat( float &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadBool( bool &value ) { return Read( &value, sizeof( value ) ); }
int idFile::ReadString( idStr &string ) { string = ""; return 0; }
int idFile::ReadVec2( idVec2 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 2 ); }
int idFile::ReadVec3( idVec3 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 3 ); }
int idFile::ReadVec4( idVec4 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 4 ); }
int idFile::ReadVec6( idVec6 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 6 ); }
int idFile::ReadMat3( idMat3 &mat ) { return Read( mat.ToFloatPtr(), sizeof( float ) * 9 ); }
int idFile::WriteInt( const int value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteUnsignedInt( const unsigned int value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteShort( const short value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteUnsignedShort( unsigned short value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteChar( const char value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteUnsignedChar( unsigned char value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteFloat( const float value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteBool( const bool value ) { return Write( &value, sizeof( value ) ); }
int idFile::WriteString( const char *string ) { return Write( string, idStr::Length( string ) + 1 ); }
int idFile::WriteVec2( const idVec2 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 2 ); }
int idFile::WriteVec3( const idVec3 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 3 ); }
int idFile::WriteVec4( const idVec4 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 4 ); }
int idFile::WriteVec6( const idVec6 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 6 ); }
int idFile::WriteMat3( const idMat3 &mat ) { return Write( mat.ToFloatPtr(), sizeof( float ) * 9 ); }

class TestFailure : public std::runtime_error {
public:
	explicit TestFailure( const std::string &message ) : std::runtime_error( message ) {}
};

static void Expect( bool condition, const char *message ) {
	if ( !condition ) {
		throw TestFailure( message );
	}
}

static void ExpectString( const char *actual, const char *expected, const char *message ) {
	if ( idStr::Icmp( actual, expected ) != 0 ) {
		throw TestFailure( std::string( message ) + ": expected '" + expected + "', got '" + actual + "'" );
	}
}

static void ExpectContains( const char *actual, const char *expected, const char *message ) {
	if ( std::string( actual ).find( expected ) == std::string::npos ) {
		throw TestFailure( std::string( message ) + ": expected '" + actual + "' to contain '" + expected + "'" );
	}
}

static void ExpectNear( float actual, float expected, float epsilon, const char *message ) {
	if ( idMath::Fabs( actual - expected ) > epsilon ) {
		char buffer[256];
		std::snprintf( buffer, sizeof( buffer ), "%s: expected %.3f, got %.3f", message, expected, actual );
		throw TestFailure( buffer );
	}
}

class TestCommon : public idCommon {
public:
	virtual void Init( int argc, char **argv ) {}
	virtual void Shutdown( void ) {}
	virtual void Quit( void ) {}
	virtual bool IsInitialized( void ) const { return true; }
	virtual void Frame( void ) {}
	virtual void GUIFrame( bool execCmd, bool network ) {}
	virtual void Async( void ) {}
	virtual void StartupVariable( const char *match, bool once ) {}
	virtual void InitTool( const toolFlag_t tool, const idDict *dict ) {}
	virtual void ActivateTool( bool active ) {}
	virtual void WriteConfigToFile( const char *filename ) {}
	virtual void WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) {}
	virtual void BeginRedirect( char *buffer, int buffersize, void ( *flush )( const char * ) ) {}
	virtual void EndRedirect( void ) {}
	virtual void SetRefreshOnPrint( bool set ) {}
	virtual void Printf( const char *fmt, ... ) {
		va_list args;
		va_start( args, fmt );
		std::vfprintf( stdout, fmt, args );
		va_end( args );
	}
	virtual void VPrintf( const char *fmt, va_list arg ) { std::vfprintf( stdout, fmt, arg ); }
	virtual void DPrintf( const char *fmt, ... ) {}
	virtual void Warning( const char *fmt, ... ) {
		va_list args;
		va_start( args, fmt );
		std::vfprintf( stderr, fmt, args );
		std::fputc( '\n', stderr );
		va_end( args );
	}
	virtual void DWarning( const char *fmt, ... ) {}
	virtual void PrintWarnings( void ) {}
	virtual void ClearWarnings( const char *reason ) {}
	virtual void Error( const char *fmt, ... ) {
		char buffer[4096];
		va_list args;
		va_start( args, fmt );
		std::vsnprintf( buffer, sizeof( buffer ), fmt, args );
		va_end( args );
		throw TestFailure( buffer );
	}
	virtual void FatalError( const char *fmt, ... ) {
		char buffer[4096];
		va_list args;
		va_start( args, fmt );
		std::vsnprintf( buffer, sizeof( buffer ), fmt, args );
		va_end( args );
		throw TestFailure( buffer );
	}
	virtual const idLangDict *GetLanguageDict( void ) { return NULL; }
	virtual const char *KeysFromBinding( const char *bind ) { return ""; }
	virtual const char *BindingFromKey( const char *key ) { return ""; }
	virtual int ButtonState( int key ) { return 0; }
	virtual int KeyState( int key ) { return 0; }
	virtual bool SetCallback( CallbackType cbt, FunctionPointer cb, void *userArg ) { return false; }
	virtual bool GetAdditionalFunction( FunctionType ft, FunctionPointer *out_fnptr, void **out_userArg ) {
		if ( out_fnptr != NULL ) {
			*out_fnptr = NULL;
		}
		if ( out_userArg != NULL ) {
			*out_userArg = NULL;
		}
		return false;
	}
};

class TestFile : public idFile {
public:
	TestFile( const char *fileName, const std::string &contents, bool forWriting ) :
		name( fileName ),
		data( contents ),
		position( 0 ),
		writable( forWriting ) {
	}

	virtual const char *GetName( void ) { return name.c_str(); }
	virtual const char *GetFullPath( void ) { return name.c_str(); }
	virtual int Read( void *buffer, int len ) {
		if ( writable || len <= 0 ) {
			return 0;
		}
		int remaining = static_cast<int>( data.size() ) - position;
		int count = len < remaining ? len : remaining;
		if ( count > 0 ) {
			std::memcpy( buffer, data.data() + position, count );
			position += count;
		}
		return count;
	}
	virtual int Write( const void *buffer, int len ) {
		if ( !writable || len <= 0 ) {
			return 0;
		}
		data.append( static_cast<const char *>( buffer ), len );
		position += len;
		return len;
	}
	virtual int Length( void ) { return static_cast<int>( data.size() ); }
	virtual ID_TIME_T Timestamp( void ) { return 1; }
	virtual int Tell( void ) { return position; }
	virtual void ForceFlush( void ) {}
	virtual void Flush( void ) {}
	virtual int Seek( long offset, fsOrigin_t origin ) {
		int base = 0;
		if ( origin == FS_SEEK_CUR ) {
			base = position;
		} else if ( origin == FS_SEEK_END ) {
			base = static_cast<int>( data.size() );
		}
		int next = base + static_cast<int>( offset );
		if ( next < 0 ) {
			next = 0;
		}
		if ( next > static_cast<int>( data.size() ) ) {
			next = static_cast<int>( data.size() );
		}
		position = next;
		return position;
	}
	virtual void Rewind( void ) { position = 0; }
	virtual int Printf( const char *fmt, ... ) {
		char buffer[8192];
		va_list args;
		va_start( args, fmt );
		int count = std::vsnprintf( buffer, sizeof( buffer ), fmt, args );
		va_end( args );
		if ( count > 0 ) {
			Write( buffer, count );
		}
		return count;
	}
	virtual int VPrintf( const char *fmt, va_list arg ) {
		char buffer[8192];
		int count = std::vsnprintf( buffer, sizeof( buffer ), fmt, arg );
		if ( count > 0 ) {
			Write( buffer, count );
		}
		return count;
	}
	virtual int WriteFloatString( const char *fmt, ... ) {
		char buffer[8192];
		va_list args;
		va_start( args, fmt );
		int count = std::vsnprintf( buffer, sizeof( buffer ), fmt, args );
		va_end( args );
		if ( count > 0 ) {
			Write( buffer, count );
		}
		return count;
	}

	virtual int ReadInt( int &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadUnsignedInt( unsigned int &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadShort( short &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadUnsignedShort( unsigned short &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadChar( char &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadUnsignedChar( unsigned char &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadFloat( float &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadBool( bool &value ) { return Read( &value, sizeof( value ) ); }
	virtual int ReadString( idStr &string ) { return 0; }
	virtual int ReadVec2( idVec2 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 2 ); }
	virtual int ReadVec3( idVec3 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 3 ); }
	virtual int ReadVec4( idVec4 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 4 ); }
	virtual int ReadVec6( idVec6 &vec ) { return Read( vec.ToFloatPtr(), sizeof( float ) * 6 ); }
	virtual int ReadMat3( idMat3 &mat ) { return Read( mat.ToFloatPtr(), sizeof( float ) * 9 ); }

	virtual int WriteInt( const int value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteUnsignedInt( const unsigned int value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteShort( const short value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteUnsignedShort( unsigned short value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteChar( const char value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteUnsignedChar( unsigned char value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteFloat( const float value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteBool( const bool value ) { return Write( &value, sizeof( value ) ); }
	virtual int WriteString( const char *string ) { return Write( string, idStr::Length( string ) + 1 ); }
	virtual int WriteVec2( const idVec2 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 2 ); }
	virtual int WriteVec3( const idVec3 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 3 ); }
	virtual int WriteVec4( const idVec4 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 4 ); }
	virtual int WriteVec6( const idVec6 &vec ) { return Write( vec.ToFloatPtr(), sizeof( float ) * 6 ); }
	virtual int WriteMat3( const idMat3 &mat ) { return Write( mat.ToFloatPtr(), sizeof( float ) * 9 ); }

	bool IsWritable( void ) const { return writable; }
	const std::string &GetData( void ) const { return data; }

private:
	std::string name;
	std::string data;
	int position;
	bool writable;
};

class TestFileSystem : public idFileSystem {
public:
	void AddFile( const char *path, const char *contents ) {
		files[path] = contents;
	}

	const char *GetFileContents( const char *path ) const {
		std::map<std::string, std::string>::const_iterator it = files.find( path );
		return it == files.end() ? NULL : it->second.c_str();
	}

	virtual void Init( void ) {}
	virtual void Restart( void ) {}
	virtual void Shutdown( bool reloading ) {}
	virtual bool IsInitialized( void ) const { return true; }
	virtual bool PerformingCopyFiles( void ) const { return false; }
	virtual idModList *ListMods( void ) { return NULL; }
	virtual void FreeModList( idModList *modList ) {}
	virtual idFileList *ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char *gamedir = NULL ) { return NULL; }
	virtual idFileList *ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char *gamedir = NULL ) { return NULL; }
	virtual void FreeFileList( idFileList *fileList ) {}
	virtual const char *OSPathToRelativePath( const char *OSPath ) { return OSPath; }
	virtual const char *RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath" ) { return relativePath; }
	virtual const char *BuildOSPath( const char *base, const char *game, const char *relativePath ) { return relativePath; }
	virtual void CreateOSPath( const char *OSPath ) {}
	virtual bool FileIsInPAK( const char *relativePath ) { return false; }
	virtual void UpdatePureServerChecksums( void ) {}
	virtual fsPureReply_t SetPureServerChecksums( const int pureChecksums[MAX_PURE_PAKS], int missingChecksums[MAX_PURE_PAKS] ) { return PURE_OK; }
	virtual void GetPureServerChecksums( int checksums[MAX_PURE_PAKS] ) { checksums[0] = 0; }
	virtual void SetRestartChecksums( const int pureChecksums[MAX_PURE_PAKS] ) {}
	virtual void ClearPureChecksums( void ) {}
	virtual int ReadFile( const char *relativePath, void **buffer, ID_TIME_T *timestamp = NULL ) {
		std::map<std::string, std::string>::const_iterator it = files.find( relativePath );
		if ( it == files.end() ) {
			return -1;
		}
		if ( timestamp != NULL ) {
			*timestamp = 1;
		}
		if ( buffer != NULL ) {
			char *copy = static_cast<char *>( std::malloc( it->second.size() + 1 ) );
			std::memcpy( copy, it->second.data(), it->second.size() );
			copy[it->second.size()] = '\0';
			*buffer = copy;
		}
		return static_cast<int>( it->second.size() );
	}
	virtual void FreeFile( void *buffer ) { std::free( buffer ); }
	virtual int WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" ) {
		files[relativePath] = std::string( static_cast<const char *>( buffer ), size );
		return size;
	}
	virtual void RemoveFile( const char *relativePath ) { files.erase( relativePath ); }
	virtual idFile *OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char *gamedir = NULL ) {
		std::map<std::string, std::string>::const_iterator it = files.find( relativePath );
		return it == files.end() ? NULL : new TestFile( relativePath, it->second, false );
	}
	virtual idFile *OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath" ) {
		return new TestFile( relativePath, "", true );
	}
	virtual idFile *OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_basepath" ) { return NULL; }
	virtual idFile *OpenFileByMode( const char *relativePath, fsMode_t mode ) {
		if ( mode == FS_READ ) {
			return OpenFileRead( relativePath );
		}
		if ( mode == FS_WRITE ) {
			return OpenFileWrite( relativePath );
		}
		return NULL;
	}
	virtual idFile *OpenExplicitFileRead( const char *OSPath ) { return OpenFileRead( OSPath ); }
	virtual idFile *OpenExplicitFileWrite( const char *OSPath ) { return OpenFileWrite( OSPath ); }
	virtual void CloseFile( idFile *f ) {
		TestFile *testFile = static_cast<TestFile *>( f );
		if ( testFile->IsWritable() ) {
			files[testFile->GetName()] = testFile->GetData();
		}
		delete testFile;
	}
	virtual void BackgroundDownload( backgroundDownload_t *bgl ) {}
	virtual void ResetReadCount( void ) {}
	virtual int GetReadCount( void ) { return 0; }
	virtual void AddToReadCount( int c ) {}
	virtual void FindDLL( const char *basename, char dllPath[MAX_OSPATH] ) { dllPath[0] = '\0'; }
	virtual void ClearDirCache( void ) {}
	virtual bool HasD3XP( void ) { return false; }
	virtual bool RunningD3XP( void ) { return false; }
	virtual void CopyFile( const char *fromOSPath, const char *toOSPath ) {}
	virtual int ValidateDownloadPakForChecksum( int checksum, char path[MAX_STRING_CHARS] ) { return 0; }
	virtual idFile *MakeTemporaryFile( void ) { return new TestFile( "<temporary>", "", true ); }
	virtual int AddZipFile( const char *path ) { return 0; }
	virtual findFile_t FindFile( const char *path, bool scheduleAddons = false ) { return files.find( path ) == files.end() ? FIND_NO : FIND_YES; }
	virtual int GetNumMaps() { return 0; }
	virtual const idDict *GetMapDecl( int i ) { return NULL; }
	virtual void FindMapScreenshot( const char *path, char *buf, int len ) {
		if ( len > 0 ) {
			buf[0] = '\0';
		}
	}
	virtual bool FilenameCompare( const char *s1, const char *s2 ) const { return idStr::IcmpPath( s1, s2 ) == 0; }

private:
	std::map<std::string, std::string> files;
};

static const char *MAPGEN_SMOKE_MAP =
	"Version 2\n"
	"// entity 0\n"
	"{\n"
	"\"classname\" \"worldspawn\"\n"
	"}\n"
	"// entity 1\n"
	"{\n"
	"\"classname\" \"func_static\"\n"
	"\"name\" \"slot_0\"\n"
	"\"origin\" \"0 0 0\"\n"
	"{\n"
	" brushDef3\n"
	" {\n"
	"  ( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
	"  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 -1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 0 1 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 0 -1 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	" }\n"
	"}\n"
	"}\n"
	"// entity 2\n"
	"{\n"
	"\"classname\" \"info_null\"\n"
	"\"name\" \"marker\"\n"
	"\"target\" \"marker\"\n"
	"\"guiTarget\" \"marker\"\n"
	"\"buddy\" \"marker\"\n"
	"\"syncLock\" \"marker\"\n"
	"\"cameraTarget\" \"marker\"\n"
	"\"bind\" \"marker\"\n"
	"\"team\" \"door_team\"\n"
	"\"angle\" \"90\"\n"
	"\"movedir\" \"0\"\n"
	"\"rotation\" \"1 0 0 0 1 0 0 0 1\"\n"
	"\"light_rotation\" \"1 0 0 0 1 0 0 0 1\"\n"
	"\"light_origin\" \"64 16 8\"\n"
	"\"light_target\" \"1 2 3\"\n"
	"\"light_right\" \"4 5 6\"\n"
	"\"light_up\" \"7 8 9\"\n"
	"\"light_start\" \"10 11 12\"\n"
	"\"light_end\" \"13 14 15\"\n"
	"\"light_center\" \"16 17 18\"\n"
	"\"origin\" \"64 0 0\"\n"
	"}\n";

static const char *MAPGEN_HORIZONTAL_SLOT_MAP =
	"Version 2\n"
	"// entity 0\n"
	"{\n"
	"\"classname\" \"worldspawn\"\n"
	"}\n"
	"// entity 1\n"
	"{\n"
	"\"classname\" \"func_static\"\n"
	"\"name\" \"slot_0\"\n"
	"\"origin\" \"0 0 0\"\n"
	"{\n"
	" brushDef3\n"
	" {\n"
	"  ( 0 0 1 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
	"  ( 0 0 -1 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 1 0 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( -1 0 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 -1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	" }\n"
	"}\n"
	"}\n"
	"// entity 2\n"
	"{\n"
	"\"classname\" \"info_null\"\n"
	"\"name\" \"marker\"\n"
	"\"movedir\" \"-1\"\n"
	"\"origin\" \"0 0 64\"\n"
	"}\n";

static const char *MAPGEN_BAD_SLOT_BRUSH_MAP =
	"Version 2\n"
	"// entity 0\n"
	"{\n"
	"\"classname\" \"worldspawn\"\n"
	"}\n"
	"// entity 1\n"
	"{\n"
	"\"classname\" \"func_static\"\n"
	"\"name\" \"slot_0\"\n"
	"\"origin\" \"0 0 0\"\n"
	"{\n"
	" brushDef3\n"
	" {\n"
	"  ( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
	"  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 -1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 0 1 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	" }\n"
	"}\n"
	"}\n";

static const char *MAPGEN_MULTIPLE_SLOT_FACES_MAP =
	"Version 2\n"
	"// entity 0\n"
	"{\n"
	"\"classname\" \"worldspawn\"\n"
	"}\n"
	"// entity 1\n"
	"{\n"
	"\"classname\" \"func_static\"\n"
	"\"name\" \"slot_0\"\n"
	"\"origin\" \"0 0 0\"\n"
	"{\n"
	" brushDef3\n"
	" {\n"
	"  ( 1 0 0 0 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
	"  ( -1 0 0 -16 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/mapgen_slot\" 0 0 0\n"
	"  ( 0 1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 -1 0 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 0 1 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	"  ( 0 0 -1 -64 ) ( ( 0.03125 0 0 ) ( 0 0.03125 0 ) ) \"textures/common/caulk\" 0 0 0\n"
	" }\n"
	"}\n"
	"}\n";

static void RunMapGenSmokeTest( TestFileSystem &testFileSystem ) {
	testFileSystem.AddFile( "maps/mapgen_smoke.map", MAPGEN_SMOKE_MAP );

	idStr outputMapName;
	idStr status;
	Expect( MapGen_DMap( "mapgen_smoke", outputMapName, status ), status.c_str() );
	ExpectString( outputMapName.c_str(), "maps/mapgen/current.map", "unexpected output map path" );
	Expect( testFileSystem.GetFileContents( "maps/mapgen/current.map" ) != NULL, "output map was not written" );

	idMapFile generatedMap;
	Expect( generatedMap.Parse( "maps/mapgen/current", true ), "generated map could not be parsed" );
	Expect( generatedMap.GetNumEntities() == 5, "expected two prefixed map instances" );

	idMapEntity *firstSlot = generatedMap.GetEntity( 1 );
	idMapEntity *firstMarker = generatedMap.GetEntity( 2 );
	idMapEntity *secondSlot = generatedMap.GetEntity( 3 );
	idMapEntity *secondMarker = generatedMap.GetEntity( 4 );

	ExpectString( firstSlot->epairs.GetString( "name" ), "m0__slot_0", "unexpected first slot name" );
	ExpectString( firstMarker->epairs.GetString( "name" ), "m0__marker", "unexpected first marker name" );
	ExpectString( secondSlot->epairs.GetString( "name" ), "m1__slot_0", "unexpected second slot name" );
	ExpectString( secondMarker->epairs.GetString( "name" ), "m1__marker", "unexpected second marker name" );

	idMapBrush *rotatedSlotBrush = static_cast<idMapBrush *>( secondSlot->GetPrimitive( 0 ) );
	bool foundRotatedSlotSide = false;
	for ( int i = 0; i < rotatedSlotBrush->GetNumSides(); i++ ) {
		idMapBrushSide *side = rotatedSlotBrush->GetSide( i );
		if ( idStr::Icmp( side->GetMaterial(), "textures/common/mapgen_slot" ) != 0 ) {
			continue;
		}
		foundRotatedSlotSide = true;
		ExpectNear( side->GetPlane().Normal().x, -1.0f, 0.01f, "rotated slot normal x" );
		ExpectNear( side->GetPlane().Dist(), 0.0f, 0.01f, "rotated slot plane distance" );
	}
	Expect( foundRotatedSlotSide, "rotated slot brush has no mapgen slot side" );

	ExpectString( firstMarker->epairs.GetString( "target" ), "m0__marker", "unexpected first target" );
	ExpectString( firstMarker->epairs.GetString( "team" ), "m0__door_team", "unexpected first team" );

	ExpectString( secondMarker->epairs.GetString( "target" ), "m1__marker", "unexpected second target" );
	ExpectString( secondMarker->epairs.GetString( "guiTarget" ), "m1__marker", "unexpected second guiTarget" );
	ExpectString( secondMarker->epairs.GetString( "buddy" ), "m1__marker", "unexpected second buddy" );
	ExpectString( secondMarker->epairs.GetString( "syncLock" ), "m1__marker", "unexpected second syncLock" );
	ExpectString( secondMarker->epairs.GetString( "cameraTarget" ), "m1__marker", "unexpected second cameraTarget" );
	ExpectString( secondMarker->epairs.GetString( "bind" ), "m1__marker", "unexpected second bind" );
	ExpectString( secondMarker->epairs.GetString( "team" ), "m1__door_team", "unexpected second team" );

	float movedir;
	Expect( secondMarker->epairs.GetFloat( "movedir", "0", movedir ), "rotated movedir is missing" );
	ExpectNear( movedir, 180.0f, 0.01f, "rotated movedir" );

	float angle;
	Expect( secondMarker->epairs.GetFloat( "angle", "0", angle ), "rotated angle is missing" );
	ExpectNear( angle, 270.0f, 0.01f, "rotated angle" );

	idMat3 rotation;
	Expect( secondMarker->epairs.GetMatrix( "rotation", "", rotation ), "rotated rotation matrix is missing" );
	ExpectNear( rotation[0].x, -1.0f, 0.01f, "rotated rotation x axis x" );
	ExpectNear( rotation[1].y, -1.0f, 0.01f, "rotated rotation y axis y" );
	ExpectNear( rotation[2].z, 1.0f, 0.01f, "rotated rotation z axis z" );

	idMat3 lightRotation;
	Expect( secondMarker->epairs.GetMatrix( "light_rotation", "", lightRotation ), "rotated light_rotation matrix is missing" );
	ExpectNear( lightRotation[0].x, -1.0f, 0.01f, "rotated light_rotation x axis x" );
	ExpectNear( lightRotation[1].y, -1.0f, 0.01f, "rotated light_rotation y axis y" );
	ExpectNear( lightRotation[2].z, 1.0f, 0.01f, "rotated light_rotation z axis z" );

	idVec3 lightOrigin;
	secondMarker->epairs.GetVector( "light_origin", "0 0 0", lightOrigin );
	ExpectNear( lightOrigin.x, -64.0f, 0.01f, "rotated light_origin x" );
	ExpectNear( lightOrigin.y, -16.0f, 0.01f, "rotated light_origin y" );
	ExpectNear( lightOrigin.z, 8.0f, 0.01f, "rotated light_origin z" );

	idVec3 lightTarget;
	secondMarker->epairs.GetVector( "light_target", "0 0 0", lightTarget );
	ExpectNear( lightTarget.x, -1.0f, 0.01f, "rotated light_target x" );
	ExpectNear( lightTarget.y, -2.0f, 0.01f, "rotated light_target y" );
	ExpectNear( lightTarget.z, 3.0f, 0.01f, "rotated light_target z" );

	idVec3 rotatedOrigin;
	secondMarker->epairs.GetVector( "origin", "0 0 0", rotatedOrigin );
	ExpectNear( rotatedOrigin.x, -64.0f, 0.01f, "rotated origin x" );
	ExpectNear( rotatedOrigin.y, 0.0f, 0.01f, "rotated origin y" );
	ExpectNear( rotatedOrigin.z, 0.0f, 0.01f, "rotated origin z" );
}

static void RunMapGenHorizontalSlotTest( TestFileSystem &testFileSystem ) {
	testFileSystem.AddFile( "maps/mapgen_horizontal.map", MAPGEN_HORIZONTAL_SLOT_MAP );

	idStr outputMapName;
	idStr status;
	Expect( MapGen_DMap( "mapgen_horizontal", outputMapName, status ), status.c_str() );

	idMapFile generatedMap;
	Expect( generatedMap.Parse( "maps/mapgen/current", true ), "horizontal generated map could not be parsed" );
	Expect( generatedMap.GetNumEntities() == 5, "expected two horizontal map instances" );

	idMapEntity *secondSlot = generatedMap.GetEntity( 3 );
	idMapBrush *rotatedSlotBrush = static_cast<idMapBrush *>( secondSlot->GetPrimitive( 0 ) );
	bool foundRotatedSlotSide = false;
	for ( int i = 0; i < rotatedSlotBrush->GetNumSides(); i++ ) {
		idMapBrushSide *side = rotatedSlotBrush->GetSide( i );
		if ( idStr::Icmp( side->GetMaterial(), "textures/common/mapgen_slot" ) != 0 ) {
			continue;
		}
		foundRotatedSlotSide = true;
		ExpectNear( side->GetPlane().Normal().z, -1.0f, 0.01f, "horizontal rotated slot normal z" );
		ExpectNear( side->GetPlane().Dist(), 0.0f, 0.01f, "horizontal rotated slot plane distance" );
	}
	Expect( foundRotatedSlotSide, "horizontal rotated slot brush has no mapgen slot side" );

	idMapEntity *secondMarker = generatedMap.GetEntity( 4 );
	ExpectString( secondMarker->epairs.GetString( "name" ), "m1__marker", "unexpected horizontal second marker name" );
	ExpectString( secondMarker->epairs.GetString( "movedir" ), "-2", "unexpected horizontal movedir" );

	idVec3 rotatedOrigin;
	secondMarker->epairs.GetVector( "origin", "0 0 0", rotatedOrigin );
	ExpectNear( rotatedOrigin.x, 0.0f, 0.01f, "horizontal rotated origin x" );
	ExpectNear( rotatedOrigin.y, 0.0f, 0.01f, "horizontal rotated origin y" );
	ExpectNear( rotatedOrigin.z, -64.0f, 0.01f, "horizontal rotated origin z" );
}

static void RunMapGenInvalidSlotTests( TestFileSystem &testFileSystem ) {
	idStr outputMapName;
	idStr status;

	testFileSystem.RemoveFile( "maps/mapgen/current.map" );
	testFileSystem.AddFile( "maps/mapgen_bad_slot.map", MAPGEN_BAD_SLOT_BRUSH_MAP );
	Expect( !MapGen_DMap( "mapgen_bad_slot", outputMapName, status ), "bad slot brush unexpectedly succeeded" );
	ExpectContains( status.c_str(), "must be a six-sided brush", "unexpected bad slot brush status" );
	Expect( testFileSystem.GetFileContents( "maps/mapgen/current.map" ) == NULL, "bad slot brush wrote an output map" );

	status.Clear();
	outputMapName.Clear();
	testFileSystem.RemoveFile( "maps/mapgen/current.map" );
	testFileSystem.AddFile( "maps/mapgen_multiple_slot_faces.map", MAPGEN_MULTIPLE_SLOT_FACES_MAP );
	Expect( !MapGen_DMap( "mapgen_multiple_slot_faces", outputMapName, status ), "multiple slot faces unexpectedly succeeded" );
	ExpectContains( status.c_str(), "multiple 'textures/common/mapgen_slot' faces", "unexpected multiple slot faces status" );
	Expect( testFileSystem.GetFileContents( "maps/mapgen/current.map" ) == NULL, "multiple slot faces wrote an output map" );
}

int main( int argc, char **argv ) {
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
		RunMapGenSmokeTest( testFileSystem );
		RunMapGenHorizontalSlotTest( testFileSystem );
		RunMapGenInvalidSlotTests( testFileSystem );
	} catch ( const std::exception &ex ) {
		if ( idLibInitialized ) {
			idLib::ShutDown();
		}
		std::fprintf( stderr, "mapgen_test failed: %s\n", ex.what() );
		return 1;
	}

	idLib::ShutDown();
	std::printf( "mapgen_test passed\n" );
	return 0;
}
