
set(FOUNDATION_DIR ${TOOLS_DIR}/foundation)

add_subdirectory(resample)

# 将模块添加到自定义目录中
SET_PROPERTY(TARGET resample PROPERTY FOLDER "tutorial")