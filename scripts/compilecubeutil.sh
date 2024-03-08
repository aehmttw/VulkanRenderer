cd "$(dirname "$0")"
cd ../../cubeutil
echo "Compiling..."
g++ -std=c++20 -O3 ../code/vecmath.hpp ../code/util/cubeutil.cpp
echo "Done!"