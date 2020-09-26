#include <git2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "version.h"

typedef struct {
  char a;
} status_data;

typedef struct {
  char *repo;
  char *migrations;
  version *version;
} conf;

typedef struct {
  int file_count;
  char *repo_path;
  char *resource_path;
  version* latest_version;
  git_index *index;
} config;


int file_cb(const git_diff_delta *delta, float progress, void *payload)
{
  (void)progress;
  config *conf = (config*) payload;
  version *v = conf->latest_version;
  version this_version = {0,0};

  const char* path = delta->new_file.path;
  const char *repo_dir = conf->repo_path;
  const char *resource_dir = conf->resource_path;
  git_index *index = conf->index;

  if(git_oid_iszero(&delta->new_file.id)) {

    if(path[0] == 'V'){
      version_from_string(&this_version, path);
      if(comp_version(v, &this_version) > 0){

        if(v->minor == 99) {
          v->major++;
          v->minor = 0;
        } else {
          v->minor++;
        }

        printf("mv %s V%d_%02d%s\n", path, v->major, v->minor, strstr(path, "__"));

        chdir(repo_dir);
        chdir(resource_dir);

        char *dest = malloc(strlen(path) + 2);
        sprintf(dest, "V%d_%02d%s", v->major, v->minor, strstr(path, "__"));
        rename(path, dest);

        char *add_file_path = malloc(strlen(conf->resource_path) + strlen(dest) + 2);
        sprintf(add_file_path, "%s/%s", conf->resource_path, dest);

        if(git_index_add_bypath(index, add_file_path)) {
          const git_error *e = git_error_last();
          printf("revparse_head_error error: %s\n", e->message);
          exit(-1);
        }

        char *delete_file_path = malloc(strlen(conf->resource_path) + strlen(path) + 2);
        sprintf(delete_file_path, "%s/%s", conf->resource_path, path);

        if( git_index_remove_bypath(index, delete_file_path)) {
          printf("Could not delete_file_path by path to index\n");
          const git_error *e = git_error_last();
          printf("revparse_head_error error: %s\n", e->message);
          exit(-1);
        }
        conf->file_count++;
        free(add_file_path);
        free(delete_file_path);
        free(dest);
      } else {
        printf("file: %s version is newer then master\n", path);
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


git_oid commit(git_repository* repo) {
  git_oid parent_id, commit_id;
  const char *message = "Updated db migration file version";
  git_oid tree_id = {0};

  git_signature *signature = NULL;
  git_commit *parent = NULL;
  git_index *index = NULL;
  git_tree *tree = NULL;

  git_repository_index(&index, repo);

  git_index_write_tree(&tree_id, index);
  git_tree_lookup(&tree, repo, &tree_id);
  git_signature_default(&signature, repo);

  git_reference_name_to_id(&parent_id, repo, "HEAD");
  git_commit_lookup(&parent, repo, &parent_id);

  git_commit_create_v(&commit_id,
                      repo,
                      "HEAD",     /* The commit will update the position of HEAD */
                      signature,
                      signature,
                      NULL,       /* UTF-8 encoding */
                      message,
                      tree,       /* The tree from the index */
                      1,          /* Only one parent */
                      parent      /* No need to make a list with create_v */
                      );

  git_signature_free(signature);
  git_index_free(index);
  git_tree_free(tree);
  git_commit_free(parent);

  return commit_id;
}

int main(int argc, char* argv[])
{

  git_repository *repo = NULL;

  git_libgit2_init();

  config conf = {0};

  if (argc > 1) {
    printf("using repo %s\n", argv[1]);
    conf.repo_path  = argv[1];
  } else {
    printf("using repo .\n");
    conf.repo_path = (char*)(".");
  }

  if (argc > 2) {
    printf("using resource %s\n", argv[2]);
    conf.resource_path  = argv[2];
  } else {
    printf("using resource: resources/db/migrations\n");
    conf.resource_path = "resources/db/migrations";
  }

  if(git_repository_init(&repo, conf.repo_path, false)) {
    printf("Could not init repo");
    exit(-1);
  }

  git_object *this_tree = NULL;

  char *headref = (char*) malloc(strlen("HEAD:") + strlen(conf.resource_path) + 1);
  sprintf(headref, "HEAD:%s", conf.resource_path);

  if(git_revparse_single(&this_tree, repo, headref)) {
    const git_error *e = git_error_last();
    printf("revparse_head_error error: %s\n", e->message);
    exit(-1);
  }

  git_object *that_tree = NULL;

  char *masterref = (char*) malloc(strlen("master:") + strlen(conf.resource_path) + 1);
  sprintf(masterref, "master:%s", conf.resource_path);

  if(git_revparse_single(&that_tree, repo, masterref)) {
    const git_error *e = git_error_last();
    printf("revparse_master_error error\n%s\n", e->message);
    exit(-1);
  }


  git_index *index = NULL;
  if(git_repository_index(&index, repo)) {
    puts("Could not get index");
    exit(-1);
  }

  conf.index = index;

  version version = {0,0};
  conf.latest_version = &version;

  if(max_versoin_in_tree(&version, (const git_tree*)that_tree)) {
    printf("Could not find any versions\n");
    exit(-1);
  }

  printf("Last version in master V%d_%d\n", version.major, version.minor);

  git_diff *diff;

  if(git_diff_tree_to_tree(&diff, repo, (git_tree*)this_tree, (git_tree*)that_tree, NULL /* &opts */)) {
    const git_error *e = git_error_last();
    printf("git_diff_tree_to_tree error \n%s\n",  e->message);
    exit(-1);
  }

  if(git_diff_foreach(diff, file_cb, NULL, NULL, NULL, &conf)) {
    const git_error *e = git_error_last();
    printf("For each error\n%s\n",  e->message);
    exit(-1);
  }

  if(conf.file_count > 0) {
    git_index_write(index);
    git_index_free(index);
    commit(repo);
  } else {
    git_index_free(index);
  }


  git_libgit2_shutdown();
  return 0;
}
