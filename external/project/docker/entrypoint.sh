#!/bin/bash
echo "Ignore the following conan error!"
conan profile detect --name local >/dev/null 2>&1
mv /home/dev/.conan2/profiles/local /home/dev/.conan2/profiles/default
sed -i 's/compiler.cppstd=gnu17/compiler.cppstd=17/' /home/dev/.conan2/profiles/default
sed -i 's/compiler.version=11/compiler.version=7.5/' /home/dev/.conan2/profiles/default
cat /home/dev/.conan2/profiles/default
conan remote remove conancenter
conan remote add jfrog https://jfrog.xreal.work/artifactory/api/conan/nrsdk-public
conan remote login jfrog nrsdk -p Xreal.123
#conan remote add jfrog-3rdparty https://jfrog.xreal.work/artifactory/api/conan/nrsdk-3rdparty
#conan remote disable jfrog-3rdparty
#conan remote add jfrog-3rdparty-rel https://jfrog.xreal.work/artifactory/api/conan/nrsdk-3rdparty-rel
#conan remote disable jfrog-3rdparty-rel
echo "Ignore the previous conan error!"
echo "parameters: $@"
exec "$@"
