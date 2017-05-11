Because the OF addons system doesn't provide a proper mechanism for specifying addon version,
all the dependencies of this addon are added as submodules to this dependencies folder.

This way the submodule reference can be used to specify the proper version (commit) of the dependency.

To avoid duplicate and unreferenced addons all across your file system, the submodule points to a relative
folder where the actual addons are checkout out (since this repository is an addon itself, we can derrive the OF addons folder
as simply the parent folder of this repository).

For more specific instruction on how this work, see:
http://stackoverflow.com/questions/27379818/git-possible-to-use-same-submodule-working-copy-by-multiple-projects
