#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif

#include "maths.h"
#include "vecmat.h"

// the maximum length of a filename
#define FILENAME_LEN			255
#define SHORT_FILENAME_LEN 13
#define MAX_HOGFILES			300

class CFilename {
	public:
		char m_buffer [FILENAME_LEN + 1];
	public:
		CFilename () { m_buffer [0] = '\0'; }
		inline CFilename& operator= (CFilename& other) { 
			memcpy (m_buffer, other.m_buffer, sizeof (m_buffer)); 
			return *this;
			}
		inline CFilename& operator= (const char* other) { 
			strncpy (m_buffer, other, sizeof (m_buffer)); 
			return *this;
			}
		inline bool operator== (CFilename& other)
			{ return !strcmp (m_buffer, other.m_buffer); }
		inline bool operator< (CFilename& other)
			{ return strcmp (m_buffer, other.m_buffer) < 0; }
		inline bool operator> (CFilename& other)
			{ return strcmp (m_buffer, other.m_buffer) > 0; }
		inline operator const char*()
			{ return reinterpret_cast<char*> (&m_buffer [0]); }
	};


typedef struct CFILE {
	FILE		*file;
	char		*filename;
	int		size;
	int		libOffset;
	int		rawPosition;
} CFILE;

class CFile {
	private:
		CFILE	m_cf;

	public:
		CFile () { Init (); }
		~CFile () { Close (); };
		void Init (void);
		int Open (const char *filename, const char *folder, const char *mode, int bUseD1Hog);
		int Length (void);							// Returns actual size of file...
		size_t Read (void *buf, size_t elsize, size_t nelem);
		int Close (void);
		int Size (const char *hogname, const char *folder, int bUseD1Hog);
		int Seek (long int offset, int where);
		int Tell (void);
		char *GetS (char *buf, size_t n);
		int EoF (void);
		int Error (void);
		int Write (const void *buf, int elsize, int nelem);
		int GetC (void);
		int PutC (int c);
		int PutS (const char *str);

		inline int Size (void) { return m_cf.size; }

		// prototypes for reading basic types from fp
		int ReadInt (void);
		uint ReadUInt (void);
		short ReadShort (void);
		ushort ReadUShort (void);
		sbyte ReadByte (void);
		ubyte ReadUByte (void);
		fix ReadFix (void);
		fixang ReadFixAng (void);
		void ReadVector (CFixVector& v);
		void ReadVector (CFloatVector3& v);
		void ReadAngVec (CAngleVector& v);
		void ReadMatrix (CFixMatrix& v);
		float ReadFloat (void);
		double ReadDouble (void);
		void ReadString (char *buf, int n);
		char *ReadData (const char *filename, const char *folder, int bUseD1Hog);

		int WriteFix (fix x);
		int WriteInt (int i);
		int WriteShort (short s);
		int WriteByte (sbyte u);
		int WriteFixAng (fixang a);
		int WriteFloat (float f);
		int WriteDouble (double d);
		void WriteAngVec (const CAngleVector& v);
		void WriteVector (const CFixVector& v);
		void WriteVector (const CFloatVector3& v);
		void WriteMatrix (const CFixMatrix& m);
		int WriteString (const char *buf);

		int Copy (const char *pszSrc, const char *pszDest);
		int Extract (const char *filename, const char *folder, int bUseD1Hog, const char *szDest);
		time_t Date (const char *filename, const char *folder, int bUseD1Hog);

		static int Exist (const char *filename, const char *folder, int bUseD1Hog);	// Returns true if file exists on disk (1) or in hog (2).
		static int Delete (const char *filename, const char* folder);
		static int Rename (const char *oldname, const char *newname, const char *folder);
		static int MkDir (const char *pathname);
		static FILE *GetFileHandle (const char *filename, const char *folder, const char *mode);
		static void SplitPath (const char *szFullPath, char *szFolder, char *szFile, char *szExt);
		static void ChangeFilenameExtension (char *dest, const char *src, const char *new_ext);

		inline FILE*& File () { return m_cf.file; }
		inline char* Name () { return m_cf.filename; }
	};


typedef struct tGameFolders {
	char szHomeDir [FILENAME_LEN];
// wii edit: 
//#if defined (__unix__) || defined (__macosx__)
	char szSharePath [FILENAME_LEN];
//#endif
	char szGameDir [FILENAME_LEN];
	char szDataDir [FILENAME_LEN];
	char szDownloadDir [FILENAME_LEN];
	char szShaderDir [FILENAME_LEN];
	char szModelDir [3][FILENAME_LEN];
	char szModelCacheDir [3][FILENAME_LEN];
	char szSoundDir [7][FILENAME_LEN];
	char szTextureDir [4][FILENAME_LEN];
	char szTextureCacheDir [4][FILENAME_LEN];
	char szWallpaperDir [FILENAME_LEN];
	char szMissionDir [FILENAME_LEN];
	char szMissionDownloadDir [FILENAME_LEN];
	char szConfigDir [FILENAME_LEN];
	char szSaveDir [FILENAME_LEN];
	char szProfDir [FILENAME_LEN];
	char szMovieDir [FILENAME_LEN];
	char szScrShotDir [FILENAME_LEN];
	char szDemoDir [FILENAME_LEN];
	char szAltHogDir [FILENAME_LEN];
	char szMissionDirs [2][FILENAME_LEN];
	char szMsnSubDir [FILENAME_LEN];
	char szCacheDir [FILENAME_LEN];
	char szModDir [2][FILENAME_LEN];
	char szMusicDir [FILENAME_LEN];
	char szModName [FILENAME_LEN];
	int bAltHogDirInited;
} tGameFolders;

int GetAppFolder (const char *szRootDir, char *szFolder, const char *szName, const char *szFilter);
char *GameDataFilename (char *pszFilename, const char *pszExt, int nLevel, int nType);
void MakeTexSubFolders (char* pszParentFolder);
void MakeModFolders (const char* pszMission);
void ResetModFolders (void);

#ifdef _WIN32
char *UnicodeToAsc (char *str, const wchar_t *w_str);
wchar_t *AscToUnicode (wchar_t *w_str, const char *str);
#endif

extern int nCFileError;
extern tGameFolders	gameFolders;

#ifdef _WIN32_WCE
# define errno -1
# define strerror (x) "Unknown Error"
#endif

#endif
