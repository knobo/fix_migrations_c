cmake_minimum_required( VERSION 2.6 )

project (fix_migrations)

enable_testing()
add_compile_options(-Wall -Wextra -pedantic -Werror)
add_executable(fix_migrations fix_migrations.c version.c)

target_link_libraries (fix_migrations LINK_PUBLIC git2)

add_test(program_is_running fix_migrations)
