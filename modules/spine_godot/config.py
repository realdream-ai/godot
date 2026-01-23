def can_build(env, platform):
    return env.get("module_spine_godot_enabled", True)

def configure(env):
    pass

def get_doc_path():
    return "docs"

def get_doc_classes():
    return [
        "SpineAnimation",
        "SpineAnimationState",
        "SpineAnimationTrack",
        "SpineAtlasResource",
        "SpineAttachment",
        "SpineBone",
        "SpineBoneData",
        "SpineBoneNode",
        "SpineConstraintData",
        "SpineEvent",
        "SpineIkConstraint",
        "SpineIkConstraintData",
        "SpinePathConstraint",
        "SpinePathConstraintData",
        "SpineSkeleton",
        "SpineSkeletonDataResource",
        "SpineSkeletonFileResource",
        "SpineSkin",
        "SpineSlot",
        "SpineSlotData",
        "SpineSlotNode",
        "SpineSprite",
        "SpineTimeline",
        "SpineTrackEntry",
        "SpineTransformConstraint",
        "SpineTransformConstraintData"
    ]
