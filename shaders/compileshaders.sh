cd "$(dirname "$0")"
echo Compiling shaders...
for f in $(find . \( -name "*.vert" -o -name "*.frag" \)); do
  ~/VulkanSDK/1.3.268.1/macOS/bin/glslc $f -o spv/$f.spv;
  echo $f
done
echo Done!