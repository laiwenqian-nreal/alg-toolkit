This project help you initialize project quickly.
githook sub-directory is used to check git command.
project sub-directory is used to generate project skeleton.

You can use install.sh to generate.


USAGE:
1. mkdir ${root dir}
2. cd ${root dir}
3. git init
4. git remote add origin git@github.com:xxx/yyy.git
5. mkdir -p external && cd external && ln -s ../../project .
6. cd project  (not cd -P)
7. ./install.sh
8. cd ${root dir}
9. git add .
10. git commit -m "feat: INIT"
11. ./compile.sh -c native install
