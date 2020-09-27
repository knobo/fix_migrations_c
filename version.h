#ifndef VERSION_H_   /* Include guard */
#define VERSION_H_

typedef struct {
  int major;
  int minor;
} version;

int version_from_string(version *v, const char* s);
int comp_versions(version* v1, version* v2);
void inc_version(version*);

#endif // VERSION_H_
