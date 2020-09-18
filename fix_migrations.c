#include <git2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"

typedef struct {
  char a;
} status_data;

int file_cb(const git_diff_delta *delta,
            float progress,
            void *payload)
{
  version *v = (version*) payload;
  version this_version = {0,0};
  (void)progress;
  const char* path = delta->new_file.path;

  if(git_oid_iszero(&delta->new_file.id)) {
    
    if(path[0] == 'V'){
      version_from_string(&this_version, path);
      if(comp_version(v, &this_version)){
        if(v->minor == 99) {
          v->major++;
          v->minor = 0;
        } else {
          v->minor++;        
        }
        printf("mv %s V%d_%02d%s\n", path, v->major, v->minor, strstr(path, "__"));
      }
    }
  }
  return 0;
}

int max_versoin_in_tree(version* version, const git_tree *tree)
{
  size_t i, max_i = (int)git_tree_entrycount(tree);
  char oidstr[GIT_OID_HEXSZ + 1];
  const git_tree_entry *te;

  version->major = 0;
  version->minor = 0;
  
  for (i = 0; i < max_i; ++i) {
    te = git_tree_entry_byindex(tree, i);

    git_oid_tostr(oidstr, sizeof(oidstr), git_tree_entry_id(te));

    const char* filename = git_tree_entry_name(te);

    if(filename[0] == 'V') {
      version_from_string(version, filename);
    } 
  }

  return 0;
}


int main(int argc, char* argv[])
{

  git_repository *repo = NULL;
  
  git_libgit2_init();
  char* path;

  if (argc > 1) {
    path = argv[1];
  } else {
    path = (char*)("/home/knobo/prog/testgit");
  }
           
  int init_error = git_repository_init(&repo, path, false);

  if(init_error) {
    printf("Could not init repo");
    exit(-1);
  }
  
  git_object *this_tree = NULL;
  int revparse_head_error = git_revparse_single(&this_tree, repo, "HEAD:resources/db/migrations");

  if(revparse_head_error) {
    const git_error *e = git_error_last();
    printf("revparse_head_error error %d\n%s\n", revparse_head_error, e->message);
    exit(-1);
  }
  
  git_object *that_tree = NULL;
  int revparse_master_error = git_revparse_single(&that_tree, repo, "master:resources/db/migrations");

  version version = {0,0};
  int version_error = max_versoin_in_tree(&version, (const git_tree*)that_tree);

  if(version_error) {
    printf("Could not find any versions\n");
  }
  
  printf("Max version %d-%d\n", version.major, version.minor);
  
  if(revparse_master_error) {
    const git_error *e = git_error_last();
    printf("revparse_master_error error %d\n%s\n", revparse_master_error, e->message);
    exit(-1);
  }

  
  git_diff *diff;  
  int diff_error = git_diff_tree_to_tree(&diff, repo, (git_tree*)this_tree, (git_tree*)that_tree, NULL /* &opts */);
  
  if(diff_error) {
    const git_error *e = git_error_last();
    printf("git_diff_tree_to_tree error %d\n%s\n", diff_error, e->message);
    exit(-1);
  }
    
  int for_each_error = git_diff_foreach(diff, file_cb, NULL, NULL, NULL, &version);

  printf("For each done\n");
  
  if(for_each_error) {
    const git_error *e = git_error_last();

    printf("For each error %d\n%s\n", for_each_error, e->message);
    exit(-1);
  }
      
  
  git_libgit2_shutdown();
  
}
