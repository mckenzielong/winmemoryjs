
#include "process.h"
#include "process_entry.h"

Napi::Value Process::convertProcessEntryArray(Napi::Env env, const std::vector<PROCESSENTRY32> &processEntries) {
  //Loop through all processes and add them to our return value
  Napi::Array processes = Napi::Array::New(env);
  uint32_t  i = 0;
  for (const auto &entry : processEntries) {
    processes[i++] = MemoryAPI::ProcessEntry::New(env, entry);
  }
  return processes;
}

std::vector<PROCESSENTRY32> getProcessesInternal(char** errorMessage) {
  // Take a snapshot of all processes.
  HANDLE hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  PROCESSENTRY32 pEntry;

  if (hProcessSnapshot == INVALID_HANDLE_VALUE) {
    *errorMessage = "method failed to take snapshot of the process";
  }

  // Before use, set the structure size.
  pEntry.dwSize = sizeof(pEntry);

  // Exit if unable to find the first process.
  if (!Process32First(hProcessSnapshot, &pEntry)) {
    CloseHandle(hProcessSnapshot);
    *errorMessage = "method failed to retrieve the first process";
  }

  std::vector<PROCESSENTRY32> processes;

  // Loop through processes.
  do {
    // Add the process to the vector
    processes.push_back(pEntry);
  } while (Process32Next(hProcessSnapshot, &pEntry));

  CloseHandle(hProcessSnapshot);
  return processes;
}

std::vector<PROCESSENTRY32> Process::getWindowsProcesses(Napi::Env env) {
  char* err = "";
  auto processes = getProcessesInternal(&err);
  if (strcmp(err, "")) {
    throw Napi::Error::New(env, err);
  }

  return processes;
}

HANDLE Process::openProcess(long processId, std::string* errorMessage) {
  HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

  if (handle == NULL) {
    *errorMessage = "unable to find process";
  }
  
  return handle;
}

void Process::closeProcess(HANDLE hProcess, std::string* errorMessage){
  bool success = CloseHandle(hProcess);
  if (!success) {
    *errorMessage = "unable to close file handle";
  }
}

Napi::Value Process::getProcesses(Napi::Env env) {
  // Define error message that may be set by the function that gets the processes
  char* errorMessage = "";
  std::vector<PROCESSENTRY32> processEntries = getProcessesInternal(&errorMessage);
  if (strcmp(errorMessage, "")) {
    throw Napi::Error::New(env, errorMessage);
  }

  //Loop through all processes and add them to our return value
  Napi::Array processes = Napi::Array::New(env);
  uint32_t  i = 0;
  for (const auto &entry : processEntries) {
    processes[i++] = MemoryAPI::ProcessEntry::New(env, entry);
  }
  return processes;
}