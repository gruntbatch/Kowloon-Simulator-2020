include assets.Config.mk
BLENDER_MAC = $(BLENDER)/Contents/MacOS/Blender


#
# Assets
#
RAW_DIR = assets
RAW_BIN_DIR = assets_bin
COOKED_DIR = $(BIN_DIR)/assets


# Find all files in a given directory $(1) that share a given
# extension $(2), and return their filepaths relative to that
# directory
# find_assets(dir, ext) -> [able/baker.ext, charlie/dog.ext]
find_assets = $(patsubst $(1)/%,%,$(shell find $(1) -name "*.$(2)"))

# Create a list of files by finding all files in a directory $(1) that
# share a given extension $(2), relative to that directory, moving
# those files to a new directory $(3) and giving them a new extension
# $(4)
# raw_to_cooked(dir1, ext1, dir2, ext2) -> [dir2/able/baker.ext2, dir2/charlie/dog.ext2]
raw_to_cooked = $(patsubst %.$(2),$(3)/%.$(4),$(call find_assets,$(1),$(2)))


ASSET_FILES =
ASSET_FILES += $(call raw_to_cooked,$(RAW_BIN_DIR),blend,$(COOKED_DIR),blend_sentinel)
ASSET_FILES += $(call raw_to_cooked,$(RAW_DIR),frag,$(COOKED_DIR),frag)
ASSET_FILES += $(call raw_to_cooked,$(RAW_BIN_DIR),png,$(COOKED_DIR),png)
ASSET_FILES += $(call raw_to_cooked,$(RAW_DIR),vert,$(COOKED_DIR),vert)
$(info $(ASSET_FILES))


$(COOKED_DIR)/%.blend_sentinel: $(RAW_BIN_DIR)/%.blend tools/io_kowl/*.py
	mkdir -p $(@D)
	$(BLENDER_MAC) -b --factory-startup $< --python tools/io_kowl/area.py -- $(COOKED_DIR)
#	touch $@


$(COOKED_DIR)/%.frag: $(RAW_DIR)/%.frag $(RAW_DIR)/shaders/*.glsl tools/glsl_includer.py
	mkdir -p $(@D)
	python tools/glsl_includer.py $< $@

$(COOKED_DIR)/%.png: $(RAW_BIN_DIR)/%.png
	mkdir -p $(@D)
	cp $< $@

$(COOKED_DIR)/%.vert: $(RAW_DIR)/%.vert $(RAW_DIR)/shaders/*.glsl tools/glsl_includer.py
	mkdir -p $(@D)
	python tools/glsl_includer.py $< $@

.PHONY: assets
assets: $(ASSET_FILES)

.PHONY: clean_assets
clean_assets:
	-$(RM) $(ASSET_FILES)
