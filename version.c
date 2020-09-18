#include <stdlib.h>
#include "version.h"

int version_from_string(version *v, const char* s) {

  int idx = 0;
  
  int major = atoi(s + 1);
  while (*s++ != '_') idx++;      
  
  int minor = atoi(s);
  
  if (major > v->major) {
    v->major = major;
    v->minor = minor;
    return 2;
  } else if (v->major == major && minor > v->minor) {
    v->minor = minor;
    return 1;
  }
  
  return 0;
}

int comp_version(version* v1, version* v2)
{
  
  if(v1->major == v2->major && v1->minor > v2->minor){
    return 1;
  }

  if(v1->major == v2->major && v1->minor < v2->minor){
    return -1;
  }

  if(v1->major > v2->major){
    return 1;
  }

  if(v1->major < v2->major){
    return -1;
  }

  return 0;
}
