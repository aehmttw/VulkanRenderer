cd "$(dirname "$0")"
export CPATH=~/VulkanSDK/1.3.280.1/macOS/include:/opt/homebrew/include:$CPATH
export LIBRARY_PATH=~/VulkanSDK/1.3.280.1/macOS/lib:/opt/homebrew/lib:$LIBRARY_PATH
./compileshaders.sh
cd ../../runtime
echo "Compiling..."
g++ -std=c++20 -O3 -Wno-deprecated-declarations ../code/renderer.cpp ../code/vecmath.hpp ../code/json.hpp -lglfw -lvulkan
echo "Done!"