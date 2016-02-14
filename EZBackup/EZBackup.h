#pragma once

#include "resource.h"

#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "myfiledirdll.h"
#include "database.h"
#include "MD5.h"

#define MAX_LOADSTRING 100
#define FILE_OPEN_1 10
#define FILE_OPEN_2 11
#define FILE_PATH_1 12
#define FILE_PATH_2 13
#define MERGE_FILES 14

#define REPO_NAME "repo.db"
// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

												// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Done(HWND, UINT, WPARAM, LPARAM);
int buttonW = 100, buttonH = 40;
int baseX = 30, baseY = 20;

int winWidth = 1000;
int winHeight = 300;

using namespace std;

string destPath;
string srcPath;
int numDupes = 0;
DataBase db;
string curRepoFile;


struct FileStats
{
	string name;
	string path;
	string hash;
	long fileSize;

	string getInsertString()
	{
		return ("\"" + name + "\",\"" + hash + "\",\"" + path + "\"," + to_string(fileSize) + ",1");
	};
};


long GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}


//http://stackoverflow.com/questions/1220046/how-to-get-the-md5-hash-of-a-file-in-c
string createMD5Hash(string fileName)
{
	//Start opening your file
	ifstream inputFile;
	inputFile.open(fileName, std::ios::binary | std::ios::in);

	//Find length of file
	inputFile.seekg(0, std::ios::end);
	long len = inputFile.tellg();
	inputFile.seekg(0, std::ios::beg);

	//read in the data from your file
	char * InFileData = new char[len];
	inputFile.read(InFileData, len);

	//Calculate MD5 hash
	string returnString = md5(InFileData, len);

	//Clean up
	delete[] InFileData;

	return returnString;
}

void getFileStats(string path, FileStats &stats)
{
	stats.name = MyFileDirDll::getFileNameFromPathString(path);
	stats.path = MyFileDirDll::getPathFromFullyQualifiedPathString(path);
	stats.hash = createMD5Hash(path);
	stats.fileSize = GetFileSize(path);
}

//------------------------------------------------------------------------------------------------
string BrowseToFile(HWND hWnd, LPCTSTR filename)
{
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
	HWND hwnd;              // owner window
	HANDLE hf;              // file handle

							// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Text\0*.m3u\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;// | OFN_FILEMUSTEXIST;

								  // Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE)
	{
		//return std::string(ofn.lpstrFile); 
		string temp = ofn.lpstrFile;
		return temp;
	}
	/*hf = CreateFile(ofn.lpstrFile,
	GENERIC_READ,
	0,
	(LPSECURITY_ATTRIBUTES) NULL,
	OPEN_EXISTING,
	FILE_ATTRIBUTE_NORMAL,
	(HANDLE) NULL);*/
	return "";
}
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	if (uMsg == BFFM_INITIALIZED)
	{
		string tmp = (const char *)lpData;
		cout << "path: " << tmp << std::endl;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

string BrowseFolder(string saved_path)
{
	TCHAR path[MAX_PATH];

	const char * path_param = saved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = ("Browse for folder...");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc * imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		return path;
	}

	return "";
}

bool doesRepoExist(string path)
{
	if(!MyFileDirDll::doesPathExist(path))
		return false;

	string repoFile = (path + "\\" + REPO_NAME);
	DWORD attr = GetFileAttributes(repoFile.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
		return false;   //  not a file

	return true;
}

void createRepo(string path)
{
	string output;
	curRepoFile = (path + "\\"+REPO_NAME);
	db.openDataBase(curRepoFile, output);

	if (!db.createTable("Repo", "fileName TEXT, hash TEXT, path TEXT, fileSize INTEGER, version INTEGER"))
	{
		MessageBox(0, db.getLastError().c_str(), "Oh Oh...", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	MyFileDirDll::startDirTreeStep(path);
	while (!MyFileDirDll::isFinished())
	{
		string curDir = MyFileDirDll::nextDirTreeStep();
		//if the current node has no files, then move on
		if (MyFileDirDll::getCurNodeNumFiles() == 0)
			continue;

		list<string> curFiles = MyFileDirDll::getCurNodeFileList();

		for (list<string>::iterator it = curFiles.begin(); it != curFiles.end(); ++it)
		{
			string curFilePath = (curDir +"\\"+ *it);
			//dont add tis own repo.db to itself!
			if (curFilePath == curRepoFile)
				continue;

			FileStats stats;
			getFileStats(curFilePath,stats);

			string querey = stats.getInsertString();

			if (!db.insertData(querey))
			{
				//MessageBox(0, db.getLastError().c_str(), "Oh Oh...", MB_ICONEXCLAMATION | MB_OK);
			}
		}
	}
	
cout << "done"; 
}


void checkRepo(vector<string> &listOfDirs)
{
	for (size_t i = 0; i < listOfDirs.size(); i++)
	{
		if (MyFileDirDll::getNumFilesInDir(listOfDirs[i]) == 0)
			continue;

		vector<string> curFiles = MyFileDirDll::getAllFileNamesInDir(listOfDirs[i]);

		for (size_t j = 0; j < curFiles.size(); j++)
		{
			string curFilePath = (listOfDirs[i] + "\\" + curFiles[j]);
			//dont add tis own repo.db to itself!
			if (curFilePath == curRepoFile)
				continue;

			FileStats stats;
			getFileStats(curFilePath, stats);

			string output;
			string querey = "SELECT * FROM Repo WHERE hash = \"";
			querey += stats.hash;
			querey += "\"";

			db.executeSQL(querey, output);
			if (output.empty())
			{
				string querey = stats.getInsertString();
				if (!db.insertData(querey))
				{
					//MessageBox(0, db.getLastError().c_str(), "Oh Oh...", MB_ICONEXCLAMATION | MB_OK);
				}
				//now that info is in DB, move the file to the new dir

			}
		}
	}
}
vector<string> treeOnDisk;




void getRepoData(string path, vector<string> &dbPAths)
{

	//parse all the files in the "repo" and add them to the db if not in already
	MyFileDirDll::addDirTree(path, 10);
	sort(dbPAths.begin(), dbPAths.end());
	
}