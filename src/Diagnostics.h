#ifndef Diagnostics_h
#define Diagnostics_h

#ifdef __cplusplus
extern "C" {
#endif

void RealmzDiagnostics_Init(int argc, char** argv);
void RealmzDiagnostics_LogEvent(const char* event_name);
void RealmzDiagnostics_SetContext(const char* key, const char* value);
const char* RealmzDiagnostics_LogPath(void);

#ifdef __cplusplus
}
#endif

#endif // Diagnostics_h
