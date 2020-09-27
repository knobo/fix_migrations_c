
#include<git2.h>


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
