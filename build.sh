
BUILD_ARGS='platform=web 
    target=template_release
    threads=no
    extra_cflags=["-sMINIFY=2","--profiling-funcs"] 
    '
MINI_GAME_ARGS='
    optimize=debug 
    extra_suffix=minigame 
    module_mobile_vr_enabled=no 
    module_openxr_enabled=no 
    module_webxr_enabled=no 
    module_text_server_adv_enabled=no 
    module_text_server_fb_enabled=yes 
    module_webrtc_enabled=no 
 '
echo $BUILD_ARGS $MINI_GAME_ARGS
scons $BUILD_ARGS $MINI_GAME_ARGS

# scons platform=web target=template_debug threads="no" && \
#     optimize="debug" extra_cflags=["-sMINIFY=0"] && \
#     extra_suffix="minigame" && \
#     module_mobile_vr_enabled="no" && \
#     module_openxr_enabled="no" && \
#     module_webxr_enabled="no" && \
#     module_text_server_adv_enabled="no" && \
#     module_text_server_fb_enabled="yes" && \
#     module_webrtc_enabled="no"