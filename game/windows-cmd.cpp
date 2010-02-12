#ifdef _WIN32

#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>
#include <string>

#define BUFSIZE 4096 
 
HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;
 
void CreateChildProcess(int argc, char* argv[]); 
void ReadFromPipe(void); 
void ErrorExit(PTSTR); 
int main(int argc, char* argv[])
{ 
   SECURITY_ATTRIBUTES saAttr; 
 
   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
   saAttr.bInheritHandle = TRUE; 
   saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
 
   if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) ) 
	  ErrorExit(TEXT("StdoutRd CreatePipe")); 
	// Ensure the read handle to the pipe for STDOUT is not inherited.
   if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
	  ErrorExit(TEXT("Stdout SetHandleInformation")); 

   // Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) 
	  ErrorExit(TEXT("Stdin CreatePipe")); 

// Ensure the write handle to the pipe for STDIN is not inherited. 
 
   if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
	  ErrorExit(TEXT("Stdin SetHandleInformation")); 

	
	// Create the child process. 
	CreateChildProcess(argc, argv);

	// Read from pipe that is the standard output for child process. 
	ReadFromPipe(); 
	return 0; 
} 
 
void CreateChildProcess(const int argc, char* argv[])
// Create a child process that uses the previously created pipes for STDOUT.
{
	std::string cmdline;
	cmdline = "performous.exe";
   for(int i = 1; i < argc; i++){
	  cmdline +=  " \"";
	  cmdline += argv[i];
	  cmdline += "\"";
   }
   fprintf(stdout, cmdline.c_str());
   fprintf(stdout, "\n");

   char* str;
   str = new char[cmdline.length() + 1];
   strcpy(str, cmdline.c_str());
   LPSTR temp;
   temp = str;
   
   PROCESS_INFORMATION piProcInfo; 
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
	// Set up members of the PROCESS_INFORMATION structure. 
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = g_hChildStd_OUT_Wr;
   siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
   bSuccess = CreateProcess(NULL, 
	  temp,     // command line 
	  NULL,          // process security attributes 
	  NULL,          // primary thread security attributes 
	  TRUE,          // handles are inherited 
	  0,             // creation flags 
	  NULL,          // use parent's environment 
	  NULL,          // use parent's current directory 
	  &siStartInfo,  // STARTUPINFO pointer 
	  &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
	  ErrorExit(TEXT("CreateProcess"));
   else 
   {
	  // Close handles to the child process and its primary thread.
	  // Some applications might keep these handles to monitor the status
	  // of the child process, for example. 

	  CloseHandle(piProcInfo.hProcess);
	  CloseHandle(piProcInfo.hThread);
   }

   delete[] temp;
}
 

void ReadFromPipe(void) 
// Read output from the child process's pipe for STDOUT
// and write to the parent process's pipe for STDOUT. 
// Stop when there is no more data. 
{ 
   DWORD dwRead, dwWritten; 
   CHAR chBuf[BUFSIZE]; 
   BOOL bSuccess = FALSE;
   HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

// Close the write end of the pipe before reading from the 
// read end of the pipe, to control child process execution.
// The pipe is assumed to have enough buffer space to hold the
// data the child process has already written to it.
 
   if (!CloseHandle(g_hChildStd_OUT_Wr)) 
	  ErrorExit(TEXT("StdOutWr CloseHandle")); 
 
   for (;;) 
   { 
	  bSuccess = ReadFile( g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
	  if( ! bSuccess || dwRead == 0 ) break; 

	  bSuccess = WriteFile(hParentStdOut, chBuf, 
						   dwRead, &dwWritten, NULL);
	  if (! bSuccess ) break; 
   } 
} 
 
void ErrorExit(PTSTR lpszFunction) 
// Format a readable error message, display a message box, 
// and exit from the application.
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}

#endif