include assets.Config.mk
BLENDER_WIN32 = $(BLENDER)


#
# Tools
#
IO_KOWL = $(shell dir /a /b /s "tools/io_kowl/*.py" 2>nul)
GLSL_FILES = $(shell dir /a /b /s "$(RAW_DIR)/shaders/*.glsl" 2>nul)


#
# Assets
#
RAW_DIR = assets
RAW_BIN_DIR = assets_bin
COOKED_DIR = $(BIN_DIR)\assets


# 
find = $(patsubst $(shell cd)\\%,%,$(shell dir /a /b /s "$(1)\*.$(2)" 2>nul))

# Find all files in a given directory $(1) that share a given
# extension $(2), and return their filepaths relative to that
# directory
# find_assets(dir, ext) -> [able/baker.ext, charlie/dog.ext]
find_assets = $(patsubst $(1)\\%,%,$(call find,$(1),$(2)))

# Create a list of files by finding all files in a directory $(1) that
# share a given extension $(2), relative to that directory, moving
# those files to a new directory $(3) and giving them a new extension
# $(4)
# raw_to_cooked(dir1, ext1, dir2, ext2) -> [dir2/able/baker.ext2, dir2/charlie/dog.ext2]
raw_to_cooked = $(patsubst %.$(2),$(3)\\%.$(4),$(call find_assets,$(1),$(2)))


AREA_INDEX = $(COOKED_DIR)\area.index
BLEND_SENTINEL_FILES += $(call raw_to_cooked,$(RAW_BIN_DIR),blend,$(COOKED_DIR),blend_sentinel)
FRAG_FILES += $(call raw_to_cooked,$(RAW_DIR),frag,$(COOKED_DIR),frag)
PNG_FILES += $(call raw_to_cooked,$(RAW_BIN_DIR),png,$(COOKED_DIR),png)
VERT_FILES += $(call raw_to_cooked,$(RAW_DIR),vert,$(COOKED_DIR),vert)


ASSET_FILES = $(AREA_INDEX)
ASSET_FILES += $(BLEND_SENTINEL_FILES)
ASSET_FILES += $(FRAG_FILES)
ASSET_FILES += $(PNG_FILES)
ASSET_FILES += $(VERT_FILES)


$(COOKED_DIR)\area.index: $(BLEND_SENTINEL_FILES) tools\area_indexer.py
	if not exist $(@D) mkdir $(@D)
	python tools/area_indexer.py $@

$(COOKED_DIR)\\%.blend_sentinel: $(RAW_BIN_DIR)\%.blend $(IO_KOWL)
	if not exist $(@D) mkdir $(@D)
	$(BLENDER_WIN32) -b --factory-startup $< --python tools\io_kowl\area.py -- $(COOKED_DIR)
	type nul > $@

$(COOKED_DIR)\\%.frag: $(RAW_DIR)\%.frag $(GLSL_FILES) tools\glsl_includer.py
	if not exist $(@D) mkdir $(@D)
	python tools\glsl_includer.py $< $@


$(COOKED_DIR)\\%.png: $(RAW_BIN_DIR)\%.png
	if not exist $(@D) mkdir $(@D)
	xcopy /I /D $< $@*

$(COOKED_DIR)\\%.vert: $(RAW_DIR)\%.vert $(GLSL_FILES) tools\glsl_includer.py
	if not exist $(@D) mkdir $(@D)
	python Tools\glsl_includer.py $< $@

.PHONY: assets
assets: $(ASSET_FILES)

.PHONY: clean_assets
clean_assets:
#	del $(ASSET_FILES) 2>nul
	rmdir /S /Q $(COOKED_DIR) 2>nul
