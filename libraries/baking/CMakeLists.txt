# Copyright 2013-2019, High Fidelity, Inc.
# Copyright 2021-2023 Overte e.V.
# SPDX-License-Identifier: Apache-2.0

set(TARGET_NAME baking)
setup_hifi_library(Concurrent)

link_hifi_libraries(shared shaders graphics networking procedural graphics-scripting ktx image model-serializers model-baker task script-engine)
include_hifi_library_headers(gpu)
include_hifi_library_headers(hfm)
include_hifi_library_headers(material-networking)
