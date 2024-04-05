cd "$(dirname "$0")"
cd ../shaders
echo Compiling shaders...
for f in $(find . \( -name "*.vert" -o -name "*.frag" \)); do
  ~/VulkanSDK/1.3.280.1/macOS/bin/glslc $f -o ../../runtime/spv/$f.spv;
  echo $f
done
echo Done!