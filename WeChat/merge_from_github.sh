
root=`git rev-parse --show-toplevel`

branch="WeChat/iOS"
githubBranch="pre_v1.1.0"
git checkout ${githubBranch}
git submodule update sqlcipher
git pull --rebase github ${githubBranch}:${githubBranch}
git submodule update sqlcipher
git checkout ${branch}
git submodule update sqlcipher
git merge ${githubBranch}

sh ${root}/tools/version/bump.sh
git add ${root}/objc/source/utility/Version.h
git commit -m "Re-bump version"
git push origin ${branch}:${branch}
