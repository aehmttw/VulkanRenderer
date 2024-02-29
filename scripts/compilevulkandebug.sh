cd "$(dirname "$0")"
export CPATH=~/VulkanSDK/1.3.268.1/macOS/include:/opt/homebrew/include:$CPATH
export LIBRARY_PATH=~/VulkanSDK/1.3.268.1/macOS/lib:/opt/homebrew/lib:$LIBRARY_PATH
./compileshaders.sh
echo "Compiling..."
cd ../../runtime
g++ -std=c++20 -g -O0 ../code/renderer.cpp ../code/vecmath.hpp ../code/json.hpp -lglfw -lvulkan
echo "Done!"
