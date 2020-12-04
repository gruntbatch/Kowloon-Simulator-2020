#
# Assets
#
UNCOOKED_DIR = assets
COOKED_DIR = $(BIN_DIR)\assets


# This function is pretty simple. All it does is find any resource
# with a given extension.
# list_res (config.json) -> [ x.config.json, y.config.json, ... ]
list_res = $(patsubst $(shell cd)\\%,%,$(shell dir /a /b /s "$(UNCOOKED_DIR)\*.$(1)" 2>nul))

# strip_path is used to get filepath relative to $(UNCOOKED_DIR). It
# essentially strips the leading folder from a given path.
# strip_path ([ assets/foo/bar, ... ]) -> [ foo/bar, ... ]
strip_path = $(patsubst $(UNCOOKED_DIR)\\%,%,$(call list_res,$(1)))


# res_to_ass finds all resources with a given extension and supplies a
# list of assets with another extension. It does this by finding all
# of the resources, rebasing their file path from $(UNCOOKED_DIR) to
# $(COOKED_DIR), and changing their file extension.
# res_to_ass (foo,bar) -> [ assets/x.bar, assets/y.bar, ... ]
res_to_ass =  $(patsubst %.$(1),$(COOKED_DIR)\\%.$(2),$(call strip_path,$(1)))


# TODO This needs a neater name
GLSL_ASSETS = $(call list_res,glsl)

FRAG_ASSETS = $(call res_to_ass,frag,frag)
MESH_ASSETS = $(call res_to_ass,mesh,mesh)
NAV_ASSETS = $(call res_to_ass,nav,nav)
PTL_ASSETS = $(call res_to_ass,ptl,ptl)
TEXTURE_ASSETS = $(call res_to_ass,png,png)
VERT_ASSETS = $(call res_to_ass,vert,vert)

ASSET_FILES = $(FRAG_ASSETS) $(MESH_ASSETS) $(NAV_ASSETS) $(PTL_ASSETS) $(TEXTURE_ASSETS) $(VERT_ASSETS)


$(COOKED_DIR)\\%.frag: $(UNCOOKED_DIR)\\%.frag $(GLSL_ASSETS)
	if not exist $(@D) mkdir $(@D)
	python tools\\glsl_includer.py $< $@

$(COOKED_DIR)\\%.mesh: $(UNCOOKED_DIR)\\%.mesh
	if not exist $(@D) mkdir $(@D)
	xcopy $< $@ /D

$(COOKED_DIR)\\%.nav: $(UNCOOKED_DIR)\\%.nav
	if not exist $(@D) mkdir $(@D)
	xcopy $< $@ /D

$(COOKED_DIR)\\%.ptl: $(UNCOOKED_DIR)\\%.ptl
	if not exist $(@D) mkdir $(@D)
	xcopy $< $@ /D

$(COOKED_DIR)\\%.png: $(UNCOOKED_DIR)\\%.png
	if not exist $(@D) mkdir $(@D)
	xcopy $< $@ /D

$(COOKED_DIR)\\%.vert: $(UNCOOKED_DIR)\\%.vert $(GLSL_ASSETS)
	if not exist $(@D) mkdir $(@D)
	python Tools\\glsl_includer.py $< $@

.PHONY: assets
assets: $(ASSET_FILES)

.PHONY: clean_assets
clean_assets:
	-$(RM) $(ASSET_FILES)
