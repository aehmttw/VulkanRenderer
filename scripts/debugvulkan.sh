cd "$(dirname "$0")"
./compilevulkandebug.sh || { echo "oh noes! compilation failed!"; exit 1; }
export VK_ICD_FILENAMES=~/VulkanSDK/1.3.280.1/macOS/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=~/VulkanSDK/1.3.280.1/macOS/share/vulkan/explicit_layer.d
cd ../../runtime
lldb ./a.out